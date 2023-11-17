//------------------------------------------------------------------------------
// lightcontext.cc
// (C) 2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "lightcontext.h"
#include "graphics/graphicsserver.h"
#include "graphics/view.h"
#include "graphics/cameracontext.h"
#include "csmutil.h"
#include "frame/framesubgraph.h"
#include "frame/framesubpassbatch.h"
#include "frame/framecode.h"
#include "resources/resourceserver.h"
#include "visibility/visibilitycontext.h"
#include "clustering/clustercontext.h"
#include "core/cvar.h"
#ifndef PUBLIC_BUILD
#include "dynui/im3d/im3dcontext.h"
#include "debug/framescriptinspector.h"
#endif

#include "graphics/globalconstants.h"

#include "shared.h"
#include "lights_cluster.h"
#include "combine.h"
#include "csmblur.h"

#define CLUSTERED_LIGHTING_DEBUG 0

namespace Lighting
{

LightContext::GenericLightAllocator LightContext::genericLightAllocator;
LightContext::PointLightAllocator LightContext::pointLightAllocator;
LightContext::SpotLightAllocator LightContext::spotLightAllocator;
LightContext::AreaLightAllocator LightContext::areaLightAllocator;
LightContext::DirectionalLightAllocator LightContext::directionalLightAllocator;
LightContext::ShadowCasterAllocator LightContext::shadowCasterAllocator;
Util::HashTable<Graphics::GraphicsEntityId, uint, 16, 1> LightContext::shadowCasterSliceMap;
__ImplementContext(LightContext, LightContext::genericLightAllocator);

struct
{
    Graphics::GraphicsEntityId globalLightEntity = Graphics::GraphicsEntityId::Invalid();

    Util::Array<Graphics::GraphicsEntityId> spotLightEntities;
    Util::RingBuffer<Graphics::GraphicsEntityId> shadowcastingLocalLights;

    alignas(16) Shared::ShadowViewConstants shadowMatrixUniforms;
    CoreGraphics::TextureId localLightShadows = CoreGraphics::InvalidTextureId;
    CoreGraphics::TextureId globalLightShadowMap = CoreGraphics::InvalidTextureId;
    CoreGraphics::TextureId terrainShadowMap = CoreGraphics::InvalidTextureId;
    uint terrainShadowMapSize;
    uint terrainSize;
    CoreGraphics::BatchGroup::Code spotlightsBatchCode;
    CoreGraphics::BatchGroup::Code globalLightsBatchCode;

    CSMUtil csmUtil{ Shared::NUM_CASCADES };

