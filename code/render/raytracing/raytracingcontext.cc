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
#include "materials/shaderconfig.h"
#include "materials/materialloader.h"

#include "render/raytracing/shaders/raytracetest.h"
#include "render/raytracing/shaders/brdfhit.h"
#include "render/raytracing/shaders/bsdfhit.h"
#include "render/raytracing/shaders/gltfhit.h"

#include "render/raytracing/shaders/light_grid_cs.h"

#include "frame/default.h"
#include "lighting/lightcontext.h"

namespace Raytracing
{

RaytracingContext::RaytracingAllocator RaytracingContext::raytracingContextAllocator;
__ImplementContext(RaytracingContext, raytracingContextAllocator);

static const uint NUM_GRID_CELLS = 64;
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

    Util::HashTable<CoreGraphics::MeshId, Util::Tuple<uint, Util::Array<CoreGraphics::BlasId>>> blasLookup;
    CoreGraphics::BufferWithStaging blasInstanceBuffer;

    CoreGraphics::ResourceTableSet raytracingTables;
    CoreGraphics::ResourceTableId raytracingTestOutputTable;

    CoreGraphics::BufferId geometryBindingBuffer;
    CoreGraphics::BufferWithStaging objectBindingBuffer;
    Util::Array<Raytracetest::Object> objects;

