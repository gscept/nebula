//------------------------------------------------------------------------------
// raytracingcontext.cc
// (C) 2023 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "raytracingcontext.h"
#include "models/modelcontext.h"
#include "frame/framesubgraph.h"

namespace Raytracing
{

RaytracingContext::RaytracingAllocator RaytracingContext::raytracingContextAllocator;
__ImplementContext(RaytracingContext, raytracingContextAllocator);

struct
{
    Util::Array<CoreGraphics::BlasInstanceId> blasInstances;
    Util::Array<CoreGraphics::BlasId> blasesToRebuild;
    Util::Array<CoreGraphics::BlasId> blases;
    CoreGraphics::TlasId toplevelAccelerationStructure;
    Memory::RangeAllocator blasInstanceAllocator;
    bool topLevelNeedsRebuild, topLevelNeedsUpdate;


    Memory::ArenaAllocator<sizeof(Frame::FrameCode) * 1> frameOpAllocator;
    Util::HashTable<CoreGraphics::MeshId, CoreGraphics::BlasId> blasLookup;
    CoreGraphics::BufferWithStaging blasInstanceBuffer;

    Threading::Event jobWaitEvent;

    SizeT maxAllowedInstances = 0;
    SizeT numRegisteredInstances = 0;
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
    __CreateContext();

#ifndef PUBLIC_BUILD
    __bundle.OnRenderDebug = RaytracingContext::OnRenderDebug;
#endif
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    Frame::FrameCode* blasUpdate = state.frameOpAllocator.Alloc<Frame::FrameCode>();

    blasUpdate->SetName("Bottom Level Acceleration Structure Update");
    blasUpdate->domain = CoreGraphics::BarrierDomain::Global;
    blasUpdate->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        // Update bottom level acceleration structures
        if (state.blasesToRebuild.Size() > 0)
        {
            CoreGraphics::CmdBeginMarker(cmdBuf, NEBULA_MARKER_ORANGE, "Bottom Level Acceleration Structure Build");
            for (IndexT i = 0; i < state.blasesToRebuild.Size(); i++)
            {
                CoreGraphics::CmdBuildBlas(cmdBuf, state.blasesToRebuild[i]);
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
        if (state.topLevelNeedsRebuild)
        {
            CoreGraphics::CmdBeginMarker(cmdBuf, NEBULA_MARKER_ORANGE, "Top Level Acceleration Structure Build");
            CoreGraphics::TlasInitBuild(state.toplevelAccelerationStructure);
            CoreGraphics::CmdBuildTlas(cmdBuf, state.toplevelAccelerationStructure);
            CoreGraphics::CmdEndMarker(cmdBuf);
        }
        else if (state.topLevelNeedsUpdate)
        {
            CoreGraphics::CmdBeginMarker(cmdBuf, NEBULA_MARKER_ORANGE, "Top Level Acceleration Structure Update");
            CoreGraphics::TlasInitUpdate(state.toplevelAccelerationStructure);
            CoreGraphics::CmdBuildTlas(cmdBuf, state.toplevelAccelerationStructure);
            CoreGraphics::CmdEndMarker(cmdBuf);
        }
    };
    Frame::AddSubgraph("Raytracing Structures Update", { blasUpdate });

    // Create buffers for updating blas instances
    CoreGraphics::BufferCreateInfo bufInfo;
    bufInfo.elementSize = CoreGraphics::BlasInstanceGetSize();
    bufInfo.size = settings.maxNumAllowedInstances;
    bufInfo.usageFlags = CoreGraphics::BufferUsageFlag::ShaderAddress;
    state.blasInstanceBuffer = CoreGraphics::BufferWithStaging(bufInfo);

    state.maxAllowedInstances = settings.maxNumAllowedInstances;

    state.blasInstanceAllocator = Memory::RangeAllocator(0xFFFFF, settings.maxNumAllowedInstances);
}

//------------------------------------------------------------------------------
/**
*/
void
RaytracingContext::Setup(const Graphics::GraphicsEntityId id)
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
        CoreGraphics::MeshId mesh = Models::ModelContext::NodeInstances.renderable.nodeMeshes[i];

        // Create Blas if we haven't registered it yet
        IndexT blasIndex = state.blasLookup.FindIndex(mesh);
        if (blasIndex == InvalidIndex)
        {
            CoreGraphics::BlasCreateInfo createInfo;
            createInfo.mesh = mesh;
            createInfo.flags = CoreGraphics::AccelerationStructureBuildFlags::FastTrace;
            CoreGraphics::BlasId blas = CoreGraphics::CreateBlas(createInfo);
            state.blases.Append(blas);
            blasIndex = state.blasLookup.Add(mesh, blas);

            state.blasesToRebuild.Append(blas);
        }

        // Setup instance
        CoreGraphics::BlasInstanceCreateInfo createInfo;
        createInfo.flags = CoreGraphics::BlasInstanceFlags::NoFlags;
        createInfo.mask = 0x0;
        createInfo.shaderOffset = 0;
        createInfo.instanceIndex = alloc.offset + counter;
        createInfo.buffer = state.blasInstanceBuffer.HostBuffer();
        createInfo.blas = state.blasLookup.ValueAtIndex(mesh, blasIndex);
        createInfo.transform = Models::ModelContext::NodeInstances.transformable.nodeTransforms[Models::ModelContext::NodeInstances.renderable.nodeTransformIndex[i]];
        createInfo.offset = (alloc.offset + counter) * CoreGraphics::BlasInstanceGetSize();
        state.blasInstances[alloc.offset + counter] = CoreGraphics::CreateBlasInstance(createInfo);
        counter++;
    }
    state.topLevelNeedsRebuild = true;
}

//------------------------------------------------------------------------------
/**
*/
void
RaytracingContext::RebuildToplevelAcceleration(const Graphics::FrameContext& ctx)
{
    if (state.topLevelNeedsRebuild)
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
                const Models::NodeInstanceRange& NodeInstances = Models::ModelContext::GetModelRenderableRange(gid);
                const Models::ModelContext::ModelInstance::Renderable& renderables = Models::ModelContext::GetModelRenderables();
                const Models::ModelContext::ModelInstance::Transformable& transformables = Models::ModelContext::GetModelTransformables();
                const Memory::RangeAllocation alloc = raytracingContextAllocator.Get<Raytracing_Allocation>(cid.id);

                const uint numNodes = NodeInstances.end - NodeInstances.begin;
                uint offset = Threading::Interlocked::Add(&context->counter, numNodes);
                uint counter = 0;
                for (IndexT j = NodeInstances.begin; j < NodeInstances.end; j++)
                {
                    const Math::mat4& transform = transformables.nodeTransforms[renderables.nodeTransformIndex[j]];
                    CoreGraphics::BlasInstanceIdLock _0(state.blasInstances[alloc.offset + counter]);
                    CoreGraphics::BlasInstanceSetTransform(state.blasInstances[alloc.offset + counter], transform);
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
    state.topLevelNeedsRebuild = true;
}

} // namespace Raytracing
