//------------------------------------------------------------------------------
//  volumetricfogcontext.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "volumetricfogcontext.h"
#include "graphics/graphicsserver.h"
#include "clustering/clustercontext.h"
#include "graphics/cameracontext.h"
#include "graphics/view.h"
#include "frame/framesubgraph.h"
#include "frame/framecode.h"
#include "imgui.h"
#include "graphics/globalconstants.h"
#include "core/cvar.h"

#include "gpulang/render/system_shaders/volumefog.h"
#include "gpulang/render/system_shaders/blur/blur_2d_rgba16f_cs.h"

#include "frame/default.h"
namespace Fog
{

VolumetricFogContext::FogGenericVolumeAllocator VolumetricFogContext::fogGenericVolumeAllocator;
VolumetricFogContext::FogBoxVolumeAllocator VolumetricFogContext::fogBoxVolumeAllocator;
VolumetricFogContext::FogSphereVolumeAllocator VolumetricFogContext::fogSphereVolumeAllocator;
__ImplementContext(VolumetricFogContext, VolumetricFogContext::fogGenericVolumeAllocator);

struct
{
    CoreGraphics::ShaderId classificationShader;
    CoreGraphics::ShaderProgramId cullProgram;
    CoreGraphics::ShaderProgramId renderProgram;

    CoreGraphics::BufferId clusterFogIndexLists;

    CoreGraphics::BufferSet stagingClusterFogLists;
    CoreGraphics::BufferId clusterFogLists;

    Util::FixedArray<CoreGraphics::ResourceTableId> resourceTables;
    float turbidity;
    Math::vec3 color;

    // these are used to update the light clustering
    Volumefog::FogBox fogBoxes[128];
    Volumefog::FogSphere fogSpheres[128];

