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
#include "coregraphics/shader.h"
#include "graphics/globalconstants.h"

#include "materials/materialloader.h"

#include "raytracing/shaders/raytracetest.h"
#include "raytracing/shaders/brdfhit.h"
#include "raytracing/shaders/bsdfhit.h"
#include "raytracing/shaders/gltfhit.h"

#include "raytracing/shaders/light_grid_cs.h"

namespace Raytracing
{

RaytracingContext::RaytracingAllocator RaytracingContext::raytracingContextAllocator;
__ImplementContext(RaytracingContext, raytracingContextAllocator);


struct
{
    Threading::CriticalSection blasLock;
    Util::Array<CoreGraphics::BlasInstanceId> blasInstances;
    Util::Array<CoreGraphics::MeshId> blasInstanceMeshes;
    Util::Array<CoreGraphics::BlasId> blasesToRebuild;
    Util::Array<CoreGraphics::BlasId> blases;
    CoreGraphics::TlasId toplevelAccelerationStructure;
    Memory::RangeAllocator blasInstanceAllocator;
    bool topLevelNeedsReconstruction, topLevelNeedsBuild, topLevelNeedsUpdate;

    Memory::ArenaAllocator<sizeof(Frame::FrameCode) * 1> frameOpAllocator;
    Util::HashTable<CoreGraphics::MeshId, Util::Tuple<uint, CoreGraphics::BlasId>> blasLookup;
    Util::HashTable<Graphics::ContextEntityId, Util::Array<CoreGraphics::BlasId>> terrainBlases;
    CoreGraphics::BufferWithStaging blasInstanceBuffer;

    CoreGraphics::ResourceTableSet raytracingTestTables;
    CoreGraphics::TextureId raytracingTestOutput;

    CoreGraphics::BufferId geometryBindingBuffer;
    CoreGraphics::BufferWithStaging objectBindingBuffer;
    Util::Array<Raytracetest::Object> objects;

    CoreGraphics::BufferId lightGrid;
    CoreGraphics::BufferId lightGridConstants;
    CoreGraphics::ShaderId lightGridShader;
    CoreGraphics::ResourceTableSet lightGridResourceTables;
    CoreGraphics::ShaderProgramId lightGridGenProgram, lightGridCullProgram;
    CoreGraphics::BufferId lightGridIndexLists;

    CoreGraphics::PipelineRayTracingTable raytracingBundle;

    Threading::Event jobWaitEvent;

