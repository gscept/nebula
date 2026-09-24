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
#include "coregraphics/meshloader.h"
#include "core/cvar.h"
#include "imgui.h"

#include "gpulang/render/raytracing/shaders/raytracetest.h"
#include "gpulang/render/raytracing/shaders/brdfhit.h"
#include "gpulang/render/raytracing/shaders/bsdfhit.h"
#include "gpulang/render/raytracing/shaders/gltfhit.h"
#include "gpulang/render/raytracing/shaders/light_grid_cs.h"


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
    Util::FixedArray<CoreGraphics::TlasId> toplevelAccelerationStructures;
    Memory::RangeAllocator blasInstanceAllocator;
    bool topLevelNeedsReconstruction, topLevelNeedsBuild, topLevelNeedsUpdate;

    Util::HashTable<CoreGraphics::MeshId, Util::Tuple<uint, Util::Array<CoreGraphics::BlasId>>> blasLookup;
    CoreGraphics::BufferWithStaging blasInstanceBuffer;

    CoreGraphics::ResourceTableSet raytracingTables;
    CoreGraphics::ResourceTableId raytracingTestOutputTable;

    CoreGraphics::BufferWithStaging objectBindingBuffer;
    Util::Array<Raytracetest::TlasInstance> objects;

    CoreGraphics::BufferId gridBuffer;
    CoreGraphics::BufferId lightGridConstants;
    CoreGraphics::ShaderId lightGridShader;
    CoreGraphics::ResourceTableSet lightGridResourceTables;
    CoreGraphics::ShaderProgramId lightGridGenProgram, lightGridCullProgram;
    CoreGraphics::BufferId lightGridIndexLists;

    CoreGraphics::PipelineRayTracingTable raytracingBundle;
    CoreGraphics::ShaderProgramId hitPrograms[NumObjectTypes];

    CoreGraphics::MeshId placeholderMesh;
    Materials::MaterialId placeholderMaterial;

    Threading::Event jobWaitEvent;

    SizeT maxAllowedInstances = 0;
    SizeT numRegisteredInstances = 0;
    SizeT pendingBlasSetups = 0;
    SizeT numInstancesToFlush;
    uint lastBuildInstanceCount = 0;
    uint lastBuildBlasCount = 0;
} state;