    Core::CVar* r_show_fog_params;
} fogState;

struct
{
    CoreGraphics::ShaderId blurShader;
    CoreGraphics::ShaderProgramId blurXProgram, blurYProgram;
    Util::FixedArray<CoreGraphics::ResourceTableId> blurXTable, blurYTable;
    CoreGraphics::BufferId blurConstants;
} blurState;


//------------------------------------------------------------------------------
/**
*/
VolumetricFogContext::VolumetricFogContext()
{

}

//------------------------------------------------------------------------------
/**
*/
VolumetricFogContext::~VolumetricFogContext()
{

}

//------------------------------------------------------------------------------
/**
*/
void
VolumetricFogContext::Create()
{
    __CreateContext();

    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    using namespace CoreGraphics;
    fogState.classificationShader = CoreGraphics::ShaderGet("shd:system_shaders/volumefog.gplb");

    BufferCreateInfo rwbInfo;
    rwbInfo.name = "FogIndexListsBuffer";
    rwbInfo.size = 1;
    rwbInfo.elementSize = sizeof(Volumefog::FogIndexLists::STRUCT);
    rwbInfo.mode = BufferAccessMode::DeviceLocal;
    rwbInfo.usageFlags = CoreGraphics::BufferUsage::ReadWrite | CoreGraphics::BufferUsage::TransferDestination;
    rwbInfo.queueSupport = CoreGraphics::GraphicsQueueSupport | CoreGraphics::ComputeQueueSupport;
    fogState.clusterFogIndexLists = CreateBuffer(rwbInfo);

    rwbInfo.name = "FogLists";
    rwbInfo.elementSize = sizeof(Volumefog::FogLists::STRUCT);
    fogState.clusterFogLists = CreateBuffer(rwbInfo);

    rwbInfo.name = "FogListsStagingBuffer";
    rwbInfo.mode = BufferAccessMode::HostLocal;
    rwbInfo.usageFlags = CoreGraphics::BufferUsage::TransferSource;
    fogState.stagingClusterFogLists.Create(rwbInfo);

    fogState.cullProgram = ShaderGetProgram(fogState.classificationShader, CoreGraphics::ShaderFeatureMask("Cull"));
    fogState.renderProgram = ShaderGetProgram(fogState.classificationShader, CoreGraphics::ShaderFeatureMask("Render"));

    fogState.resourceTables.Resize(CoreGraphics::GetNumBufferedFrames());
    for (IndexT i = 0; i < fogState.resourceTables.Size(); i++)
    {
        fogState.resourceTables[i] = ShaderCreateResourceTable(fogState.classificationShader, NEBULA_BATCH_GROUP, fogState.resourceTables.Size());
    }

    for (IndexT i = 0; i < CoreGraphics::GetNumBufferedFrames(); i++)
    {
        CoreGraphics::ResourceTableId frameResourceTable = Graphics::GetFrameResourceTable(i);

        ResourceTableSetRWBuffer(frameResourceTable, { fogState.clusterFogIndexLists, Shared::FogIndexLists::BINDING, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
        ResourceTableSetRWBuffer(frameResourceTable, { fogState.clusterFogLists, Shared::FogLists::BINDING, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
        ResourceTableSetConstantBuffer(frameResourceTable, { CoreGraphics::GetConstantBuffer(i), Shared::VolumeFogUniforms::BINDING, 0, sizeof(Shared::VolumeFogUniforms::STRUCT), 0 });
        ResourceTableCommitChanges(frameResourceTable);
    }

    blurState.blurShader = CoreGraphics::ShaderGet("shd:system_shaders/blur/blur_2d_rgba16f_cs.gplb");
    blurState.blurXProgram = ShaderGetProgram(blurState.blurShader, CoreGraphics::ShaderFeatureMask("BlurX"));
    blurState.blurYProgram = ShaderGetProgram(blurState.blurShader, CoreGraphics::ShaderFeatureMask("BlurY"));
    blurState.blurXTable.Resize(CoreGraphics::GetNumBufferedFrames());
    blurState.blurYTable.Resize(CoreGraphics::GetNumBufferedFrames());

    CoreGraphics::BufferCreateInfo bufInfo;
    bufInfo.byteSize = sizeof(Blur2dRgba16fCs::BlurUniforms::STRUCT);
    bufInfo.usageFlags = CoreGraphics::BufferUsage::ConstantBuffer;
    bufInfo.mode = CoreGraphics::BufferAccessMode::DeviceAndHost;
    blurState.blurConstants = CoreGraphics::CreateBuffer(bufInfo);

    for (IndexT i = 0; i < blurState.blurXTable.Size(); i++)
    {
        blurState.blurXTable[i] = ShaderCreateResourceTable(blurState.blurShader, NEBULA_BATCH_GROUP, blurState.blurXTable.Size());
        blurState.blurYTable[i] = ShaderCreateResourceTable(blurState.blurShader, NEBULA_BATCH_GROUP, blurState.blurXTable.Size());
    }

    fogState.turbidity = 0.1f;
    fogState.color = Math::vec3(1);

    FrameScript_default::RegisterSubgraph_FogCopy_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CoreGraphics::BufferCopy from, to;
        from.offset = 0;
        to.offset = 0;
        CmdCopy(cmdBuf, fogState.stagingClusterFogLists.buffers[bufferIndex], { from }, fogState.clusterFogLists, { to }, sizeof(Volumefog::FogLists::STRUCT));
    }, {
        { FrameScript_default::BufferIndex::ClusterFogList, CoreGraphics::PipelineStage::TransferWrite }
    });

    FrameScript_default::RegisterSubgraph_FogCull_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, fogState.cullProgram);

        // Run chunks of 1024 threads at a time
        std::array<SizeT, 3> dimensions = Clustering::ClusterContext::GetClusterDimensions();

        CmdDispatch(cmdBuf, Math::ceil((dimensions[0] * dimensions[1] * dimensions[2]) / 64.0f), 1, 1);
    }, {
        { FrameScript_default::BufferIndex::ClusterFogList, CoreGraphics::PipelineStage::ComputeShaderRead }
        , { FrameScript_default::BufferIndex::ClusterLightIndexLists, CoreGraphics::PipelineStage::ComputeShaderWrite }
        , { FrameScript_default::BufferIndex::ClusterBuffer, CoreGraphics::PipelineStage::ComputeShaderRead }
    });

