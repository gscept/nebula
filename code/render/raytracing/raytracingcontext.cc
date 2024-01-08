//------------------------------------------------------------------------------
// raytracingcontext.cc
// (C) 2023 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "raytracingcontext.h"
#include "models/modelcontext.h"
#include "frame/framesubgraph.h"
#include "models/nodes/primitivenode.h"
#include "coregraphics/pipeline.h"
#include "coregraphics/meshresource.h"

#include "raytracetest.h"

namespace Raytracing
{

RaytracingContext::RaytracingAllocator RaytracingContext::raytracingContextAllocator;
__ImplementContext(RaytracingContext, raytracingContextAllocator);

struct
{
    Threading::CriticalSection blasLock;
    Util::Array<CoreGraphics::BlasInstanceId> blasInstances;
    Util::Array<CoreGraphics::BlasId> blasesToRebuild;
    Util::Array<CoreGraphics::BlasId> blases;
    CoreGraphics::TlasId toplevelAccelerationStructure;
    Memory::RangeAllocator blasInstanceAllocator;
    bool topLevelNeedsReconstruction, topLevelNeedsBuild, topLevelNeedsUpdate;


    Memory::ArenaAllocator<sizeof(Frame::FrameCode) * 1> frameOpAllocator;
    Util::HashTable<CoreGraphics::MeshId, CoreGraphics::BlasId> blasLookup;
    CoreGraphics::BufferWithStaging blasInstanceBuffer;

    CoreGraphics::ShaderId raytracingTestShader;
    CoreGraphics::ShaderProgramId raytracingTestProgram;
    CoreGraphics::ResourceTableSet raytracingTestTables;
    CoreGraphics::TextureId raytracingTestOutput;

    CoreGraphics::PipelineRayTracingTable raytracingBundle;

    Threading::Event jobWaitEvent;

