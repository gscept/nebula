//------------------------------------------------------------------------------
// lightcontext.cc
// (C) 2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "lightcontext.h"
#include "graphics/graphicsserver.h"
#include "graphics/view.h"
#include "graphics/cameracontext.h"
#include "resources/resourceserver.h"
#include "visibility/visibilitycontext.h"
#include "clustering/clustercontext.h"
#include "core/cvar.h"
#include "util/occupancyquadtree.h"
#ifndef PUBLIC_BUILD
#include "dynui/im3d/im3dcontext.h"
#include "debug/framescriptinspector.h"
#endif

#include "materials/gpulang/materialtemplatesgpulang.h"
#include "materials/materialloader.h"

#include "graphics/globalconstants.h"

#include "gpulang/render/system_shaders/shared.h"
#include "gpulang/render/system_shaders/combine.h"

#include "frame/default.h"
#include "frame/shadows.h"

#define CLUSTERED_LIGHTING_DEBUG 0

namespace Lighting
{

LightContext::GenericLightAllocator LightContext::genericLightAllocator;
LightContext::PointLightAllocator LightContext::pointLightAllocator;
LightContext::SpotLightAllocator LightContext::spotLightAllocator;
LightContext::AreaLightAllocator LightContext::areaLightAllocator;
LightContext::DirectionalLightAllocator LightContext::directionalLightAllocator;
LightContext::ShadowCasterAllocator LightContext::shadowCasterAllocator;
Util::HashTable<Graphics::GraphicsEntityId, uint, 16, 1> LightContext::shadowCasterIndexMap;
__ImplementContext(LightContext, LightContext::genericLightAllocator);

struct
{
    Util::Array<Graphics::GraphicsEntityId> spotLightEntities;
    Util::Array<Ids::Id32> shadowCastingLights;

    CoreGraphics::TextureId terrainShadowMap = CoreGraphics::InvalidTextureId;
    uint terrainShadowMapSize;
    uint terrainSize;

    Util::OccupancyQuadTree shadowAtlasTileOctree;
    CoreGraphics::TextureId shadowAtlas;
    CoreGraphics::TextureViewId shadowAtlasView;
    CoreGraphics::PassId shadowPass;

} lightServerState;

struct
{
    CoreGraphics::ShaderId classificationShader;
    CoreGraphics::ShaderProgramId cullProgram;
    CoreGraphics::ShaderProgramId debugProgram;
    CoreGraphics::ShaderProgramId renderProgram;
    CoreGraphics::BufferId clusterLightIndexLists;
    CoreGraphics::BufferSet stagingClusterLightsList;
    CoreGraphics::BufferId clusterLightsList;
    CoreGraphics::BufferId clusterPointLights;
    CoreGraphics::BufferId clusterSpotLights;

    CoreGraphics::ResourceTableId resourceTable;

    uint numThreadsThisFrame;

    // these are used to update the light clustering
    alignas(16) LightsCluster::LightLists::STRUCT lightList;
    LightsCluster::LightUniforms::STRUCT consts;

} clusterState;

struct
{
    CoreGraphics::ShaderId combineShader;
    CoreGraphics::ShaderProgramId combineProgram;
    CoreGraphics::BufferId combineConstants;
    Util::FixedArray<CoreGraphics::ResourceTableId> resourceTables;
} combineState;

struct
{