    FrameScript_default::RegisterSubgraph_FogCompute_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, fogState.renderProgram);

        // Set frame resources
        CmdSetResourceTable(cmdBuf, fogState.resourceTables[bufferIndex], NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);

        // run volumetric fog compute
        auto [scaleX, scaleY] = FrameScript_default::TextureRelativeScale_VolumetricFogBuffer0();
        CmdDispatch(cmdBuf, Math::divandroundup(viewport.width() * scaleX, 64), viewport.height() * scaleY, 1);
    }, {
        { FrameScript_default::BufferIndex::ClusterFogIndexLists, CoreGraphics::PipelineStage::ComputeShaderRead }
    }, {
        { FrameScript_default::TextureIndex::VolumetricFogBuffer0, CoreGraphics::PipelineStage::ComputeShaderWrite }
        , { FrameScript_default::TextureIndex::ZBuffer, CoreGraphics::PipelineStage::ComputeShaderRead }
    });

    FrameScript_default::RegisterSubgraph_FogBlurX_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, blurState.blurXProgram);
        CmdSetResourceTable(cmdBuf, blurState.blurXTable[bufferIndex], NEBULA_BATCH_GROUP, ComputePipeline, nullptr);

        auto [scaleX, scaleY] = FrameScript_default::TextureRelativeScale_VolumetricFogBuffer0();
        Blur2dRgba16fCs::BlurUniforms::STRUCT blurUniforms;
        blurUniforms.Viewport[0] = viewport.width() * scaleX;
        blurUniforms.Viewport[1] = viewport.height() * scaleY;
        CoreGraphics::BufferUpdate(blurState.blurConstants, blurUniforms);

        // fog0 -> read, fog1 -> write
        CmdDispatch(cmdBuf, Math::divandroundup(viewport.width() * scaleX, Blur2dRgba16fCs::BlurTileWidth), viewport.height() * scaleY, 1);

    }, nullptr, {
        { FrameScript_default::TextureIndex::VolumetricFogBuffer0, CoreGraphics::PipelineStage::ComputeShaderRead }
        , { FrameScript_default::TextureIndex::VolumetricFogBuffer1, CoreGraphics::PipelineStage::ComputeShaderWrite }
    });
    FrameScript_default::RegisterSubgraph_FogBlurY_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, blurState.blurYProgram);
        CmdSetResourceTable(cmdBuf, blurState.blurYTable[bufferIndex], NEBULA_BATCH_GROUP, ComputePipeline, nullptr);

        auto [scaleX, scaleY] = FrameScript_default::TextureRelativeScale_VolumetricFogBuffer1();
        Blur2dRgba16fCs::BlurUniforms::STRUCT blurUniforms;
        blurUniforms.Viewport[0] = viewport.width() * scaleX;
        blurUniforms.Viewport[1] = viewport.height() * scaleY;
        CoreGraphics::BufferUpdate(blurState.blurConstants, blurUniforms);

        // fog0 -> read, fog1 -> write
        CmdDispatch(cmdBuf, Math::divandroundup(viewport.height() * scaleY, Blur2dRgba16fCs::BlurTileWidth), viewport.width() * scaleX, 1);
    }, nullptr, {
        { FrameScript_default::TextureIndex::VolumetricFogBuffer0, CoreGraphics::PipelineStage::ComputeShaderWrite }
        , { FrameScript_default::TextureIndex::VolumetricFogBuffer1, CoreGraphics::PipelineStage::ComputeShaderRead }
    });

    fogState.r_show_fog_params = Core::CVarCreate(Core::CVar_Int, "r_show_fog_params", "0", "Show/hide fog parameters debug ui");
}

//------------------------------------------------------------------------------
/**
*/
void
VolumetricFogContext::Discard()
{
}

//------------------------------------------------------------------------------
/**
*/
void
VolumetricFogContext::SetupBoxVolume(
    const Graphics::GraphicsEntityId id,
    const Math::mat4& transform,
    const float density,
    const Math::vec3& absorption)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    Ids::Id32 fog = fogBoxVolumeAllocator.Alloc();
    fogBoxVolumeAllocator.Set<FogBoxVolume_Transform>(fog, transform);

    fogGenericVolumeAllocator.Set<FogVolume_Type>(cid.id, BoxVolume);
    fogGenericVolumeAllocator.Set<FogVolume_TypedId>(cid.id, fog);
    fogGenericVolumeAllocator.Set<FogVolume_Turbidity>(cid.id, density);
    fogGenericVolumeAllocator.Set<FogVolume_Absorption>(cid.id, absorption);
}