    SizeT maxAllowedInstances = 0;
    SizeT numRegisteredInstances = 0;
    SizeT numInstancesToFlush;
} state;

//------------------------------------------------------------------------------
/**
*/
RaytracingContext::RaytracingContext()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
RaytracingContext::~RaytracingContext()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
RaytracingContext::Create(const RaytracingSetupSettings& settings)
{
    n_assert(CoreGraphics::RayTracingSupported);
    __CreateContext();

    state.raytracingTestShader = CoreGraphics::ShaderServer::Instance()->GetShader("shd:raytracetest.fxb");
    state.raytracingTestProgram = CoreGraphics::ShaderGetProgram(state.raytracingTestShader, CoreGraphics::ShaderFeatureFromString("test"));
    state.raytracingTestTables = CoreGraphics::ShaderCreateResourceTableSet(state.raytracingTestShader, NEBULA_BATCH_GROUP, 3);

    state.raytracingBundle = CoreGraphics::CreateRaytracingPipeline({ state.raytracingTestProgram });

#ifndef PUBLIC_BUILD
    __bundle.OnRenderDebug = RaytracingContext::OnRenderDebug;
#endif
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    Frame::FrameCode* blasUpdate = state.frameOpAllocator.Alloc<Frame::FrameCode>();

    blasUpdate->SetName("Bottom Level Acceleration Structure Update");
    blasUpdate->domain = CoreGraphics::BarrierDomain::Global;
    blasUpdate->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        state.blasLock.Enter();

        // Update bottom level acceleration structures
        if (state.blasesToRebuild.Size() > 0)
        {
            
            CoreGraphics::CmdBeginMarker(cmdBuf, NEBULA_MARKER_ORANGE, "Bottom Level Acceleration Structure Build");
            for (IndexT i = 0; i < state.blasesToRebuild.Size(); i++)
            {
                CoreGraphics::CmdBarrier(
                    cmdBuf,
                    CoreGraphics::PipelineStage::AccelerationStructureRead,
                    CoreGraphics::PipelineStage::AccelerationStructureWrite,
                    CoreGraphics::BarrierDomain::Global,
                    nullptr,
                    nullptr,
                    {
                        CoreGraphics::AccelerationStructureBarrierInfo {.blas = state.blasesToRebuild[i], .type = CoreGraphics::AccelerationStructureBarrierInfo::BlasBarrier }
                    }
                );
                CoreGraphics::CmdBuildBlas(cmdBuf, state.blasesToRebuild[i]);
                CoreGraphics::CmdBarrier(
                    cmdBuf,
                    CoreGraphics::PipelineStage::AccelerationStructureWrite,
                    CoreGraphics::PipelineStage::AccelerationStructureRead,
                    CoreGraphics::BarrierDomain::Global,
                    nullptr,
                    nullptr,
                    {
                        CoreGraphics::AccelerationStructureBarrierInfo {.blas = state.blasesToRebuild[i], .type = CoreGraphics::AccelerationStructureBarrierInfo::BlasBarrier}
                    }
                );
            }
            CoreGraphics::CmdEndMarker(cmdBuf);
            state.blasesToRebuild.Clear();
        }

        // Copy instances from staging to host
        if (state.numRegisteredInstances > 0)
        {
            CoreGraphics::CmdBeginMarker(cmdBuf, NEBULA_MARKER_TRANSFER, "Bottom Level Instance Copy");
            state.blasInstanceBuffer.Flush(cmdBuf, state.numRegisteredInstances * CoreGraphics::BlasInstanceGetSize());
            CoreGraphics::CmdEndMarker(cmdBuf);
        }

        // Update top level acceleration
        if (state.topLevelNeedsBuild)
        {
            CoreGraphics::CmdBarrier(
                cmdBuf,
                CoreGraphics::PipelineStage::AccelerationStructureRead,
                CoreGraphics::PipelineStage::AccelerationStructureWrite,
                CoreGraphics::BarrierDomain::Global,
                nullptr,
                nullptr,
                {
                    CoreGraphics::AccelerationStructureBarrierInfo {.tlas = state.toplevelAccelerationStructure, .type = CoreGraphics::AccelerationStructureBarrierInfo::TlasBarrier }
                }
            );

            CoreGraphics::CmdBeginMarker(cmdBuf, NEBULA_MARKER_ORANGE, "Top Level Acceleration Structure Build");
            CoreGraphics::TlasInitBuild(state.toplevelAccelerationStructure);
            CoreGraphics::CmdBuildTlas(cmdBuf, state.toplevelAccelerationStructure);
            CoreGraphics::CmdEndMarker(cmdBuf);

            CoreGraphics::CmdBarrier(
                cmdBuf,
                CoreGraphics::PipelineStage::AccelerationStructureWrite,
                CoreGraphics::PipelineStage::AccelerationStructureRead,
                CoreGraphics::BarrierDomain::Global,
                nullptr,
                nullptr,
                {
                    CoreGraphics::AccelerationStructureBarrierInfo {.tlas = state.toplevelAccelerationStructure, .type = CoreGraphics::AccelerationStructureBarrierInfo::TlasBarrier }
                }
            );
        }
        else if (state.topLevelNeedsUpdate)
        {
            CoreGraphics::CmdBarrier(
                cmdBuf,
                CoreGraphics::PipelineStage::AccelerationStructureRead,
                CoreGraphics::PipelineStage::AccelerationStructureWrite,
                CoreGraphics::BarrierDomain::Global,
                nullptr,
                nullptr,
                {
                    CoreGraphics::AccelerationStructureBarrierInfo {.tlas = state.toplevelAccelerationStructure, .type = CoreGraphics::AccelerationStructureBarrierInfo::TlasBarrier }
                }
            );

            CoreGraphics::CmdBeginMarker(cmdBuf, NEBULA_MARKER_ORANGE, "Top Level Acceleration Structure Update");
            CoreGraphics::TlasInitUpdate(state.toplevelAccelerationStructure);
            CoreGraphics::CmdBuildTlas(cmdBuf, state.toplevelAccelerationStructure);
            CoreGraphics::CmdEndMarker(cmdBuf);

            CoreGraphics::CmdBarrier(
                cmdBuf,
                CoreGraphics::PipelineStage::AccelerationStructureWrite,
                CoreGraphics::PipelineStage::AccelerationStructureRead,
                CoreGraphics::BarrierDomain::Global,
                nullptr,
                nullptr,
                {
                    CoreGraphics::AccelerationStructureBarrierInfo {.tlas = state.toplevelAccelerationStructure, .type = CoreGraphics::AccelerationStructureBarrierInfo::TlasBarrier }
                }
            );
        }
        state.blasLock.Leave();
    };
    Frame::AddSubgraph("Raytracing Structures Update", { blasUpdate });