static uint MaterialPropertyMappings[(uint)MaterialTemplatesGPULang::MaterialProperties::Num];
static Core::CVar* r_RaytracingDispatch = Core::CVarCreate(Core::CVar_Int, "r_RaytracingDispatch", "2", "Ray dispatch: 0=build only, 1=1x1 capture stub, 2=full test (640x480)");
static Core::CVar* r_RaytracingDDGI = Core::CVarCreate(Core::CVar_Int, "r_RaytracingDDGI", "1", "DDGI probe rays [0,1]. Independent of r_RaytracingDispatch.");
static Core::CVar* r_RaytracingValidate = Core::CVarCreate(Core::CVar_Int, "r_RaytracingValidate", "1", "Assert TLAS instance records on the CPU before the GPU build [0,1]");
static Core::CVar* r_RaytracingDebug = Core::CVarCreate(Core::CVar_Int, "r_RaytracingDebug", "0", "Show raytracing instance inspector [0,1]");
//------------------------------------------------------------------------------
/**
*/
RaytracingContext::RaytracingContext()
{
    static_assert(sizeof(Brdfhit::TlasInstance) == sizeof(Bsdfhit::TlasInstance), "All raytracing object bindings must be same size");
    static_assert(sizeof(Bsdfhit::TlasInstance) == sizeof(Gltfhit::TlasInstance), "All raytracing object bindings must be same size");
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


    auto raygenShader = CoreGraphics::ShaderGet("shd:raytracing/shaders/raytracetest.gplb");
    auto raygenProgram = CoreGraphics::ShaderGetProgram(raygenShader, CoreGraphics::ShaderFeatureMask("test"));
    state.lightGridShader = CoreGraphics::ShaderGet("shd:raytracing/shaders/light_grid_cs.gplb");
    state.lightGridCullProgram = CoreGraphics::ShaderGetProgram(state.lightGridShader, CoreGraphics::ShaderFeatureMask("Cull"));
    state.lightGridGenProgram = CoreGraphics::ShaderGetProgram(state.lightGridShader, CoreGraphics::ShaderFeatureMask("AABBGenerate"));

    for (IndexT i = 0; i < NumObjectTypes; i++)
    {
        auto shader = CoreGraphics::ShaderGet(HitShaderPaths[i]);
        state.hitPrograms[i] = CoreGraphics::ShaderGetProgram(shader, CoreGraphics::ShaderFeatureMask("Hit"));
    }

    Util::Array<CoreGraphics::ShaderProgramId> shaderMappings;
    shaderMappings.Append(raygenProgram);
    shaderMappings.AppendArray(state.hitPrograms, NumObjectTypes);

    for (uint i = 0; i < (uint)MaterialTemplatesGPULang::MaterialProperties::Num; i++)
        MaterialPropertyMappings[i] = 0xFFFFFFFF;
    MaterialPropertyMappings[(uint)MaterialTemplatesGPULang::MaterialProperties::BRDF] = BRDFObject;
    MaterialPropertyMappings[(uint)MaterialTemplatesGPULang::MaterialProperties::BSDF] = BSDFObject;
    MaterialPropertyMappings[(uint)MaterialTemplatesGPULang::MaterialProperties::GLTF] = GLTFObject;
    MaterialPropertyMappings[(uint)MaterialTemplatesGPULang::MaterialProperties::Terrain] = TerrainObject;

    state.raytracingTables = CoreGraphics::ShaderCreateResourceTableSet(raygenShader, NEBULA_BATCH_GROUP, 3, "Raytracing Descriptors");
    state.raytracingTestOutputTable = CoreGraphics::ShaderCreateResourceTable(raygenShader, NEBULA_SYSTEM_GROUP);
    state.raytracingBundle = CoreGraphics::CreateRaytracingPipeline(shaderMappings);

    n_assert(LightGridCs::NUM_CLUSTER_ENTRIES >= NUM_GRID_CELLS * NUM_GRID_CELLS * NUM_GRID_CELLS);
    CoreGraphics::BufferCreateInfo gridBufferInfo;
    gridBufferInfo.name = "RaytracingAABBGrid";
    gridBufferInfo.size = NUM_GRID_CELLS * NUM_GRID_CELLS * NUM_GRID_CELLS;
    gridBufferInfo.elementSize = sizeof(LightGridCs::ClusterAABBs::STRUCT);
    gridBufferInfo.mode = CoreGraphics::BufferAccessMode::DeviceLocal;
    gridBufferInfo.usageFlags = CoreGraphics::BufferUsage::ReadWrite;
    gridBufferInfo.queueSupport = CoreGraphics::GraphicsQueueSupport | CoreGraphics::ComputeQueueSupport;
    state.gridBuffer = CoreGraphics::CreateBuffer(gridBufferInfo);

    CoreGraphics::BufferCreateInfo indexListInfo;
    indexListInfo.name = "RaytracingLightIndexListsBuffer";
    indexListInfo.byteSize = sizeof(LightGridCs::LightIndexLists::STRUCT);
    indexListInfo.mode = CoreGraphics::BufferAccessMode::DeviceLocal;
    indexListInfo.usageFlags = CoreGraphics::BufferUsage::ReadWrite;
    indexListInfo.queueSupport = CoreGraphics::GraphicsQueueSupport | CoreGraphics::ComputeQueueSupport;
    state.lightGridIndexLists = CoreGraphics::CreateBuffer(indexListInfo);

    CoreGraphics::BufferCreateInfo lightGridConstantsInfo;
    lightGridConstantsInfo.byteSize = sizeof(LightGridCs::ClusterUniforms::STRUCT);
    lightGridConstantsInfo.mode = CoreGraphics::BufferAccessMode::DeviceAndHost;
    lightGridConstantsInfo.usageFlags = CoreGraphics::BufferUsage::ConstantBuffer;
    lightGridConstantsInfo.queueSupport = CoreGraphics::ComputeQueueSupport;
    state.lightGridConstants = CoreGraphics::CreateBuffer(lightGridConstantsInfo);

    state.lightGridResourceTables = CoreGraphics::ShaderCreateResourceTableSet(state.lightGridShader, NEBULA_FRAME_GROUP, 3, "Raytracing Grid Descriptor Set");
    for (IndexT i = 0; i < CoreGraphics::GetNumBufferedFrames(); i++)
    {
        ResourceTableSetRWBuffer(state.lightGridResourceTables.tables[i], { state.gridBuffer, LightGridCs::ClusterAABBs::BINDING, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
        ResourceTableSetConstantBuffer(state.lightGridResourceTables.tables[i], { CoreGraphics::GetConstantBuffer(i, CoreGraphics::ComputeQueueType), LightGridCs::LightUniforms::BINDING, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
        ResourceTableSetRWBuffer(state.lightGridResourceTables.tables[i], { state.lightGridIndexLists, LightGridCs::LightIndexLists::BINDING, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
        ResourceTableSetConstantBuffer(state.lightGridResourceTables.tables[i], { state.lightGridConstants, LightGridCs::ClusterUniforms::BINDING, 0, sizeof(LightGridCs::ClusterUniforms::STRUCT), 0 });
        ResourceTableCommitChanges(state.lightGridResourceTables.tables[i]);
    }

    // Create buffers for updating blas instances
    CoreGraphics::BufferCreateInfo bufInfo;
    bufInfo.name = "BLAS Instance Buffer";
    bufInfo.elementSize = CoreGraphics::BlasInstanceGetSize();
    bufInfo.size = settings.maxNumAllowedInstances;  // This is a virtual max-size, as this buffer is virtual memory managed
    bufInfo.queueSupport = CoreGraphics::BufferQueueSupport::ComputeQueueSupport | CoreGraphics::BufferQueueSupport::GraphicsQueueSupport;
    bufInfo.usageFlags = CoreGraphics::BufferUsage::ShaderAddress | CoreGraphics::BufferUsage::AccelerationStructureInstances;
    state.blasInstanceBuffer.Create(bufInfo);
    for (IndexT i = 0; i < state.blasInstanceBuffer.hostBuffers.buffers.Size(); i++)
    {
        CoreGraphics::BufferId host = state.blasInstanceBuffer.hostBuffers.buffers[i];
        memset(CoreGraphics::BufferMap(host), 0, CoreGraphics::BufferGetByteSize(host));
    }

    CoreGraphics::BufferCreateInfo objectBindingBufferCreateInfo;
    objectBindingBufferCreateInfo.name = "Raytracing Object Binding Buffer";
    objectBindingBufferCreateInfo.byteSize = sizeof(Raytracetest::TlasInstance) * settings.maxNumAllowedInstances;
    objectBindingBufferCreateInfo.usageFlags = CoreGraphics::BufferUsage::ShaderAddress | CoreGraphics::BufferUsage::ReadWrite;
    objectBindingBufferCreateInfo.queueSupport = CoreGraphics::BufferQueueSupport::ComputeQueueSupport;
    state.objectBindingBuffer.Create(objectBindingBufferCreateInfo);

    FrameScript_default::Bind_RayTracingObjectBindings(state.objectBindingBuffer.DeviceBuffer());
    FrameScript_default::Bind_GridBuffer(state.gridBuffer);
    FrameScript_default::Bind_GridLightIndexLists(state.lightGridIndexLists);

    FrameScript_default::RegisterSubgraph_RaytracingLightGridGen_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CoreGraphics::CmdSetShaderProgram(cmdBuf, state.lightGridGenProgram, queue);
        CoreGraphics::CmdSetResourceTable(cmdBuf, state.lightGridResourceTables.tables[bufferIndex], NEBULA_FRAME_GROUP, CoreGraphics::ComputePipeline, nullptr);
        CoreGraphics::CmdDispatch(cmdBuf, NUM_GRID_CELLS * NUM_GRID_CELLS, 1, 1);
    }, {
        { FrameScript_default::BufferIndex::GridBuffer, CoreGraphics::PipelineStage::ComputeShaderWrite }
    });

    FrameScript_default::RegisterSubgraph_RaytracingLightGridCull_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CoreGraphics::CmdSetShaderProgram(cmdBuf, state.lightGridCullProgram, queue);
        CoreGraphics::CmdSetResourceTable(cmdBuf, state.lightGridResourceTables.tables[bufferIndex], NEBULA_FRAME_GROUP, CoreGraphics::ComputePipeline, nullptr);
        CoreGraphics::CmdDispatch(cmdBuf, NUM_GRID_CELLS*NUM_GRID_CELLS, 1, 1);
    }, {
        { FrameScript_default::BufferIndex::GridBuffer, CoreGraphics::PipelineStage::ComputeShaderRead }
        , { FrameScript_default::BufferIndex::LightList, CoreGraphics::PipelineStage::ComputeShaderRead }
        , { FrameScript_default::BufferIndex::GridLightIndexLists, CoreGraphics::PipelineStage::ComputeShaderWrite }
    });

    FrameScript_default::RegisterSubgraph_RaytracingStructuresUpdate_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
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
            state.lastBuildBlasCount = (uint)state.blasesToRebuild.Size();
            state.blasesToRebuild.Clear();
        }

        const uint instanceCount = (state.pendingBlasSetups == 0) ? (uint)state.blasInstances.Size() : 0;
        if (instanceCount != state.lastBuildInstanceCount)
        {
            n_log(Raytracing, "TLAS build instances=%u pending=%d blases_built=%u objects=%d registered=%d buffer=%d", instanceCount, state.pendingBlasSetups, state.lastBuildBlasCount, state.objects.Size(), state.numRegisteredInstances, bufferIndex);
            state.lastBuildInstanceCount = instanceCount;
        }
        if (instanceCount > 0 && Core::CVarReadInt(r_RaytracingValidate) != 0)
        {
            for (uint i = 0; i < instanceCount; i++)
            {
                n_assert_fmt(state.blasInstances[i] != CoreGraphics::InvalidBlasInstanceId, "TLAS slot %u has no BLAS instance", i);
                CoreGraphics::BlasInstanceIdLock _0(state.blasInstances[i]);
                const CoreGraphics::BlasInstanceInfo info = CoreGraphics::BlasInstanceGetInfo(state.blasInstances[i]);
                n_assert_fmt(info.blasDeviceAddress != 0, "TLAS slot %u has a null BLAS device address (mask=%u sbt=%u custom=%u)", i, info.mask, info.shaderOffset, info.customIndex);
                n_assert_fmt(info.shaderOffset < NumObjectTypes, "TLAS slot %u SBT offset %u is past hit program count %u", i, info.shaderOffset, NumObjectTypes);
                n_assert_fmt(info.customIndex < (uint)state.objects.Size(), "TLAS slot %u custom index %u is past object count %d", i, info.customIndex, state.objects.Size());
                n_assert_fmt(state.objects[info.customIndex].PositionsPtr != 0, "TLAS slot %u object %u has a null PositionsPtr", i, info.customIndex);
                n_assert_fmt(state.objects[info.customIndex].IndexPtr != 0, "TLAS slot %u object %u has a null IndexPtr", i, info.customIndex);
            }
        }
        if (instanceCount > 0)
        {
            CoreGraphics::CmdBeginMarker(cmdBuf, NEBULA_MARKER_TRANSFER, "Bottom Level Instance Copy");
            state.blasInstanceBuffer.Flush(cmdBuf, instanceCount * CoreGraphics::BlasInstanceGetSize());
            CoreGraphics::CmdBarrier(
                cmdBuf,
                CoreGraphics::PipelineStage::TransferWrite,
                CoreGraphics::PipelineStage::AccelerationStructureRead,
                CoreGraphics::BarrierDomain::Global,
                {
                    CoreGraphics::BufferBarrierInfo{ .buf = state.blasInstanceBuffer.DeviceBuffer(), .subres = CoreGraphics::BufferSubresourceInfo() }
                }
            );
            CoreGraphics::CmdEndMarker(cmdBuf);
        }

        n_assert(bufferIndex < state.toplevelAccelerationStructures.Size());
        const CoreGraphics::TlasId tlas = state.toplevelAccelerationStructures[bufferIndex];
        n_assert(tlas != CoreGraphics::InvalidTlasId);
        CoreGraphics::CmdBarrier(
            cmdBuf,
            CoreGraphics::PipelineStage::AccelerationStructureRead,
            CoreGraphics::PipelineStage::AccelerationStructureWrite,
            CoreGraphics::BarrierDomain::Global,
            nullptr,
            nullptr,
            {
                CoreGraphics::AccelerationStructureBarrierInfo{ .tlas = tlas, .type = CoreGraphics::AccelerationStructureBarrierInfo::TlasBarrier }
            }
        );

        CoreGraphics::CmdBeginMarker(cmdBuf, NEBULA_MARKER_ORANGE, "Top Level Acceleration Structure Build");
        CoreGraphics::TlasInitBuild(tlas, instanceCount);
        CoreGraphics::CmdBuildTlas(cmdBuf, tlas);
        CoreGraphics::CmdEndMarker(cmdBuf);

        CoreGraphics::CmdBarrier(
            cmdBuf,
            CoreGraphics::PipelineStage::AccelerationStructureWrite,
            CoreGraphics::PipelineStage::RayTracingShaderRead,
            CoreGraphics::BarrierDomain::Global,
            nullptr,
            nullptr,
            {
                CoreGraphics::AccelerationStructureBarrierInfo{ .tlas = tlas, .type = CoreGraphics::AccelerationStructureBarrierInfo::TlasBarrier }
            }
        );
        state.blasLock.Leave();
    }, {
        { FrameScript_default::BufferIndex::RayTracingObjectBindings, CoreGraphics::PipelineStage::TransferWrite }
    });

    FrameScript_default::RegisterSubgraph_RaytracingTest_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        const int dispatchMode = Core::CVarReadInt(r_RaytracingDispatch);
        if (dispatchMode > 0)
        {
            const int dimX = (dispatchMode == 1) ? 1 : 640;
            const int dimY = (dispatchMode == 1) ? 1 : 480;
            CoreGraphics::CmdBarrier(cmdBuf, CoreGraphics::PipelineStage::AllShadersRead, CoreGraphics::PipelineStage::RayTracingShaderWrite, CoreGraphics::BarrierDomain::Global, { CoreGraphics::TextureBarrierInfo{.tex = FrameScript_default::Texture_RayTracingTestOutput(), .subres = CoreGraphics::TextureSubresourceInfo::ColorNoMipNoLayer() }}, nullptr, nullptr);
            CoreGraphics::CmdSetRayTracingPipeline(cmdBuf, state.raytracingBundle.pipeline, queue);
            CoreGraphics::CmdSetResourceTable(cmdBuf,  state.raytracingTestOutputTable, NEBULA_SYSTEM_GROUP, CoreGraphics::RayTracingPipeline, nullptr);
            CoreGraphics::CmdSetResourceTable(cmdBuf, state.raytracingTables.tables[bufferIndex], NEBULA_BATCH_GROUP, CoreGraphics::RayTracingPipeline, nullptr);
            CoreGraphics::CmdSetResourceTable(cmdBuf, state.lightGridResourceTables.tables[bufferIndex], NEBULA_FRAME_GROUP, CoreGraphics::RayTracingPipeline, nullptr);
            CoreGraphics::CmdRaysDispatch(cmdBuf, state.raytracingBundle.table, dimX, dimY, 1);
            CoreGraphics::CmdBarrier(cmdBuf, CoreGraphics::PipelineStage::RayTracingShaderWrite, CoreGraphics::PipelineStage::AllShadersRead, CoreGraphics::BarrierDomain::Global, { CoreGraphics::TextureBarrierInfo{.tex = FrameScript_default::Texture_RayTracingTestOutput(), .subres = CoreGraphics::TextureSubresourceInfo::ColorNoMipNoLayer() }}, nullptr, nullptr);
        }
    }, {
        { FrameScript_default::BufferIndex::GridLightIndexLists, CoreGraphics::PipelineStage::RayTracingShaderRead }
        , { FrameScript_default::BufferIndex::GridBuffer, CoreGraphics::PipelineStage::RayTracingShaderRead }
        , { FrameScript_default::BufferIndex::RayTracingObjectBindings, CoreGraphics::PipelineStage::RayTracingShaderRead }
    });


    state.maxAllowedInstances = settings.maxNumAllowedInstances;
    state.topLevelNeedsReconstruction = false;
    state.blasInstanceAllocator = Memory::RangeAllocator(0xFFFFF, settings.maxNumAllowedInstances);

    state.placeholderMesh = CoreGraphics::MeshResourceGetMesh(Resources::CreateResource(IO::Path::File("system", "placeholder", "msh"), "system", nullptr, nullptr, true, false), 0);
    state.placeholderMaterial = Resources::CreateResource(IO::Path::File("system", "placeholder", "mat"), "system", nullptr, nullptr, true, false);
    n_assert(Materials::MaterialGetBufferBinding(state.placeholderMaterial) != -1);
    n_assert(MaterialPropertyMappings[(uint)Materials::MaterialGetTemplate(state.placeholderMaterial)->properties] != 0xFFFFFFFF);

    const CoreGraphics::VertexLayoutId placeholderLayout = CoreGraphics::MeshGetVertexLayout(state.placeholderMesh);
    const Util::Array<CoreGraphics::VertexComponent>& placeholderComps = CoreGraphics::VertexLayoutGetComponents(placeholderLayout);
    const Util::Array<CoreGraphics::PrimitiveGroup>& placeholderPrimGroups = CoreGraphics::MeshGetPrimitiveGroups(state.placeholderMesh);
    n_assert(placeholderPrimGroups.Size() > 0);
    Util::Array<CoreGraphics::BlasId> placeholderBlases;
    placeholderBlases.Reserve(placeholderPrimGroups.Size());
    for (auto& group : placeholderPrimGroups)
    {
        CoreGraphics::BlasCreateInfo createInfo;
        createInfo.vbo = CoreGraphics::MeshGetVertexBuffer(state.placeholderMesh, 0);
        createInfo.ibo = CoreGraphics::MeshGetIndexBuffer(state.placeholderMesh);
        createInfo.indexType = CoreGraphics::MeshGetIndexType(state.placeholderMesh);
        createInfo.positionsFormat = placeholderComps[0].GetFormat();
        createInfo.stride = CoreGraphics::VertexLayoutGetStreamSize(placeholderLayout, 0);
        createInfo.vertexOffset = CoreGraphics::MeshGetVertexOffset(state.placeholderMesh, 0);
        createInfo.indexOffset = CoreGraphics::MeshGetIndexOffset(state.placeholderMesh);
        createInfo.primGroup = group;
        createInfo.flags = CoreGraphics::AccelerationStructureBuildFlags::FastTrace;
        CoreGraphics::BlasId blas = CoreGraphics::CreateBlas(createInfo);
        placeholderBlases.Append(blas);
        state.blases.Append(blas);
        CoreGraphics::BlasIdRelease(blas);
        state.blasesToRebuild.Append(blas);
    }
    state.blasLookup.Add(state.placeholderMesh, Util::MakeTuple(1, placeholderBlases));

    n_assert(settings.maxNumAllowedInstances > 0);
    CoreGraphics::TlasCreateInfo tlasInfo;
    tlasInfo.numInstances = settings.maxNumAllowedInstances;
    tlasInfo.instanceBuffer = state.blasInstanceBuffer.deviceBuffer;
    tlasInfo.flags = CoreGraphics::AccelerationStructureBuildFlags::FastBuild | CoreGraphics::AccelerationStructureBuildFlags::Dynamic;
    const SizeT numFrames = CoreGraphics::GetNumBufferedFrames();
    state.toplevelAccelerationStructures.Resize(numFrames);
    for (IndexT i = 0; i < numFrames; i++)
        state.toplevelAccelerationStructures[i] = CoreGraphics::CreateTlas(tlasInfo);
    state.topLevelNeedsBuild = false;
    state.topLevelNeedsReconstruction = false;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ShaderProgramId*