//------------------------------------------------------------------------------
/**
*/
void
VolumetricFogContext::SetBoxTransform(const Graphics::GraphicsEntityId id, const Math::mat4& transform)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    Ids::Id32 lid = fogGenericVolumeAllocator.Get<FogVolume_TypedId>(cid.id);
    FogVolumeType type = fogGenericVolumeAllocator.Get<FogVolume_Type>(cid.id);
    n_assert(type == FogVolumeType::BoxVolume);
    fogBoxVolumeAllocator.Set<FogBoxVolume_Transform>(lid, transform);
}

//------------------------------------------------------------------------------
/**
*/
void
VolumetricFogContext::SetupSphereVolume(
    const Graphics::GraphicsEntityId id,
    const Math::vec3& position,
    float radius,
    const float density,
    const Math::vec3& absorption)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    Ids::Id32 fog = fogSphereVolumeAllocator.Alloc();
    fogSphereVolumeAllocator.Set<FogSphereVolume_Position>(fog, position);
    fogSphereVolumeAllocator.Set<FogSphereVolume_Radius>(fog, radius);

    fogGenericVolumeAllocator.Set<FogVolume_Type>(cid.id, SphereVolume);
    fogGenericVolumeAllocator.Set<FogVolume_TypedId>(cid.id, fog);
    fogGenericVolumeAllocator.Set<FogVolume_Turbidity>(cid.id, density);
    fogGenericVolumeAllocator.Set<FogVolume_Absorption>(cid.id, absorption);
}

//------------------------------------------------------------------------------
/**
*/
void
VolumetricFogContext::SetSpherePosition(const Graphics::GraphicsEntityId id, const Math::vec3& position)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    Ids::Id32 lid = fogGenericVolumeAllocator.Get<FogVolume_TypedId>(cid.id);
    FogVolumeType type = fogGenericVolumeAllocator.Get<FogVolume_Type>(cid.id);
    n_assert(type == FogVolumeType::SphereVolume);
    fogSphereVolumeAllocator.Set<FogSphereVolume_Position>(lid, position);
}

//------------------------------------------------------------------------------
/**
*/
void
VolumetricFogContext::SetSphereRadius(const Graphics::GraphicsEntityId id, const float radius)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    Ids::Id32 lid = fogGenericVolumeAllocator.Get<FogVolume_TypedId>(cid.id);
    FogVolumeType type = fogGenericVolumeAllocator.Get<FogVolume_Type>(cid.id);
    n_assert(type == FogVolumeType::SphereVolume);
    fogSphereVolumeAllocator.Set<FogSphereVolume_Radius>(lid, radius);
}

//------------------------------------------------------------------------------
/**
*/
void
VolumetricFogContext::SetTurbidity(const Graphics::GraphicsEntityId id, const float turbidity)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    fogGenericVolumeAllocator.Set<FogVolume_Turbidity>(cid.id, turbidity);
}

//------------------------------------------------------------------------------
/**
*/
void
VolumetricFogContext::SetAbsorption(const Graphics::GraphicsEntityId id, const Math::vec3& absorption)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    fogGenericVolumeAllocator.Set<FogVolume_Absorption>(cid.id, absorption);
}