    SizeT maxAllowedInstances = 0;
    SizeT numRegisteredInstances = 0;
    SizeT numInstancesToFlush;
} state;

static uint MaterialPropertyMappings[(uint)Materials::MaterialProperties::Num];
//------------------------------------------------------------------------------
/**
*/
RaytracingContext::RaytracingContext()
{
    static_assert(sizeof(Brdfhit::Object) == sizeof(Bsdfhit::Object), "All raytracing object bindings must be same size");
    static_assert(sizeof(Bsdfhit::Object) == sizeof(Gltfhit::Object), "All raytracing object bindings must be same size");
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
    if (!CoreGraphics::RayTracingSupported)
        return;
    __CreateContext();

    auto raygenShader = CoreGraphics::ShaderGet("shd:raytracing/shaders/raytracetest.fxb");
    auto raygenProgram = CoreGraphics::ShaderGetProgram(raygenShader, CoreGraphics::ShaderFeatureMask("test"));
    auto brdfHitShader = CoreGraphics::ShaderGet("shd:raytracing/shaders/brdfhit.fxb");
    auto brdfHitProgram = CoreGraphics::ShaderGetProgram(brdfHitShader, CoreGraphics::ShaderFeatureMask("Hit"));
    auto bsdfHitShader = CoreGraphics::ShaderGet("shd:raytracing/shaders/bsdfhit.fxb");
    auto bsdfHitProgram = CoreGraphics::ShaderGetProgram(bsdfHitShader, CoreGraphics::ShaderFeatureMask("Hit"));
    auto gltfHitShader = CoreGraphics::ShaderGet("shd:raytracing/shaders/gltfhit.fxb");
    auto gltfHitProgram = CoreGraphics::ShaderGetProgram(gltfHitShader, CoreGraphics::ShaderFeatureMask("Hit"));
    auto terrainHitShader = CoreGraphics::ShaderGet("shd:raytracing/shaders/terrainhit.fxb");
    auto terrainHitProgram = CoreGraphics::ShaderGetProgram(terrainHitShader, CoreGraphics::ShaderFeatureMask("Hit"));
    state.lightGridShader = CoreGraphics::ShaderGet("shd:raytracing/shaders/light_grid_cs.fxb");
    state.lightGridCullProgram = CoreGraphics::ShaderGetProgram(state.lightGridShader, CoreGraphics::ShaderFeatureMask("Cull"));
    state.lightGridGenProgram = CoreGraphics::ShaderGetProgram(state.lightGridShader, CoreGraphics::ShaderFeatureMask("AABBGenerate"));

    Util::Array<CoreGraphics::ShaderProgramId> shaderMappings((SizeT)Materials::MaterialProperties::Num + 1, 0);
    shaderMappings.Append(raygenProgram);
    shaderMappings.Append(brdfHitProgram);
    shaderMappings.Append(bsdfHitProgram);
    shaderMappings.Append(gltfHitProgram);
    shaderMappings.Append(CoreGraphics::InvalidShaderProgramId);
    shaderMappings.Append(CoreGraphics::InvalidShaderProgramId);
    shaderMappings.Append(CoreGraphics::InvalidShaderProgramId);
    shaderMappings.Append(CoreGraphics::InvalidShaderProgramId);
    shaderMappings.Append(CoreGraphics::InvalidShaderProgramId);
    shaderMappings.Append(CoreGraphics::InvalidShaderProgramId);
    shaderMappings.Append(terrainHitProgram);

    uint bindingCounter = 0;
    MaterialPropertyMappings[(uint)Materials::MaterialProperties::BRDF] = bindingCounter++;
    MaterialPropertyMappings[(uint)Materials::MaterialProperties::BSDF] = bindingCounter++;
    MaterialPropertyMappings[(uint)Materials::MaterialProperties::GLTF] = bindingCounter++;
    MaterialPropertyMappings[(uint)Materials::MaterialProperties::Unlit] = 0xFFFFFFFF;
    MaterialPropertyMappings[(uint)Materials::MaterialProperties::Unlit2] = 0xFFFFFFFF;
    MaterialPropertyMappings[(uint)Materials::MaterialProperties::Unlit3] = 0xFFFFFFFF;
    MaterialPropertyMappings[(uint)Materials::MaterialProperties::Unlit4] = 0xFFFFFFFF;
    MaterialPropertyMappings[(uint)Materials::MaterialProperties::Skybox] = 0xFFFFFFFF;
    MaterialPropertyMappings[(uint)Materials::MaterialProperties::Legacy] = 0xFFFFFFFF;
    MaterialPropertyMappings[(uint)Materials::MaterialProperties::Terrain] = bindingCounter++;

    state.raytracingTestTables = CoreGraphics::ShaderCreateResourceTableSet(raygenShader, NEBULA_BATCH_GROUP, 3);
    state.raytracingBundle = CoreGraphics::CreateRaytracingPipeline(shaderMappings);

    Raytracetest::Geometry geometryBindings;
    geometryBindings.Index32Ptr = CoreGraphics::BufferGetDeviceAddress(CoreGraphics::GetIndexBuffer());
    geometryBindings.Index16Ptr = CoreGraphics::BufferGetDeviceAddress(CoreGraphics::GetIndexBuffer());
    geometryBindings.PositionsPtr = CoreGraphics::BufferGetDeviceAddress(CoreGraphics::GetVertexBuffer());
    geometryBindings.NormalsPtr = CoreGraphics::BufferGetDeviceAddress(CoreGraphics::GetVertexBuffer());
    geometryBindings.SecondaryUVPtr = CoreGraphics::BufferGetDeviceAddress(CoreGraphics::GetVertexBuffer());
    geometryBindings.ColorsPtr = CoreGraphics::BufferGetDeviceAddress(CoreGraphics::GetVertexBuffer());
    geometryBindings.SkinPtr = CoreGraphics::BufferGetDeviceAddress(CoreGraphics::GetVertexBuffer());

    CoreGraphics::BufferCreateInfo geometryBindingBufferCreateInfo;
    geometryBindingBufferCreateInfo.byteSize = sizeof(Raytracetest::Geometry);
    geometryBindingBufferCreateInfo.mode = CoreGraphics::BufferAccessMode::DeviceLocal;
    geometryBindingBufferCreateInfo.usageFlags = CoreGraphics::BufferUsageFlag::ShaderAddress | CoreGraphics::BufferUsageFlag::ReadWriteBuffer;
    geometryBindingBufferCreateInfo.data = &geometryBindings;
    geometryBindingBufferCreateInfo.dataSize = sizeof(Raytracetest::Geometry);
    state.geometryBindingBuffer = CoreGraphics::CreateBuffer(geometryBindingBufferCreateInfo);

    CoreGraphics::BufferCreateInfo lightGridInfo;
    lightGridInfo.name = "RaytracingAABBGrid";
    lightGridInfo.size = 64 * 64 * 64;
    lightGridInfo.elementSize = sizeof(LightGridCs::ClusterAABB);
    lightGridInfo.mode = CoreGraphics::BufferAccessMode::DeviceLocal;
    lightGridInfo.usageFlags = CoreGraphics::ReadWriteBuffer;
    lightGridInfo.queueSupport = CoreGraphics::GraphicsQueueSupport | CoreGraphics::ComputeQueueSupport;
    state.lightGrid = CoreGraphics::CreateBuffer(lightGridInfo);

    CoreGraphics::BufferCreateInfo indexListInfo;
    indexListInfo.name = "LightIndexListsBuffer";
    indexListInfo.byteSize = sizeof(LightGridCs::LightIndexLists);
    indexListInfo.mode = CoreGraphics::BufferAccessMode::DeviceLocal;
    indexListInfo.usageFlags = CoreGraphics::ReadWriteBuffer;
    indexListInfo.queueSupport = CoreGraphics::GraphicsQueueSupport | CoreGraphics::ComputeQueueSupport;
    state.lightGridIndexLists = CoreGraphics::CreateBuffer(indexListInfo);

    CoreGraphics::BufferCreateInfo lightGridConstantsInfo;
    lightGridConstantsInfo.byteSize = sizeof(LightGridCs::ClusterUniforms);
    lightGridConstantsInfo.mode = CoreGraphics::BufferAccessMode::DeviceAndHost;
    lightGridConstantsInfo.usageFlags = CoreGraphics::ConstantBuffer;
    lightGridConstantsInfo.queueSupport = CoreGraphics::ComputeQueueSupport;
    state.lightGridConstants = CoreGraphics::CreateBuffer(lightGridConstantsInfo);

    state.lightGridResourceTables = CoreGraphics::ShaderCreateResourceTableSet(state.lightGridShader, NEBULA_FRAME_GROUP, 3);
    for (IndexT i = 0; i < CoreGraphics::GetNumBufferedFrames(); i++)
    {
        ResourceTableSetRWBuffer(state.lightGridResourceTables.tables[i], { state.lightGrid, LightGridCs::Table_Frame::ClusterAABBs_SLOT, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
        ResourceTableSetRWBuffer(state.lightGridResourceTables.tables[i], { state.lightGridIndexLists, LightGridCs::Table_Frame::LightIndexLists_SLOT, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
        ResourceTableSetConstantBuffer(state.lightGridResourceTables.tables[i], { state.lightGridConstants, LightGridCs::Table_Frame::ClusterUniforms_SLOT, 0, sizeof(LightGridCs::ClusterUniforms), 0 });
        ResourceTableCommitChanges(state.lightGridResourceTables.tables[i]);
    }

#ifndef PUBLIC_BUILD
    __bundle.OnRenderDebug = RaytracingContext::OnRenderDebug;
#endif
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    Frame::FrameCode* lightGridGen = state.frameOpAllocator.Alloc<Frame::FrameCode>();
    lightGridGen->SetName("Raytracing Light Grid Update");
    lightGridGen->domain = CoreGraphics::BarrierDomain::Global;
    lightGridGen->queue = CoreGraphics::QueueType::ComputeQueueType;
    lightGridGen->bufferDeps.Add(state.lightGrid,
                                  {
                                      "Light Grid"
                                      , CoreGraphics::PipelineStage::ComputeShaderWrite
                                      , CoreGraphics::BufferSubresourceInfo()
                                  });
    lightGridGen->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        CoreGraphics::CmdSetShaderProgram(cmdBuf, state.lightGridGenProgram);
        CoreGraphics::CmdSetResourceTable(cmdBuf, state.lightGridResourceTables.tables[bufferIndex], NEBULA_FRAME_GROUP, CoreGraphics::ComputePipeline, nullptr);
        CoreGraphics::CmdDispatch(cmdBuf, 64 * 64, 1, 1);
    };

    Frame::FrameCode* lightGridCull = state.frameOpAllocator.Alloc<Frame::FrameCode>();
    lightGridCull->SetName("Raytracing Light Grid Cull");
    lightGridCull->domain = CoreGraphics::BarrierDomain::Global;
    lightGridCull->queue = CoreGraphics::QueueType::ComputeQueueType;
    lightGridCull->bufferDeps.Add(state.lightGrid,
                                  {
                                      "Light Grid"
                                      , CoreGraphics::PipelineStage::ComputeShaderRead
                                      , CoreGraphics::BufferSubresourceInfo()
                                  });
    lightGridCull->bufferDeps.Add(state.lightGridIndexLists,
                                  {
                                      "Light Grid Index Lists"
                                      , CoreGraphics::PipelineStage::ComputeShaderWrite
                                      , CoreGraphics::BufferSubresourceInfo()
                                  });
    lightGridCull->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        CoreGraphics::CmdSetShaderProgram(cmdBuf, state.lightGridCullProgram);
        CoreGraphics::CmdSetResourceTable(cmdBuf, state.lightGridResourceTables.tables[bufferIndex], NEBULA_FRAME_GROUP, CoreGraphics::ComputePipeline, nullptr);
        CoreGraphics::CmdDispatch(cmdBuf, 64 * 64, 1, 1);
    };
    Frame::AddSubgraph("Raytracing Light Update", { lightGridGen, lightGridCull });

    Frame::FrameCode* blasUpdate = state.frameOpAllocator.Alloc<Frame::FrameCode>();
    blasUpdate->SetName("Bottom Level Acceleration Structure Update");
    blasUpdate->domain = CoreGraphics::BarrierDomain::Global;
    blasUpdate->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        if (!state.objects.IsEmpty())
            state.objectBindingBuffer.Flush(cmdBuf, state.objects.ByteSize());
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
    testRays->bufferDeps.Add(state.lightGridIndexLists,
                              {
                                "Light Grid Index Lists"
                                , CoreGraphics::PipelineStage::RayTracingShaderRead
                                , CoreGraphics::BufferSubresourceInfo()
                              });
    testRays->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        if (state.toplevelAccelerationStructure != CoreGraphics::InvalidTlasId)
        {
            CoreGraphics::CmdSetRayTracingPipeline(cmdBuf, state.raytracingBundle.pipeline);
            CoreGraphics::CmdSetResourceTable(cmdBuf, state.raytracingTestTables.tables[bufferIndex], NEBULA_BATCH_GROUP, CoreGraphics::RayTracingPipeline, {});
            CoreGraphics::CmdSetResourceTable(cmdBuf, state.lightGridResourceTables.tables[bufferIndex], NEBULA_FRAME_GROUP, CoreGraphics::RayTracingPipeline, nullptr);
            CoreGraphics::CmdRaysDispatch(cmdBuf, state.raytracingBundle.table, 640, 480, 1);
        }
    };
    Frame::AddSubgraph("Raytracing Test", { testRays });

    // Create buffers for updating blas instances
    CoreGraphics::BufferCreateInfo bufInfo;
    bufInfo.elementSize = CoreGraphics::BlasInstanceGetSize();
    bufInfo.size = settings.maxNumAllowedInstances;  // This is a virtual max-size, as this buffer is virtual memory managed
    bufInfo.usageFlags = CoreGraphics::BufferUsageFlag::ShaderAddress | CoreGraphics::BufferUsageFlag::AccelerationStructureInstances;
    state.blasInstanceBuffer = CoreGraphics::BufferWithStaging(bufInfo);

    CoreGraphics::BufferCreateInfo objectBindingBufferCreateInfo;
    objectBindingBufferCreateInfo.name = "Raytracing Object Binding Buffer";
    objectBindingBufferCreateInfo.byteSize = sizeof(Raytracetest::Object) * settings.maxNumAllowedInstances;
    objectBindingBufferCreateInfo.usageFlags = CoreGraphics::BufferUsageFlag::ShaderAddress | CoreGraphics::BufferUsageFlag::ReadWriteBuffer;
    state.objectBindingBuffer = CoreGraphics::BufferWithStaging(objectBindingBufferCreateInfo);

    state.raytracingTestOutput = settings.script->GetTexture("RayTracingTestOutput");

    state.maxAllowedInstances = settings.maxNumAllowedInstances;

    state.blasInstanceAllocator = Memory::RangeAllocator(0xFFFFF, settings.maxNumAllowedInstances);
}

//------------------------------------------------------------------------------
/**
*/
void
RaytracingContext::SetupModel(const Graphics::GraphicsEntityId id, CoreGraphics::BlasInstanceFlags flags, uchar mask)
{
    if (!CoreGraphics::RayTracingSupported)
        return;

    Graphics::ContextEntityId contextId = GetContextId(id);
    const Models::NodeInstanceRange& nodes = Models::ModelContext::GetModelRenderableRange(id);
    SizeT numObjects = nodes.end - nodes.begin;
    n_assert((state.blasInstances.Size() + numObjects) < state.maxAllowedInstances);
    Memory::RangeAllocation alloc = state.blasInstanceAllocator.Alloc(numObjects);
    state.blasInstances.Extend(alloc.offset + numObjects);
    state.blasInstanceMeshes.Extend(alloc.offset + numObjects);
    state.objects.Extend(alloc.offset + numObjects);
    
    raytracingContextAllocator.Set<Raytracing_Allocation>(contextId.id, alloc);
    raytracingContextAllocator.Set<Raytracing_NumStructures>(contextId.id, numObjects);
    raytracingContextAllocator.Set<Raytracing_ObjectType>(contextId.id, ObjectType::Dynamic);

    IndexT counter = 0;
    for (IndexT i = nodes.begin; i < nodes.end; i++)
    {
        Models::PrimitiveNode* pNode = static_cast<Models::PrimitiveNode*>(Models::ModelContext::NodeInstances.renderable.nodes[i]);
        Resources::CreateResourceListener(pNode->GetMeshResource(), [flags, mask, offset = alloc.offset, counter, i, pNode](Resources::ResourceId id)
        {
            Threading::CriticalScope _s(&state.blasLock);
            CoreGraphics::MeshResourceId meshRes = id;

            // Setup material
            Materials::MaterialId mat = pNode->GetMaterial();
            const MaterialTemplates::Entry* temp = Materials::MaterialGetTemplate(mat);
            IndexT bufferBinding = Materials::MaterialGetBufferBinding(mat);

            // Create Blas if we haven't registered it yet
            CoreGraphics::MeshId mesh = MeshResourceGetMesh(meshRes, pNode->GetMeshIndex());
            IndexT blasIndex = state.blasLookup.FindIndex(mesh);
            if (blasIndex == InvalidIndex)
            {
                const CoreGraphics::VertexLayoutId layout = CoreGraphics::MeshGetVertexLayout(mesh);
                auto& comps = CoreGraphics::VertexLayoutGetComponents(layout);

                CoreGraphics::BlasCreateInfo createInfo;
                createInfo.vbo = CoreGraphics::GetVertexBuffer();
                createInfo.ibo = CoreGraphics::GetIndexBuffer();
                createInfo.indexType = CoreGraphics::MeshGetIndexType(mesh);
                createInfo.positionsFormat = comps[0].GetFormat();
                createInfo.stride = CoreGraphics::VertexLayoutGetStreamSize(layout, 0);
                createInfo.vertexOffset = CoreGraphics::MeshGetVertexOffset(mesh, 0);
                createInfo.indexOffset = CoreGraphics::MeshGetIndexOffset(mesh);
                createInfo.primitiveGroups = CoreGraphics::MeshGetPrimitiveGroups(mesh);
                createInfo.flags = CoreGraphics::AccelerationStructureBuildFlags::FastTrace;
                CoreGraphics::BlasId blas = CoreGraphics::CreateBlas(createInfo);
                state.blases.Append(blas);
                blasIndex = state.blasLookup.Add(mesh, Util::MakeTuple(1, blas));

                CoreGraphics::BlasIdRelease(blas);
                state.blasesToRebuild.Append(blas);
            }

            // Increment ref count
            auto& [refCount, blas] = state.blasLookup.ValueAtIndex(mesh, blasIndex);
            refCount++;

            Raytracetest::Object constants;
            constants.MaterialOffset = bufferBinding;

            CoreGraphics::BufferIdLock _1(CoreGraphics::GetVertexBuffer());
            CoreGraphics::BufferIdLock _2(CoreGraphics::GetIndexBuffer());

            // Because the smallest machine unit is 4 bytes, the offset must be in integers, not in bytes
            CoreGraphics::PrimitiveGroup group = CoreGraphics::MeshGetPrimitiveGroup(mesh, counter);
            constants.Use16BitIndex = CoreGraphics::MeshGetIndexType(mesh) == CoreGraphics::IndexType::Index16 ? 1 : 0;
            constants.PositionsPtr = CoreGraphics::BufferGetDeviceAddress(CoreGraphics::GetVertexBuffer()) + CoreGraphics::MeshGetVertexOffset(mesh, 0);
            constants.AttrPtr = CoreGraphics::BufferGetDeviceAddress(CoreGraphics::GetVertexBuffer()) + CoreGraphics::MeshGetVertexOffset(mesh, 1);
            constants.IndexPtr = CoreGraphics::BufferGetDeviceAddress(CoreGraphics::GetIndexBuffer()) + CoreGraphics::MeshGetIndexOffset(mesh);
            state.objects[offset + counter] = constants;

            // Setup instance
            CoreGraphics::BlasInstanceCreateInfo createInfo;
            createInfo.flags = flags;
            createInfo.mask = mask;
            createInfo.shaderOffset = MaterialPropertyMappings[(uint)temp->properties];
            createInfo.instanceIndex = offset;
            createInfo.blas = blas;
            createInfo.transform = Models::ModelContext::NodeInstances.transformable.nodeTransforms[Models::ModelContext::NodeInstances.renderable.nodeTransformIndex[i]];

            // Disable instance if the vertex layout isn't supported
            if (temp->vertexLayout != CoreGraphics::VertexLayoutType::Normal)
                createInfo.mask = 0x0;

            CoreGraphics::BlasIdLock _0(createInfo.blas);
            state.blasInstances[offset + counter] = CoreGraphics::CreateBlasInstance(createInfo);
            state.blasInstanceMeshes[offset + counter] = mesh;

            state.numRegisteredInstances++;
            state.topLevelNeedsReconstruction = true;
        });
        counter++;
    }
    state.topLevelNeedsReconstruction = true;
}

//------------------------------------------------------------------------------
/**
*/
void
RaytracingContext::SetupTerrain(
    const Graphics::GraphicsEntityId id
    , const CoreGraphics::VertexComponent::Format format
    , const CoreGraphics::IndexType::Code indexType
    , const CoreGraphics::VertexAlloc& vertices
    , const CoreGraphics::VertexAlloc& indices
    , const CoreGraphics::PrimitiveGroup& patchPrimGroup
    , SizeT vertexOffsetStride
    , SizeT patchVertexStride
    , Util::Array<Math::mat4> transforms
    , MaterialInterface::TerrainMaterial material
)
{
    Graphics::ContextEntityId contextId = GetContextId(id);

    state.blasLock.Enter();
    Memory::RangeAllocation alloc = state.blasInstanceAllocator.Alloc(transforms.Size());
    state.blasInstances.Extend(alloc.offset + transforms.Size());
    state.blasInstanceMeshes.Extend(alloc.offset + transforms.Size());
    state.objects.Extend(alloc.offset + transforms.Size());

    raytracingContextAllocator.Set<Raytracing_Allocation>(contextId.id, alloc);
    raytracingContextAllocator.Set<Raytracing_NumStructures>(contextId.id, transforms.Size());
    raytracingContextAllocator.Set<Raytracing_ObjectType>(contextId.id, ObjectType::Static);

    // For each patch, setup a separate BLAS
    Util::Array<CoreGraphics::BlasId> blases;
    IndexT patchCounter = 0;
    for (IndexT i = alloc.offset; i < alloc.offset + transforms.Size(); i++)
    {
        // Create BLAS for terrain
        CoreGraphics::BlasCreateInfo createInfo;
        createInfo.vbo = CoreGraphics::GetVertexBuffer();
        createInfo.ibo = CoreGraphics::GetIndexBuffer();
        createInfo.indexType = indexType;
        createInfo.positionsFormat = format;
        createInfo.stride = vertexOffsetStride;
        createInfo.vertexOffset = vertices.offset + patchCounter * patchVertexStride;
        createInfo.indexOffset = indices.offset;
        createInfo.flags = CoreGraphics::AccelerationStructureBuildFlags::FastTrace;
        createInfo.primitiveGroups = { patchPrimGroup };
        CoreGraphics::BlasId blas = CoreGraphics::CreateBlas(createInfo);
        blases.Append(blas);
        state.blasesToRebuild.Append(blas);
        CoreGraphics::BlasIdLock _0(blas);

        CoreGraphics::BlasInstanceCreateInfo instanceCreateInfo;
        instanceCreateInfo.flags = CoreGraphics::BlasInstanceFlags::NoFlags;
        instanceCreateInfo.mask = 0xFF;
        instanceCreateInfo.shaderOffset = MaterialPropertyMappings[(uint)Materials::MaterialProperties::Terrain];
        instanceCreateInfo.instanceIndex = 0;
        instanceCreateInfo.blas = blas;
        instanceCreateInfo.transform = transforms[patchCounter];
        state.blasInstances[i] = CoreGraphics::CreateBlasInstance(instanceCreateInfo);
        state.blasInstanceMeshes[i] = CoreGraphics::InvalidMeshId;

        // Update buffer
        CoreGraphics::BlasInstanceIdLock _1(state.blasInstances[i]);
        CoreGraphics::BlasInstanceUpdate(state.blasInstances[i], transforms[patchCounter], state.blasInstanceBuffer.HostBuffer(), i * CoreGraphics::BlasInstanceGetSize());

        CoreGraphics::BufferIdLock _2(CoreGraphics::GetVertexBuffer());
        CoreGraphics::BufferIdLock _3(CoreGraphics::GetIndexBuffer());

        Raytracetest::Object constants;
        constants.Use16BitIndex = indexType == CoreGraphics::IndexType::Index16 ? 1 : 0;
        constants.MaterialOffset = Materials::MaterialLoader::RegisterTerrainMaterial(material);
        constants.IndexPtr = CoreGraphics::BufferGetDeviceAddress(CoreGraphics::GetIndexBuffer()) + indices.offset;
        constants.PositionsPtr = CoreGraphics::BufferGetDeviceAddress(CoreGraphics::GetVertexBuffer()) + vertices.offset + patchCounter * vertexOffsetStride;
        state.objects[i] = constants;

        state.numRegisteredInstances++;
        patchCounter++;
    }
    state.terrainBlases.Add(contextId, blases);
    state.blasLock.Leave();
    state.topLevelNeedsReconstruction = true;
}

//------------------------------------------------------------------------------
/**
*/
void
RaytracingContext::ReconstructTopLevelAcceleration(const Graphics::FrameContext& ctx)
{
    if (!CoreGraphics::RayTracingSupported)
        return;

    if (state.topLevelNeedsReconstruction)
    {
        if (state.toplevelAccelerationStructure != CoreGraphics::InvalidTlasId)
        {
            CoreGraphics::DestroyTlas(state.toplevelAccelerationStructure);
        }
        CoreGraphics::TlasCreateInfo createInfo;
        createInfo.numInstances = state.blasInstances.Size();
        createInfo.instanceBuffer = state.blasInstanceBuffer.deviceBuffer;
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
            CoreGraphics::ResourceTableTexture(state.raytracingTestOutput, Raytracetest::Table_Batch::RaytracingOutput_SLOT)
        );

        CoreGraphics::ResourceTableSetRWBuffer(
            state.raytracingTestTables.tables[ctx.bufferIndex],
            CoreGraphics::ResourceTableBuffer(state.geometryBindingBuffer, Raytracetest::Table_Batch::Geometry_SLOT)
        );

        CoreGraphics::ResourceTableSetRWBuffer(
            state.raytracingTestTables.tables[ctx.bufferIndex],
            CoreGraphics::ResourceTableBuffer(Materials::MaterialLoader::GetMaterialBindingBuffer(), Raytracetest::Table_Batch::MaterialBindings_SLOT)
        );

        CoreGraphics::ResourceTableSetRWBuffer(
            state.raytracingTestTables.tables[ctx.bufferIndex],
            CoreGraphics::ResourceTableBuffer(state.objectBindingBuffer.DeviceBuffer(), Raytracetest::Table_Batch::ObjectBuffer_SLOT)
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
    if (!CoreGraphics::RayTracingSupported)
        return;

    LightGridCs::ClusterUniforms uniformData;
    uniformData.NumCells[0] = 64;
    uniformData.NumCells[1] = 64;
    uniformData.NumCells[2] = 64;
    CoreGraphics::BufferUpdate(state.lightGridConstants, uniformData);
    CoreGraphics::BufferFlush(state.lightGridConstants);

    const Util::Array<Graphics::GraphicsEntityId>& entities = RaytracingContext::__state.entities;

    if (!entities.IsEmpty() && state.toplevelAccelerationStructure != CoreGraphics::InvalidTlasId)
    {
        static Util::Array<uint32> nodes;
        nodes.Clear();
        nodes.Resize(entities.Size());

        static Threading::AtomicCounter idCounter;
        idCounter = 1;

        // Run job to collect model node ids
        Jobs2::JobDispatch(
            [
                ids = entities.Begin()
            ]
        (SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset) mutable
        {
            N_SCOPE(RaytracingTransformJob, Graphics);
            for (IndexT i = 0; i < groupSize; i++)
            {
                IndexT index = i + invocationOffset;
                if (index >= totalJobs)
                    return;

                // Get node range and update ids buffer
                Graphics::GraphicsEntityId gid = ids[index];
                Graphics::ContextEntityId cid = GetContextId(gid);
                const ObjectType type = raytracingContextAllocator.Get<Raytracing_ObjectType>(cid.id);
                const Memory::RangeAllocation alloc = raytracingContextAllocator.Get<Raytracing_Allocation>(cid.id);
                const SizeT numObjects = raytracingContextAllocator.Get<Raytracing_NumStructures>(cid.id);
                if (numObjects == 0)
                    continue;

                if (type == ObjectType::Static)
                {
                    for (IndexT j = 0; j < numObjects; j++)
                    {
                        Math::mat4 transform;
                        CoreGraphics::BlasInstanceIdLock _0(state.blasInstances[alloc.offset + j]);
                        CoreGraphics::BlasInstanceUpdate(state.blasInstances[alloc.offset + j], state.blasInstanceBuffer.HostBuffer(), (alloc.offset + j) * CoreGraphics::BlasInstanceGetSize());
                    }
                }
                else
                {
                    const Models::NodeInstanceRange& renderableRange = Models::ModelContext::GetModelRenderableRange(gid);
                    const Models::NodeInstanceRange& transformableRange = Models::ModelContext::GetModelTransformableRange(gid);

                    const Models::ModelContext::ModelInstance::Renderable& renderables = Models::ModelContext::GetModelRenderables();
                    const Models::ModelContext::ModelInstance::Transformable& transformables = Models::ModelContext::GetModelTransformables();

                    const uint numNodes = renderableRange.end - renderableRange.begin;
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
            }
        }, entities.Size(), 1024, { &Models::ModelContext::TransformsUpdateCounter }, &idCounter, &state.jobWaitEvent);

        // Copy over object bindings
        void* objectData = CoreGraphics::BufferMap(state.objectBindingBuffer.HostBuffer());
        memcpy(objectData, state.objects.Begin(), state.objects.ByteSize());
        CoreGraphics::BufferUnmap(state.objectBindingBuffer.HostBuffer());

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
    if (!CoreGraphics::RayTracingSupported)
        return;

    N_MARKER_BEGIN(WaitForRaytracingJobs, Graphics);
    state.jobWaitEvent.Wait();
    N_MARKER_END();
}

//------------------------------------------------------------------------------
/**
*/
void
RaytracingContext::UpdateViewResources(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx)
{
    if (!CoreGraphics::RayTracingSupported)
        return;

    IndexT tickCbo, viewCbo, shadowCbo;
    Graphics::GetOffsets(tickCbo, viewCbo, shadowCbo);
    ResourceTableSetConstantBuffer(state.lightGridResourceTables.tables[ctx.bufferIndex], { CoreGraphics::GetConstantBuffer(ctx.bufferIndex), LightGridCs::Table_Frame::ViewConstants_SLOT, 0, sizeof(LightGridCs::ViewConstants), (SizeT)viewCbo });
    ResourceTableSetConstantBuffer(state.lightGridResourceTables.tables[ctx.bufferIndex], { CoreGraphics::GetConstantBuffer(ctx.bufferIndex), LightGridCs::Table_Frame::ShadowViewConstants_SLOT, 0, sizeof(LightGridCs::ShadowViewConstants), (SizeT)shadowCbo });
    ResourceTableCommitChanges(state.lightGridResourceTables.tables[ctx.bufferIndex]);
}

//------------------------------------------------------------------------------
/**
*/
void
RaytracingContext::OnRenderDebug(uint32_t flags)
{
    if (!CoreGraphics::RayTracingSupported)
        return;
}

//------------------------------------------------------------------------------
/**
*/
void
RaytracingContext::Dealloc(Graphics::ContextEntityId id)
{
    if (!CoreGraphics::RayTracingSupported)
        return;

    // clean up old stuff, but don't deallocate entity
    Memory::RangeAllocation range = raytracingContextAllocator.Get<Raytracing_Allocation>(id.id);
    SizeT numAllocs = raytracingContextAllocator.Get<Raytracing_NumStructures>(id.id);
    for (IndexT i = range.offset; i < numAllocs; i++)
    {
        CoreGraphics::DestroyBlasInstance(state.blasInstances[i]);
        CoreGraphics::MeshId mesh = state.blasInstanceMeshes[i];

        // Delete a mesh
        if (mesh != CoreGraphics::InvalidMeshId)
        {
            auto& [refCount, blas] = state.blasLookup.ValueAtIndex(mesh, i);
            if (refCount == 1)
            {
                CoreGraphics::DestroyBlas(blas);
                state.blasLookup.EraseIndex(mesh, i);
            }
            else
                refCount--;
        }
        else
        {
            IndexT blasesListIndex = state.terrainBlases.FindIndex(id);
            n_assert(blasesListIndex != InvalidIndex);
            const Util::Array<CoreGraphics::BlasId> blases = state.terrainBlases.ValueAtIndex(id, blasesListIndex);
            for (IndexT j = 0; j < blases.Size(); j++)
            {
                CoreGraphics::DestroyBlas(blases[j]);
            }
        }
        state.blasInstances[i] = CoreGraphics::InvalidBlasInstanceId;
    }
    
    raytracingContextAllocator.Dealloc(id.id);
    state.topLevelNeedsReconstruction = true;
}

} // namespace Raytracing