RaytracingContext::GetHitPrograms()
{
    return state.hitPrograms;
}

//------------------------------------------------------------------------------
/**
*/
void
RaytracingContext::Discard()
{
    state.raytracingTables.Destroy();
    state.lightGridResourceTables.Destroy();
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

    state.blasLock.Enter();
    state.pendingBlasSetups += numObjects;
    Memory::RangeAllocation alloc = state.blasInstanceAllocator.Alloc(numObjects);
    state.blasInstances.Extend((SizeT)alloc.offset + numObjects);
    state.blasInstanceMeshes.Extend((SizeT)alloc.offset + numObjects);
    state.objects.Extend((SizeT)alloc.offset + numObjects);

    // Create bogus constants
    for (uint i = 0; i < numObjects; i++)
    {
        Raytracetest::TlasInstance constants;
        constants.PositionsPtr = CoreGraphics::BufferGetDeviceAddress(CoreGraphics::GetVertexBuffer());
        constants.AttrPtr = CoreGraphics::BufferGetDeviceAddress(CoreGraphics::GetVertexBuffer());
        constants.IndexPtr = CoreGraphics::BufferGetDeviceAddress(CoreGraphics::GetIndexBuffer());
        constants.MaterialOffset = 0;
        constants.AttributeStride = 0;
        constants.Use16BitIndex = false;
        constants.VertexLayout = (uint)CoreGraphics::VertexLayoutType::Normal;
        state.objects[(uint)alloc.offset + i] = constants;
    }

    raytracingContextAllocator.Set<Raytracing_Allocation>(contextId.id, alloc);
    raytracingContextAllocator.Set<Raytracing_NumStructures>(contextId.id, numObjects);
    raytracingContextAllocator.Set<Raytracing_UpdateType>(contextId.id, UpdateType::Dynamic);
    state.blasLock.Leave();
    IndexT instanceCounter = 0;
    for (IndexT i = nodes.begin; i < nodes.end; i++)
    {
        Models::PrimitiveNode* pNode = static_cast<Models::PrimitiveNode*>(Models::ModelContext::NodeInstances.renderable.nodes[i]);
        const auto setupLambda = [flags, mask, offset = alloc.offset, instanceCounter, i, pNode](Resources::ResourceId id)
        {
            Threading::CriticalScope _s(&state.blasLock);
            CoreGraphics::MeshResourceId meshRes = id;

            // Setup material
            Materials::MaterialId mat = pNode->GetMaterial();
            const MaterialTemplatesGPULang::Entry* temp = Materials::MaterialGetTemplate(mat);
            IndexT bufferBinding = Materials::MaterialGetBufferBinding(mat);
            CoreGraphics::MeshId mesh = MeshResourceGetMesh(meshRes, pNode->GetMeshIndex());
            uint primGroupIndex = pNode->GetPrimitiveGroupIndex();
            uint shaderOffset = MaterialPropertyMappings[(uint)temp->properties];
            if (temp->vertexLayout == CoreGraphics::VertexLayoutType::Particle)
                shaderOffset = ParticleObject;

            if (bufferBinding == -1 || shaderOffset == 0xFFFFFFFF)
            {
                n_log_source(Raytracing, "Material '%s' cannot be used for raytracing, using placeholder!", Materials::MaterialGetName(mat).Value());
                mat = state.placeholderMaterial;
                temp = Materials::MaterialGetTemplate(mat);
                bufferBinding = Materials::MaterialGetBufferBinding(mat);
                shaderOffset = MaterialPropertyMappings[(uint)temp->properties];
                n_assert(bufferBinding != -1);
                n_assert(shaderOffset != 0xFFFFFFFF);
                mesh = state.placeholderMesh;
                primGroupIndex = 0;
            }
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

            Raytracetest::TlasInstance constants;
            constants.MaterialOffset = bufferBinding;

            CoreGraphics::BufferId vbo = CoreGraphics::MeshGetVertexBuffer(mesh, 0);
            CoreGraphics::BufferId ibo = CoreGraphics::MeshGetIndexBuffer(mesh);
            CoreGraphics::BufferIdLock _1(vbo);
            CoreGraphics::BufferIdLock _2(ibo);

            // Because the smallest machine unit is 4 bytes, the offset must be in integers, not in bytes
            CoreGraphics::IndexType::Code indexType = CoreGraphics::MeshGetIndexType(mesh);
            uint positionsStride = (uint)CoreGraphics::VertexLayoutGetStreamSize(CoreGraphics::MeshGetVertexLayout(mesh), 0);
            uint attributeStride = (uint)CoreGraphics::VertexLayoutGetStreamSize(CoreGraphics::MeshGetVertexLayout(mesh), 1);
            CoreGraphics::PrimitiveGroup group = CoreGraphics::MeshGetPrimitiveGroup(mesh, primGroupIndex);
            CoreGraphics::DeviceAddress positionsAddress = CoreGraphics::BufferGetDeviceAddress(vbo) + CoreGraphics::MeshGetVertexOffset(mesh, 0);
            CoreGraphics::DeviceAddress attributeAddress = CoreGraphics::BufferGetDeviceAddress(vbo) + CoreGraphics::MeshGetVertexOffset(mesh, 1);
            CoreGraphics::DeviceAddress indexAddress = CoreGraphics::BufferGetDeviceAddress(ibo) + CoreGraphics::MeshGetIndexOffset(mesh);
            constants.PositionsPtr = positionsAddress + (CoreGraphics::DeviceAddress)group.GetBaseVertex() * positionsStride;
            constants.AttrPtr = attributeAddress + (CoreGraphics::DeviceAddress)group.GetBaseVertex() * attributeStride;
            constants.IndexPtr = indexAddress + (CoreGraphics::DeviceAddress)group.GetBaseIndex() * CoreGraphics::IndexType::SizeOf(indexType);
            constants.Use16BitIndex = indexType == CoreGraphics::IndexType::Index16 ? 1 : 0;
            constants.AttributeStride = attributeStride;
            constants.VertexLayout = (uint)temp->vertexLayout;

            uint instanceIndex = (uint)offset + instanceCounter;
            state.objects[instanceIndex] = constants;

            // Setup instance
            CoreGraphics::BlasInstanceCreateInfo createIntInfo;
            createIntInfo.flags = flags;
            createIntInfo.mask = mask;
            createIntInfo.shaderOffset = shaderOffset;
            createIntInfo.instanceIndex = instanceIndex;
            createIntInfo.blas = blases[primGroupIndex];
            createIntInfo.transform = Models::ModelContext::NodeInstances.transformable.nodeTransforms[Models::ModelContext::NodeInstances.renderable.nodeTransformIndex[i]];

            // Disable instance if the vertex layout isn't supported
            CoreGraphics::BlasIdLock _0(createIntInfo.blas);
            state.blasInstances[instanceIndex] = CoreGraphics::CreateBlasInstance(createIntInfo);
            state.blasInstanceMeshes[instanceIndex] = mesh;

            const SizeT instanceOffset = instanceIndex * CoreGraphics::BlasInstanceGetSize();
            CoreGraphics::BlasInstanceIdLock _4(state.blasInstances[instanceIndex]);
            for (IndexT b = 0; b < state.blasInstanceBuffer.hostBuffers.buffers.Size(); b++)
                CoreGraphics::BlasInstanceUpdate(state.blasInstances[instanceIndex], createIntInfo.transform, state.blasInstanceBuffer.hostBuffers.buffers[b], instanceOffset);

            state.numRegisteredInstances++;
            n_assert(state.pendingBlasSetups > 0);
            state.pendingBlasSetups--;
            if (state.pendingBlasSetups == 0)
                state.topLevelNeedsReconstruction = true;
        };
        Resources::CreateResourceListener(pNode->GetMeshResource(), setupLambda, setupLambda);
        instanceCounter++;
    }
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
    , const size_t vertexOffsetStride
    , const size_t patchVertexStride
    , const Util::Array<Math::mat4> transforms
    , const uint materialTableOffset
    , const MaterialTemplatesGPULang::MaterialProperties shader
    , const CoreGraphics::VertexLayoutType vertexLayout
)
{
    if (!CoreGraphics::RayTracingSupported)
        return;

    Graphics::ContextEntityId contextId = GetContextId(id);

    state.blasLock.Enter();
    Memory::RangeAllocation alloc = state.blasInstanceAllocator.Alloc(transforms.Size());
    state.blasInstances.Extend((uint)alloc.offset + transforms.Size());
    state.blasInstanceMeshes.Extend((uint)alloc.offset + transforms.Size());
    state.objects.Extend((uint)alloc.offset + transforms.Size());

    raytracingContextAllocator.Set<Raytracing_Allocation>(contextId.id, alloc);
    raytracingContextAllocator.Set<Raytracing_NumStructures>(contextId.id, transforms.Size());
    raytracingContextAllocator.Set<Raytracing_UpdateType>(contextId.id, objectType);
    Util::FixedArray<CoreGraphics::BlasId>& blases = raytracingContextAllocator.Get<Raytracing_Blases>(contextId.id);
    blases.Resize(transforms.Size());

    // For each patch, setup a separate BLAS
    IndexT patchCounter = 0;
    for (IndexT i = (uint)alloc.offset; i < (uint)alloc.offset + transforms.Size(); i++)
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

        const SizeT instanceOffset = i * CoreGraphics::BlasInstanceGetSize();
        CoreGraphics::BlasInstanceIdLock _4(state.blasInstances[i]);
        for (IndexT b = 0; b < state.blasInstanceBuffer.hostBuffers.buffers.Size(); b++)
            CoreGraphics::BlasInstanceUpdate(state.blasInstances[i], transforms[patchCounter], state.blasInstanceBuffer.hostBuffers.buffers[b], instanceOffset);

        CoreGraphics::BufferIdLock _2(CoreGraphics::GetVertexBuffer());
        CoreGraphics::BufferIdLock _3(CoreGraphics::GetIndexBuffer());

        Raytracetest::TlasInstance constants;
        constants.Use16BitIndex = indexType == CoreGraphics::IndexType::Index16 ? 1 : 0;
        constants.MaterialOffset = materialTableOffset;
        CoreGraphics::DeviceAddress indexAddress = CoreGraphics::BufferGetDeviceAddress(CoreGraphics::GetIndexBuffer()) + createInfo.indexOffset;
        CoreGraphics::DeviceAddress positionsAddress = CoreGraphics::BufferGetDeviceAddress(CoreGraphics::GetVertexBuffer()) + createInfo.vertexOffset;
        constants.IndexPtr = indexAddress;
        constants.PositionsPtr = positionsAddress;
        constants.AttributeStride = 0x0;
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

    Threading::CriticalScope _s(&state.blasLock);
    const CoreGraphics::TlasId tlas = state.toplevelAccelerationStructures[ctx.bufferIndex];
    n_assert(tlas != CoreGraphics::InvalidTlasId);
    state.topLevelNeedsReconstruction = false;

    CoreGraphics::ResourceTableSetRWTexture(state.raytracingTestOutputTable,
        CoreGraphics::ResourceTableTexture(FrameScript_default::Texture_RayTracingTestOutput(), Raytracetest::RaytracingOutput::BINDING)
    );
    CoreGraphics::ResourceTableSetAccelerationStructure(
        state.raytracingTables.tables[ctx.bufferIndex],
        CoreGraphics::ResourceTableTlas(tlas, Raytracetest::TLAS::BINDING)
    );

    CoreGraphics::ResourceTableSetConstantBuffer(
        state.raytracingTables.tables[ctx.bufferIndex],
        CoreGraphics::ResourceTableBuffer(Materials::MaterialLoader::GetMaterialBindingBuffer(), Raytracetest::MaterialPointers::BINDING)
    );

    CoreGraphics::ResourceTableSetRWBuffer(
        state.raytracingTables.tables[ctx.bufferIndex],
        CoreGraphics::ResourceTableBuffer(state.objectBindingBuffer.DeviceBuffer(), Raytracetest::TlasInstanceBuffer::BINDING)
    );

    CoreGraphics::ResourceTableCommitChanges(state.raytracingTables.tables[ctx.bufferIndex]);
    CoreGraphics::ResourceTableCommitChanges(state.raytracingTestOutputTable);
}