    Frame::FrameCode* testRays = state.frameOpAllocator.Alloc<Frame::FrameCode>();
    testRays->SetName("Ray Tracing Test Shader");
    testRays->domain = CoreGraphics::BarrierDomain::Global;
    testRays->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        if (state.toplevelAccelerationStructure != CoreGraphics::InvalidTlasId)
        {
            CoreGraphics::CmdSetRayTracingPipeline(cmdBuf, state.raytracingBundle.pipeline);
            CoreGraphics::CmdSetResourceTable(cmdBuf, state.raytracingTestTables.tables[bufferIndex], NEBULA_BATCH_GROUP, CoreGraphics::RayTracingPipeline, {});
            CoreGraphics::CmdRaysDispatch(cmdBuf, state.raytracingBundle.table, 640, 480, 1);
        }
    };
    Frame::AddSubgraph("Raytracing Test", { testRays });

    // Create buffers for updating blas instances
    CoreGraphics::BufferCreateInfo bufInfo;
    bufInfo.elementSize = CoreGraphics::BlasInstanceGetSize();
    bufInfo.size = settings.maxNumAllowedInstances;
    bufInfo.usageFlags = CoreGraphics::BufferUsageFlag::ShaderAddress | CoreGraphics::BufferUsageFlag::AccelerationStructureInstances;
    state.blasInstanceBuffer = CoreGraphics::BufferWithStaging(bufInfo);

    state.raytracingTestOutput = settings.script->GetTexture("RayTracingTestOutput");

    state.maxAllowedInstances = settings.maxNumAllowedInstances;

    state.blasInstanceAllocator = Memory::RangeAllocator(0xFFFFF, settings.maxNumAllowedInstances);
}