    CoreGraphics::BufferId gridBuffer;
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

static uint MaterialPropertyMappings[(uint)MaterialTemplates::MaterialProperties::Num];
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
#ifndef PUBLIC_BUILD
    __bundle.OnRenderDebug = RaytracingContext::OnRenderDebug;
#endif
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);


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

    Util::Array<CoreGraphics::ShaderProgramId> shaderMappings((SizeT)MaterialTemplates::MaterialProperties::Num + 1, 0);
    shaderMappings.Append(raygenProgram);
    shaderMappings.Append(brdfHitProgram);
    shaderMappings.Append(bsdfHitProgram);
    shaderMappings.Append(gltfHitProgram);
    shaderMappings.Append(terrainHitProgram);

    uint bindingCounter = 0;
    MaterialPropertyMappings[(uint)MaterialTemplates::MaterialProperties::BRDF] = bindingCounter++;
    MaterialPropertyMappings[(uint)MaterialTemplates::MaterialProperties::BSDF] = bindingCounter++;
    MaterialPropertyMappings[(uint)MaterialTemplates::MaterialProperties::GLTF] = bindingCounter++;
    MaterialPropertyMappings[(uint)MaterialTemplates::MaterialProperties::BlendAdd] = 0xFFFFFFFF;
    MaterialPropertyMappings[(uint)MaterialTemplates::MaterialProperties::Skybox] = 0xFFFFFFFF;
    MaterialPropertyMappings[(uint)MaterialTemplates::MaterialProperties::Terrain] = bindingCounter++;

    state.raytracingTables = CoreGraphics::ShaderCreateResourceTableSet(raygenShader, NEBULA_BATCH_GROUP, 3, "Raytracing Descriptors");
    state.raytracingTestOutputTable = CoreGraphics::ShaderCreateResourceTable(raygenShader, NEBULA_SYSTEM_GROUP);
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

    CoreGraphics::BufferCreateInfo gridBufferInfo;
    gridBufferInfo.name = "RaytracingAABBGrid";
    gridBufferInfo.size = NUM_GRID_CELLS * NUM_GRID_CELLS * NUM_GRID_CELLS;
    gridBufferInfo.elementSize = sizeof(LightGridCs::ClusterAABB);
    gridBufferInfo.mode = CoreGraphics::BufferAccessMode::DeviceLocal;
    gridBufferInfo.usageFlags = CoreGraphics::ReadWriteBuffer;
    gridBufferInfo.queueSupport = CoreGraphics::GraphicsQueueSupport | CoreGraphics::ComputeQueueSupport;
    state.gridBuffer = CoreGraphics::CreateBuffer(gridBufferInfo);

    CoreGraphics::BufferCreateInfo indexListInfo;
    indexListInfo.name = "RaytracingLightIndexListsBuffer";
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

    state.lightGridResourceTables = CoreGraphics::ShaderCreateResourceTableSet(state.lightGridShader, NEBULA_FRAME_GROUP, 3, "Raytracing Grid Descriptor Set");
    for (IndexT i = 0; i < CoreGraphics::GetNumBufferedFrames(); i++)
    {
        ResourceTableSetRWBuffer(state.lightGridResourceTables.tables[i], { state.gridBuffer, LightGridCs::Table_Frame::ClusterAABBs_SLOT, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
        ResourceTableSetConstantBuffer(state.lightGridResourceTables.tables[i], { CoreGraphics::GetConstantBuffer(i), LightGridCs::Table_Frame::LightUniforms_SLOT, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
        ResourceTableSetRWBuffer(state.lightGridResourceTables.tables[i], { state.lightGridIndexLists, LightGridCs::Table_Frame::LightIndexLists_SLOT, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
        ResourceTableSetConstantBuffer(state.lightGridResourceTables.tables[i], { state.lightGridConstants, LightGridCs::Table_Frame::ClusterUniforms_SLOT, 0, sizeof(LightGridCs::ClusterUniforms), 0 });
        ResourceTableCommitChanges(state.lightGridResourceTables.tables[i]);
    }

    // Create buffers for updating blas instances
    CoreGraphics::BufferCreateInfo bufInfo;
    bufInfo.name = "BLAS Instance Buffer";
    bufInfo.elementSize = CoreGraphics::BlasInstanceGetSize();
    bufInfo.size = settings.maxNumAllowedInstances;  // This is a virtual max-size, as this buffer is virtual memory managed
    bufInfo.queueSupport = CoreGraphics::BufferQueueSupport::ComputeQueueSupport | CoreGraphics::BufferQueueSupport::GraphicsQueueSupport;
    bufInfo.usageFlags = CoreGraphics::BufferUsageFlag::ShaderAddress | CoreGraphics::BufferUsageFlag::AccelerationStructureInstances;
    state.blasInstanceBuffer = CoreGraphics::BufferWithStaging(bufInfo);

    CoreGraphics::BufferCreateInfo objectBindingBufferCreateInfo;
    objectBindingBufferCreateInfo.name = "Raytracing Object Binding Buffer";
    objectBindingBufferCreateInfo.byteSize = sizeof(Raytracetest::Object) * settings.maxNumAllowedInstances;
    objectBindingBufferCreateInfo.usageFlags = CoreGraphics::BufferUsageFlag::ShaderAddress | CoreGraphics::BufferUsageFlag::ReadWriteBuffer;
    objectBindingBufferCreateInfo.queueSupport = CoreGraphics::BufferQueueSupport::ComputeQueueSupport;
    state.objectBindingBuffer = CoreGraphics::BufferWithStaging(objectBindingBufferCreateInfo);

    FrameScript_default::Bind_RayTracingObjectBindings(state.objectBindingBuffer.DeviceBuffer());
    FrameScript_default::Bind_GridBuffer(state.gridBuffer);
    FrameScript_default::Bind_GridLightIndexLists(state.lightGridIndexLists);

    FrameScript_default::RegisterSubgraph_RaytracingLightGridGen_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CoreGraphics::CmdSetShaderProgram(cmdBuf, state.lightGridGenProgram);
        CoreGraphics::CmdSetResourceTable(cmdBuf, state.lightGridResourceTables.tables[bufferIndex], NEBULA_FRAME_GROUP, CoreGraphics::ComputePipeline, nullptr);
        CoreGraphics::CmdDispatch(cmdBuf, NUM_GRID_CELLS * NUM_GRID_CELLS, 1, 1);
    }, {
        { FrameScript_default::BufferIndex::GridBuffer, CoreGraphics::PipelineStage::ComputeShaderWrite }
    });

    FrameScript_default::RegisterSubgraph_RaytracingLightGridCull_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CoreGraphics::CmdSetShaderProgram(cmdBuf, state.lightGridCullProgram);
        CoreGraphics::CmdSetResourceTable(cmdBuf, state.lightGridResourceTables.tables[bufferIndex], NEBULA_FRAME_GROUP, CoreGraphics::ComputePipeline, nullptr);
        CoreGraphics::CmdDispatch(cmdBuf, NUM_GRID_CELLS*NUM_GRID_CELLS, 1, 1);
    }, {
        { FrameScript_default::BufferIndex::GridBuffer, CoreGraphics::PipelineStage::ComputeShaderRead }
        , { FrameScript_default::BufferIndex::LightList, CoreGraphics::PipelineStage::ComputeShaderRead }
        , { FrameScript_default::BufferIndex::GridLightIndexLists, CoreGraphics::PipelineStage::ComputeShaderWrite }
    });

    FrameScript_default::RegisterSubgraph_RaytracingStructuresUpdate_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
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
                        CoreGraphics::AccelerationStructureBarrierInfo{ .blas = state.blasesToRebuild[i], .type = CoreGraphics::AccelerationStructureBarrierInfo::BlasBarrier }
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
                        CoreGraphics::AccelerationStructureBarrierInfo{ .blas = state.blasesToRebuild[i], .type = CoreGraphics::AccelerationStructureBarrierInfo::BlasBarrier }
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
                    CoreGraphics::AccelerationStructureBarrierInfo{ .tlas = state.toplevelAccelerationStructure, .type = CoreGraphics::AccelerationStructureBarrierInfo::TlasBarrier }
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
                    CoreGraphics::AccelerationStructureBarrierInfo{ .tlas = state.toplevelAccelerationStructure, .type = CoreGraphics::AccelerationStructureBarrierInfo::TlasBarrier }
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
                    CoreGraphics::AccelerationStructureBarrierInfo{ .tlas = state.toplevelAccelerationStructure, .type = CoreGraphics::AccelerationStructureBarrierInfo::TlasBarrier }
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
                    CoreGraphics::AccelerationStructureBarrierInfo{ .tlas = state.toplevelAccelerationStructure, .type = CoreGraphics::AccelerationStructureBarrierInfo::TlasBarrier }
                }
            );
        }
        state.blasLock.Leave();
    }, {
        { FrameScript_default::BufferIndex::RayTracingObjectBindings, CoreGraphics::PipelineStage::TransferWrite }
    });

    FrameScript_default::RegisterSubgraph_RaytracingTest_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        if (state.toplevelAccelerationStructure != CoreGraphics::InvalidTlasId)
        {
            CoreGraphics::CmdBarrier(cmdBuf, CoreGraphics::PipelineStage::AllShadersRead, CoreGraphics::PipelineStage::RayTracingShaderWrite, CoreGraphics::BarrierDomain::Global, { CoreGraphics::TextureBarrierInfo{.tex = FrameScript_default::Texture_RayTracingTestOutput(), .subres = CoreGraphics::TextureSubresourceInfo::ColorNoMipNoLayer() }}, nullptr, nullptr);
            CoreGraphics::CmdSetRayTracingPipeline(cmdBuf, state.raytracingBundle.pipeline);
            CoreGraphics::CmdSetResourceTable(cmdBuf,  state.raytracingTestOutputTable, NEBULA_SYSTEM_GROUP, CoreGraphics::RayTracingPipeline, nullptr);
            CoreGraphics::CmdSetResourceTable(cmdBuf, state.raytracingTables.tables[bufferIndex], NEBULA_BATCH_GROUP, CoreGraphics::RayTracingPipeline, nullptr);
            CoreGraphics::CmdSetResourceTable(cmdBuf, state.lightGridResourceTables.tables[bufferIndex], NEBULA_FRAME_GROUP, CoreGraphics::RayTracingPipeline, nullptr);
            CoreGraphics::CmdRaysDispatch(cmdBuf, state.raytracingBundle.table, 640, 480, 1);
            CoreGraphics::CmdBarrier(cmdBuf, CoreGraphics::PipelineStage::RayTracingShaderWrite, CoreGraphics::PipelineStage::AllShadersRead, CoreGraphics::BarrierDomain::Global, { CoreGraphics::TextureBarrierInfo{.tex = FrameScript_default::Texture_RayTracingTestOutput(), .subres = CoreGraphics::TextureSubresourceInfo::ColorNoMipNoLayer() }}, nullptr, nullptr);
        }
    }, {
        { FrameScript_default::BufferIndex::GridLightIndexLists, CoreGraphics::PipelineStage::RayTracingShaderRead }
        , { FrameScript_default::BufferIndex::GridBuffer, CoreGraphics::PipelineStage::RayTracingShaderRead }
        , { FrameScript_default::BufferIndex::RayTracingObjectBindings, CoreGraphics::PipelineStage::RayTracingShaderRead }
    });


    state.maxAllowedInstances = settings.maxNumAllowedInstances;
    state.topLevelNeedsReconstruction = true;
    state.blasInstanceAllocator = Memory::RangeAllocator(0xFFFFF, settings.maxNumAllowedInstances);
}