    CoreGraphics::TextureId ltcLut0;
    CoreGraphics::TextureId ltcLut1;

} textureState;

//------------------------------------------------------------------------------
/**
*/
LightContext::LightContext()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
LightContext::~LightContext()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::Create()
{
    __CreateContext();

#ifndef PUBLIC_BUILD
    Core::CVarCreate(Core::CVarType::CVar_Int, "r_shadow_debug", "0", "Show shadowmap framescript inspector [0,1]");
#endif

    __bundle.OnViewportResized = LightContext::Resize;
#ifndef PUBLIC_BUILD
    __bundle.OnRenderDebug = LightContext::OnRenderDebug;
#endif
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    lightServerState.shadowAtlasTileOctree.Setup(0x2000, 4096, 256);

    CoreGraphics::TextureCreateInfo shadowMapInfo;
    shadowMapInfo.name = "ShadowAtlas";
    shadowMapInfo.width = 0x2000;
    shadowMapInfo.height = 0x2000;
    shadowMapInfo.format = CoreGraphics::PixelFormat::D32;
    shadowMapInfo.usage = CoreGraphics::TextureUsage::Render;
    lightServerState.shadowAtlas = CoreGraphics::CreateTexture(shadowMapInfo);
    CoreGraphics::TextureViewCreateInfo shadowMapViewInfo;
    shadowMapViewInfo.name = "ShadowAtlas Attachment";
    shadowMapViewInfo.tex = lightServerState.shadowAtlas;
    shadowMapViewInfo.format = CoreGraphics::PixelFormat::D32;
    shadowMapViewInfo.bits = CoreGraphics::ImageBits::DepthBits;
    lightServerState.shadowAtlasView = CoreGraphics::CreateTextureView(shadowMapViewInfo);

    CoreGraphics::AttachmentFlagBits flags = CoreGraphics::AttachmentFlagBits::Clear | CoreGraphics::AttachmentFlagBits::Store;
    Math::vec4 clearValue = Math::vec4(0.0f);
    clearValue.x = 1.0f;

    CoreGraphics::Subpass subpass;
    subpass.depth = 0;
    subpass.numViewports = 1;
    subpass.numScissors = 1;

    CoreGraphics::PassCreateInfo passInfo;
    passInfo.name = "Shadow Atlas Render";
    passInfo.attachments.Append(lightServerState.shadowAtlasView);
    passInfo.attachmentFlags.Append(flags);
    passInfo.attachmentClears.Append(clearValue);
    passInfo.attachmentDepthStencil.Append(true);
    passInfo.subpasses.Append(subpass);
    lightServerState.shadowPass = CoreGraphics::CreatePass(passInfo);

    using namespace CoreGraphics;

    clusterState.classificationShader = CoreGraphics::ShaderGet("shd:system_shaders/lights_cluster.gplb");

    clusterState.cullProgram = ShaderGetProgram(clusterState.classificationShader, CoreGraphics::ShaderFeatureMask("Cull"));
#ifdef CLUSTERED_LIGHTING_DEBUG
    clusterState.debugProgram = ShaderGetProgram(clusterState.classificationShader, CoreGraphics::ShaderFeatureMask("Debug"));
#endif

    textureState.ltcLut0 = Resources::CreateResource("systex:ltc_1.dds", "system", nullptr, nullptr, true);
    textureState.ltcLut1 = Resources::CreateResource("systex:ltc_2.dds", "system", nullptr, nullptr, true);

    BufferCreateInfo rwbInfo;
    rwbInfo.name = "LightIndexListsBuffer";
    rwbInfo.byteSize = sizeof(LightsCluster::LightIndexLists::STRUCT);
    rwbInfo.mode = BufferAccessMode::DeviceLocal;
    rwbInfo.usageFlags = CoreGraphics::BufferUsage::ReadWrite | CoreGraphics::BufferUsage::TransferDestination;
    rwbInfo.queueSupport = CoreGraphics::GraphicsQueueSupport | CoreGraphics::ComputeQueueSupport;
    clusterState.clusterLightIndexLists = CreateBuffer(rwbInfo);

    rwbInfo.name = "LightLists";
    rwbInfo.byteSize = sizeof(LightsCluster::LightLists::STRUCT);
    clusterState.clusterLightsList = CreateBuffer(rwbInfo);

    rwbInfo.name = "LightListsStagingBuffer";
    rwbInfo.mode = BufferAccessMode::HostCached;
    rwbInfo.usageFlags = CoreGraphics::BufferUsage::TransferSource;
    clusterState.stagingClusterLightsList.Create(rwbInfo);

    // setup combine
    combineState.combineShader = CoreGraphics::ShaderGet("shd:system_shaders/combine.gplb");

    BufferCreateInfo combineBufferInfo;
    combineBufferInfo.name = "CombineConstants";
    combineBufferInfo.byteSize = sizeof(Combine::CombineUniforms::STRUCT);
    combineBufferInfo.mode = BufferAccessMode::DeviceLocal;
    combineBufferInfo.usageFlags = CoreGraphics::BufferUsage::ConstantBuffer | CoreGraphics::BufferUsage::TransferDestination;
    combineBufferInfo.queueSupport = CoreGraphics::GraphicsQueueSupport;
    combineState.combineConstants = CoreGraphics::CreateBuffer(combineBufferInfo);
    combineState.combineProgram = ShaderGetProgram(combineState.combineShader, CoreGraphics::ShaderFeatureMask("Combine"));
    combineState.resourceTables.Resize(CoreGraphics::GetNumBufferedFrames());

    for (IndexT i = 0; i < combineState.resourceTables.Size(); i++)
    {
        combineState.resourceTables[i] = ShaderCreateResourceTable(combineState.combineShader, NEBULA_BATCH_GROUP, combineState.resourceTables.Size());
        //ResourceTableSetConstantBuffer(combineState.resourceTables[i], { CoreGraphics::GetComputeConstantBuffer(MainThreadConstantBuffer), combineState.combineUniforms, 0, false, false, sizeof(Combine::CombineUniforms), 0 });
        ResourceTableSetRWTexture(combineState.resourceTables[i], { FrameScript_default::Texture_LightBuffer(), Combine::Lighting::BINDING, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableSetTexture(combineState.resourceTables[i], { FrameScript_default::Texture_SSAOBuffer(), Combine::AO::BINDING, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableSetTexture(combineState.resourceTables[i], { FrameScript_default::Texture_VolumetricFogBuffer0(), Combine::Fog::BINDING, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableSetTexture(combineState.resourceTables[i], { FrameScript_default::Texture_ReflectionBuffer(), Combine::Reflections::BINDING, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableSetTexture(combineState.resourceTables[i], { FrameScript_default::Texture_AlphaLightBuffer(), Combine::AlphaLighting::BINDING, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableSetConstantBuffer(combineState.resourceTables[i], ResourceTableBuffer(combineState.combineConstants, Combine::CombineUniforms::BINDING));
        ResourceTableCommitChanges(combineState.resourceTables[i]);
    }

    // Update main resource table
    clusterState.resourceTable = ShaderCreateResourceTable(clusterState.classificationShader, NEBULA_BATCH_GROUP);
    ResourceTableSetRWTexture(clusterState.resourceTable, { FrameScript_default::Texture_LightBuffer(), LightsCluster::Lighting::BINDING, 0, CoreGraphics::InvalidSamplerId });
#ifdef CLUSTERED_LIGHTING_DEBUG
    ResourceTableSetRWTexture(clusterState.resourceTable, { FrameScript_default::Texture_LightDebugBuffer(), LightsCluster::DebugOutput::BINDING, 0, CoreGraphics::InvalidSamplerId });
#endif
    ResourceTableCommitChanges(clusterState.resourceTable);

    // allow 256 shadow casting local lights
    lightServerState.shadowCastingLights.Reserve(256);

    FrameScript_shadows::RegisterSubgraph_ShadowAtlasUpdate_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(lightServerState.shadowAtlas);

        CoreGraphics::CmdBeginPass(cmdBuf, lightServerState.shadowPass);
        static const Util::FixedArray<Shared::RenderTargetParameters> RenderTargetParams = {Shared::RenderTargetParameters {
            .Dimensions = {(float)dims.width, (float)dims.height, 1.0f / dims.width, 1.0f / dims.height}, .Scale = {1, 1}
        }};
        CoreGraphics::PassSetRenderTargetParameters(lightServerState.shadowPass, RenderTargetParams);
        IndexT i;
        for (i = 0; i < lightServerState.shadowCastingLights.Size(); i++)
        {
            Ids::Id32 typedId = genericLightAllocator.Get<Light_TypedLightId>(lightServerState.shadowCastingLights[i]);
            const LightType type = genericLightAllocator.Get<Light_Type>(lightServerState.shadowCastingLights[i]);
            if (type == LightType::DirectionalLightType)
            {
                const Util::FixedArray<Graphics::GraphicsEntityId>& observers = directionalLightAllocator.Get<DirectionalLight_CascadeObservers>(typedId);
                const Util::FixedArray<Math::rectangle<int>>& tiles = directionalLightAllocator.Get<DirectionalLight_CascadeTiles>(typedId);
                for (IndexT j = 0; j < observers.Size(); j++)
                {
                    Ids::Id32 shadowCasterIndex = shadowCasterIndexMap[observers[j]];

                    // draw it!
                    CoreGraphics::CmdSetViewport(cmdBuf, tiles[j], 0);
                    CoreGraphics::CmdSetScissorRect(cmdBuf, tiles[j], 0);
                    Frame::DrawBatch(cmdBuf, MaterialTemplatesGPULang::BatchGroup::DirectionalLightShadow, observers[j], 1, shadowCasterIndex, bufferIndex);
                }
            }
            else if (type == LightType::SpotLightType)
            {
                Graphics::GraphicsEntityId entity = genericLightAllocator.Get<Light_Entity>(lightServerState.shadowCastingLights[i]);

                // Since these light types only need a single observer, the entity itself is the observer
                Ids::Id32 shadowCasterIndex = shadowCasterIndexMap[entity];
                const Math::rectangle<int>& shadowTile = spotLightAllocator.Get<SpotLight_ShadowTile>(typedId);
                CoreGraphics::CmdSetViewport(cmdBuf, shadowTile, 0);
                CoreGraphics::CmdSetScissorRect(cmdBuf, shadowTile, 0);
                Frame::DrawBatch(cmdBuf, MaterialTemplatesGPULang::BatchGroup::SpotLightShadow, entity, 1, shadowCasterIndex, bufferIndex);
            }
            if (type == LightType::AreaLightType)
            {

            }
            else if (type == LightType::PointLightType)
            {
                const std::array<Graphics::GraphicsEntityId, 6>& observers = pointLightAllocator.Get<PointLight_Observers>(typedId);
                const std::array<Math::rectangle<int>, 6>& tiles = pointLightAllocator.Get<PointLight_ShadowTiles>(typedId);
                for (IndexT j = 0; j < observers.size(); j++)
                {
                    Ids::Id32 shadowCasterIndex = shadowCasterIndexMap[observers[j]];

                    // draw it!
                    CoreGraphics::CmdSetViewport(cmdBuf, tiles[j], 0);
                    CoreGraphics::CmdSetScissorRect(cmdBuf, tiles[j], 0);
                    Frame::DrawBatch(cmdBuf, MaterialTemplatesGPULang::BatchGroup::PointLightShadow, observers[j], 1, shadowCasterIndex, bufferIndex);
                }
            }
        }
        lightServerState.shadowCastingLights.Clear();

        CoreGraphics::CmdEndPass(cmdBuf);
    });

    FrameScript_shadows::RegisterSubgraph_SpotlightShadows_Pass([](const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {

        /*
        IndexT i;
        for (i = 0; i < lightServerState.shadowCastingLights.Size(); i++)
        {
            // draw it!
            Ids::Id32 shadowCasterIndex = shadowCasterIndexMap[lightServerState.shadowCastingLights[i]];
            Frame::DrawBatch(cmdBuf, lightServerState.spotlightsBatchCode, lightServerState.shadowCastingLights[i], 1, slice, bufferIndex);
        }
        */
    });

    FrameScript_shadows::RegisterSubgraph_SunShadows_Pass([](const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {

    });

    // Bind shadows
    FrameScript_default::Bind_LightList(clusterState.clusterLightsList);
    FrameScript_default::Bind_ClusterLightIndexLists(clusterState.clusterLightIndexLists);
    FrameScript_default::RegisterSubgraph_LightsCopy_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CoreGraphics::BufferCopy from, to;
        from.offset = 0;
        to.offset = 0;
        CmdCopy(cmdBuf, clusterState.stagingClusterLightsList.buffers[bufferIndex], { from }, clusterState.clusterLightsList, { to }, sizeof(LightsCluster::LightLists::STRUCT));
    }, {
        { FrameScript_default::BufferIndex::LightList, CoreGraphics::PipelineStage::TransferWrite }
    });

    FrameScript_default::RegisterSubgraph_LightsCull_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, clusterState.cullProgram, queue);

        // run chunks of 1024 threads at a time
        std::array<SizeT, 3> dimensions = Clustering::ClusterContext::GetClusterDimensions();
        CmdDispatch(cmdBuf, Math::ceil((dimensions[0] * dimensions[1] * dimensions[2]) / 64.0f), 1, 1);
    }, {
        { FrameScript_default::BufferIndex::LightList, CoreGraphics::PipelineStage::ComputeShaderRead }
        , { FrameScript_default::BufferIndex::ClusterLightIndexLists, CoreGraphics::PipelineStage::ComputeShaderWrite }
        , { FrameScript_default::BufferIndex::ClusterBuffer, CoreGraphics::PipelineStage::ComputeShaderRead }
    });

    FrameScript_default::RegisterSubgraph_LightsCombine_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::QueueType queue, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, combineState.combineProgram, queue);
        CmdSetResourceTable(cmdBuf, combineState.resourceTables[bufferIndex], NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
        CoreGraphics::TextureDimensions dims = TextureGetDimensions(FrameScript_default::Texture_LightBuffer());
        Combine::CombineUniforms::STRUCT combineConsts;
        combineConsts.LowresResolution[0] = 1.0f / dims.width;
        combineConsts.LowresResolution[1] = 1.0f / dims.height;
        combineConsts.Viewport[0] = viewport.width();
        combineConsts.Viewport[1] = viewport.height();
        CmdUpdateBuffer(cmdBuf, combineState.combineConstants, 0, sizeof(Combine::CombineUniforms::STRUCT), &combineConsts);

        // perform debug output
        CmdDispatch(cmdBuf, Math::divandroundup(viewport.width(), 64), viewport.height(), 1);
    }, nullptr, {
        { FrameScript_default::TextureIndex::LightBuffer, CoreGraphics::PipelineStage::ComputeShaderWrite }
        , { FrameScript_default::TextureIndex::SSAOBuffer, CoreGraphics::PipelineStage::ComputeShaderRead }
        , { FrameScript_default::TextureIndex::VolumetricFogBuffer0, CoreGraphics::PipelineStage::ComputeShaderRead }
        , { FrameScript_default::TextureIndex::ReflectionBuffer, CoreGraphics::PipelineStage::ComputeShaderRead }
    });
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::Discard()
{
    Graphics::GraphicsServer::Instance()->UnregisterGraphicsContext(&__bundle);
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::SetupDirectionalLight(const Graphics::GraphicsEntityId id, const DirectionalLightSetupInfo& info)
{
    n_assert(id != Graphics::GraphicsEntityId::Invalid());
    n_assert(directionalLightAllocator.Size() < Shared::MAX_DIRECTIONAL_LIGHTS);

    auto lid = directionalLightAllocator.Alloc();

    const Graphics::ContextEntityId cid = GetContextId(id);
    genericLightAllocator.Set<Light_Entity>(cid.id, id);
    genericLightAllocator.Set<Light_Type>(cid.id, LightType::DirectionalLightType);
    genericLightAllocator.Set<Light_Color>(cid.id, info.color);
    genericLightAllocator.Set<Light_Intensity>(cid.id, info.intensity);
    genericLightAllocator.Set<Light_TypedLightId>(cid.id, lid);
    genericLightAllocator.Set<Light_StageMask>(cid.id, info.stageMask);

    Math::point sunPosition(Math::cos(info.azimuth) * Math::sin(info.zenith), Math::cos(info.zenith), Math::sin(info.azimuth) * Math::sin(info.zenith));
    Math::mat4 mat = lookat(Math::point(0.0f), sunPosition, Math::vector::upvec());

    SetDirectionalLightTransform(cid, mat, Math::xyz(sunPosition));
    directionalLightAllocator.Set<DirectionalLight_View>(lid, info.view);
    directionalLightAllocator.Set<DirectionalLight_CascadeDistances>(lid, Util::FixedArray<float>(Shared::NUM_CASCADES));
    directionalLightAllocator.Set<DirectionalLight_CascadeTransforms>(lid, Util::FixedArray<Math::mat4>(Shared::NUM_CASCADES));
    bool canCastShadows = info.castShadows;
    if (info.castShadows)
    {
        // create new graphics entity for each view
        SizeT cascadeSize = 2048;
        Util::FixedArray<Math::rectangle<int>> shadowTiles(Shared::NUM_CASCADES);
        Util::FixedArray<Graphics::GraphicsEntityId> cascadeObservers(Shared::NUM_CASCADES);
        for (IndexT i = 0; i < Shared::NUM_CASCADES; i++)
        {
            Math::uint2 section = lightServerState.shadowAtlasTileOctree.Allocate(cascadeSize);
            if (section.x == 0xFFFFFFFF)
            {
                n_warning("Shadow atlas is full! Cannot allocate shadow map for directional light!\n");
                canCastShadows = false;
                for (uint j = 0; j < i; j++)
                {
                    lightServerState.shadowAtlasTileOctree.Deallocate(Math::uint2{ (uint)shadowTiles[j].left, (uint)shadowTiles[j].top }, cascadeSize);
                }
                break;
            }
            else
            {
                shadowTiles[i] = Math::rectangle<int>(section.x, section.y, section.x + cascadeSize, section.y + cascadeSize);

                Graphics::GraphicsEntityId shadowId = Graphics::CreateEntity();
                Visibility::ObserverContext::RegisterEntity(shadowId);
                Visibility::ObserverContext::Setup(shadowId, Visibility::VisibilityEntityType::Light, Graphics::SHADOW_STAGE_MASK, true);

                // allocate shadow caster slice
                Ids::Id32 casterId = shadowCasterAllocator.Alloc();
                shadowCasterIndexMap.Add(shadowId, casterId);

                // store entity id for cleanup later
                cascadeObservers[i] = shadowId;
            }
        }
        directionalLightAllocator.Set<DirectionalLight_CascadeTiles>(lid, shadowTiles);
        directionalLightAllocator.Set<DirectionalLight_CascadeObservers>(lid, cascadeObservers);
    }
    genericLightAllocator.Set<Light_ShadowCaster>(cid.id, canCastShadows);

}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::SetupPointLight(const Graphics::GraphicsEntityId id, const PointLightSetupInfo& info)
{
    n_assert(id != Graphics::GraphicsEntityId::Invalid());
    n_assert(pointLightAllocator.Size() < Shared::MAX_POINT_LIGHTS);
    const Graphics::ContextEntityId cid = GetContextId(id);
    auto pli = pointLightAllocator.Alloc();
    bool canCastShadows = info.castShadows;
    if (info.castShadows)
    {
        std::array<Math::rectangle<int>, 6> shadowTiles;
        for (uint i = 0; i < 6; i++)
        {
            Math::uint2 section = lightServerState.shadowAtlasTileOctree.Allocate(256);
            if (section.x == 0xFFFFFFFF)
            {
                n_warning("Shadow atlas is full! Cannot allocate shadow map for point light!\n");
                canCastShadows = false;
                for (uint j = 0; j < i; j++)
                {
                    lightServerState.shadowAtlasTileOctree.Deallocate(Math::uint2{ (uint)shadowTiles[j].left, (uint)shadowTiles[j].top }, 256);
                }
                break;
            }
            else
            {
                shadowTiles[i] = Math::rectangle<int>(section.x, section.y, section.x + 256, section.y + 256);
            }
        }
        pointLightAllocator.Set<PointLight_ShadowTiles>(pli, shadowTiles);

        std::array<Graphics::GraphicsEntityId, 6> shadowEntities;
        for (uint i = 0; i < 6; i++)
        {
            Graphics::GraphicsEntityId shadowId = Graphics::CreateEntity();
            Visibility::ObserverContext::RegisterEntity(shadowId);
            Visibility::ObserverContext::Setup(shadowId, Visibility::VisibilityEntityType::Light, Graphics::SHADOW_STAGE_MASK);
            Ids::Id32 casterId = shadowCasterAllocator.Alloc();
            shadowCasterIndexMap.Add(shadowId, casterId);
            shadowEntities[i] = shadowId;
        }
        pointLightAllocator.Set<PointLight_Observers>(pli, shadowEntities);
    }
    genericLightAllocator.Set<Light_Entity>(cid.id, id);
    genericLightAllocator.Set<Light_Type>(cid.id, LightType::PointLightType);
    genericLightAllocator.Set<Light_Color>(cid.id, info.color);
    genericLightAllocator.Set<Light_Intensity>(cid.id, info.intensity);
    genericLightAllocator.Set<Light_ShadowCaster>(cid.id, canCastShadows);
    genericLightAllocator.Set<Light_Range>(cid.id, info.range);
    genericLightAllocator.Set<Light_TypedLightId>(cid.id, pli);
    genericLightAllocator.Set<Light_StageMask>(cid.id, info.stageMask);

    // set initial state
    pointLightAllocator.Set<PointLight_ProjectionTexture>(pli, info.projection);
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::SetupSpotLight(const Graphics::GraphicsEntityId id, const SpotLightSetupInfo& info)
{
    n_assert(id != Graphics::GraphicsEntityId::Invalid());
    n_assert(spotLightAllocator.Size() < Shared::MAX_SPOT_LIGHTS);
    const Graphics::ContextEntityId cid = GetContextId(id);
    auto sli = spotLightAllocator.Alloc();
    bool canCastShadows = info.castShadows;
    if (info.castShadows)
    {
        Math::uint2 section = lightServerState.shadowAtlasTileOctree.Allocate(256);
        if (section.x == 0xFFFFFFFF)
        {
            n_warning("Shadow atlas is full! Cannot allocate shadow map for spot light!\n");
            canCastShadows = false;
        }
        else
        {
            spotLightAllocator.Set<SpotLight_ShadowTile>(sli, Math::rectangle<int>(section.x, section.y, section.x + 256, section.y + 256));
        }

        // allocate shadow caster slice
        Ids::Id32 casterId = shadowCasterAllocator.Alloc();
        shadowCasterIndexMap.Add(id, casterId);

        Visibility::ObserverContext::RegisterEntity(id);
        Visibility::ObserverContext::Setup(id, Visibility::VisibilityEntityType::Light, Graphics::SHADOW_STAGE_MASK);
    }
    genericLightAllocator.Set<Light_Entity>(cid.id, id);
    genericLightAllocator.Set<Light_Type>(cid.id, LightType::SpotLightType);
    genericLightAllocator.Set<Light_Color>(cid.id, info.color);
    genericLightAllocator.Set<Light_Intensity>(cid.id, info.intensity);
    genericLightAllocator.Set<Light_ShadowCaster>(cid.id, canCastShadows);
    genericLightAllocator.Set<Light_Range>(cid.id, info.range);
    genericLightAllocator.Set<Light_TypedLightId>(cid.id, sli);
    genericLightAllocator.Set<Light_StageMask>(cid.id, info.stageMask);

    std::array<float, 2> angles = { info.innerConeAngle, info.outerConeAngle };
    if (info.innerConeAngle >= info.outerConeAngle)
        angles[0] = info.outerConeAngle - Math::deg2rad(0.1f);

    // construct projection from angle and range
    const float zNear = 0.1f;
    const float zFar = info.range;

    // use a fixed aspect of 1
    Math::mat4 proj = Math::perspfov(angles[1] * 2.0f, 1.0f, zNear, zFar);
    proj.r[1][1] *= -1.0f; // vulkan clip space

    // set initial state
    spotLightAllocator.Set<SpotLight_ProjectionTexture>(sli, info.projection);
    spotLightAllocator.Set<SpotLight_ConeAngles>(sli, angles);
    spotLightAllocator.Set<SpotLight_Observer>(sli, id);
    spotLightAllocator.Set<SpotLight_ProjectionTransform>(sli, proj);
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::SetupAreaLight(const Graphics::GraphicsEntityId id, const AreaLightSetupInfo& info)
{
    n_assert(id != Graphics::GraphicsEntityId::Invalid());
    n_assert(areaLightAllocator.Size() < Shared::MAX_AREA_LIGHTS);
    const Graphics::ContextEntityId cid = GetContextId(id);
    auto ali = areaLightAllocator.Alloc();
    genericLightAllocator.Set<Light_Entity>(cid.id, id);
    genericLightAllocator.Set<Light_Type>(cid.id, LightType::AreaLightType);
    genericLightAllocator.Set<Light_Color>(cid.id, info.color);
    genericLightAllocator.Set<Light_Intensity>(cid.id, info.intensity);
    genericLightAllocator.Set<Light_ShadowCaster>(cid.id, info.castShadows);
    genericLightAllocator.Set<Light_Range>(cid.id, info.range);
    genericLightAllocator.Set<Light_TypedLightId>(cid.id, ali);
    genericLightAllocator.Set<Light_StageMask>(cid.id, info.stageMask);

    std::array<Math::rectangle<int>, 2> shadowTiles;
    bool canCastShadows = info.castShadows;
    if (info.castShadows)
    {
        for (uint i = 0; i < shadowTiles.size(); i++)
        {
            Math::uint2 section = lightServerState.shadowAtlasTileOctree.Allocate(256);
            if (section.x == 0xFFFFFFFF)
            {
                n_warning("Shadow atlas is full! Cannot allocate shadow map for area light!\n");
                canCastShadows = false;
                for (uint j = 0; j < i; j++)
                {
                    lightServerState.shadowAtlasTileOctree.Deallocate(Math::uint2{ (uint)shadowTiles[j].left, (uint)shadowTiles[j].top }, 256);
                }
                break;
            }
            else
            {
                shadowTiles[i] = Math::rectangle<int>(section.x, section.y, section.x + 256, section.y + 256);
            }
        }
        areaLightAllocator.Set<AreaLight_ShadowTiles>(ali, shadowTiles);

        std::array<Graphics::GraphicsEntityId, 2> observerIds;

        for (uint i = 0; i < observerIds.size(); i++)
        {
            Graphics::GraphicsEntityId observerId = Graphics::CreateEntity();

            // allocate shadow caster slice
            Ids::Id32 casterId = shadowCasterAllocator.Alloc();
            shadowCasterIndexMap.Add(observerId, casterId);

            Visibility::ObserverContext::RegisterEntity(observerId);
            Visibility::ObserverContext::Setup(observerId, Visibility::VisibilityEntityType::Light, Graphics::SHADOW_STAGE_MASK);
            observerIds[i] = observerId;
        }
        areaLightAllocator.Set<AreaLight_Observers>(ali, observerIds);
    }

    // set initial state
    areaLightAllocator.Set<AreaLight_Shape>(ali, info.shape);
    areaLightAllocator.Set<AreaLight_TwoSided>(ali, info.twoSided || info.shape == AreaLightShape::Tube);
    areaLightAllocator.Set<AreaLight_RenderMesh>(ali, info.renderMesh);
    //areaLightAllocator.Set<AreaLight_Projection>(ali, proj);

    // Last step is to create a geometric proxy for the light source
    if (info.renderMesh)
    {
        // Create material
        MaterialTemplatesGPULang::Entry* matTemplate = &MaterialTemplatesGPULang::base::__AreaLight.entry;
        Materials::MaterialId material = Materials::CreateMaterial(matTemplate, "AreaLight");

        const MaterialTemplatesGPULang::MaterialTemplateValue& value = MaterialTemplatesGPULang::base::__AreaLight.__EmissiveColor;
        MaterialInterfaces::ArealightMaterial* data = ArrayAllocStack<MaterialInterfaces::ArealightMaterial>(1);
        (info.color * info.intensity).store(data->EmissiveColor);
        data->EmissiveColor[3] = 1.0f;
        Materials::MaterialSetConstants(material, data, sizeof(MaterialInterfaces::ArealightMaterial));
        ArrayFreeStack(1, data);

        CoreGraphics::MeshId mesh;
        switch (info.shape)
        {
            case AreaLightShape::Disk:
                mesh = CoreGraphics::DiskMesh;
                break;
            case AreaLightShape::Rectangle:
                mesh = CoreGraphics::RectangleMesh;
                break;
            case AreaLightShape::Tube:
                mesh = CoreGraphics::RectangleMesh;
                break;
        }

        Graphics::RegisterEntity<Models::ModelContext, Visibility::ObservableContext>(id);
        Math::bbox box;
        Models::ModelContext::Setup(
            id
            , Math::mat4()
            , box
            , material
            , mesh
            , 0
        );
        Models::ModelContext::SetTransform(id, Math::mat4());
        Visibility::ObservableContext::Setup(id, Visibility::VisibilityEntityType::Model);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::SetColor(const Graphics::GraphicsEntityId id, const Math::vec3& color)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    genericLightAllocator.Get<Light_Color>(cid.id) = color;
}

//------------------------------------------------------------------------------
/**
*/
Math::vec3
LightContext::GetColor(const Graphics::GraphicsEntityId id)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    return genericLightAllocator.Get<Light_Color>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::SetRange(const Graphics::GraphicsEntityId id, const float range)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    genericLightAllocator.Get<Light_Range>(cid.id) = range;
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::SetIntensity(const Graphics::GraphicsEntityId id, const float intensity)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    genericLightAllocator.Get<Light_Intensity>(cid.id) = intensity;
}

//------------------------------------------------------------------------------
/**
*/
float
LightContext::GetIntensity(const Graphics::GraphicsEntityId id)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    return genericLightAllocator.Get<Light_Intensity>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::SetTransform(const Graphics::GraphicsEntityId id, const float azimuth, const float zenith)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    if (cid == Graphics::ContextEntityId::Invalid())
            return;

    Math::point position(Math::cos(azimuth) * Math::sin(zenith), Math::cos(zenith), Math::sin(azimuth) * Math::sin(zenith));
    Math::mat4 mat = lookat(Math::point(0.0f), position, Math::vector::upvec());

    LightType type = genericLightAllocator.Get<Light_Type>(cid.id);

    switch (type)
    {
        case LightType::DirectionalLightType:
            SetDirectionalLightTransform(cid, mat, Math::xyz(position));
            break;
        default:
            break;
    }
}

//------------------------------------------------------------------------------
/**
*/
const Math::mat4
LightContext::GetTransform(const Graphics::GraphicsEntityId id)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    LightType type = genericLightAllocator.Get<Light_Type>(cid.id);
    Ids::Id32 lightId = genericLightAllocator.Get<Light_TypedLightId>(cid.id);

    switch (type)
    {
    case LightType::DirectionalLightType:
        return directionalLightAllocator.Get<DirectionalLight_Transform>(lightId);
    case LightType::SpotLightType:
        return spotLightAllocator.Get<SpotLight_Transform>(lightId).getmatrix();
    case LightType::PointLightType:
        return pointLightAllocator.Get<PointLight_Transform>(lightId).getmatrix();
    case LightType::AreaLightType:
        return areaLightAllocator.Get<AreaLight_Transform>(lightId).getmatrix();
    default:
        return Math::mat4();
    }
}

//------------------------------------------------------------------------------
/**
*/
const Math::mat4
LightContext::GetObserverTransform(const Graphics::GraphicsEntityId id)
{
    const Ids::Id32 cid = shadowCasterIndexMap[id.id];
    return shadowCasterAllocator.Get<ShadowCaster_Transform>(cid);
}

//------------------------------------------------------------------------------
/**
*/
const void
LightContext::SetPosition(const Graphics::GraphicsEntityId id, const Math::point& position)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    if (cid == Graphics::ContextEntityId::Invalid())
        return;

    LightType type = genericLightAllocator.Get<Light_Type>(cid.id);
    auto lid = genericLightAllocator.Get<Light_TypedLightId>(cid.id);

    switch (type)
    {
        case LightType::SpotLightType:
            spotLightAllocator.Get<SpotLight_Transform>(lid).setposition(position);
            break;
        case LightType::PointLightType:
            pointLightAllocator.Get<PointLight_Transform>(lid).setposition(position);
            break;
        case LightType::AreaLightType:
            areaLightAllocator.Get<AreaLight_Transform>(lid).setposition(position);
            if (areaLightAllocator.Get<AreaLight_RenderMesh>(lid))
                Models::ModelContext::SetTransform(id, areaLightAllocator.Get<AreaLight_Transform>(lid).getmatrix());
            break;
        default: n_error("unhandled enum"); break;
    }
}

//------------------------------------------------------------------------------
/**
*/
const Math::point
LightContext::GetPosition(const Graphics::GraphicsEntityId id)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    LightType type = genericLightAllocator.Get<Light_Type>(cid.id);
    auto lid = genericLightAllocator.Get<Light_TypedLightId>(cid.id);
    switch (type)
    {
        case LightType::SpotLightType:
            return spotLightAllocator.Get<SpotLight_Transform>(lid).getposition();
        case LightType::PointLightType:
            return pointLightAllocator.Get<PointLight_Transform>(lid).getposition();
        case LightType::AreaLightType:
            return areaLightAllocator.Get<AreaLight_Transform>(lid).getposition();
        default:
            return directionalLightAllocator.Get<DirectionalLight_Direction>(lid).vec;
    }
}

//------------------------------------------------------------------------------
/**
*/
const void
LightContext::SetRotation(const Graphics::GraphicsEntityId id, const Math::quat& rotation)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    if (cid == Graphics::ContextEntityId::Invalid())
            return;

    LightType type = genericLightAllocator.Get<Light_Type>(cid.id);
    auto lid = genericLightAllocator.Get<Light_TypedLightId>(cid.id);

    switch (type)
    {
        case LightType::SpotLightType:
            spotLightAllocator.Get<SpotLight_Transform>(lid).setrotate(rotation);
            break;
        case LightType::PointLightType:
            pointLightAllocator.Get<PointLight_Transform>(lid).setrotate(rotation);
            break;
        case LightType::AreaLightType:
            areaLightAllocator.Get<AreaLight_Transform>(lid).setrotate(rotation);
            if (areaLightAllocator.Get<AreaLight_RenderMesh>(lid))
                Models::ModelContext::SetTransform(id, areaLightAllocator.Get<AreaLight_Transform>(lid).getmatrix());
            break;
        default: n_error("unhandled enum"); break;
    }
}

//------------------------------------------------------------------------------
/**
*/
const Math::quat
LightContext::GetRotation(const Graphics::GraphicsEntityId id)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    LightType type = genericLightAllocator.Get<Light_Type>(cid.id);
    auto lid = genericLightAllocator.Get<Light_TypedLightId>(cid.id);
    switch (type)
    {
        case LightType::SpotLightType:
            return spotLightAllocator.Get<SpotLight_Transform>(lid).getrotate();
        case LightType::PointLightType:
            return pointLightAllocator.Get<PointLight_Transform>(lid).getrotate();
        case LightType::AreaLightType:
            return areaLightAllocator.Get<AreaLight_Transform>(lid).getrotate();
        default:
            return Math::quat();
    }
}

//------------------------------------------------------------------------------
/**
*/
const void
LightContext::SetScale(const Graphics::GraphicsEntityId id, const Math::vec3& scale)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    if (cid == Graphics::ContextEntityId::Invalid())
            return;

    LightType type = genericLightAllocator.Get<Light_Type>(cid.id);
    auto lid = genericLightAllocator.Get<Light_TypedLightId>(cid.id);

    switch (type)
    {
        case LightType::SpotLightType:
            spotLightAllocator.Get<SpotLight_Transform>(lid).setscale(scale);
            break;
        case LightType::PointLightType:
            pointLightAllocator.Get<PointLight_Transform>(lid).setscale(scale);
            break;
        case LightType::AreaLightType:
        {
            auto shape = areaLightAllocator.Get<AreaLight_Shape>(lid);
            Math::vec3 adjustedScale = scale;
            if (shape == AreaLightShape::Tube)
            {
                adjustedScale.y = 0.1f;
                adjustedScale.z = 1.0f;
            }
            areaLightAllocator.Get<AreaLight_Transform>(lid).setscale(adjustedScale);
            if (areaLightAllocator.Get<AreaLight_RenderMesh>(lid))
                Models::ModelContext::SetTransform(id, areaLightAllocator.Get<AreaLight_Transform>(lid).getmatrix());
        } break;
        default: n_error("unhandled enum"); break;
    }
}

//------------------------------------------------------------------------------
/**
*/
const Math::vec3
LightContext::GetScale(const Graphics::GraphicsEntityId id)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    LightType type = genericLightAllocator.Get<Light_Type>(cid.id);
    auto lid = genericLightAllocator.Get<Light_TypedLightId>(cid.id);
    switch (type)
    {
        case LightType::SpotLightType:
            return spotLightAllocator.Get<SpotLight_Transform>(lid).getscale();
        case LightType::PointLightType:
            return pointLightAllocator.Get<PointLight_Transform>(lid).getscale();
        case LightType::AreaLightType:
            return areaLightAllocator.Get<AreaLight_Transform>(lid).getscale();
        default:
            return Math::vec3();
    }
}