//------------------------------------------------------------------------------
/**
*/
void
VolumetricFogContext::UpdateViewDependentResources(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx)
{
    using namespace CoreGraphics;
    IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();
    Math::mat4 viewTransform = Graphics::CameraContext::GetView(view->GetCamera());

    SizeT numFogBoxVolumes = 0;
    SizeT numFogSphereVolumes = 0;

    const Util::Array<FogVolumeType>& types = fogGenericVolumeAllocator.GetArray<FogVolume_Type>();
    const Util::Array<Ids::Id32>& typeIds = fogGenericVolumeAllocator.GetArray<FogVolume_TypedId>();
    IndexT i;
    for (i = 0; i < types.Size(); i++)
    {
        switch (types[i])
        {
        case BoxVolume:
        {
            auto& fog = fogState.fogBoxes[numFogBoxVolumes];
            fogGenericVolumeAllocator.Get<FogVolume_Absorption>(i).store(fog.absorption);
            fog.turbidity = fogGenericVolumeAllocator.Get<FogVolume_Turbidity>(i);
            fog.falloff = 64.0f;
            Math::mat4 transform = viewTransform * fogBoxVolumeAllocator.Get<FogBoxVolume_Transform>(typeIds[i]);
            Math::bbox box(transform);
            box.pmin.store3(fog.bboxMin);
            box.pmax.store3(fog.bboxMax);
            inverse(transform).store(&fog.invTransform[0][0]);
            numFogBoxVolumes++;
            break;
        }
        case SphereVolume:
        {
            auto& fog = fogState.fogSpheres[numFogSphereVolumes];
            fogGenericVolumeAllocator.Get<FogVolume_Absorption>(i).store(fog.absorption);
            fog.turbidity = fogGenericVolumeAllocator.Get<FogVolume_Turbidity>(i);
            Math::vec4 pos = Math::vec4(fogSphereVolumeAllocator.Get<FogSphereVolume_Position>(typeIds[i]), 1);
            pos = viewTransform * pos;
            pos.store3(fog.position);
            fog.radius = fogSphereVolumeAllocator.Get<FogSphereVolume_Radius>(typeIds[i]);
            fog.falloff = 64.0f;
            numFogSphereVolumes++;
            break;
        }
        }
    }

    // update list of point lights
    if (numFogBoxVolumes > 0 || numFogSphereVolumes > 0)
    {
        Volumefog::FogLists::STRUCT fogList;
        Memory::CopyElements(fogState.fogBoxes, fogList.FogBoxes, numFogBoxVolumes);
        Memory::CopyElements(fogState.fogSpheres, fogList.FogSpheres, numFogSphereVolumes);
        CoreGraphics::BufferUpdate(fogState.stagingClusterFogLists.buffers[bufferIndex], fogList);
        CoreGraphics::BufferFlush(fogState.stagingClusterFogLists.buffers[bufferIndex]);
    }

    Volumefog::VolumeFogUniforms::STRUCT fogUniforms;
    fogUniforms.NumFogBoxes = numFogBoxVolumes;
    fogUniforms.NumFogSpheres = numFogSphereVolumes;
    fogUniforms.NumVolumeFogClusters = Clustering::ClusterContext::GetNumClusters();
    fogUniforms.GlobalTurbidity = fogState.turbidity;
    fogState.color.store(fogUniforms.GlobalAbsorption);
    fogUniforms.DownscaleFog = 4;

    ResourceTableSetRWTexture(fogState.resourceTables[bufferIndex], { FrameScript_default::Texture_VolumetricFogBuffer0(), Volumefog::Lighting::BINDING, 0, CoreGraphics::InvalidSamplerId });
    ResourceTableCommitChanges(fogState.resourceTables[bufferIndex]);

    // get per-view resource tables
    CoreGraphics::ResourceTableId frameResourceTable = Graphics::GetFrameResourceTable(bufferIndex);

    uint64_t offset = SetConstants(fogUniforms);
    ResourceTableSetConstantBuffer(frameResourceTable, { GetConstantBuffer(bufferIndex), Shared::VolumeFogUniforms::BINDING, 0, sizeof(Shared::VolumeFogUniforms::STRUCT), offset });
    ResourceTableCommitChanges(frameResourceTable);

    // setup blur tables
    ResourceTableSetTexture(blurState.blurXTable[bufferIndex], { FrameScript_default::Texture_VolumetricFogBuffer0(), Blur2dRgba16fCs::InputImageX::BINDING, 0, CoreGraphics::InvalidSamplerId }); // ping
    ResourceTableSetRWTexture(blurState.blurXTable[bufferIndex], { FrameScript_default::Texture_VolumetricFogBuffer1(), Blur2dRgba16fCs::BlurImageX::BINDING, 0, CoreGraphics::InvalidSamplerId }); // pong
    ResourceTableSetTexture(blurState.blurYTable[bufferIndex], { FrameScript_default::Texture_VolumetricFogBuffer1(), Blur2dRgba16fCs::InputImageY::BINDING, 0, CoreGraphics::InvalidSamplerId }); // ping
    ResourceTableSetRWTexture(blurState.blurYTable[bufferIndex], { FrameScript_default::Texture_VolumetricFogBuffer0(), Blur2dRgba16fCs::BlurImageY::BINDING, 0, CoreGraphics::InvalidSamplerId }); // pong
    ResourceTableSetConstantBuffer(blurState.blurXTable[bufferIndex], { blurState.blurConstants, Blur2dRgba16fCs::BlurUniforms::BINDING });
    ResourceTableSetConstantBuffer(blurState.blurYTable[bufferIndex], { blurState.blurConstants, Blur2dRgba16fCs::BlurUniforms::BINDING });
    ResourceTableCommitChanges(blurState.blurXTable[bufferIndex]);
    ResourceTableCommitChanges(blurState.blurYTable[bufferIndex]);
}