//------------------------------------------------------------------------------
/**
*/
void
RaytracingContext::Discard()
{
    state.raytracingTables.Discard();
    state.lightGridResourceTables.Discard();
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
    raytracingContextAllocator.Set<Raytracing_UpdateType>(contextId.id, UpdateType::Dynamic);

    IndexT instanceCounter = 0;
    for (IndexT i = nodes.begin; i < nodes.end; i++)
    {
        Models::PrimitiveNode* pNode = static_cast<Models::PrimitiveNode*>(Models::ModelContext::NodeInstances.renderable.nodes[i]);
        Resources::CreateResourceListener(pNode->GetMeshResource(), [flags, mask, offset = alloc.offset, instanceCounter, i, pNode](Resources::ResourceId id)
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
                // Reset primitive group counter
                const CoreGraphics::VertexLayoutId layout = CoreGraphics::MeshGetVertexLayout(mesh);
                auto& comps = CoreGraphics::VertexLayoutGetComponents(layout);

                const Util::Array<CoreGraphics::PrimitiveGroup>& primGroups = CoreGraphics::MeshGetPrimitiveGroups(mesh);
                Util::Array<CoreGraphics::BlasId> blasesCreated;
                blasesCreated.Reserve(primGroups.Size());
                for (auto& group : primGroups)
                {
                    CoreGraphics::BlasCreateInfo createInfo;
                    createInfo.vbo = CoreGraphics::MeshGetVertexBuffer(mesh, 0);
                    createInfo.ibo = CoreGraphics::MeshGetIndexBuffer(mesh);
                    createInfo.indexType = CoreGraphics::MeshGetIndexType(mesh);
                    createInfo.positionsFormat = comps[0].GetFormat();
                    createInfo.stride = CoreGraphics::VertexLayoutGetStreamSize(layout, 0);
                    createInfo.vertexOffset = CoreGraphics::MeshGetVertexOffset(mesh, 0);
                    createInfo.indexOffset = CoreGraphics::MeshGetIndexOffset(mesh);
                    createInfo.primGroup = group;
                    createInfo.flags = CoreGraphics::AccelerationStructureBuildFlags::FastTrace;
                    CoreGraphics::BlasId blas = CoreGraphics::CreateBlas(createInfo);
                    blasesCreated.Append(blas);
                    state.blases.Append(blas);
                    CoreGraphics::BlasIdRelease(blas);
                    state.blasesToRebuild.Append(blas);
                }
                blasIndex = state.blasLookup.Add(mesh, Util::MakeTuple(1, blasesCreated));
            }

            // Increment ref count
            auto& [refCount, blases] = state.blasLookup.ValueAtIndex(mesh, blasIndex);
            refCount++;

            Raytracetest::Object constants;
            constants.MaterialOffset = bufferBinding;

            CoreGraphics::BufferIdLock _1(CoreGraphics::GetVertexBuffer());
            CoreGraphics::BufferIdLock _2(CoreGraphics::GetIndexBuffer());

            // Because the smallest machine unit is 4 bytes, the offset must be in integers, not in bytes
            CoreGraphics::IndexType::Code indexType = CoreGraphics::MeshGetIndexType(mesh);
            uint positionsStride = (uint)CoreGraphics::VertexLayoutGetStreamSize(CoreGraphics::MeshGetVertexLayout(mesh), 0);
            uint attributeStride = (uint)CoreGraphics::VertexLayoutGetStreamSize(CoreGraphics::MeshGetVertexLayout(mesh), 1);
            CoreGraphics::PrimitiveGroup group = CoreGraphics::MeshGetPrimitiveGroup(mesh, pNode->GetPrimitiveGroupIndex());
            constants.Use16BitIndex =  indexType == CoreGraphics::IndexType::Index16 ? 1 : 0;
            CoreGraphics::DeviceAddress positionsAddress = CoreGraphics::BufferGetDeviceAddress(CoreGraphics::GetVertexBuffer()) + CoreGraphics::MeshGetVertexOffset(mesh, 0);
            memcpy(constants.PositionsPtr, &positionsAddress, sizeof(CoreGraphics::DeviceAddress));
            CoreGraphics::DeviceAddress attributeAddress = CoreGraphics::BufferGetDeviceAddress(CoreGraphics::GetVertexBuffer()) + CoreGraphics::MeshGetVertexOffset(mesh, 1);
            memcpy(constants.AttrPtr, &attributeAddress, sizeof(CoreGraphics::DeviceAddress));
            CoreGraphics::DeviceAddress indexAddress = CoreGraphics::BufferGetDeviceAddress(CoreGraphics::GetIndexBuffer()) + CoreGraphics::MeshGetIndexOffset(mesh);
            memcpy(constants.IndexPtr, &indexAddress, sizeof(CoreGraphics::DeviceAddress));
            constants.AttributeStride = attributeStride;
            constants.VertexLayout = (uint)temp->vertexLayout;
            constants.BaseIndexOffset = (CoreGraphics::DeviceAddress)group.GetBaseIndex() * CoreGraphics::IndexType::SizeOf(indexType);
            constants.BaseVertexPositionOffset = (CoreGraphics::DeviceAddress)group.GetBaseVertex() * positionsStride;
            constants.BaseVertexAttributeOffset = (CoreGraphics::DeviceAddress)group.GetBaseVertex() * attributeStride;

            uint instanceIndex = offset + instanceCounter;
            state.objects[instanceIndex] = constants;

            // Setup instance
            CoreGraphics::BlasInstanceCreateInfo createIntInfo;
            createIntInfo.flags = flags;
            createIntInfo.mask = mask;
            createIntInfo.shaderOffset = MaterialPropertyMappings[(uint)temp->properties];
            createIntInfo.instanceIndex = instanceIndex;
            createIntInfo.blas = blases[pNode->GetPrimitiveGroupIndex()];
            createIntInfo.transform = Models::ModelContext::NodeInstances.transformable.nodeTransforms[Models::ModelContext::NodeInstances.renderable.nodeTransformIndex[i]];

            // Disable instance if the vertex layout isn't supported
            CoreGraphics::BlasIdLock _0(createIntInfo.blas);
            state.blasInstances[instanceIndex] = CoreGraphics::CreateBlasInstance(createIntInfo);
            state.blasInstanceMeshes[instanceIndex] = mesh;

            state.numRegisteredInstances++;
            state.topLevelNeedsReconstruction = true;
        });
        instanceCounter++;
    }
    state.topLevelNeedsReconstruction = true;
}