    Memory::ArenaAllocator<sizeof(Frame::FrameCode) * 9> frameOpAllocator;


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
    alignas(16) LightsCluster::LightLists lightList;

} clusterState;

struct
{
    CoreGraphics::ShaderId combineShader;
    CoreGraphics::ShaderProgramId combineProgram;
    Util::FixedArray<CoreGraphics::ResourceTableId> resourceTables;
} combineState;

struct
{
    CoreGraphics::TextureId lightingTexture;
    CoreGraphics::TextureId fogTexture;
    CoreGraphics::TextureId reflectionTexture;
    CoreGraphics::TextureId aoTexture;
#ifdef CLUSTERED_LIGHTING_DEBUG
    CoreGraphics::TextureId clusterDebugTexture;
#endif

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
LightContext::Create(const Ptr<Frame::FrameScript>& frameScript)
{
    __CreateContext();

#ifndef PUBLIC_BUILD
    Core::CVarCreate(Core::CVarType::CVar_Int, "r_shadow_debug", "0", "Show shadowmap framescript inspector [0,1]");
#endif

    __bundle.OnWindowResized = LightContext::WindowResized;
#ifndef PUBLIC_BUILD
    __bundle.OnRenderDebug = LightContext::OnRenderDebug;
#endif
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    textureState.fogTexture = frameScript->GetTexture("VolumetricFogBuffer0");
    textureState.reflectionTexture = frameScript->GetTexture("ReflectionBuffer");
    textureState.aoTexture = frameScript->GetTexture("SSAOBuffer");
    textureState.lightingTexture = frameScript->GetTexture("LightBuffer");
#ifdef CLUSTERED_LIGHTING_DEBUG
    textureState.clusterDebugTexture = frameScript->GetTexture("LightDebugBuffer");
#endif

    // create shadow mapping frame script
    lightServerState.spotlightsBatchCode = CoreGraphics::BatchGroup::FromName("SpotLightShadow");
    lightServerState.globalLightsBatchCode = CoreGraphics::BatchGroup::FromName("GlobalShadow");
    lightServerState.globalLightShadowMap = frameScript->GetTexture("SunShadowDepth");
    lightServerState.localLightShadows = frameScript->GetTexture("LocalLightShadow");
    if (lightServerState.terrainShadowMap == CoreGraphics::InvalidTextureId)
        lightServerState.terrainShadowMap = Resources::CreateResource("systex:white.dds", "system");

    using namespace CoreGraphics;

    clusterState.classificationShader = ShaderServer::Instance()->GetShader("shd:lights_cluster.fxb");

    clusterState.cullProgram = ShaderGetProgram(clusterState.classificationShader, ShaderServer::Instance()->FeatureStringToMask("Cull"));
#ifdef CLUSTERED_LIGHTING_DEBUG
    clusterState.debugProgram = ShaderGetProgram(clusterState.classificationShader, ShaderServer::Instance()->FeatureStringToMask("Debug"));
#endif

    textureState.ltcLut0 = Resources::CreateResource("systex:ltc_1.dds", "system", nullptr, nullptr, true);
    textureState.ltcLut1 = Resources::CreateResource("systex:ltc_2.dds", "system", nullptr, nullptr, true);

    BufferCreateInfo rwbInfo;
    rwbInfo.name = "LightIndexListsBuffer";
    rwbInfo.byteSize = sizeof(LightsCluster::LightIndexLists);
    rwbInfo.mode = BufferAccessMode::DeviceLocal;
    rwbInfo.usageFlags = CoreGraphics::ReadWriteBuffer | CoreGraphics::TransferBufferDestination;
    rwbInfo.queueSupport = CoreGraphics::GraphicsQueueSupport | CoreGraphics::ComputeQueueSupport;
    clusterState.clusterLightIndexLists = CreateBuffer(rwbInfo);

    rwbInfo.name = "LightLists";
    rwbInfo.byteSize = sizeof(LightsCluster::LightLists);
    clusterState.clusterLightsList = CreateBuffer(rwbInfo);

    rwbInfo.name = "LightListsStagingBuffer";
    rwbInfo.mode = BufferAccessMode::HostCached;
    rwbInfo.usageFlags = CoreGraphics::TransferBufferSource;
    clusterState.stagingClusterLightsList = std::move(BufferSet(rwbInfo));

    for (IndexT i = 0; i < CoreGraphics::GetNumBufferedFrames(); i++)
    {
        CoreGraphics::ResourceTableId frameResourceTable = Graphics::GetFrameResourceTable(i);

        ResourceTableSetRWBuffer(frameResourceTable, { clusterState.clusterLightIndexLists, Shared::Table_Frame::LightIndexLists::SLOT, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
        ResourceTableSetRWBuffer(frameResourceTable, { clusterState.clusterLightsList, Shared::Table_Frame::LightLists::SLOT, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
        ResourceTableSetConstantBuffer(frameResourceTable, { CoreGraphics::GetGraphicsConstantBuffer(i), Shared::Table_Frame::LightUniforms::SLOT, 0, sizeof(LightsCluster::LightUniforms), 0 });
        ResourceTableCommitChanges(frameResourceTable);
    }

    // setup combine
    combineState.combineShader = ShaderServer::Instance()->GetShader("shd:combine.fxb");
    combineState.combineProgram = ShaderGetProgram(combineState.combineShader, ShaderServer::Instance()->FeatureStringToMask("Combine"));
    combineState.resourceTables.Resize(CoreGraphics::GetNumBufferedFrames());

    for (IndexT i = 0; i < combineState.resourceTables.Size(); i++)
    {
        combineState.resourceTables[i] = ShaderCreateResourceTable(combineState.combineShader, NEBULA_BATCH_GROUP, combineState.resourceTables.Size());
        //ResourceTableSetConstantBuffer(combineState.resourceTables[i], { CoreGraphics::GetComputeConstantBuffer(MainThreadConstantBuffer), combineState.combineUniforms, 0, false, false, sizeof(Combine::CombineUniforms), 0 });
        ResourceTableSetRWTexture(combineState.resourceTables[i], { textureState.lightingTexture, Combine::Table_Batch::Lighting_SLOT, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableSetTexture(combineState.resourceTables[i], { textureState.aoTexture, Combine::Table_Batch::AO_SLOT, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableSetTexture(combineState.resourceTables[i], { textureState.fogTexture, Combine::Table_Batch::Fog_SLOT, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableSetTexture(combineState.resourceTables[i], { textureState.reflectionTexture, Combine::Table_Batch::Reflections_SLOT, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableCommitChanges(combineState.resourceTables[i]);
    }

    // Update main resource table
    clusterState.resourceTable = ShaderCreateResourceTable(clusterState.classificationShader, NEBULA_BATCH_GROUP);
    ResourceTableSetRWTexture(clusterState.resourceTable, { textureState.lightingTexture, LightsCluster::Table_Batch::Lighting_SLOT, 0, CoreGraphics::InvalidSamplerId });
#ifdef CLUSTERED_LIGHTING_DEBUG
    ResourceTableSetRWTexture(clusterState.resourceTable, { textureState.clusterDebugTexture, LightsCluster::Table_Batch::DebugOutput_SLOT, 0, CoreGraphics::InvalidSamplerId });
#endif
    ResourceTableCommitChanges(clusterState.resourceTable);

    // allow 16 shadow casting local lights
    lightServerState.shadowcastingLocalLights.SetCapacity(16);

    // Pass is defined in the frame script, but this is just to take control over the batch rendering
    auto spotlightShadowsRender = lightServerState.frameOpAllocator.Alloc<Frame::FrameCode>();
    spotlightShadowsRender->domain = CoreGraphics::BarrierDomain::Pass;
    spotlightShadowsRender->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        IndexT i;
        for (i = 0; i < lightServerState.shadowcastingLocalLights.Size(); i++)
        {
            // draw it!
            Ids::Id32 slice = shadowCasterSliceMap[lightServerState.shadowcastingLocalLights[i]];
            Frame::FrameSubpassBatch::DrawBatch(cmdBuf, lightServerState.spotlightsBatchCode, lightServerState.shadowcastingLocalLights[i], 1, slice, bufferIndex);
        }
    };
    Frame::AddSubgraph("Spotlight Shadows", { spotlightShadowsRender });

    // Run blur pass for spot lights, which is disabled at the moment
    auto spotlightShadowsBlur = lightServerState.frameOpAllocator.Alloc<Frame::FrameCode>();
    spotlightShadowsBlur->domain = CoreGraphics::BarrierDomain::Pass;
    spotlightShadowsBlur->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
    };
    spotlightShadowsBlur->SetEnabled(false);
    Frame::AddSubgraph("Spotlight Blur", { spotlightShadowsBlur });

    // Run sun render pass, going over observers and feeding a cascade per each
    auto sunShadowsRender = lightServerState.frameOpAllocator.Alloc<Frame::FrameCode>();
    sunShadowsRender->domain = CoreGraphics::BarrierDomain::Pass;
    sunShadowsRender->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        if (lightServerState.globalLightEntity != Graphics::GraphicsEntityId::Invalid())
        {
            // get cameras associated with sun
            Graphics::ContextEntityId ctxId = GetContextId(lightServerState.globalLightEntity);
            Ids::Id32 typedId = genericLightAllocator.Get<TypedLightId>(ctxId.id);
            const Util::Array<Graphics::GraphicsEntityId>& observers = directionalLightAllocator.Get<DirectionalLight_CascadeObservers>(typedId);
            for (IndexT i = 0; i < observers.Size(); i++)
            {
                // draw it!
                Frame::FrameSubpassBatch::DrawBatch(cmdBuf, lightServerState.globalLightsBatchCode, observers[i], 1, i, bufferIndex);
            }
        }
    };
    Frame::AddSubgraph("Sun Shadows", { sunShadowsRender });

    // Copy lights from CPU buffer to GPU buffer
    auto lightsCopy = lightServerState.frameOpAllocator.Alloc<Frame::FrameCode>();
    lightsCopy->domain = CoreGraphics::BarrierDomain::Global;
    lightsCopy->queue = CoreGraphics::QueueType::ComputeQueueType;
    lightsCopy->bufferDeps.Add(clusterState.clusterLightsList,
                                 {
                                     "ClusterLightsList"
                                     , CoreGraphics::PipelineStage::TransferWrite
                                     , CoreGraphics::BufferSubresourceInfo()
                                 });
    lightsCopy->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        CoreGraphics::BufferCopy from, to;
        from.offset = 0;
        to.offset = 0;
        CmdCopy(cmdBuf, clusterState.stagingClusterLightsList.buffers[bufferIndex], { from }, clusterState.clusterLightsList, { to }, sizeof(LightsCluster::LightLists));
    };

    // Classify and cull is dependent on the light index lists and the AABB buffer being ready
    auto lightsClassifyAndCull = lightServerState.frameOpAllocator.Alloc<Frame::FrameCode>();
    lightsClassifyAndCull->domain = CoreGraphics::BarrierDomain::Global;
    lightsClassifyAndCull->queue = CoreGraphics::QueueType::ComputeQueueType;
    lightsClassifyAndCull->bufferDeps.Add(clusterState.clusterLightIndexLists,
                                 {
                                     "ClusterLightIndexLists"
                                     , CoreGraphics::PipelineStage::ComputeShaderWrite
                                     , CoreGraphics::BufferSubresourceInfo()
                                 });
    lightsClassifyAndCull->bufferDeps.Add(clusterState.clusterLightsList,
                                 {
                                     "ClusterLightsList"
                                     , CoreGraphics::PipelineStage::ComputeShaderRead
                                     , CoreGraphics::BufferSubresourceInfo()
                                 });
    lightsClassifyAndCull->bufferDeps.Add(Clustering::ClusterContext::GetClusterBuffer(),
                                 {
                                     "Cluster AABB"
                                     , CoreGraphics::PipelineStage::ComputeShaderRead
                                     , CoreGraphics::BufferSubresourceInfo()
                                 });
    lightsClassifyAndCull->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, clusterState.cullProgram);

        // run chunks of 1024 threads at a time
        std::array<SizeT, 3> dimensions = Clustering::ClusterContext::GetClusterDimensions();
        CmdDispatch(cmdBuf, Math::ceil((dimensions[0] * dimensions[1] * dimensions[2]) / 64.0f), 1, 1);
    };
    Frame::AddSubgraph("Lights Cull", { lightsCopy, lightsClassifyAndCull });

    // Final pass combines all the lighting buffers together, later in the frame
    auto lightsCombine = lightServerState.frameOpAllocator.Alloc<Frame::FrameCode>();
    lightsCombine->domain = CoreGraphics::BarrierDomain::Global;
    lightsCombine->textureDeps.Add(textureState.lightingTexture,
                                {
                                    "LightBuffer"
                                    , CoreGraphics::PipelineStage::ComputeShaderWrite
                                    , CoreGraphics::TextureSubresourceInfo::Color(textureState.lightingTexture)
                                });
    lightsCombine->textureDeps.Add(textureState.aoTexture,
                               {
                                   "SSAOBuffer"
                                   , CoreGraphics::PipelineStage::ComputeShaderRead
                                   , CoreGraphics::TextureSubresourceInfo::Color(textureState.aoTexture)
                               });
    lightsCombine->textureDeps.Add(textureState.fogTexture,
                                {
                                    "VolumeFogBuffer"
                                    , CoreGraphics::PipelineStage::ComputeShaderRead
                                    , CoreGraphics::TextureSubresourceInfo::Color(textureState.fogTexture)
                                });
    lightsCombine->textureDeps.Add(textureState.reflectionTexture,
                                {
                                    "ReflectionBuffer"
                                    , CoreGraphics::PipelineStage::ComputeShaderRead
                                    , CoreGraphics::TextureSubresourceInfo::Color(textureState.reflectionTexture)
                                });
    lightsCombine->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetShaderProgram(cmdBuf, combineState.combineProgram);
        CmdSetResourceTable(cmdBuf, combineState.resourceTables[bufferIndex], NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);

        // perform debug output
        CoreGraphics::TextureDimensions dims = TextureGetDimensions(textureState.lightingTexture);
        CmdDispatch(cmdBuf, Math::divandroundup(dims.width, 64), dims.height, 1);
    };
    Frame::AddSubgraph("Lights Combine", { lightsCombine });
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::Discard()
{
    lightServerState.frameOpAllocator.Release();
    Graphics::GraphicsServer::Instance()->UnregisterGraphicsContext(&__bundle);
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::SetupGlobalLight(
        const Graphics::GraphicsEntityId id,
        const Math::vec3& color,
        const float intensity,
        const Math::vec3& ambient,
        const Math::vec3& backlight,
        const float backlightFactor,
        const float zenith,
        const float azimuth,
        bool castShadows)
{
    n_assert(id != Graphics::GraphicsEntityId::Invalid());
    n_assert(lightServerState.globalLightEntity == Graphics::GraphicsEntityId::Invalid());

    auto lid = directionalLightAllocator.Alloc();

    const Graphics::ContextEntityId cid = GetContextId(id);
    genericLightAllocator.Get<Type>(cid.id) = LightType::DirectionalLightType;
    genericLightAllocator.Get<Color>(cid.id) = color;
    genericLightAllocator.Get<Intensity>(cid.id) = intensity;
    genericLightAllocator.Get<ShadowCaster>(cid.id) = castShadows;
    genericLightAllocator.Get<TypedLightId>(cid.id) = lid;

    Math::point sunPosition(Math::cos(azimuth) * Math::sin(zenith), Math::cos(zenith), Math::sin(azimuth) * Math::sin(zenith));
    Math::mat4 mat = lookatrh(Math::point(0.0f), sunPosition, Math::vector::upvec());
    
    SetGlobalLightTransform(cid, mat, Math::xyz(sunPosition));
    directionalLightAllocator.Get<DirectionalLight_Backlight>(lid) = backlight;
    directionalLightAllocator.Get<DirectionalLight_Ambient>(lid) = ambient;
    directionalLightAllocator.Get<DirectionalLight_BacklightOffset>(lid) = backlightFactor;

    if (castShadows)
    {
        // create new graphics entity for each view
        for (IndexT i = 0; i < Shared::NUM_CASCADES; i++)
        {
            Graphics::GraphicsEntityId shadowId = Graphics::CreateEntity();
            Visibility::ObserverContext::RegisterEntity(shadowId);
            Visibility::ObserverContext::Setup(shadowId, Visibility::VisibilityEntityType::Light, true);

            // allocate shadow caster slice
            Ids::Id32 casterId = shadowCasterAllocator.Alloc();
            shadowCasterSliceMap.Add(shadowId, casterId);

            // if there is a previous light, setup a dependency
            //if (!globalLightAllocator.Get<DirectionalLight_CascadeObservers>(lid).IsEmpty())
            //	Visibility::ObserverContext::MakeDependency(globalLightAllocator.Get<DirectionalLight_CascadeObservers>(lid).Back(), shadowId, Visibility::DependencyMode_Masked);

            // store entity id for cleanup later
            directionalLightAllocator.Get<DirectionalLight_CascadeObservers>(lid).Append(shadowId);
        }
    }

    lightServerState.globalLightEntity = id;
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::SetupPointLight(const Graphics::GraphicsEntityId id, 
    const Math::vec3& color, 
    const float intensity, 
    const float range, 
    bool castShadows, 
    const CoreGraphics::TextureId projection)
{
    n_assert(id != Graphics::GraphicsEntityId::Invalid());
    const Graphics::ContextEntityId cid = GetContextId(id);
    genericLightAllocator.Get<Type>(cid.id) = LightType::PointLightType;
    genericLightAllocator.Get<Color>(cid.id) = color;
    genericLightAllocator.Get<Intensity>(cid.id) = intensity;
    genericLightAllocator.Get<ShadowCaster>(cid.id) = castShadows;
    genericLightAllocator.Get<Range>(cid.id) = range;

    auto pli = pointLightAllocator.Alloc();
    genericLightAllocator.Get<TypedLightId>(cid.id) = pli;

    // set initial state
    pointLightAllocator.Get<PointLight_DynamicOffsets>(pli).Resize(2);
    pointLightAllocator.Get<PointLight_DynamicOffsets>(pli)[0] = 0;
    pointLightAllocator.Get<PointLight_DynamicOffsets>(pli)[1] = 0;
    pointLightAllocator.Get<PointLight_ProjectionTexture>(pli) = projection;

    if (castShadows)
    {
        // Allocate shadow caster slices for each side
        for (IndexT i = 0; i < 6; i++)
        {
            Ids::Id32 casterId = shadowCasterAllocator.Alloc();
            shadowCasterSliceMap.Add(id, casterId);
        }

        Visibility::ObserverContext::RegisterEntity(id);
        Visibility::ObserverContext::Setup(id, Visibility::VisibilityEntityType::Light);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::SetupSpotLight(
    const Graphics::GraphicsEntityId id, 
    const Math::vec3& color,
    const float intensity, 
    const float innerConeAngle,
    const float outerConeAngle,
    const float range,
    bool castShadows, 
    const CoreGraphics::TextureId projection)
{
    n_assert(id != Graphics::GraphicsEntityId::Invalid());
    const Graphics::ContextEntityId cid = GetContextId(id);
    genericLightAllocator.Set<Type>(cid.id, LightType::SpotLightType);
    genericLightAllocator.Set<Color>(cid.id, color);
    genericLightAllocator.Set<Intensity>(cid.id, intensity);
    genericLightAllocator.Set<ShadowCaster>(cid.id, castShadows);
    genericLightAllocator.Set<Range>(cid.id, range);

    auto sli = spotLightAllocator.Alloc();
    genericLightAllocator.Set<TypedLightId>(cid.id, sli);

    std::array<float, 2> angles = { innerConeAngle, outerConeAngle };
    if (innerConeAngle >= outerConeAngle)
        angles[0] = outerConeAngle - Math::deg2rad(0.1f);

    // construct projection from angle and range
    const float zNear = 0.1f;
    const float zFar = range;

    // use a fixed aspect of 1
    Math::mat4 proj = Math::perspfovrh(angles[1] * 2.0f, 1.0f, zNear, zFar);
    proj.r[1] = -proj.r[1];

    // set initial state
    spotLightAllocator.Get<SpotLight_DynamicOffsets>(sli).Resize(2);
    spotLightAllocator.Get<SpotLight_DynamicOffsets>(sli)[0] = 0;
    spotLightAllocator.Get<SpotLight_DynamicOffsets>(sli)[1] = 0;
    spotLightAllocator.Set<SpotLight_ProjectionTexture>(sli, projection);
    spotLightAllocator.Set<SpotLight_ConeAngles>(sli, angles);
    spotLightAllocator.Set<SpotLight_Observer>(sli, id);
    spotLightAllocator.Set<SpotLight_ProjectionTransform>(sli, proj);

    if (castShadows)
    {
        // allocate shadow caster slice
        Ids::Id32 casterId = shadowCasterAllocator.Alloc();
        shadowCasterSliceMap.Add(id, casterId);

        Visibility::ObserverContext::RegisterEntity(id);
        Visibility::ObserverContext::Setup(id, Visibility::VisibilityEntityType::Light);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::SetupAreaLight(
    const Graphics::GraphicsEntityId id
    , const AreaLightShape shape
    , const Math::vec3& color
    , const float intensity
    , const float range
    , bool twoSided
    , bool castShadows
)
{
    n_assert(id != Graphics::GraphicsEntityId::Invalid());
    const Graphics::ContextEntityId cid = GetContextId(id);
    genericLightAllocator.Set<Type>(cid.id, LightType::AreaLightType);
    genericLightAllocator.Set<Color>(cid.id, color);
    genericLightAllocator.Set<Intensity>(cid.id, intensity);
    genericLightAllocator.Set<ShadowCaster>(cid.id, castShadows);
    genericLightAllocator.Set<Range>(cid.id, range);

    auto ali = areaLightAllocator.Alloc();
    genericLightAllocator.Set<TypedLightId>(cid.id, ali);

    // set initial state
    areaLightAllocator.Get<AreaLight_DynamicOffsets>(ali).Resize(2);
    areaLightAllocator.Get<AreaLight_DynamicOffsets>(ali)[0] = 0;
    areaLightAllocator.Get<AreaLight_DynamicOffsets>(ali)[1] = 0;
    areaLightAllocator.Set<AreaLight_Observer>(ali, id);
    areaLightAllocator.Set<AreaLight_Shape>(ali, shape);
    areaLightAllocator.Set<AreaLight_TwoSided>(ali, twoSided || shape == AreaLightShape::Tube);
    //areaLightAllocator.Set<AreaLight_Projection>(ali, proj);

    if (castShadows)
    {
        // allocate shadow caster slice
        Ids::Id32 casterId = shadowCasterAllocator.Alloc();
        shadowCasterSliceMap.Add(id, casterId);

        Visibility::ObserverContext::RegisterEntity(id);
        Visibility::ObserverContext::Setup(id, Visibility::VisibilityEntityType::Light);
    }

    // Last step is to create a geometric proxy for the light source

    // Create material
    Materials::ShaderConfigServer* server = Materials::ShaderConfigServer::Instance();
    Materials::ShaderConfig* config = server->GetShaderConfig("AreaLight");
    Materials::MaterialId material = Materials::CreateMaterial({ config });

    IndexT binding = config->GetConstantIndex("EmissiveColor");
    Materials::MaterialVariant defaultVal = config->GetConstantDefault(binding);

    Materials::MaterialVariant var = server->AllocateVariantMemory(defaultVal.type);
    var.Set(color * intensity);
    Materials::MaterialSetConstant(material, binding, var);

    CoreGraphics::MeshId mesh;
    switch (shape)
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

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::SetColor(const Graphics::GraphicsEntityId id, const Math::vec3& color)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    genericLightAllocator.Get<Color>(cid.id) = color;
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::SetRange(const Graphics::GraphicsEntityId id, const float range)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    genericLightAllocator.Get<Range>(cid.id) = range;
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::SetIntensity(const Graphics::GraphicsEntityId id, const float intensity)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    genericLightAllocator.Get<Intensity>(cid.id) = intensity;
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::SetTransform(const Graphics::GraphicsEntityId id, const float azimuth, const float zenith)
{
    Math::point position(Math::cos(azimuth) * Math::sin(zenith), Math::cos(zenith), Math::sin(azimuth) * Math::sin(zenith));
    Math::mat4 mat = lookatrh(Math::point(0.0f), position, Math::vector::upvec());

    const Graphics::ContextEntityId cid = GetContextId(id);
    LightType type = genericLightAllocator.Get<Type>(cid.id);

    switch (type)
    {
        case LightType::DirectionalLightType:
            SetGlobalLightTransform(cid, mat, Math::xyz(position));
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
    LightType type = genericLightAllocator.Get<Type>(cid.id);
    Ids::Id32 lightId = genericLightAllocator.Get<TypedLightId>(cid.id);

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
    const Ids::Id32 cid = shadowCasterSliceMap[id.id];
    return shadowCasterAllocator.Get<ShadowCaster_Transform>(cid);
}

//------------------------------------------------------------------------------
/**
*/
const void
LightContext::SetPosition(const Graphics::GraphicsEntityId id, const Math::point& position)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    LightType type = genericLightAllocator.Get<Type>(cid.id);
    auto lid = genericLightAllocator.Get<TypedLightId>(cid.id);

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
            Models::ModelContext::SetTransform(id, areaLightAllocator.Get<AreaLight_Transform>(lid).getmatrix());
            break;
    }
}

//------------------------------------------------------------------------------
/**
*/
const Math::point
LightContext::GetPosition(const Graphics::GraphicsEntityId id)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    LightType type = genericLightAllocator.Get<Type>(cid.id);
    auto lid = genericLightAllocator.Get<TypedLightId>(cid.id);
    switch (type)
    {
        case LightType::SpotLightType:
            return spotLightAllocator.Get<SpotLight_Transform>(lid).getposition();
        case LightType::PointLightType:
            return pointLightAllocator.Get<PointLight_Transform>(lid).getposition();
        case LightType::AreaLightType:
            return areaLightAllocator.Get<AreaLight_Transform>(lid).getposition();
        default:
            return directionalLightAllocator.Get<DirectionalLight_Transform>(lid).position;
    }
}

//------------------------------------------------------------------------------
/**
*/
const void
LightContext::SetRotation(const Graphics::GraphicsEntityId id, const Math::quat& rotation)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    LightType type = genericLightAllocator.Get<Type>(cid.id);
    auto lid = genericLightAllocator.Get<TypedLightId>(cid.id);

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
            Models::ModelContext::SetTransform(id, areaLightAllocator.Get<AreaLight_Transform>(lid).getmatrix());
            break;
    }
}

//------------------------------------------------------------------------------
/**
*/
const Math::quat
LightContext::GetRotation(const Graphics::GraphicsEntityId id)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    LightType type = genericLightAllocator.Get<Type>(cid.id);
    auto lid = genericLightAllocator.Get<TypedLightId>(cid.id);
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
    LightType type = genericLightAllocator.Get<Type>(cid.id);
    auto lid = genericLightAllocator.Get<TypedLightId>(cid.id);

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
            Models::ModelContext::SetTransform(id, areaLightAllocator.Get<AreaLight_Transform>(lid).getmatrix());
            break;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
const Math::vec3
LightContext::GetScale(const Graphics::GraphicsEntityId id)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    LightType type = genericLightAllocator.Get<Type>(cid.id);
    auto lid = genericLightAllocator.Get<TypedLightId>(cid.id);
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
    return genericLightAllocator.Get<Type>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::GetInnerOuterAngle(const Graphics::GraphicsEntityId id, float& inner, float& outer)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    LightType type = genericLightAllocator.Get<Type>(cid.id);
    n_assert(type == LightType::SpotLightType);
    Ids::Id32 lightId = genericLightAllocator.Get<TypedLightId>(cid.id);
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
    LightType type = genericLightAllocator.Get<Type>(cid.id);
    n_assert(type == LightType::SpotLightType);
    Ids::Id32 lightId = genericLightAllocator.Get<TypedLightId>(cid.id);
    if (inner >= outer)
        inner = outer - Math::deg2rad(0.1f);
    spotLightAllocator.Get<SpotLight_ConeAngles>(lightId)[0] = inner;
    spotLightAllocator.Get<SpotLight_ConeAngles>(lightId)[1] = outer;
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::OnPrepareView(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx)
{
    const Graphics::ContextEntityId cid = GetContextId(lightServerState.globalLightEntity);

    // Setup global light view transform
    if (genericLightAllocator.Get<ShadowCaster>(cid.id))
    {
        lightServerState.csmUtil.SetCameraEntity(view->GetCamera());
        lightServerState.csmUtil.SetGlobalLight(lightServerState.globalLightEntity);
        lightServerState.csmUtil.SetShadowBox(Math::bbox(Math::point(0), Math::vector(500)));
        lightServerState.csmUtil.Compute(view->GetCamera(), lightServerState.globalLightEntity);

        auto lid = genericLightAllocator.Get<TypedLightId>(cid.id);
        const Util::Array<Graphics::GraphicsEntityId>& observers = directionalLightAllocator.Get<DirectionalLight_CascadeObservers>(lid);
        for (IndexT i = 0; i < observers.Size(); i++)
        {
            // do reverse lookup to find shadow caster
            Ids::Id32 ctxId = shadowCasterSliceMap[observers[i]];
            Math::mat4 cascadeProj = lightServerState.csmUtil.GetCascadeViewProjection(i);
            
            shadowCasterAllocator.Get<ShadowCaster_Transform>(ctxId) = cascadeProj;
            cascadeProj.store(lightServerState.shadowMatrixUniforms.LightViewMatrix[ctxId]);
        }

#if __DX12__
        Math::mat4 textureScale = Math::scaling(0.5f, -0.5f, 1.0f);
#elif __VULKAN__
        Math::mat4 textureScale = Math::scaling(0.5f, 0.5f, 1.0f);
#endif
        Math::mat4 textureTranslation = Math::translation(0.5f, 0.5f, 0);
        const Util::FixedArray<Math::mat4> transforms = lightServerState.csmUtil.GetCascadeProjectionTransforms();
        Math::vec4 cascadeScales[Shared::NUM_CASCADES];
        Math::vec4 cascadeOffsets[Shared::NUM_CASCADES];

        for (IndexT splitIndex = 0; splitIndex < Shared::NUM_CASCADES; ++splitIndex)
        {
            Math::mat4 shadowTexture = (textureTranslation * textureScale) * transforms[splitIndex];
            Math::vec4 scale = Math::vec4(
                shadowTexture.row0.x,
                shadowTexture.row1.y,
                shadowTexture.row2.z,
                1);
            Math::vec4 offset = shadowTexture.row3;
            offset.w = 0;
            cascadeOffsets[splitIndex] = offset;
            cascadeScales[splitIndex] = scale;
        }

        memcpy(lightServerState.shadowMatrixUniforms.CascadeOffset, cascadeOffsets, sizeof(Math::vec4) * Shared::NUM_CASCADES);
        memcpy(lightServerState.shadowMatrixUniforms.CascadeScale, cascadeScales, sizeof(Math::vec4) * Shared::NUM_CASCADES);
        memcpy(lightServerState.shadowMatrixUniforms.CascadeDistances, lightServerState.csmUtil.GetCascadeDistances().Begin(), sizeof(float) * Shared::NUM_CASCADES);
    }

    const Util::Array<LightType>& types = genericLightAllocator.GetArray<Type>();
    const Util::Array<float>& ranges = genericLightAllocator.GetArray<Range>();
    const Util::Array<bool>& castShadow = genericLightAllocator.GetArray<ShadowCaster>();
    const Util::Array<Ids::Id32>& typeIds = genericLightAllocator.GetArray<TypedLightId>();
    lightServerState.shadowcastingLocalLights.Reset();

    // prepare shadow casting local lights
    IndexT shadowCasterCount = 0;
    for (IndexT i = 0; i < types.Size(); i++)
    {
        if (castShadow[i])
        {
            switch (types[i])
            {
                case LightType::SpotLightType:
                {
                    std::array<float, 2> angles = spotLightAllocator.Get<SpotLight_ConeAngles>(typeIds[i]);

                    // setup a perpsective transform with a fixed z near and far and aspect
                    Math::mat4 projection = spotLightAllocator.Get<SpotLight_ProjectionTransform>(typeIds[i]);
                    Math::mat4 view = spotLightAllocator.Get<SpotLight_Transform>(typeIds[i]).getmatrix();
                    Math::mat4 viewProjection = projection * inverse(view);
                    Graphics::GraphicsEntityId observer = spotLightAllocator.Get<SpotLight_Observer>(typeIds[i]);
                    Ids::Id32 ctxId = shadowCasterSliceMap[observer];
                    shadowCasterAllocator.Get<ShadowCaster_Transform>(ctxId) = viewProjection;

                    lightServerState.shadowcastingLocalLights.Add(observer);
                    viewProjection.store(lightServerState.shadowMatrixUniforms.LightViewMatrix[ctxId]);
                    lightServerState.shadowMatrixUniforms.ShadowTiles[ctxId / 4][ctxId % 4] = shadowCasterCount++;
                    break;
                }

                case LightType::PointLightType:
                {
                    // TODO: IMPLEMENT!
                    break;
                }
            }
        }

        // we reached our shadow caster max
        if (shadowCasterCount == 16)
            break;
    }

    // apply shadow uniforms
    Graphics::UpdateShadowConstants(lightServerState.shadowMatrixUniforms);
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
const CoreGraphics::TextureId
LightContext::GetLightingTexture()
{
    return textureState.lightingTexture;
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
void 
LightContext::SetGlobalLightTransform(const Graphics::ContextEntityId id, const Math::mat4& transform, const Math::vector& direction)
{
    auto lid = genericLightAllocator.Get<TypedLightId>(id.id);
    directionalLightAllocator.Get<DirectionalLight_Direction>(lid) = direction;
    directionalLightAllocator.Get<DirectionalLight_Transform>(lid) = transform;
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::SetGlobalLightViewProjTransform(const Graphics::ContextEntityId id, const Math::mat4& transform)
{
    auto lid = genericLightAllocator.Get<TypedLightId>(id.id);
    directionalLightAllocator.Get<DirectionalLight_ViewProjTransform>(lid) = transform;
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::UpdateViewDependentResources(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx)
{
    N_SCOPE(UpdateLightResources, Lighting);
    const Graphics::ContextEntityId cid = GetContextId(lightServerState.globalLightEntity);
    using namespace CoreGraphics;

    // get camera view
    const Math::mat4 viewTransform = Graphics::CameraContext::GetView(view->GetCamera());
    const Math::mat4 invViewTransform = Graphics::CameraContext::GetTransform(view->GetCamera());

    // update constant buffer
    Ids::Id32 globalLightId = genericLightAllocator.Get<TypedLightId>(cid.id);
    Shared::PerTickParams params = Graphics::GetTickParams();
    (genericLightAllocator.Get<Color>(cid.id) * genericLightAllocator.Get<Intensity>(cid.id)).store(params.GlobalLightColor);
    directionalLightAllocator.Get<DirectionalLight_Direction>(globalLightId).store(params.GlobalLightDirWorldspace);
    directionalLightAllocator.Get<DirectionalLight_Backlight>(globalLightId).store(params.GlobalBackLightColor);
    directionalLightAllocator.Get<DirectionalLight_Ambient>(globalLightId).store(params.GlobalAmbientLightColor);
    Math::vec4 viewSpaceLightDir = viewTransform * Math::vec4(directionalLightAllocator.Get<DirectionalLight_Direction>(globalLightId), 0.0f);
    normalize(viewSpaceLightDir).store3(params.GlobalLightDir);
    params.GlobalBackLightOffset = directionalLightAllocator.Get<DirectionalLight_BacklightOffset>(globalLightId);

    params.ltcLUT0 = CoreGraphics::TextureGetBindlessHandle(textureState.ltcLut0);
    params.ltcLUT1 = CoreGraphics::TextureGetBindlessHandle(textureState.ltcLut1);

    uint flags = 0;

    if (genericLightAllocator.Get<ShadowCaster>(cid.id))
    {
        params.GlobalLightShadowBuffer = CoreGraphics::TextureGetBindlessHandle(lightServerState.globalLightShadowMap);
        params.TerrainShadowBuffer = CoreGraphics::TextureGetBindlessHandle(lightServerState.terrainShadowMap);
        params.TerrainShadowMapSize[0] = params.TerrainShadowMapSize[1] = lightServerState.terrainShadowMapSize;
        params.InvTerrainSize[0] = params.InvTerrainSize[1] = 1.0f / Math::max(1u, lightServerState.terrainSize);
        params.TerrainShadowMapPixelSize[0] = params.TerrainShadowMapPixelSize[1] = 1.0f / Math::max(1u, lightServerState.terrainShadowMapSize);
        lightServerState.csmUtil.GetShadowView().store(params.CSMShadowMatrix);

        flags |= LightsCluster::USE_SHADOW_BITFLAG;
    }
    params.GlobalLightFlags = flags;
    params.GlobalLightShadowBias = 0.0001f;																			 
    params.GlobalLightShadowIntensity = 1.0f;
    auto shadowDims = CoreGraphics::TextureGetDimensions(lightServerState.globalLightShadowMap);
    params.GlobalLightShadowMapSize[0] = 1.0f / shadowDims.width;
    params.GlobalLightShadowMapSize[1] = 1.0f / shadowDims.height;
    Graphics::UpdateTickParams(params);

    // go through and update local lights
    const Util::Array<LightType>& types		= genericLightAllocator.GetArray<Type>();
    const Util::Array<Math::vec3>& color	= genericLightAllocator.GetArray<Color>();
    const Util::Array<float>& intensity		= genericLightAllocator.GetArray<Intensity>();
    const Util::Array<float>& range			= genericLightAllocator.GetArray<Range>();
    const Util::Array<bool>& castShadow		= genericLightAllocator.GetArray<ShadowCaster>();
    const Util::Array<Ids::Id32>& typeIds	= genericLightAllocator.GetArray<TypedLightId>();
    SizeT numPointLights = 0;
    SizeT numSpotLights = 0;
    SizeT numSpotLightShadows = 0;
    SizeT numShadowLights = 0;
    SizeT numSpotLightsProjection = 0;
    SizeT numAreaLights = 0;
    SizeT numAreaLightShadows = 0;
    SizeT numAreaLightsProjection = 0;

    IndexT i;
    for (i = 0; i < types.Size(); i++)
    {
        switch (types[i])
        {
            case LightType::PointLightType:
            {
                const Math::point& trans = pointLightAllocator.Get<PointLight_Transform>(typeIds[i]).getposition();
                CoreGraphics::TextureId tex = pointLightAllocator.Get<PointLight_ProjectionTexture>(typeIds[i]);
                auto& pointLight = clusterState.lightList.PointLights[numPointLights];

                uint flags = 0;

                // update shadow data
                if (castShadow[i])
                {
                    flags |= LightsCluster::USE_SHADOW_BITFLAG;
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
                numPointLights++;
            }
            break;

            case LightType::SpotLightType:
            {
                const Math::mat4 trans = spotLightAllocator.Get<SpotLight_Transform>(typeIds[i]).getmatrix();
                CoreGraphics::TextureId tex = spotLightAllocator.Get<SpotLight_ProjectionTexture>(typeIds[i]);
                auto angles = spotLightAllocator.Get<SpotLight_ConeAngles>(typeIds[i]);
                auto& spotLight = clusterState.lightList.SpotLights[numSpotLights];
                Math::mat4 shadowProj;
                if (tex != InvalidTextureId || castShadow[i])
                {
                    Graphics::GraphicsEntityId observer = spotLightAllocator.Get<SpotLight_Observer>(typeIds[i]);
                    Graphics::ContextEntityId ctxId = shadowCasterSliceMap[observer];
                    shadowProj = shadowCasterAllocator.Get<ShadowCaster_Transform>(ctxId.id);
                }
                spotLight.shadowExtension = -1;
                spotLight.projectionExtension = -1;

                uint flags = 0;

                // update shadow data
                if (castShadow[i] && numShadowLights < 16)
                {
                    flags |= LightsCluster::USE_SHADOW_BITFLAG;
                    spotLight.shadowExtension = numSpotLightShadows;
                    auto& shadow = clusterState.lightList.SpotLightShadow[numSpotLightShadows];
                    shadowProj.store(shadow.projection);
                    shadow.shadowMap = CoreGraphics::TextureGetBindlessHandle(lightServerState.localLightShadows);
                    shadow.shadowIntensity = 1.0f;
                    shadow.shadowSlice = numShadowLights;
                    numSpotLightShadows++;
                    numShadowLights++;
                }

                // check if we should use projection
                if (tex != InvalidTextureId && numSpotLightsProjection < 256)
                {
                    flags |= LightsCluster::USE_PROJECTION_TEX_BITFLAG;
                    spotLight.projectionExtension = numSpotLightsProjection;
                    auto& projection = clusterState.lightList.SpotLightProjection[numSpotLightsProjection];
                    shadowProj.store(projection.projection);
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
                trans.setscale(Math::vector(width * range[i], height * range[i], twoSided ? range[i] * 2 : -range[i]));
                trans.setposition(trans.getposition() + Math::vector(0, 0, twoSided ? 0 : -range[i] / 2));
                Math::mat4 viewSpace = viewTransform * trans.getmatrix();
                Math::bbox bbox(viewSpace);
                
                bbox.pmin.store3(areaLight.bboxMin);
                areaLight.range = range[i];
                bbox.pmax.store3(areaLight.bboxMax);
                areaLight.radius = scale.y;

                uint flags = 0;

                // update shadow data
                Math::mat4 shadowProj;
                if (castShadow[i] && numShadowLights < 16)
                {
                    Graphics::GraphicsEntityId observer = areaLightAllocator.Get<AreaLight_Observer>(typeIds[i]);
                    Graphics::ContextEntityId ctxId = shadowCasterSliceMap[observer];
                    shadowProj = shadowCasterAllocator.Get<ShadowCaster_Transform>(ctxId.id);

                    flags |= LightsCluster::USE_SHADOW_BITFLAG;
                    areaLight.shadowExtension = numAreaLightShadows;
                    auto& shadow = clusterState.lightList.AreaLightShadow[numAreaLightShadows];
                    shadowProj.store(shadow.projection);
                    shadow.shadowMap = CoreGraphics::TextureGetBindlessHandle(lightServerState.localLightShadows);
                    shadow.shadowIntensity = 1.0f;
                    shadow.shadowSlice = numShadowLights;
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
                numAreaLights++;
            }
            break;
        }
    }

    IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();

    // update list of point lights
    if (numPointLights > 0 || numSpotLights > 0 || numAreaLights > 0)
    {
        CoreGraphics::BufferUpdate(clusterState.stagingClusterLightsList.buffers[bufferIndex], clusterState.lightList);
        CoreGraphics::BufferFlush(clusterState.stagingClusterLightsList.buffers[bufferIndex]);
    }

    // get per-view resource tables
    CoreGraphics::ResourceTableId frameResourceTable = Graphics::GetFrameResourceTable(bufferIndex);

    LightsCluster::LightUniforms consts;
    consts.NumSpotLights = numSpotLights;
    consts.NumPointLights = numPointLights;
    consts.NumAreaLights = numAreaLights;
    consts.NumLightClusters = Clustering::ClusterContext::GetNumClusters();
    consts.SSAOBuffer = CoreGraphics::TextureGetBindlessHandle(textureState.aoTexture);
    IndexT offset = SetConstants(consts);
    ResourceTableSetConstantBuffer(frameResourceTable, { GetGraphicsConstantBuffer(bufferIndex), Shared::Table_Frame::LightUniforms::SLOT, 0, Shared::Table_Frame::LightUniforms::SIZE, (SizeT)offset });
    ResourceTableCommitChanges(frameResourceTable);

    TextureDimensions dims = TextureGetDimensions(textureState.lightingTexture);
    Combine::CombineUniforms combineConsts;
    combineConsts.LowresResolution[0] = 1.0f / dims.width;
    combineConsts.LowresResolution[1] = 1.0f / dims.height;
    offset = SetConstants(combineConsts);
    ResourceTableSetConstantBuffer(combineState.resourceTables[bufferIndex], { GetGraphicsConstantBuffer(bufferIndex), Combine::Table_Batch::CombineUniforms::SLOT, 0, Combine::Table_Batch::CombineUniforms::SIZE, (SizeT)offset });
    ResourceTableCommitChanges(combineState.resourceTables[bufferIndex]);
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::WindowResized(const CoreGraphics::WindowId windowId, SizeT width, SizeT height)
{
    // If window has resized, we need to update the resource table
    ResourceTableSetRWTexture(clusterState.resourceTable, { textureState.lightingTexture, LightsCluster::Table_Batch::Lighting_SLOT, 0, CoreGraphics::InvalidSamplerId });

#ifdef CLUSTERED_LIGHTING_DEBUG
    ResourceTableSetRWTexture(clusterState.resourceTable, { textureState.clusterDebugTexture, LightsCluster::Table_Batch::DebugOutput_SLOT, 0, CoreGraphics::InvalidSamplerId });
#endif
    ResourceTableCommitChanges(clusterState.resourceTable);

    for (IndexT i = 0; i < combineState.resourceTables.Size(); i++)
    {
        ResourceTableSetRWTexture(combineState.resourceTables[i], { textureState.lightingTexture, Combine::Table_Batch::Lighting_SLOT, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableSetTexture(combineState.resourceTables[i], { textureState.aoTexture, Combine::Table_Batch::AO_SLOT, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableSetTexture(combineState.resourceTables[i], { textureState.fogTexture, Combine::Table_Batch::Fog_SLOT, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableSetTexture(combineState.resourceTables[i], { textureState.reflectionTexture, Combine::Table_Batch::Reflections_SLOT, 0, CoreGraphics::InvalidSamplerId });
        ResourceTableCommitChanges(combineState.resourceTables[i]);
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
    LightType type = genericLightAllocator.Get<Type>(id.id);
    Ids::Id32 lightId = genericLightAllocator.Get<TypedLightId>(id.id);

    switch (type)
    {
    case LightType::DirectionalLightType:
    {
        // dealloc observers
        Util::Array<Graphics::GraphicsEntityId>& observers = directionalLightAllocator.Get<DirectionalLight_CascadeObservers>(lightId);
        for (IndexT i = 0; i < observers.Size(); i++)
        {
            Visibility::ObserverContext::DeregisterEntity(observers[i]);
            Graphics::DestroyEntity(observers[i]);
        }

        directionalLightAllocator.Dealloc(lightId);
        break;
    }
    case LightType::SpotLightType:
        spotLightAllocator.Dealloc(lightId);
        break;
    case LightType::PointLightType:
        spotLightAllocator.Dealloc(lightId);
        break;
    case LightType::AreaLightType:
        areaLightAllocator.Dealloc(lightId);
        break;
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
    auto const& types = genericLightAllocator.GetArray<Type>();    
    auto const& colors = genericLightAllocator.GetArray<Color>();
    auto const& ranges = genericLightAllocator.GetArray<Range>();
    auto const& ids = genericLightAllocator.GetArray<TypedLightId>();
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
                std::array<float, 2> angles = spotLightAllocator.Get<SpotLight_ConeAngles>(ids[i]);

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