//------------------------------------------------------------------------------
/**
*/
void
VolumetricFogContext::RenderUI(const Graphics::FrameContext& ctx)
{
    int showUI = Core::CVarReadInt(fogState.r_show_fog_params);
    if (showUI)
    {
        float col[3];
        fogState.color.storeu(col);
        Shared::PerTickParams::STRUCT tickParams = Graphics::GetTickParams();
        if (ImGui::Begin("Volumetric Fog Params"))
        {
            ImGui::SetWindowSize(ImVec2(240, 400), ImGuiCond_Once);
            ImGui::SliderFloat("Turbidity", &fogState.turbidity, 0, 200.0f);
            ImGui::ColorEdit3("Fog Color", col);
        }
        fogState.color.loadu(col);
        Graphics::UpdateTickParams(tickParams);

        ImGui::End();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
VolumetricFogContext::SetGlobalTurbidity(float f)
{
    fogState.turbidity = f;
}

//------------------------------------------------------------------------------
/**
*/
void
VolumetricFogContext::SetGlobalAbsorption(const Math::vec3& color)
{
    fogState.color = color;
}

//------------------------------------------------------------------------------
/**
*/
Math::mat4
VolumetricFogContext::GetTransform(const Graphics::GraphicsEntityId id)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    FogVolumeType type = fogGenericVolumeAllocator.Get<FogVolume_Type>(cid.id);
    Ids::Id32 tid = fogGenericVolumeAllocator.Get<FogVolume_TypedId>(cid.id);
    switch (type)
    {
    case BoxVolume:
        return fogBoxVolumeAllocator.Get<FogBoxVolume_Transform>(tid);
    case SphereVolume:
        return Math::translation(fogSphereVolumeAllocator.Get<FogSphereVolume_Position>(tid));
    };
    return Math::mat4();
}

//------------------------------------------------------------------------------
/**
*/
void
VolumetricFogContext::SetTransform(const Graphics::GraphicsEntityId id, const Math::mat4& mat)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    FogVolumeType type = fogGenericVolumeAllocator.Get<FogVolume_Type>(cid.id);
    Ids::Id32 tid = fogGenericVolumeAllocator.Get<FogVolume_TypedId>(cid.id);
    switch (type)
    {
    case BoxVolume:
        return fogBoxVolumeAllocator.Set<FogBoxVolume_Transform>(tid, mat);
    case SphereVolume:
        return fogSphereVolumeAllocator.Set<FogSphereVolume_Position>(tid, xyz(mat.position));
    };
}

//------------------------------------------------------------------------------
/**
*/
Graphics::ContextEntityId
VolumetricFogContext::Alloc()
{
    return fogGenericVolumeAllocator.Alloc();
}

//------------------------------------------------------------------------------
/**
*/
void
VolumetricFogContext::Dealloc(Graphics::ContextEntityId id)
{
    fogGenericVolumeAllocator.Dealloc(id.id);
}

} // namespace Fog