//------------------------------------------------------------------------------
/**
*/
void RaytracingContext::SetupMesh(
    const Graphics::GraphicsEntityId id
    , const UpdateType objectType
    , const CoreGraphics::VertexComponent::Format format
    , const CoreGraphics::IndexType::Code indexType
    , const CoreGraphics::VertexAlloc& vertices
    , const CoreGraphics::VertexAlloc& indices
    , const CoreGraphics::PrimitiveGroup& patchPrimGroup
    , const SizeT vertexOffsetStride
    , const SizeT patchVertexStride
    , const Util::Array<Math::mat4> transforms
    , const uint materialTableOffset
    , const MaterialTemplates::MaterialProperties shader
    , const CoreGraphics::VertexLayoutType vertexLayout
)
{
    if (!CoreGraphics::RayTracingSupported)
        return;

    Graphics::ContextEntityId contextId = GetContextId(id);

    state.blasLock.Enter();
    Memory::RangeAllocation alloc = state.blasInstanceAllocator.Alloc(transforms.Size());
    state.blasInstances.Extend(alloc.offset + transforms.Size());
    state.blasInstanceMeshes.Extend(alloc.offset + transforms.Size());
    state.objects.Extend(alloc.offset + transforms.Size());

    raytracingContextAllocator.Set<Raytracing_Allocation>(contextId.id, alloc);
    raytracingContextAllocator.Set<Raytracing_NumStructures>(contextId.id, transforms.Size());
    raytracingContextAllocator.Set<Raytracing_UpdateType>(contextId.id, objectType);
    Util::FixedArray<CoreGraphics::BlasId>& blases = raytracingContextAllocator.Get<Raytracing_Blases>(contextId.id);
    blases.Resize(transforms.Size());

    // For each patch, setup a separate BLAS
    IndexT patchCounter = 0;
    for (IndexT i = alloc.offset; i < alloc.offset + transforms.Size(); i++)
    {
        // Create BLAS for each patch
        CoreGraphics::BlasCreateInfo createInfo;
        createInfo.vbo = CoreGraphics::GetVertexBuffer();
        createInfo.ibo = CoreGraphics::GetIndexBuffer();
        createInfo.indexType = indexType;
        createInfo.positionsFormat = format;
        createInfo.stride = vertexOffsetStride;
        createInfo.vertexOffset = vertices.offset + patchCounter * patchVertexStride;
        createInfo.indexOffset = indices.offset;
        createInfo.flags = CoreGraphics::AccelerationStructureBuildFlags::FastTrace;
        createInfo.primGroup = patchPrimGroup;
        CoreGraphics::BlasId blas = CoreGraphics::CreateBlas(createInfo);
        blases[patchCounter] = blas;
        state.blasesToRebuild.Append(blas);
        CoreGraphics::BlasIdLock _0(blas);

        CoreGraphics::BlasInstanceCreateInfo instanceCreateInfo;
        instanceCreateInfo.flags = CoreGraphics::BlasInstanceFlags::NoFlags;
        instanceCreateInfo.mask = 0xFF;
        instanceCreateInfo.shaderOffset = MaterialPropertyMappings[(uint)shader];
        instanceCreateInfo.instanceIndex = i;
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
        constants.MaterialOffset = materialTableOffset;
        CoreGraphics::DeviceAddress indexAddress = CoreGraphics::BufferGetDeviceAddress(CoreGraphics::GetIndexBuffer()) + createInfo.indexOffset;
        memcpy(constants.IndexPtr, &indexAddress, sizeof(CoreGraphics::DeviceAddress));
        CoreGraphics::DeviceAddress positionsAddress = CoreGraphics::BufferGetDeviceAddress(CoreGraphics::GetVertexBuffer()) + createInfo.vertexOffset;
        memcpy(constants.PositionsPtr, &positionsAddress, sizeof(CoreGraphics::DeviceAddress));
        constants.AttributeStride = 0x0;
        constants.BaseVertexPositionOffset = 0x0;
        constants.BaseVertexAttributeOffset = 0x0;
        constants.BaseIndexOffset = 0x0;
        constants.VertexLayout = (uint)vertexLayout;
        state.objects[i] = constants;

        state.numRegisteredInstances++;
        patchCounter++;
    }
    state.blasLock.Leave();
    state.topLevelNeedsReconstruction = true;
}