//------------------------------------------------------------------------------
/**
*/
void
RaytracingContext::Setup(const Graphics::GraphicsEntityId id, CoreGraphics::BlasInstanceFlags flags, uint mask, uint shaderOffset)
{
    Graphics::ContextEntityId contextId = GetContextId(id);
    const Models::NodeInstanceRange& nodes = Models::ModelContext::GetModelRenderableRange(id);
    SizeT numObjects = nodes.end - nodes.begin;
    n_assert((state.blasInstances.Size() + numObjects) < state.maxAllowedInstances);
    Memory::RangeAllocation alloc = state.blasInstanceAllocator.Alloc(numObjects);
    state.blasInstances.Resize(alloc.offset + numObjects);
    
    raytracingContextAllocator.Set<Raytracing_Allocation>(contextId.id, alloc);
    raytracingContextAllocator.Set<Raytracing_NumStructures>(contextId.id, numObjects);

    IndexT counter = 0;
    for (IndexT i = nodes.begin; i < nodes.end; i++)
    {
        Models::PrimitiveNode* pNode = static_cast<Models::PrimitiveNode*>(Models::ModelContext::NodeInstances.renderable.nodes[i]);
        Resources::CreateResourceListener(pNode->GetMeshResource(), [flags, mask, shaderOffset, offset = alloc.offset, counter, i, pNode](Resources::ResourceId id)
        {
            state.blasLock.Enter();
            CoreGraphics::MeshResourceId meshRes = id;

            // Create Blas if we haven't registered it yet
            CoreGraphics::MeshId mesh = MeshResourceGetMesh(meshRes, pNode->GetMeshIndex());
            IndexT blasIndex = state.blasLookup.FindIndex(mesh);
            if (blasIndex == InvalidIndex)
            {
                CoreGraphics::BlasCreateInfo createInfo;
                createInfo.mesh = mesh;
                createInfo.flags = CoreGraphics::AccelerationStructureBuildFlags::FastTrace;
                CoreGraphics::BlasId blas = CoreGraphics::CreateBlas(createInfo);
                state.blases.Append(blas);
                blasIndex = state.blasLookup.Add(mesh, blas);

                CoreGraphics::BlasIdRelease(blas);
                state.blasesToRebuild.Append(blas);
            }

            // Setup instance
            CoreGraphics::BlasInstanceCreateInfo createInfo;
            createInfo.flags = flags;
            createInfo.mask = mask;
            createInfo.shaderOffset = shaderOffset;
            createInfo.instanceIndex = offset + counter;
            createInfo.blas = state.blasLookup.ValueAtIndex(mesh, blasIndex);
            createInfo.transform = Models::ModelContext::NodeInstances.transformable.nodeTransforms[Models::ModelContext::NodeInstances.renderable.nodeTransformIndex[i]];

            CoreGraphics::BlasIdLock _0(createInfo.blas);
            state.blasInstances[offset + counter] = CoreGraphics::CreateBlasInstance(createInfo);

            state.blasLock.Leave();
        });
        counter++;
    }
    state.topLevelNeedsReconstruction = true;
}