//------------------------------------------------------------------------------
/**
*/
LightContext::LightType
LightContext::GetType(const Graphics::GraphicsEntityId id)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    return genericLightAllocator.Get<Light_Type>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::GetInnerOuterAngle(const Graphics::GraphicsEntityId id, float& inner, float& outer)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    LightType type = genericLightAllocator.Get<Light_Type>(cid.id);
    n_assert(type == LightType::SpotLightType);
    Ids::Id32 lightId = genericLightAllocator.Get<Light_TypedLightId>(cid.id);
    inner = spotLightAllocator.Get<SpotLight_ConeAngles>(lightId)[0];
    outer = spotLightAllocator.Get<SpotLight_ConeAngles>(lightId)[1];
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::SetInnerOuterAngle(const Graphics::GraphicsEntityId id, float inner, float outer)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    if (cid == Graphics::ContextEntityId::Invalid())
            return;

    LightType type = genericLightAllocator.Get<Light_Type>(cid.id);
    n_assert(type == LightType::SpotLightType);
    Ids::Id32 lightId = genericLightAllocator.Get<Light_TypedLightId>(cid.id);
    if (inner >= outer)
        inner = outer - Math::deg2rad(0.1f);
    spotLightAllocator.Get<SpotLight_ConeAngles>(lightId)[0] = inner;
    spotLightAllocator.Get<SpotLight_ConeAngles>(lightId)[1] = outer;
}