//------------------------------------------------------------------------------
/**
*/
void
RaytracingContext::InvalidateBLAS(const Graphics::GraphicsEntityId id)
{
    Graphics::ContextEntityId contextId = GetContextId(id);
    Util::FixedArray<CoreGraphics::BlasId>& blases = raytracingContextAllocator.Get<Raytracing_Blases>(contextId.id);
    state.blasesToRebuild.AppendArray(blases.Begin(), blases.Size());
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
        CoreGraphics::ResourceTableSetRWTexture(state.raytracingTestOutputTable,
            CoreGraphics::ResourceTableTexture(FrameScript_default::Texture_RayTracingTestOutput(), Raytracetest::Table_System::RaytracingOutput_SLOT)
        );
        CoreGraphics::ResourceTableSetAccelerationStructure(
            state.raytracingTables.tables[ctx.bufferIndex],
            CoreGraphics::ResourceTableTlas(state.toplevelAccelerationStructure, Raytracetest::Table_Batch::TLAS_SLOT)
        );

        CoreGraphics::ResourceTableSetRWBuffer(
            state.raytracingTables.tables[ctx.bufferIndex],
            CoreGraphics::ResourceTableBuffer(state.geometryBindingBuffer, Raytracetest::Table_Batch::Geometry_SLOT)
        );

        CoreGraphics::ResourceTableSetRWBuffer(
            state.raytracingTables.tables[ctx.bufferIndex],
            CoreGraphics::ResourceTableBuffer(Materials::MaterialLoader::GetMaterialBindingBuffer(), Raytracetest::Table_Batch::MaterialBindings_SLOT)
        );

        CoreGraphics::ResourceTableSetRWBuffer(
            state.raytracingTables.tables[ctx.bufferIndex],
            CoreGraphics::ResourceTableBuffer(state.objectBindingBuffer.DeviceBuffer(), Raytracetest::Table_Batch::ObjectBuffer_SLOT)
        );

        CoreGraphics::ResourceTableCommitChanges(state.raytracingTables.tables[ctx.bufferIndex]);
        CoreGraphics::ResourceTableCommitChanges(state.raytracingTestOutputTable);
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
    uniformData.NumCells[0] = NUM_GRID_CELLS;
    uniformData.NumCells[1] = NUM_GRID_CELLS;
    uniformData.NumCells[2] = NUM_GRID_CELLS;
    uniformData.BlockSize[0] = NUM_GRID_CELLS;
    uniformData.BlockSize[1] = 10.0f; // Size of grid cells
    CoreGraphics::BufferUpdate(state.lightGridConstants, uniformData);

    const Util::Array<Graphics::GraphicsEntityId>& entities = RaytracingContext::__state.entities;

    if (!entities.IsEmpty() && state.toplevelAccelerationStructure != CoreGraphics::InvalidTlasId)
    {
        static Util::Array<uint32_t> nodes;
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
                const UpdateType type = raytracingContextAllocator.Get<Raytracing_UpdateType>(cid.id);
                const Memory::RangeAllocation alloc = raytracingContextAllocator.Get<Raytracing_Allocation>(cid.id);
                const SizeT numObjects = raytracingContextAllocator.Get<Raytracing_NumStructures>(cid.id);
                if (numObjects == 0)
                    continue;

                if (type == UpdateType::Static)
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

    LightsCluster::LightUniforms uniforms = Lighting::LightContext::GetLightUniforms();
    uniforms.NumLightClusters = NUM_GRID_CELLS*NUM_GRID_CELLS*NUM_GRID_CELLS;
    uint64_t offset = CoreGraphics::SetConstants(uniforms);
    uint64_t tickCbo, viewCbo, shadowCbo;
    Graphics::GetOffsets(tickCbo, viewCbo, shadowCbo);
    ResourceTableSetConstantBuffer(state.lightGridResourceTables.tables[ctx.bufferIndex], { CoreGraphics::GetConstantBuffer(ctx.bufferIndex), Shared::Table_Frame::ViewConstants_SLOT, 0, sizeof(Shared::ViewConstants), viewCbo });
    ResourceTableSetConstantBuffer(state.lightGridResourceTables.tables[ctx.bufferIndex], { CoreGraphics::GetConstantBuffer(ctx.bufferIndex), Shared::Table_Frame::ShadowViewConstants_SLOT, 0, sizeof(Shared::ShadowViewConstants), shadowCbo });
    ResourceTableSetRWBuffer(state.lightGridResourceTables.tables[ctx.bufferIndex], { state.gridBuffer, Shared::Table_Frame::ClusterAABBs_SLOT, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
    ResourceTableSetRWBuffer(state.lightGridResourceTables.tables[ctx.bufferIndex], { state.lightGridIndexLists, Shared::Table_Frame::LightIndexLists_SLOT, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
    ResourceTableSetRWBuffer(state.lightGridResourceTables.tables[ctx.bufferIndex], { Lighting::LightContext::GetLightsBuffer(), Shared::Table_Frame::LightLists_SLOT, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
    ResourceTableSetConstantBuffer(state.lightGridResourceTables.tables[ctx.bufferIndex], { state.lightGridConstants, Shared::Table_Frame::ClusterUniforms_SLOT, 0, sizeof(Shared::ClusterUniforms), 0 });
    ResourceTableSetConstantBuffer(state.lightGridResourceTables.tables[ctx.bufferIndex], { CoreGraphics::GetConstantBuffer(ctx.bufferIndex), Shared::Table_Frame::LightUniforms_SLOT, 0, sizeof(Shared::LightUniforms), offset });
    ResourceTableCommitChanges(state.lightGridResourceTables.tables[ctx.bufferIndex]);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ResourceTableId
RaytracingContext::GetLightGridResourceTable(IndexT bufferIndex)
{
    return state.lightGridResourceTables.tables[bufferIndex];
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::TlasId
RaytracingContext::GetTLAS()
{
    return state.toplevelAccelerationStructure;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::BufferId
RaytracingContext::GetObjectBindingBuffer()
{
    return state.objectBindingBuffer.DeviceBuffer();
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ResourceTableId
RaytracingContext::GetRaytracingTable(const IndexT bufferIndex)
{
    return state.raytracingTables.tables[bufferIndex];
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
        CoreGraphics::BlasInstanceIdLock _0(state.blasInstances[i]);
        CoreGraphics::DestroyBlasInstance(state.blasInstances[i]);
        CoreGraphics::MeshId mesh = state.blasInstanceMeshes[i];

        // Delete a mesh
        if (mesh != CoreGraphics::InvalidMeshId)
        {
            IndexT index = state.blasLookup.FindIndex(mesh);
            if (index != InvalidIndex)
            {
                auto& [refCount, blases] = state.blasLookup.ValueAtIndex(mesh, index);
                if (refCount == 1)
                {
                    for (auto blas : blases)
                        CoreGraphics::DestroyBlas(blas);
                    state.blasLookup.EraseIndex(mesh, i);
                    state.blasInstanceMeshes[i] = CoreGraphics::InvalidMeshId;
                }
                else
                    refCount--;
            }
        }
        else
        {
            const Util::FixedArray<CoreGraphics::BlasId>& blases = raytracingContextAllocator.Get<Raytracing_Blases>(id.id);
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