//------------------------------------------------------------------------------
/**
*/
void
RaytracingContext::ReconstructTopLevelAcceleration(const Graphics::FrameContext& ctx)
{
    if (state.topLevelNeedsReconstruction)
    {
        if (state.toplevelAccelerationStructure != CoreGraphics::InvalidTlasId)
        {
            CoreGraphics::DestroyTlas(state.toplevelAccelerationStructure);
        }
        CoreGraphics::TlasCreateInfo createInfo;
        createInfo.numInstances = state.blasInstances.Size();
        createInfo.instanceBuffer = state.blasInstanceBuffer.HostBuffer();
        createInfo.flags = CoreGraphics::AccelerationStructureBuildFlags::FastBuild | CoreGraphics::AccelerationStructureBuildFlags::Dynamic;
        state.toplevelAccelerationStructure = CoreGraphics::CreateTlas(createInfo);
        state.topLevelNeedsReconstruction = false;
        state.topLevelNeedsBuild = true;
    }
    else if (state.toplevelAccelerationStructure != CoreGraphics::InvalidTlasId)
    {
        state.topLevelNeedsBuild = true;
    }

    if (state.toplevelAccelerationStructure != CoreGraphics::InvalidTlasId)
    {
        CoreGraphics::ResourceTableSetAccelerationStructure(
            state.raytracingTestTables.tables[ctx.bufferIndex],
            CoreGraphics::ResourceTableTlas(state.toplevelAccelerationStructure, Raytracetest::Table_Batch::TLAS_SLOT)
        );
        CoreGraphics::ResourceTableSetRWTexture(
            state.raytracingTestTables.tables[ctx.bufferIndex],
            CoreGraphics::ResourceTableTexture(state.raytracingTestOutput, Raytracetest::Table_Batch::Output_SLOT)
        );
        CoreGraphics::ResourceTableCommitChanges(state.raytracingTestTables.tables[ctx.bufferIndex]);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
RaytracingContext::UpdateTransforms(const Graphics::FrameContext& ctx)
{
    const Util::Array<Graphics::GraphicsEntityId>& entities = RaytracingContext::__state.entities;

    if (!entities.IsEmpty())
    {
        static Util::Array<uint32> nodes;
        nodes.Clear();
        nodes.Resize(entities.Size());

        static Threading::AtomicCounter idCounter;
        idCounter = 1;
        struct TransformUpdateContext
        {
            Graphics::GraphicsEntityId* ids;
            Threading::AtomicCounter counter;
        } idCtx;

        idCtx.ids = entities.Begin();
        idCtx.counter = 0;

        // Run job to collect model node ids
        Jobs2::JobDispatch([](SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset, void* ctx)
        {
            N_SCOPE(RaytracingTransformJob, Graphics);

            auto context = static_cast<TransformUpdateContext*>(ctx);
            for (IndexT i = 0; i < groupSize; i++)
            {
                IndexT index = i + invocationOffset;
                if (index >= totalJobs)
                    return;

                // Get node range and update ids buffer
                Graphics::GraphicsEntityId gid = context->ids[index];
                Graphics::ContextEntityId cid = GetContextId(gid);
                const Models::NodeInstanceRange& renderableRange = Models::ModelContext::GetModelRenderableRange(gid);
                const Models::NodeInstanceRange& transformableRange = Models::ModelContext::GetModelTransformableRange(gid);

                const Models::ModelContext::ModelInstance::Renderable& renderables = Models::ModelContext::GetModelRenderables();
                const Models::ModelContext::ModelInstance::Transformable& transformables = Models::ModelContext::GetModelTransformables();
                const Memory::RangeAllocation alloc = raytracingContextAllocator.Get<Raytracing_Allocation>(cid.id);

                const uint numNodes = renderableRange.end - renderableRange.begin;
                Threading::Interlocked::Add(&context->counter, numNodes);
                uint counter = 0;
                for (IndexT j = renderableRange.begin; j < renderableRange.end; j++)
                {
                    const Math::mat4& transform = transformables.nodeTransforms[transformableRange.begin + renderables.nodeTransformIndex[j]];
                    if (state.blasInstances[alloc.offset + counter] != CoreGraphics::InvalidBlasInstanceId)
                    {
                        CoreGraphics::BlasInstanceIdLock _0(state.blasInstances[alloc.offset + counter]);
                        CoreGraphics::BlasInstanceUpdate(state.blasInstances[alloc.offset + counter], transform, state.blasInstanceBuffer.HostBuffer(), (alloc.offset + counter) * CoreGraphics::BlasInstanceGetSize());
                    }
                    counter++;
                }
            }
        }, entities.Size(), 1024, idCtx, { &Models::ModelContext::TransformsUpdateCounter }, &idCounter, &state.jobWaitEvent);

        state.topLevelNeedsUpdate = true;
    }
    else
    {
        state.jobWaitEvent.Signal();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
RaytracingContext::WaitForJobs(const Graphics::FrameContext& ctx)
{
    N_MARKER_BEGIN(WaitForRaytracingJobs, Graphics);
    state.jobWaitEvent.Wait();
    N_MARKER_END();
}

//------------------------------------------------------------------------------
/**
*/
void
RaytracingContext::OnRenderDebug(uint32_t flags)
{
}

//------------------------------------------------------------------------------
/**
*/
void
RaytracingContext::Dealloc(Graphics::ContextEntityId id)
{
    // clean up old stuff, but don't deallocate entity
    Memory::RangeAllocation range = raytracingContextAllocator.Get<Raytracing_Allocation>(id.id);
    SizeT numAllocs = raytracingContextAllocator.Get<Raytracing_NumStructures>(id.id);
    for (IndexT i = range.offset; i < numAllocs; i++)
    {
        CoreGraphics::DestroyBlas(state.blases[i]);
    }
    raytracingContextAllocator.Dealloc(id.id);
    state.topLevelNeedsReconstruction = true;
}

} // namespace Raytracing