//------------------------------------------------------------------------------
/**
*/
void
CalculateCSMSplits(
    const Graphics::GraphicsEntityId camera,
    const Math::vec3 lightDirection,
    const Util::FixedArray<float>& distances,
    Util::FixedArray<Math::mat4>& projections,
    Util::FixedArray<Math::mat4>& viewProjections,
    const float shadowTileSize,
    const Math::bbox& shadowRegion
)
{
    n_assert(camera != Graphics::GraphicsEntityId::Invalid());
    n_assert(distances.Size() == projections.Size());
    n_assert(distances.Size() == viewProjections.Size());
    const Graphics::CameraSettings& camSettings = Graphics::CameraContext::GetSettings(camera);
    float aspect = camSettings.GetAspect();
    float fov = camSettings.GetFov();
    Math::mat4 invCameraView = Math::inverse(Graphics::CameraContext::GetView(camera));
    Math::mat4 invCameraProjection = Math::inverse(Graphics::CameraContext::GetProjection(camera));

    // Setup device coordinates
    Math::vec4 ndcPoints[8] = {
        Math::vec4(-1, -1, 0, 1), Math::vec4(1, -1, 0, 1), Math::vec4(1, 1, 0, 1), Math::vec4(-1, 1, 0, 1),
        Math::vec4(-1, -1, 1, 1), Math::vec4(1, -1, 1, 1), Math::vec4(1, 1, 1, 1), Math::vec4(-1, 1, 1, 1)
    };

    // Deproject and invert camera view to get world space position of frustum corners
    Math::vec4 frustumPoints[8];
    for (uint i = 0; i < 8; i++)
    {
        frustumPoints[i] = (invCameraProjection * ndcPoints[i]);
        frustumPoints[i] *= 1.0f / frustumPoints[i].w;
        frustumPoints[i] = invCameraView * frustumPoints[i];
    }

    float intervalStart = camSettings.GetZNear();
    float intervalEnd = 0;
    for (int cascadeIndex = 0; cascadeIndex < distances.Size(); ++cascadeIndex)
    {
        intervalEnd = distances[cascadeIndex];

        // Expand cascade a little
        distances[cascadeIndex] *= 1.005f;

        float tNear = (intervalStart - camSettings.GetZNear()) / (camSettings.GetZFar() - camSettings.GetZNear());
        float tFar = (intervalEnd - camSettings.GetZNear()) / (camSettings.GetZFar() - camSettings.GetZNear());

        // Calculate new corner points at interval offset
        Math::vec4 cascadePoints[8];
        for (uint i = 0; i < 4; i++)
        {
            Math::vec4 nearCorner = frustumPoints[i];
            Math::vec4 farCorner = frustumPoints[i+4];
            Math::vec4 dir = farCorner - nearCorner;

            cascadePoints[i] = nearCorner + dir * tNear;
            cascadePoints[i + 4] = nearCorner + dir * tFar;
        }

        // Find the centroid
        Math::vec4 center = Math::vec4(0);
        for (auto& point : cascadePoints)
            center += point;
        center *= 1.0f / 8;
        center.w = 1.0f;

        // Calculate the radius to get a symmetrically enclosing sphere.
        float radius = 0;
        for (const auto& point : cascadePoints)
            radius = Math::max(radius, Math::length3(point - center));

        radius = Math::ceil(radius * 16.0f) / 16.0f;

        Math::vec4 lightPos = center - Math::vec4(-lightDirection * radius, 0);
        Math::mat4 lightView = Math::lookat(lightPos, center, Math::vector::upvec());

        Math::vec4 centerLS = lightView * center;
        float worldUnitsPerTexel = (2 * radius) / shadowTileSize;
        centerLS.x = Math::floor(centerLS.x / worldUnitsPerTexel) * worldUnitsPerTexel;
        centerLS.y = Math::floor(centerLS.y / worldUnitsPerTexel) * worldUnitsPerTexel;
        Math::vec4 snappedCenter = Math::inverse(lightView) * centerLS;
        lightPos = snappedCenter - Math::vec4(-lightDirection * radius, 0);
        lightView = Math::lookat(lightPos, snappedCenter, Math::vector::upvec());

        static const Math::vec4 extentsMap[] =
        {
            Math::vec4(1.0f, 1.0f, -1.0f, 1.0f),
            Math::vec4(-1.0f, 1.0f, -1.0f, 1.0f),
            Math::vec4(1.0f, -1.0f, -1.0f, 1.0f),
            Math::vec4(-1.0f, -1.0f, -1.0f, 1.0f),
            Math::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            Math::vec4(-1.0f, 1.0f, 1.0f, 1.0f),
            Math::vec4(1.0f, -1.0f, 1.0f, 1.0f),
            Math::vec4(-1.0f, -1.0f, 1.0f, 1.0f)
        };

        Math::vec4 sceneCenter = shadowRegion.center();
        Math::vec4 sceneExtents = shadowRegion.extents();
        Math::vec4 sceneAABBLightPoints[8];
        for (int index = 0; index < 8; ++index)
        {
            sceneAABBLightPoints[index] = lightView * multiplyadd(extentsMap[index], sceneExtents, sceneCenter);
        }

        Math::vec4 lightCameraOrthographicMin = Math::vec4(FLT_MAX);
        Math::vec4 lightCameraOrthographicMax = Math::vec4(-FLT_MAX);
        for (const auto& point : cascadePoints)
        {
            const Math::vec4 lightViewPoint = lightView * point;
            lightCameraOrthographicMin = Math::minimize(lightCameraOrthographicMin, lightViewPoint);
            lightCameraOrthographicMax = Math::maximize(lightCameraOrthographicMax, lightViewPoint);
        }

        Math::mat4 cascadeProjectionMatrix = Math::orthooffcenter(
            -radius,
            radius,
            -radius,
            radius,
            -Math::length(sceneExtents),
            Math::length(sceneExtents)
        );

        projections[cascadeIndex] = cascadeProjectionMatrix;
        viewProjections[cascadeIndex] = cascadeProjectionMatrix * lightView;

        intervalStart = intervalEnd;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::OnPrepareView(const Graphics::ViewId view, const Graphics::FrameContext& ctx)
{
    Shared::ShadowViewConstants::STRUCT& shadowConstants = ViewGetShadowConstants(view);

    const Util::Array<LightType>& types = genericLightAllocator.GetArray<Light_Type>();
    const Util::Array<Ids::Id32>& typeIds = genericLightAllocator.GetArray<Light_TypedLightId>();
    const Util::Array<Graphics::StageMask>& stageMasks = genericLightAllocator.GetArray<Light_StageMask>();
    const Util::Array<bool>& shadowCasters = genericLightAllocator.GetArray<Light_ShadowCaster>();
    IndexT localLightShadowCasterCount = 0;
    CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(lightServerState.shadowAtlas);

    for (IndexT i = 0; i < types.Size(); i++)
    {
        if (types[i] == LightType::DirectionalLightType)
        {
            if (stageMasks[i] & Graphics::ViewGetStageMask(view) && shadowCasters[i])
            {
                const Math::vector& direction = directionalLightAllocator.Get<DirectionalLight_Direction>(typeIds[i]);
                const Util::FixedArray<Math::rectangle<int>>& tiles = directionalLightAllocator.Get<DirectionalLight_CascadeTiles>(typeIds[i]);

                const Util::FixedArray<float> distances = { 5, 15, 50, 100 };
                Util::FixedArray<Math::mat4> projections(4);
                Util::FixedArray<Math::mat4> viewProjections(4);
                CalculateCSMSplits(
                    ViewGetCamera(view),
                    direction,
                    distances,
                    projections,
                    viewProjections,
                    tiles[i].width(),
                    Math::bbox(Math::point(0), Math::vector(500))
                );

                const Util::FixedArray<Graphics::GraphicsEntityId>& observers = directionalLightAllocator.Get<DirectionalLight_CascadeObservers>(typeIds[i]);

                for (IndexT j = 0; j < observers.Size(); j++)
                {
                    // do reverse lookup to find shadow caster
                    Ids::Id32 ctxId = shadowCasterIndexMap[observers[j]];

                    shadowCasterAllocator.Set<ShadowCaster_Transform>(ctxId, viewProjections[j]);
                    viewProjections[j].store(&shadowConstants.ShadowViewProjection[ctxId][0][0]);

                    float tileScaleX = tiles[j].width() / (float)dims.width;
                    float tileScaleY = tiles[j].height() / (float)dims.height;
                    float tileOffsetX = tiles[j].left / (float)dims.width;
                    float tileOffsetY = tiles[j].top / (float)dims.height;
                    Math::mat4 atlasOffset = Math::translation(tileOffsetX, tileOffsetY, 0);
                    Math::mat4 atlasScale = Math::scaling(tileScaleX, tileScaleY, 1);
                    Math::mat4 textureTranslation = Math::translation(0.5f, 0.5f, 0);
                    Math::mat4 textureScale = Math::scaling(0.5f, 0.5f, 1.0f);

                    Math::mat4 shadowTexture = atlasOffset * atlasScale * textureTranslation * textureScale * viewProjections[j];

                    directionalLightAllocator.Get<DirectionalLight_CascadeDistances>(typeIds[i])[j] = distances[j];
                    directionalLightAllocator.Get<DirectionalLight_CascadeTransforms>(typeIds[i])[j] = shadowTexture;
                }
            }
        }
        else if (types[i] == LightType::SpotLightType && shadowCasters[i])
        {

            Math::mat4 projection, view;
            Graphics::GraphicsEntityId observer;

            projection = spotLightAllocator.Get<SpotLight_ProjectionTransform>(typeIds[i]);
            view = spotLightAllocator.Get<SpotLight_Transform>(typeIds[i]).getmatrix();
            observer = spotLightAllocator.Get<SpotLight_Observer>(typeIds[i]);
            Math::mat4 viewProjection = projection * inverse(view);

            Ids::Id32 ctxId = shadowCasterIndexMap[observer];
            shadowCasterAllocator.Set<ShadowCaster_Transform>(ctxId, viewProjection);
            viewProjection.store(&shadowConstants.ShadowViewProjection[ctxId][0][0]);

            const Math::rectangle<int>& shadowTile = spotLightAllocator.Get<SpotLight_ShadowTile>(typeIds[i]);
            float tileScaleX = shadowTile.width() / (float)dims.width;
            float tileScaleY = shadowTile.height() / (float)dims.height;
            float tileOffsetX = shadowTile.left / (float)dims.width;
            float tileOffsetY = shadowTile.top / (float)dims.height;
            Math::mat4 atlasOffset = Math::translation(tileOffsetX, tileOffsetY, 0);
            Math::mat4 atlasScale = Math::scaling(tileScaleX, tileScaleY, 1);
            Math::mat4 textureTranslation = Math::translation(0.5f, 0.5f, 0);
            Math::mat4 textureScale = Math::scaling(0.5f, 0.5f, 1.0f);
            spotLightAllocator.Set<SpotLight_ShadowProjectionTransform>(typeIds[i], atlasOffset * atlasScale * textureTranslation * textureScale * viewProjection);
        }
        else if (types[i] == LightType::AreaLightType)
        {

        }
        else if (types[i] == LightType::PointLightType && shadowCasters[i])
        {
            Math::mat4 projection = Math::perspfov(Math::deg2rad(90.0f), 1.0f, 0.1f, genericLightAllocator.Get<Light_Range>(i));
            projection.r[1][1] *= -1.0f;
            Math::point center = pointLightAllocator.Get<PointLight_Transform>(typeIds[i]).getposition();
            const std::array<Graphics::GraphicsEntityId, 6>& observers = pointLightAllocator.Get<PointLight_Observers>(typeIds[i]);
            const std::array<Math::rectangle<int>, 6>& shadowTiles = pointLightAllocator.Get<PointLight_ShadowTiles>(typeIds[i]);
            std::array<Math::mat4, 6> shadowProjectionTransforms;

            Math::vector directions[] = {
                Math::vector(1, 0, 0), Math::vector(-1, 0, 0),
                Math::vector(0, 1, 0), Math::vector(0, -1, 0),
                Math::vector(0, 0, 1), Math::vector(0, 0, -1)
            };

            Math::vector up[] = {
                Math::vector(0, 1, 0), Math::vector(0, 1, 0),
                Math::vector(0, 0, -1), Math::vector(0, 0, 1),
                Math::vector(0, 1, 0), Math::vector(0, 1, 0)
            };
            for (IndexT j = 0; j < observers.size(); j++)
            {
                Math::mat4 view = Math::lookto(center, directions[j], up[j]);
                Ids::Id32 ctxId = shadowCasterIndexMap[observers[j]];
                Math::mat4 viewProj = projection * view;
                shadowCasterAllocator.Set<ShadowCaster_Transform>(ctxId, viewProj);
                viewProj.store(&shadowConstants.ShadowViewProjection[ctxId][0][0]);

                float tileScaleX = shadowTiles[j].width() / (float)dims.width;
                float tileScaleY = shadowTiles[j].height() / (float)dims.height;
                float tileOffsetX = shadowTiles[j].left / (float)dims.width;
                float tileOffsetY = shadowTiles[j].top / (float)dims.height;
                Math::mat4 atlasOffset = Math::translation(tileOffsetX, tileOffsetY, 0);
                Math::mat4 atlasScale = Math::scaling(tileScaleX, tileScaleY, 1);
                Math::mat4 textureTranslation = Math::translation(0.5f, 0.5f, 0);
                Math::mat4 textureScale = Math::scaling(0.5f, 0.5f, 1.0f);
                shadowProjectionTransforms[j] = atlasOffset * atlasScale * textureTranslation * textureScale * viewProj;
            }
            pointLightAllocator.Set<PointLight_ShadowProjectionTransforms>(typeIds[i], shadowProjectionTransforms);
        }

        // we reached our shadow caster max
        if (lightServerState.shadowCastingLights.Size() == 256)
            break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::SetupTerrainShadows(const CoreGraphics::TextureId terrainShadowMap, const uint worldSize)
{
    lightServerState.terrainShadowMap = terrainShadowMap;
    lightServerState.terrainShadowMapSize = CoreGraphics::TextureGetDimensions(terrainShadowMap).width;
    lightServerState.terrainSize = worldSize;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::BufferId
LightContext::GetLightIndexBuffer()
{
    return clusterState.clusterLightIndexLists;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::BufferId
LightContext::GetLightsBuffer()
{
    return clusterState.clusterLightsList;
}

//------------------------------------------------------------------------------
/**
*/
const LightsCluster::LightUniforms::STRUCT&
LightContext::GetLightUniforms()
{
    return clusterState.consts;
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::SetDirectionalLightTransform(const Graphics::ContextEntityId id, const Math::mat4& transform, const Math::vector& direction)
{
    auto lid = genericLightAllocator.Get<Light_TypedLightId>(id.id);
    directionalLightAllocator.Get<DirectionalLight_Direction>(lid) = direction;
    directionalLightAllocator.Get<DirectionalLight_Transform>(lid) = transform;
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::UpdateLights(const Graphics::FrameContext& ctx)
{
    N_SCOPE(UpdateLightResources, Lighting);
    using namespace CoreGraphics;

    // update constant buffer
    Shared::PerTickParams::STRUCT params = Graphics::GetTickParams();
    params.ltcLUT0 = CoreGraphics::TextureGetBindlessHandle(textureState.ltcLut0);
    params.ltcLUT1 = CoreGraphics::TextureGetBindlessHandle(textureState.ltcLut1);

    params.EnableTerrainShadows = (lightServerState.terrainShadowMap == CoreGraphics::InvalidTextureId) ? 0 : 1;
    if (params.EnableTerrainShadows)
    {
        params.TerrainShadowBuffer = CoreGraphics::TextureGetBindlessHandle(lightServerState.terrainShadowMap);
        params.TerrainShadowMapSize[0] = params.TerrainShadowMapSize[1] = lightServerState.terrainShadowMapSize;
        params.InvTerrainSize[0] = params.InvTerrainSize[1] = 1.0f / Math::max(1u, lightServerState.terrainSize);
        params.TerrainShadowMapPixelSize[0] = params.TerrainShadowMapPixelSize[1] = 1.0f / Math::max(1u, lightServerState.terrainShadowMapSize);
    }

    auto shadowDims = CoreGraphics::TextureGetDimensions(lightServerState.shadowAtlas);
    Graphics::UpdateTickParams(params);

    // go through and update local lights
    const Util::Array<LightType>& types		            = genericLightAllocator.GetArray<Light_Type>();
    const Util::Array<Math::vec3>& color	            = genericLightAllocator.GetArray<Light_Color>();
    const Util::Array<float>& intensity		            = genericLightAllocator.GetArray<Light_Intensity>();
    const Util::Array<float>& range			            = genericLightAllocator.GetArray<Light_Range>();
    const Util::Array<bool>& castShadow		            = genericLightAllocator.GetArray<Light_ShadowCaster>();
    const Util::Array<Ids::Id32>& typeIds	            = genericLightAllocator.GetArray<Light_TypedLightId>();
    const Util::Array<Graphics::StageMask>& stageMasks	= genericLightAllocator.GetArray<Light_StageMask>();
    SizeT numDirectionalLights = 0;
    SizeT numPointLights = 0;
    SizeT numPointLightShadows = 0;
    SizeT numSpotLights = 0;
    SizeT numSpotLightShadows = 0;
    SizeT numShadowLights = 0;
    SizeT numSpotLightsProjection = 0;
    SizeT numAreaLights = 0;
    SizeT numAreaLightShadows = 0;

    IndexT i;
    for (i = 0; i < types.Size(); i++)
    {
        if (castShadow[i])
            lightServerState.shadowCastingLights.Append(i);

        switch (types[i])
        {
            case LightType::DirectionalLightType:
            {
                auto& directionalLight = clusterState.lightList.DirectionalLights[numDirectionalLights];
                (genericLightAllocator.Get<Light_Color>(typeIds[i]) * genericLightAllocator.Get<Light_Intensity>(typeIds[i])).store(directionalLight.color);
                directionalLightAllocator.Get<DirectionalLight_Direction>(typeIds[i]).store(directionalLight.direction);
                const Util::FixedArray<float>& cascadeDistances = directionalLightAllocator.Get<DirectionalLight_CascadeDistances>(typeIds[i]);
                const Util::FixedArray<Math::mat4> cascadeTransforms = directionalLightAllocator.Get<DirectionalLight_CascadeTransforms>(typeIds[i]);
                directionalLight.shadowMap = CoreGraphics::TextureGetBindlessHandle(lightServerState.shadowAtlas);
                directionalLight.shadowIntensity = 1.0f;
                directionalLight.shadowBias = 0.0001f;
                auto shadowDims = CoreGraphics::TextureGetDimensions(lightServerState.shadowAtlas);
                directionalLight.shadowMapPixelSize[0] = 1.0f / shadowDims.width;
                directionalLight.shadowMapPixelSize[1] = 1.0f / shadowDims.height;
                directionalLight.flags |= castShadow[i] ? LightsCluster::USE_SHADOW_BITFLAG : 0x0;
                for (uint j = 0; j < cascadeDistances.Size(); j++)
                {
                    directionalLight.cascadeDistances[j] = cascadeDistances[j];
                    cascadeTransforms[j].store(&directionalLight.cascadeTransforms[j][0][0]);
                }

                directionalLight.stageMask = stageMasks[i];
                numDirectionalLights++;
            }
            break;
            case LightType::PointLightType:
            {
                const Math::point& trans = pointLightAllocator.Get<PointLight_Transform>(typeIds[i]).getposition();
                CoreGraphics::TextureId tex = pointLightAllocator.Get<PointLight_ProjectionTexture>(typeIds[i]);
                auto& pointLight = clusterState.lightList.PointLights[numPointLights];
                pointLight.shadowExtension = -1;
                uint flags = 0;

                // update shadow data
                if (castShadow[i])
                {
                    flags |= LightsCluster::USE_SHADOW_BITFLAG;
                    pointLight.shadowExtension = numPointLightShadows;
                    auto& shadow = clusterState.lightList.PointLightShadow[numPointLightShadows];
                    auto& viewProjections = pointLightAllocator.Get<PointLight_ShadowProjectionTransforms>(typeIds[i]);
                    for (uint j = 0; j < 6; j++)
                    {
                        viewProjections[j].store(&shadow.projection[j][0][0]);
                    }
                    shadow.shadowMap = CoreGraphics::TextureGetBindlessHandle(lightServerState.shadowAtlas);
                    shadow.shadowIntensity = 1.0f;
                    shadow.shadowMapPixelSize[0] = 1.0f / shadowDims.width;
                    shadow.shadowMapPixelSize[1] = 1.0f / shadowDims.height;
                    numPointLightShadows++;
                    numShadowLights++;
                }

                // check if we should use projection
                if (tex != InvalidTextureId)
                {
                    flags |= LightsCluster::USE_PROJECTION_TEX_BITFLAG;
                }

                trans.store3(pointLight.position);
                (color[i] * intensity[i]).store(pointLight.color);
                pointLight.range = range[i];
                pointLight.flags = flags;
                pointLight.stageMask = stageMasks[i];
                numPointLights++;
            }
            break;

            case LightType::SpotLightType:
            {
                const Math::mat4 trans = spotLightAllocator.Get<SpotLight_Transform>(typeIds[i]).getmatrix();
                CoreGraphics::TextureId tex = spotLightAllocator.Get<SpotLight_ProjectionTexture>(typeIds[i]);
                auto angles = spotLightAllocator.Get<SpotLight_ConeAngles>(typeIds[i]);
                auto& spotLight = clusterState.lightList.SpotLights[numSpotLights];
                spotLight.shadowExtension = -1;
                spotLight.projectionExtension = -1;

                uint flags = 0;

                // update shadow data
                if (castShadow[i])
                {
                    flags |= LightsCluster::USE_SHADOW_BITFLAG;
                    spotLight.shadowExtension = numSpotLightShadows;
                    auto& shadow = clusterState.lightList.SpotLightShadow[numSpotLightShadows];
                    spotLightAllocator.Get<SpotLight_ShadowProjectionTransform>(typeIds[i]).store(&shadow.projection[0][0]);
                    shadow.shadowMap = CoreGraphics::TextureGetBindlessHandle(lightServerState.shadowAtlas);
                    shadow.shadowIntensity = 1.0f;
                    shadow.shadowMapPixelSize[0] = 1.0f / shadowDims.width;
                    shadow.shadowMapPixelSize[1] = 1.0f / shadowDims.height;
                    numSpotLightShadows++;
                    numShadowLights++;
                }

                // check if we should use projection
                if (tex != InvalidTextureId && numSpotLightsProjection < 256)
                {
                    flags |= LightsCluster::USE_PROJECTION_TEX_BITFLAG;
                    spotLight.projectionExtension = numSpotLightsProjection;
                    auto& projection = clusterState.lightList.SpotLightProjection[numSpotLightsProjection];
                    spotLightAllocator.Get<SpotLight_ProjectionTransform>(typeIds[i]).store(&projection.projection[0][0]);
                    projection.projectionTexture = CoreGraphics::TextureGetBindlessHandle(tex);
                    numSpotLightsProjection++;
                }

                Math::vec4 forward = normalize(trans.z_axis);
                if (angles[0] == angles[1])
                    spotLight.angleFade = 1.0f;
                else
                    spotLight.angleFade = 1.0f / (angles[1] - angles[0]);

                trans.position.store3(spotLight.position);
                forward.store3(spotLight.forward);
                (color[i] * intensity[i]).store(spotLight.color);

                // calculate sine and cosine
                spotLight.angleSinCos[0] = Math::sin(angles[1]);
                spotLight.angleSinCos[1] = Math::cos(angles[1]);
                spotLight.range = range[i];
                spotLight.flags = flags;
                spotLight.stageMask = stageMasks[i];

                numSpotLights++;
            }
            break;

            case LightType::AreaLightType:
            {
                Math::transform44 trans = areaLightAllocator.Get<AreaLight_Transform>(typeIds[i]);
                bool twoSided = areaLightAllocator.Get<AreaLight_TwoSided>(typeIds[i]);
                auto shape = areaLightAllocator.Get<AreaLight_Shape>(typeIds[i]);

                auto& areaLight = clusterState.lightList.AreaLights[numAreaLights];
                areaLight.shadowExtension = -1;

                Math::point pos = trans.getposition();
                Math::mat4 rotation = Math::rotationquat(trans.getrotate());
                Math::vec4 xAxis = rotation.x_axis;
                Math::vec4 yAxis = rotation.y_axis;

                xAxis.store3(areaLight.xAxis);
                areaLight.width = trans.getscale().x * 0.5f;
                yAxis.store3(areaLight.yAxis);
                areaLight.height = trans.getscale().y * 0.5f;
                pos.store3(areaLight.position);

                Math::vec3 scale = trans.getscale();
                float width = scale.x;
                float height = shape == AreaLightShape::Tube ? 1.0f : scale.y;
                trans.setscale(Math::vector(width * range[i], height * range[i], twoSided ? range[i] * 2 : range[i]));
                trans.setposition(trans.getposition() + Math::vector(0, 0, 0));

                trans.getmatrix().position.store3(areaLight.position);
                Math::bbox box = trans.getmatrix();
                //Math::vec3 localExtents = Math::vec3((abs(trans.getscale().x) + abs(trans.getscale().y) + abs(trans.getscale().z)) * 0.5f);
                //(box.extents() * 2).store(areaLight.extents);
                box.pmin.store(areaLight.bboxMin);
                box.pmax.store(areaLight.bboxMax);

                areaLight.range = range[i];
                areaLight.radius = scale.y;

                uint flags = 0;

                // update shadow data
                Math::mat4 shadowProj;
                if (castShadow[i])
                {
                    std::array<Math::mat4, 2> shadowProjArray = areaLightAllocator.Get<AreaLight_ShadowProjectionTransforms>(typeIds[i]);

                    flags |= LightsCluster::USE_SHADOW_BITFLAG;
                    areaLight.shadowExtension = numAreaLightShadows;
                    auto& shadow = clusterState.lightList.AreaLightShadow[numAreaLightShadows];
                    shadowProjArray[0].store(&shadow.frontProjection[0][0]);
                    shadowProjArray[1].store(&shadow.backProjection[0][0]);
                    shadow.shadowMap = CoreGraphics::TextureGetBindlessHandle(lightServerState.shadowAtlas);
                    shadow.shadowMapPixelSize[0] = 1.0f / shadowDims.width;
                    shadow.shadowMapPixelSize[1] = 1.0f / shadowDims.height;
                    shadow.shadowIntensity = 1.0f;
                    numAreaLightShadows++;
                    numShadowLights++;
                }

                static const uint ShapeFlagLookup[] = {
                    LightsCluster::AREA_LIGHT_SHAPE_DISK
                    , LightsCluster::AREA_LIGHT_SHAPE_RECT
                    , LightsCluster::AREA_LIGHT_SHAPE_TUBE
                };
                flags |= ShapeFlagLookup[uint(shape)];
                flags |= twoSided ? LightsCluster::AREA_LIGHT_TWOSIDED : 0;

                (color[i] * intensity[i]).store(areaLight.color);

                // calculate sine and cosine
                areaLight.flags = flags;
                areaLight.stageMask = stageMasks[i];

                numAreaLights++;
            }
            break;
            default: break;
        }
    }

    IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();

    // update list of point lights
    if (numDirectionalLights > 0 || numPointLights > 0 || numSpotLights > 0 || numAreaLights > 0)
    {
        CoreGraphics::BufferUpdate(clusterState.stagingClusterLightsList.buffers[bufferIndex], clusterState.lightList);
        CoreGraphics::BufferFlush(clusterState.stagingClusterLightsList.buffers[bufferIndex]);
    }

    clusterState.consts.NumDirectionalLights = numDirectionalLights;
    clusterState.consts.NumSpotLights = numSpotLights;
    clusterState.consts.NumPointLights = numPointLights;
    clusterState.consts.NumAreaLights = numAreaLights;
    clusterState.consts.NumLightClusters = Clustering::ClusterContext::GetNumClusters();
    clusterState.consts.SSAOBuffer = CoreGraphics::TextureGetBindlessHandle(FrameScript_default::Texture_SSAOBuffer());

    // get per-view resource tables
    auto frameResourceTables = Graphics::GetFrameResourceTables(bufferIndex);
    auto tableQueues = Graphics::GetTableQueues();
    IndexT tableIt = 0;
    for (auto& table : frameResourceTables)
    {
        uint64_t offset = SetConstants(clusterState.consts, tableQueues[tableIt]);
        ResourceTableSetRWBuffer(table, { clusterState.clusterLightIndexLists, Shared::LightIndexLists::BINDING, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
        ResourceTableSetRWBuffer(table, { clusterState.clusterLightsList, Shared::LightLists::BINDING, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
        ResourceTableSetConstantBuffer(table, { CoreGraphics::GetConstantBuffer(bufferIndex, tableQueues[tableIt]), Shared::LightUniforms::BINDING, 0, sizeof(LightsCluster::LightUniforms::STRUCT), offset });
        ResourceTableCommitChanges(table);
        tableIt++;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::Resize(const uint framescriptHash, SizeT width, SizeT height)
{
    if (framescriptHash == FrameScript_default::ID)
    {
        // If window has resized, we need to update the resource table
        ResourceTableSetRWTexture(
            clusterState.resourceTable,
            {FrameScript_default::Texture_LightBuffer(), LightsCluster::Lighting::BINDING, 0, CoreGraphics::InvalidSamplerId}
        );

#ifdef CLUSTERED_LIGHTING_DEBUG
        ResourceTableSetRWTexture(
            clusterState.resourceTable,
            {FrameScript_default::Texture_LightDebugBuffer(),
             LightsCluster::DebugOutput::BINDING,
             0,
             CoreGraphics::InvalidSamplerId}
        );
#endif
        ResourceTableCommitChanges(clusterState.resourceTable);

        for (IndexT i = 0; i < combineState.resourceTables.Size(); i++)
        {
            ResourceTableSetRWTexture(
                combineState.resourceTables[i],
                {FrameScript_default::Texture_LightBuffer(), Combine::Lighting::BINDING, 0, CoreGraphics::InvalidSamplerId}
            );
            ResourceTableSetTexture(
                combineState.resourceTables[i],
                {FrameScript_default::Texture_SSAOBuffer(), Combine::AO::BINDING, 0, CoreGraphics::InvalidSamplerId}
            );
            ResourceTableSetTexture(
                combineState.resourceTables[i],
                {FrameScript_default::Texture_VolumetricFogBuffer0(), Combine::Fog::BINDING, 0, CoreGraphics::InvalidSamplerId}
            );
            ResourceTableSetTexture(
                combineState.resourceTables[i],
                {FrameScript_default::Texture_ReflectionBuffer(), Combine::Reflections::BINDING, 0, CoreGraphics::InvalidSamplerId
                }
            );
            ResourceTableSetTexture(
                combineState.resourceTables[i],
                {FrameScript_default::Texture_AlphaLightBuffer(), Combine::AlphaLighting::BINDING, 0, CoreGraphics::InvalidSamplerId
                }
            );
            ResourceTableCommitChanges(combineState.resourceTables[i]);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
Graphics::ContextEntityId
LightContext::Alloc()
{
    return genericLightAllocator.Alloc();
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::Dealloc(Graphics::ContextEntityId id)
{
    LightType type = genericLightAllocator.Get<Light_Type>(id.id);
    Ids::Id32 lightId = genericLightAllocator.Get<Light_TypedLightId>(id.id);

    bool castShadows = genericLightAllocator.Get<Light_ShadowCaster>(id.id);

    switch (type)
    {
    case LightType::DirectionalLightType:
    {
        // dealloc observers
        if (castShadows)
        {
            Util::FixedArray<Graphics::GraphicsEntityId>& observers = directionalLightAllocator.Get<DirectionalLight_CascadeObservers>(lightId);
            Util::FixedArray<Math::rectangle<int>>& rects = directionalLightAllocator.Get<DirectionalLight_CascadeTiles>(lightId);
            for (IndexT i = 0; i < observers.Size(); i++)
            {
                Visibility::ObserverContext::DeregisterEntityImmediate(observers[i]);
                Graphics::DestroyEntity(observers[i]);
                Ids::Id32 shadowCasterIndex = shadowCasterIndexMap[observers[i]];
                shadowCasterIndexMap.Erase(observers[i]);
                shadowCasterAllocator.Dealloc(shadowCasterIndex);

                lightServerState.shadowAtlasTileOctree.Deallocate(Math::uint2{ (uint)rects[i].left, (uint)rects[i].top }, rects[i].width());
            }
        }

        directionalLightAllocator.Dealloc(lightId);
        break;
    }

    case LightType::PointLightType:
    {
        if (castShadows)
        {
            const auto& observers = pointLightAllocator.Get<PointLight_Observers>(lightId);
            const auto& shadowTiles = pointLightAllocator.Get<PointLight_ShadowTiles>(lightId);
            for (IndexT i = 0; i < observers.size(); i++)
            {
                lightServerState.shadowAtlasTileOctree.Deallocate(Math::uint2{ (uint)shadowTiles[i].left, (uint)shadowTiles[i].top }, (uint)shadowTiles[i].width());
                Visibility::ObserverContext::DeregisterEntityImmediate(observers[i]);
                Ids::Id32 shadowCasterIndex = shadowCasterIndexMap[observers[i]];
                shadowCasterIndexMap.Erase(observers[i]);
                shadowCasterAllocator.Dealloc(shadowCasterIndex);
                Graphics::DestroyEntity(observers[i]);
            }
            pointLightAllocator.Dealloc(lightId);
        }

        break;
    }
    case LightType::SpotLightType:
    {
        if (castShadows)
        {
            auto obsId = spotLightAllocator.Get<SpotLight_Observer>(lightId);
            Visibility::ObserverContext::DeregisterEntityImmediate(obsId);
            Ids::Id32 shadowCasterIndex = shadowCasterIndexMap[obsId];
            shadowCasterIndexMap.Erase(obsId);
            shadowCasterAllocator.Dealloc(shadowCasterIndex);

            const Math::rectangle<int>& rect = spotLightAllocator.Get<SpotLight_ShadowTile>(lightId);
            lightServerState.shadowAtlasTileOctree.Deallocate(Math::uint2{ (uint)rect.left, (uint)rect.top }, rect.width());
        }
        spotLightAllocator.Dealloc(lightId);
        break;
    }
    case LightType::AreaLightType:
    {
        auto renderMesh = areaLightAllocator.Get<AreaLight_RenderMesh>(lightId);
        if (castShadows)
        {
            auto observerIds = areaLightAllocator.Get<AreaLight_Observers>(lightId);
            auto rects = areaLightAllocator.Get<AreaLight_ShadowTiles>(lightId);

            for (IndexT i = 0; i < observerIds.size(); i++)
            {
                Visibility::ObserverContext::DeregisterEntityImmediate(observerIds[i]);
                Ids::Id32 shadowCasterIndex = shadowCasterIndexMap[observerIds[i]];
                shadowCasterIndexMap.Erase(observerIds[i]);
                shadowCasterAllocator.Dealloc(shadowCasterIndex);
                lightServerState.shadowAtlasTileOctree.Deallocate(Math::uint2{ (uint)rects[i].left, (uint)rects[i].top }, rects[i].width());
                Graphics::DestroyEntity(observerIds[i]);
            }
        }
        if (renderMesh)
        {
            auto meshId = genericLightAllocator.Get<Light_Entity>(id.id);
            Graphics::DeregisterEntityImmediate<Models::ModelContext, Visibility::ObservableContext>(meshId);
        }
        areaLightAllocator.Dealloc(lightId);
        break;
    }
    }

    genericLightAllocator.Dealloc(id.id);
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::OnRenderDebug(uint32_t flags)
{
    using namespace CoreGraphics;
    auto const& types = genericLightAllocator.GetArray<Light_Type>();
    auto const& colors = genericLightAllocator.GetArray<Light_Color>();
    auto const& ranges = genericLightAllocator.GetArray<Light_Range>();
    auto const& ids = genericLightAllocator.GetArray<Light_TypedLightId>();
    auto const& pointTrans = pointLightAllocator.GetArray<PointLight_Transform>();
    auto const& spotTrans = spotLightAllocator.GetArray<SpotLight_Transform>();
    auto const& areaTrans = areaLightAllocator.GetArray<AreaLight_Transform>();
    ShapeRenderer* shapeRenderer = ShapeRenderer::Instance();

    for (int i = 0, n = types.Size(); i < n; ++i)
    {
        switch(types[i])
        {
            case LightType::PointLightType:
            {
                Math::mat4 trans = Math::affine(Math::vec3(ranges[ids[i]]), Math::vec3(0), Math::quat(), xyz(pointTrans[ids[i]].getposition()));
                Math::vec4 col = Math::vec4(colors[i], 1.0f);
                CoreGraphics::RenderShape shape;
                shape.SetupSimpleShape(RenderShape::Sphere, RenderShape::RenderFlag(RenderShape::CheckDepth|RenderShape::Wireframe), col, trans);
                shapeRenderer->AddShape(shape);
                if (flags & Im3d::Solid)
                {
                    col.w = 0.5f;
                    shape.SetupSimpleShape(RenderShape::Sphere, RenderShape::RenderFlag(RenderShape::CheckDepth), col, trans);
                    shapeRenderer->AddShape(shape);
                }
                break;
            }
            case LightType::SpotLightType:
            {

                // setup a perpsective transform with a fixed z near and far and aspect
                Graphics::CameraSettings settings;

                // get projection
                Math::mat4 proj = spotLightAllocator.Get<SpotLight_ProjectionTransform>(ids[i]);

                // take transform, scale Z with range and move back half the range
                Math::mat4 unscaledTransform = spotTrans[ids[i]].getmatrix();
                Math::vec4 pos = unscaledTransform.position - unscaledTransform.z_axis * ranges[i] * 0.5f;
                unscaledTransform.position = Math::vec4(0,0,0,1);
                unscaledTransform.x_axis = unscaledTransform.x_axis * ranges[i];
                unscaledTransform.y_axis = unscaledTransform.y_axis * ranges[i];
                unscaledTransform.z_axis = unscaledTransform.z_axis * ranges[i];
                unscaledTransform.scale(Math::vector(2.0f)); // rescale because box should be in [-1, 1] and not [-0.5, 0.5]
                unscaledTransform.position = pos;

                // removed skewedness because we are actually just interested in transforming points
                proj.row3 = Math::vec4(0, 0, 0, 1);

                // we want the points to first get projected, then transformed v * Projection * Transform;
                Math::mat4 frustum = unscaledTransform * proj;

                Math::vec4 col = Math::vec4(colors[i], 1.0f);

                RenderShape shape;
                shape.SetupSimpleShape(
                    RenderShape::Box,
                    RenderShape::RenderFlag(RenderShape::CheckDepth | RenderShape::Wireframe),
                    col,
                    frustum);
                shapeRenderer->AddShape(shape);
                break;
            }
            case LightType::DirectionalLightType:
            {
                break;
            }
            case LightType::AreaLightType:
            {
                bool twoSided = areaLightAllocator.Get<AreaLight_TwoSided>(ids[i]);
                Math::transform44 trans = areaTrans[ids[i]];
                Math::vec3 scale = trans.getscale();
                float width = scale.x;
                float height = areaLightAllocator.Get<AreaLight_Shape>(ids[i]) == AreaLightShape::Tube ? 1.0f : scale.y;
                trans.setscale(Math::vector(width * ranges[i], height * ranges[i], twoSided ? ranges[i] * 2 : -ranges[i]));
                trans.setposition(trans.getposition() + Math::vector(0, 0, twoSided ? 0 : -ranges[i] / 2));

                RenderShape shape;
                Math::vec4 col = Math::vec4(colors[i], 1.0f);
                shape.SetupSimpleShape(
                    RenderShape::Box,
                    RenderShape::RenderFlag(RenderShape::CheckDepth),
                    col,
                    trans.getmatrix()
                );
                shapeRenderer->AddShape(shape);
                break;
            }
        }
    }
}
} // namespace Lighting