//------------------------------------------------------------------------------
/**
*/
void
RaytracingContext::RenderUI(const Graphics::FrameContext& ctx)
{
    if (!CoreGraphics::RayTracingSupported)
        return;

    if (Core::CVarReadInt(r_RaytracingDebug) == 0)
        return;

    Threading::CriticalScope _s(&state.blasLock);
    if (ImGui::Begin("Raytracing"))
    {
        ImGui::Text("Dispatch %d (0=build 1=1x1 2=full)  DDGI %d  Validate %d", Core::CVarReadInt(r_RaytracingDispatch), Core::CVarReadInt(r_RaytracingDDGI), Core::CVarReadInt(r_RaytracingValidate));
        ImGui::Text("Pending setups %d  Instances %d  Registered %d  Objects %d", state.pendingBlasSetups, state.blasInstances.Size(), state.numRegisteredInstances, state.objects.Size());
        ImGui::Text("BLAS rebuild queue %d  Last BLAS builds %u  Last TLAS instances %u", state.blasesToRebuild.Size(), state.lastBuildBlasCount, state.lastBuildInstanceCount);
        ImGui::Text("TLAS slots %d  Frame %d", state.toplevelAccelerationStructures.Size(), ctx.bufferIndex);
        if (ImGui::BeginTable("Instances", 6, ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("Slot");
            ImGui::TableSetupColumn("Mask");
            ImGui::TableSetupColumn("SBT");
            ImGui::TableSetupColumn("Custom");
            ImGui::TableSetupColumn("BLAS addr");
            ImGui::TableSetupColumn("Pos ptr");
            ImGui::TableHeadersRow();
            const SizeT rows = state.blasInstances.Size();
            for (IndexT i = 0; i < rows; i++)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%d", i);
                if (state.blasInstances[i] == CoreGraphics::InvalidBlasInstanceId)
                {
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("invalid");
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    continue;
                }
                CoreGraphics::BlasInstanceIdLock _0(state.blasInstances[i]);
                const CoreGraphics::BlasInstanceInfo info = CoreGraphics::BlasInstanceGetInfo(state.blasInstances[i]);
                ImGui::TableNextColumn();
                ImGui::Text("%u", info.mask);
                ImGui::TableNextColumn();
                ImGui::Text("%u", info.shaderOffset);
                ImGui::TableNextColumn();
                ImGui::Text("%u", info.customIndex);
                ImGui::TableNextColumn();
                ImGui::Text("%llx", (unsigned long long)info.blasDeviceAddress);
                ImGui::TableNextColumn();
                if (info.customIndex < (uint)state.objects.Size())
                    ImGui::Text("%llx", (unsigned long long)state.objects[info.customIndex].PositionsPtr);
            }
            ImGui::EndTable();
        }
        if (ImGui::Button("Dump instances to log"))
        {
            n_log(Raytracing, "Dump pending=%d instances=%d objects=%d registered=%d", state.pendingBlasSetups, state.blasInstances.Size(), state.objects.Size(), state.numRegisteredInstances);
            for (IndexT i = 0; i < state.blasInstances.Size(); i++)
            {
                if (state.blasInstances[i] == CoreGraphics::InvalidBlasInstanceId)
                {
                    n_log(Raytracing, "  [%d] invalid", i);
                    continue;
                }
                CoreGraphics::BlasInstanceIdLock _0(state.blasInstances[i]);
                const CoreGraphics::BlasInstanceInfo info = CoreGraphics::BlasInstanceGetInfo(state.blasInstances[i]);
                const Raytracetest::TlasInstance* obj = (info.customIndex < (uint)state.objects.Size()) ? &state.objects[info.customIndex] : nullptr;
                n_log(Raytracing, "  [%d] mask=%u sbt=%u custom=%u blas=0x%llx pos=0x%llx idx=0x%llx layout=%u", i, info.mask, info.shaderOffset, info.customIndex, (unsigned long long)info.blasDeviceAddress, obj ? (unsigned long long)obj->PositionsPtr : 0ull, obj ? (unsigned long long)obj->IndexPtr : 0ull, obj ? obj->VertexLayout : 0u);
            }
        }
        ImGui::End();
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

    LightGridCs::ClusterUniforms::STRUCT uniformData;
    uniformData.NumCells[0] = NUM_GRID_CELLS;
    uniformData.NumCells[1] = NUM_GRID_CELLS;
    uniformData.NumCells[2] = NUM_GRID_CELLS;
    uniformData.BlockSize[0] = NUM_GRID_CELLS;
    uniformData.BlockSize[1] = 10.0f; // Size of grid cells
    CoreGraphics::BufferUpdate(state.lightGridConstants, uniformData);

    const Util::Array<Graphics::GraphicsEntityId>& entities = RaytracingContext::__state.entities;

    if (!entities.IsEmpty())
    {
        static Util::Array<uint32_t> nodes;
        nodes.Clear();
        nodes.Resize(entities.Size());

        static Threading::Interlocked::AtomicCounter idCounter = 0;
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
                        CoreGraphics::BlasInstanceIdLock _0(state.blasInstances[(uint)alloc.offset + j]);
                        CoreGraphics::BlasInstanceUpdate(state.blasInstances[(uint)alloc.offset + j], state.blasInstanceBuffer.HostBuffer(), (alloc.offset + j) * CoreGraphics::BlasInstanceGetSize());
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
                        if (state.blasInstances[(uint)alloc.offset + counter] != CoreGraphics::InvalidBlasInstanceId)
                        {
                            CoreGraphics::BlasInstanceIdLock _0(state.blasInstances[(uint)alloc.offset + counter]);
                            CoreGraphics::BlasInstanceUpdate(state.blasInstances[(uint)alloc.offset + counter], transform, state.blasInstanceBuffer.HostBuffer(), (alloc.offset + counter) * CoreGraphics::BlasInstanceGetSize());
                        }
                        counter++;
                    }
                }
            }
        }, entities.Size(), 1024, { &Models::ModelContext::TransformsUpdateCounter }, &idCounter, &state.jobWaitEvent);

        // Copy over object bindings
        CoreGraphics::BufferUpdateArray(state.objectBindingBuffer.HostBuffer(), state.objects);
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
RaytracingContext::UpdateResources(const Graphics::FrameContext& ctx)
{
    if (!CoreGraphics::RayTracingSupported)
        return;

    LightsCluster::LightUniforms::STRUCT uniforms = Lighting::LightContext::GetLightUniforms();
    uniforms.NumLightClusters = NUM_GRID_CELLS*NUM_GRID_CELLS*NUM_GRID_CELLS;
    uint64_t offset = CoreGraphics::SetConstants(uniforms, CoreGraphics::ComputeQueueType);
    uint64_t tickCbo, viewCbo, shadowCbo;
    Graphics::GetOffsets(tickCbo, viewCbo, shadowCbo, Graphics::GlobalTables::ComputeQueue);
    ResourceTableSetConstantBuffer(state.lightGridResourceTables.tables[ctx.bufferIndex], { CoreGraphics::GetConstantBuffer(ctx.bufferIndex, CoreGraphics::ComputeQueueType), Shared::ViewConstants::BINDING, 0, sizeof(Shared::ViewConstants::STRUCT), viewCbo });
    ResourceTableSetConstantBuffer(state.lightGridResourceTables.tables[ctx.bufferIndex], { CoreGraphics::GetConstantBuffer(ctx.bufferIndex, CoreGraphics::ComputeQueueType), Shared::ShadowViewConstants::BINDING, 0, sizeof(Shared::ShadowViewConstants::STRUCT), shadowCbo });
    ResourceTableSetRWBuffer(state.lightGridResourceTables.tables[ctx.bufferIndex], { state.gridBuffer, Shared::ClusterAABBs::BINDING, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
    ResourceTableSetRWBuffer(state.lightGridResourceTables.tables[ctx.bufferIndex], { state.lightGridIndexLists, Shared::LightIndexLists::BINDING, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
    ResourceTableSetRWBuffer(state.lightGridResourceTables.tables[ctx.bufferIndex], { Lighting::LightContext::GetLightsBuffer(), Shared::LightLists::BINDING, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
    ResourceTableSetConstantBuffer(state.lightGridResourceTables.tables[ctx.bufferIndex], { state.lightGridConstants, Shared::ClusterUniforms::BINDING, 0, sizeof(Shared::ClusterUniforms::STRUCT), 0 });
    ResourceTableSetConstantBuffer(state.lightGridResourceTables.tables[ctx.bufferIndex], { CoreGraphics::GetConstantBuffer(ctx.bufferIndex, CoreGraphics::ComputeQueueType), Shared::LightUniforms::BINDING, 0, sizeof(Shared::LightUniforms::STRUCT), offset });
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
RaytracingContext::GetTLAS(const IndexT bufferIndex)
{
    n_assert(bufferIndex < state.toplevelAccelerationStructures.Size());
    return state.toplevelAccelerationStructures[bufferIndex];
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
    for (IndexT i = (uint)range.offset; i < range.offset + numAllocs; i++)
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

                // If this is the last count, nuke the BLAS
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
        for (SizeT j = 0; j < state.blasInstanceBuffer.hostBuffers.buffers.Size(); j++)
        {
            CoreGraphics::BlasInstanceUpdate(state.blasInstances[(uint)i], state.blasInstanceBuffer.hostBuffers.buffers[j], i * CoreGraphics::BlasInstanceGetSize());
        }
        state.blasInstances[i] = CoreGraphics::InvalidBlasInstanceId;
    }

    raytracingContextAllocator.Dealloc(id.id);
    state.topLevelNeedsReconstruction = true;
}

} // namespace Raytracing
