//------------------------------------------------------------------------------
// lightcontext.cc
// (C) 2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "lightcontext.h"
#include "graphics/graphicsserver.h"
#include "graphics/view.h"
#include "graphics/cameracontext.h"
#include "csmutil.h"
#include "math/polar.h"
#include "frame/framesubgraph.h"
#include "frame/framebatchtype.h"
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

// match these in lights.fx
const uint USE_SHADOW_BITFLAG = 0x1;
const uint USE_PROJECTION_TEX_BITFLAG = 0x2;

namespace Lighting
{

LightContext::GenericLightAllocator LightContext::genericLightAllocator;
LightContext::PointLightAllocator LightContext::pointLightAllocator;
LightContext::SpotLightAllocator LightContext::spotLightAllocator;
LightContext::GlobalLightAllocator LightContext::globalLightAllocator;
LightContext::ShadowCasterAllocator LightContext::shadowCasterAllocator;
Util::HashTable<Graphics::GraphicsEntityId, uint, 6, 1> LightContext::shadowCasterSliceMap;
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

    CSMUtil csmUtil{ Shared::NumCascades };

    Memory::ArenaAllocator<sizeof(Frame::FrameCode) * 9> frameOpAllocator;

} lightServerState;

struct
{
    CoreGraphics::ShaderId classificationShader;
    CoreGraphics::ShaderProgramId cullProgram;
    CoreGraphics::ShaderProgramId debugProgram;
    CoreGraphics::ShaderProgramId renderProgram;
    CoreGraphics::BufferId clusterLightIndexLists;
    Util::FixedArray<CoreGraphics::BufferId> stagingClusterLightsList;
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
        lightServerState.terrainShadowMap = Resources::CreateResource("tex:system/white.dds", "system");

    using namespace CoreGraphics;

    clusterState.classificationShader = ShaderServer::Instance()->GetShader("shd:lights_cluster.fxb");

    clusterState.cullProgram = ShaderGetProgram(clusterState.classificationShader, ShaderServer::Instance()->FeatureStringToMask("Cull"));
#ifdef CLUSTERED_LIGHTING_DEBUG
    clusterState.debugProgram = ShaderGetProgram(clusterState.classificationShader, ShaderServer::Instance()->FeatureStringToMask("Debug"));
#endif

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
    clusterState.stagingClusterLightsList.Resize(CoreGraphics::GetNumBufferedFrames());

    for (IndexT i = 0; i < clusterState.stagingClusterLightsList.Size(); i++)
    {
        clusterState.stagingClusterLightsList[i] = CreateBuffer(rwbInfo);
    }

    for (IndexT i = 0; i < CoreGraphics::GetNumBufferedFrames(); i++)
    {
        CoreGraphics::ResourceTableId computeTable = Graphics::GetFrameResourceTableCompute(i);
        CoreGraphics::ResourceTableId graphicsTable = Graphics::GetFrameResourceTableGraphics(i);

        ResourceTableSetRWBuffer(computeTable, { clusterState.clusterLightIndexLists, Shared::Table_Frame::LightIndexLists::SLOT, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
        ResourceTableSetRWBuffer(computeTable, { clusterState.clusterLightsList, Shared::Table_Frame::LightLists::SLOT, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
        ResourceTableSetConstantBuffer(computeTable, { CoreGraphics::GetComputeConstantBuffer(), Shared::Table_Frame::LightUniforms::SLOT, 0, sizeof(LightsCluster::LightUniforms), 0 });
        ResourceTableCommitChanges(computeTable);

        ResourceTableSetRWBuffer(graphicsTable, { clusterState.clusterLightIndexLists, Shared::Table_Frame::LightIndexLists::SLOT, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
        ResourceTableSetRWBuffer(graphicsTable, { clusterState.clusterLightsList, Shared::Table_Frame::LightLists::SLOT, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
        ResourceTableSetConstantBuffer(graphicsTable, { CoreGraphics::GetGraphicsConstantBuffer(), Shared::Table_Frame::LightUniforms::SLOT, 0, sizeof(LightsCluster::LightUniforms), 0 });
        ResourceTableCommitChanges(graphicsTable);
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
            int slice = shadowCasterSliceMap[lightServerState.shadowcastingLocalLights[i]];
            Frame::FrameSubpassBatch::DrawBatch(cmdBuf, lightServerState.spotlightsBatchCode, lightServerState.shadowcastingLocalLights[i], 1, slice);
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
            const Util::Array<Graphics::GraphicsEntityId>& observers = globalLightAllocator.Get<GlobalLight_CascadeObservers>(typedId);
            for (IndexT i = 0; i < observers.Size(); i++)
            {
                // draw it!
                Frame::FrameSubpassBatch::DrawBatch(cmdBuf, lightServerState.globalLightsBatchCode, observers[i], 1, i);
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
        CmdCopy(cmdBuf, clusterState.stagingClusterLightsList[bufferIndex], { from }, clusterState.clusterLightsList, { to }, sizeof(LightsCluster::LightLists));
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
                                    , CoreGraphics::TextureSubresourceInfo::ColorNoMipNoLayer()
                                });
    lightsCombine->textureDeps.Add(textureState.aoTexture,
                               {
                                   "SSAOBuffer"
                                   , CoreGraphics::PipelineStage::ComputeShaderRead
                                   , CoreGraphics::TextureSubresourceInfo::ColorNoMipNoLayer()
                               });
    lightsCombine->textureDeps.Add(textureState.fogTexture,
                                {
                                    "VolumeFogBuffer"
                                    , CoreGraphics::PipelineStage::ComputeShaderRead
                                    , CoreGraphics::TextureSubresourceInfo::ColorNoMipNoLayer()
                                });
    lightsCombine->textureDeps.Add(textureState.reflectionTexture,
                                {
                                    "ReflectionBuffer"
                                    , CoreGraphics::PipelineStage::ComputeShaderRead
                                    , CoreGraphics::TextureSubresourceInfo::ColorNoMipNoLayer()
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

    auto lid = globalLightAllocator.Alloc();

    const Graphics::ContextEntityId cid = GetContextId(id);
    genericLightAllocator.Get<Type>(cid.id) = GlobalLightType;
    genericLightAllocator.Get<Color>(cid.id) = color;
    genericLightAllocator.Get<Intensity>(cid.id) = intensity;
    genericLightAllocator.Get<ShadowCaster>(cid.id) = castShadows;
    genericLightAllocator.Get<TypedLightId>(cid.id) = lid;

    Math::point sunPosition(Math::cos(azimuth) * Math::sin(zenith), Math::cos(zenith), Math::sin(azimuth) * Math::sin(zenith));
    Math::mat4 mat = lookatrh(Math::point(0.0f), sunPosition, Math::vector::upvec());
    
    SetGlobalLightTransform(cid, mat, Math::xyz(sunPosition));
    globalLightAllocator.Get<GlobalLight_Backlight>(lid) = backlight;
    globalLightAllocator.Get<GlobalLight_Ambient>(lid) = ambient;
    globalLightAllocator.Get<GlobalLight_BacklightOffset>(lid) = backlightFactor;

    if (castShadows)
    {
        // create new graphics entity for each view
        for (IndexT i = 0; i < Shared::NumCascades; i++)
        {
            Graphics::GraphicsEntityId shadowId = Graphics::CreateEntity();
            Visibility::ObserverContext::RegisterEntity(shadowId);
            Visibility::ObserverContext::Setup(shadowId, Visibility::VisibilityEntityType::Light);

            // allocate shadow caster slice
            Ids::Id32 casterId = shadowCasterAllocator.Alloc();
            shadowCasterSliceMap.Add(shadowId, casterId);

            // if there is a previous light, setup a dependency
            //if (!globalLightAllocator.Get<GlobalLight_CascadeObservers>(lid).IsEmpty())
            //	Visibility::ObserverContext::MakeDependency(globalLightAllocator.Get<GlobalLight_CascadeObservers>(lid).Back(), shadowId, Visibility::DependencyMode_Masked);

            // store entity id for cleanup later
            globalLightAllocator.Get<GlobalLight_CascadeObservers>(lid).Append(shadowId);
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
    const Math::mat4& transform,
    const float range, 
    bool castShadows, 
    const CoreGraphics::TextureId projection)
{
    n_assert(id != Graphics::GraphicsEntityId::Invalid());
    const Graphics::ContextEntityId cid = GetContextId(id);
    genericLightAllocator.Get<Type>(cid.id) = PointLightType;
    genericLightAllocator.Get<Color>(cid.id) = color;
    genericLightAllocator.Get<Intensity>(cid.id) = intensity;
    genericLightAllocator.Get<ShadowCaster>(cid.id) = castShadows;
    genericLightAllocator.Get<Range>(cid.id) = range;

    auto pli = pointLightAllocator.Alloc();
    genericLightAllocator.Get<TypedLightId>(cid.id) = pli;
    SetPointLightTransform(cid, transform);

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
LightContext::SetupSpotLight(const Graphics::GraphicsEntityId id, 
    const Math::vec3& color,
    const float intensity, 
    const float innerConeAngle,
    const float outerConeAngle,
    const Math::mat4& transform,
    const float range,
    bool castShadows, 
    const CoreGraphics::TextureId projection)
{
    n_assert(id != Graphics::GraphicsEntityId::Invalid());
    const Graphics::ContextEntityId cid = GetContextId(id);
    genericLightAllocator.Set<Type>(cid.id, SpotLightType);
    genericLightAllocator.Set<Color>(cid.id, color);
    genericLightAllocator.Set<Intensity>(cid.id, intensity);
    genericLightAllocator.Set<ShadowCaster>(cid.id, castShadows);
    genericLightAllocator.Set<Range>(cid.id, range);

    auto sli = spotLightAllocator.Alloc();
    spotLightAllocator.Get<SpotLight_DynamicOffsets>(sli).Resize(2);
    genericLightAllocator.Set<TypedLightId>(cid.id, sli);

    // do this after we assign the typed light id
    SetSpotLightTransform(cid, transform);

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
        case GlobalLightType:
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
    case GlobalLightType:
        return globalLightAllocator.Get<GlobalLight_Transform>(lightId);
        break;
    case SpotLightType:
        return spotLightAllocator.Get<SpotLight_Transform>(lightId);
        break;
    case PointLightType:
        return pointLightAllocator.Get<PointLight_Transform>(lightId);
        break;
    default:
        return Math::mat4();
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::SetTransform(const Graphics::GraphicsEntityId id, const Math::mat4& trans)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    LightType type = genericLightAllocator.Get<Type>(cid.id);    

    switch (type)
    {
    case SpotLightType:
        SetSpotLightTransform(cid, trans);        
        break;
    case PointLightType:
        SetPointLightTransform(cid, trans);
        break;
    default:    
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
const Math::mat4
LightContext::GetObserverTransform(const Graphics::GraphicsEntityId id)
{
    const Graphics::ContextEntityId cid = shadowCasterSliceMap[id.id];
    return shadowCasterAllocator.Get<ShadowCaster_Transform>(cid.id);
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
    n_assert(type == SpotLightType);
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
    n_assert(type == SpotLightType);
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
        const Util::Array<Graphics::GraphicsEntityId>& observers = globalLightAllocator.Get<GlobalLight_CascadeObservers>(lid);
        for (IndexT i = 0; i < observers.Size(); i++)
        {
            // do reverse lookup to find shadow caster
            Graphics::ContextEntityId ctxId = shadowCasterSliceMap[observers[i]];
            shadowCasterAllocator.Get<ShadowCaster_Transform>(ctxId.id) = lightServerState.csmUtil.GetCascadeViewProjection(i);
        }

        IndexT i;
        for (i = 0; i < Shared::NumCascades; i++)
        {
            Math::mat4 cascadeProj = lightServerState.csmUtil.GetCascadeViewProjection(i);
            cascadeProj.store(lightServerState.shadowMatrixUniforms.CSMViewMatrix[i]);
        }
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
                case SpotLightType:
                {
                    std::array<float, 2> angles = spotLightAllocator.Get<SpotLight_ConeAngles>(typeIds[i]);

                    // setup a perpsective transform with a fixed z near and far and aspect
                    Math::mat4 projection = spotLightAllocator.Get<SpotLight_ProjectionTransform>(typeIds[i]);
                    Math::mat4 view = spotLightAllocator.Get<SpotLight_Transform>(typeIds[i]);
                    Math::mat4 viewProjection = projection * inverse(view);
                    Graphics::GraphicsEntityId observer = spotLightAllocator.Get<SpotLight_Observer>(typeIds[i]);
                    Graphics::ContextEntityId ctxId = shadowCasterSliceMap[observer];
                    shadowCasterAllocator.Get<ShadowCaster_Transform>(ctxId.id) = viewProjection;

                    lightServerState.shadowcastingLocalLights.Add(observer);
                    viewProjection.store(lightServerState.shadowMatrixUniforms.LightViewMatrix[shadowCasterCount++]);
                    break;
                }

                case PointLightType:
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
LightContext::SetSpotLightTransform(const Graphics::ContextEntityId id, const Math::mat4& transform)
{
    // todo, update projection and invviewprojection!!!!
    auto lid = genericLightAllocator.Get<TypedLightId>(id.id);
    spotLightAllocator.Get<SpotLight_Transform>(lid) = transform;
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::SetPointLightTransform(const Graphics::ContextEntityId id, const Math::mat4& transform)
{
    auto lid = genericLightAllocator.Get<TypedLightId>(id.id);
    pointLightAllocator.Get<PointLight_Transform>(lid) = transform;
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::SetGlobalLightTransform(const Graphics::ContextEntityId id, const Math::mat4& transform, const Math::vector& direction)
{
    auto lid = genericLightAllocator.Get<TypedLightId>(id.id);
    globalLightAllocator.Get<GlobalLight_Direction>(lid) = direction;
    globalLightAllocator.Get<GlobalLight_Transform>(lid) = transform;
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::SetGlobalLightViewProjTransform(const Graphics::ContextEntityId id, const Math::mat4& transform)
{
    auto lid = genericLightAllocator.Get<TypedLightId>(id.id);
    globalLightAllocator.Get<GlobalLight_ViewProjTransform>(lid) = transform;
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
    globalLightAllocator.Get<GlobalLight_Direction>(globalLightId).store(params.GlobalLightDirWorldspace);
    globalLightAllocator.Get<GlobalLight_Backlight>(globalLightId).store(params.GlobalBackLightColor);
    globalLightAllocator.Get<GlobalLight_Ambient>(globalLightId).store(params.GlobalAmbientLightColor);
    Math::vec4 viewSpaceLightDir = viewTransform * Math::vec4(globalLightAllocator.Get<GlobalLight_Direction>(globalLightId), 0.0f);
    normalize(viewSpaceLightDir).store3(params.GlobalLightDir);
    params.GlobalBackLightOffset = globalLightAllocator.Get<GlobalLight_BacklightOffset>(globalLightId);

    uint flags = 0;

    if (genericLightAllocator.Get<ShadowCaster>(cid.id))
    {

#if __DX12__
        Math::mat4 textureScale = Math::scaling(0.5f, -0.5f, 1.0f);
#elif __VULKAN__
        Math::mat4 textureScale = Math::scaling(0.5f, 0.5f, 1.0f);
#endif
        Math::mat4 textureTranslation = Math::translation(0.5f, 0.5f, 0);
        const Util::FixedArray<Math::mat4> transforms = lightServerState.csmUtil.GetCascadeProjectionTransforms();
        Math::vec4 cascadeScales[Shared::NumCascades];
        Math::vec4 cascadeOffsets[Shared::NumCascades];

        for (IndexT splitIndex = 0; splitIndex < Shared::NumCascades; ++splitIndex)
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

        memcpy(params.CascadeOffset, cascadeOffsets, sizeof(Math::vec4) * Shared::NumCascades);
        memcpy(params.CascadeScale, cascadeScales, sizeof(Math::vec4) * Shared::NumCascades);
        memcpy(params.CascadeDistances, lightServerState.csmUtil.GetCascadeDistances().Begin(), sizeof(float) * Shared::NumCascades);
        params.GlobalLightShadowBuffer = CoreGraphics::TextureGetBindlessHandle(lightServerState.globalLightShadowMap);
        params.TerrainShadowBuffer = CoreGraphics::TextureGetBindlessHandle(lightServerState.terrainShadowMap);
        params.TerrainShadowMapSize[0] = params.TerrainShadowMapSize[1] = lightServerState.terrainShadowMapSize;
        params.InvTerrainSize[0] = params.InvTerrainSize[1] = 1.0f / lightServerState.terrainSize;
        params.TerrainShadowMapPixelSize[0] = params.TerrainShadowMapPixelSize[1] = 1.0f / lightServerState.terrainShadowMapSize;
        (lightServerState.csmUtil.GetShadowView() * invViewTransform).store(params.CSMShadowMatrix);

        flags |= USE_SHADOW_BITFLAG;
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

    IndexT i;
    for (i = 0; i < types.Size(); i++)
    {
        switch (types[i])
        {
            case PointLightType:
            {
                Math::mat4 trans = pointLightAllocator.Get<PointLight_Transform>(typeIds[i]);
                CoreGraphics::TextureId tex = pointLightAllocator.Get<PointLight_ProjectionTexture>(typeIds[i]);
                auto& pointLight = clusterState.lightList.PointLights[numPointLights];

                uint flags = 0;

                // update shadow data
                if (castShadow[i])
                {
                    flags |= USE_SHADOW_BITFLAG;
                }

                // check if we should use projection
                if (tex != InvalidTextureId)
                {
                    flags |= USE_PROJECTION_TEX_BITFLAG;
                }

                Math::vec4 posAndRange = viewTransform * trans.position;
                posAndRange.w = range[i];
                posAndRange.store(pointLight.position);
                (color[i] * intensity[i]).store(pointLight.color);
                pointLight.flags = flags;
                numPointLights++;
            }
            break;

            case SpotLightType:
            {
                Math::mat4 trans = spotLightAllocator.Get<SpotLight_Transform>(typeIds[i]);
                CoreGraphics::TextureId tex = spotLightAllocator.Get<SpotLight_ProjectionTexture>(typeIds[i]);
                auto angles = spotLightAllocator.Get<SpotLight_ConeAngles>(typeIds[i]);
                auto& spotLight = clusterState.lightList.SpotLights[numSpotLights];
                Math::mat4 shadowProj;
                if (tex != InvalidTextureId || castShadow[i])
                {
                    Graphics::GraphicsEntityId observer = spotLightAllocator.Get<SpotLight_Observer>(typeIds[i]);
                    Graphics::ContextEntityId ctxId = shadowCasterSliceMap[observer];
                    shadowProj = shadowCasterAllocator.Get<ShadowCaster_Transform>(ctxId.id) * invViewTransform;
                }
                spotLight.shadowExtension = -1;
                spotLight.projectionExtension = -1;

                uint flags = 0;

                // update shadow data
                if (castShadow[i] && numShadowLights < 16)
                {
                    flags |= USE_SHADOW_BITFLAG;
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
                    flags |= USE_PROJECTION_TEX_BITFLAG;
                    spotLight.projectionExtension = numSpotLightsProjection;
                    auto& projection = clusterState.lightList.SpotLightProjection[numSpotLightsProjection];
                    shadowProj.store(projection.projection);
                    projection.projectionTexture = CoreGraphics::TextureGetBindlessHandle(tex);
                    numSpotLightsProjection++;
                }

                Math::mat4 viewSpace = viewTransform * trans;
                Math::vec4 posAndRange = viewSpace.position;
                posAndRange.w = range[i];

                Math::vec4 forward = normalize(viewSpace.z_axis);
                if (angles[0] == angles[1])
                    forward.w = 1.0f;
                else
                    forward.w = 1.0f / (angles[1] - angles[0]);

                posAndRange.store(spotLight.position);
                forward.store(spotLight.forward);
                (color[i] * intensity[i]).store(spotLight.color);
                
                // calculate sine and cosine
                spotLight.angleSinCos[0] = Math::sin(angles[1]);
                spotLight.angleSinCos[1] = Math::cos(angles[1]);
                spotLight.flags = flags;
                numSpotLights++;
            }
            break;
        }
    }

    IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();

    // update list of point lights
    if (numPointLights > 0 || numSpotLights > 0)
    {
        CoreGraphics::BufferUpdate(clusterState.stagingClusterLightsList[bufferIndex], clusterState.lightList);
        CoreGraphics::BufferFlush(clusterState.stagingClusterLightsList[bufferIndex]);
    }

    // get per-view resource tables
    CoreGraphics::ResourceTableId computeTable = Graphics::GetFrameResourceTableCompute(bufferIndex);
    CoreGraphics::ResourceTableId graphicsTable = Graphics::GetFrameResourceTableGraphics(bufferIndex);

    LightsCluster::LightUniforms consts;
    consts.NumSpotLights = numSpotLights;
    consts.NumPointLights = numPointLights;
    consts.NumLightClusters = Clustering::ClusterContext::GetNumClusters();
    consts.SSAOBuffer = CoreGraphics::TextureGetBindlessHandle(textureState.aoTexture);
    IndexT offset = SetConstants(consts);
    ResourceTableSetConstantBuffer(computeTable, { GetComputeConstantBuffer(), Shared::Table_Frame::LightUniforms::SLOT, 0, Shared::Table_Frame::LightUniforms::SIZE, (SizeT)offset });
    ResourceTableCommitChanges(computeTable);
    ResourceTableSetConstantBuffer(graphicsTable, { GetGraphicsConstantBuffer(), Shared::Table_Frame::LightUniforms::SLOT, 0, Shared::Table_Frame::LightUniforms::SIZE, (SizeT)offset });
    ResourceTableCommitChanges(graphicsTable);

    TextureDimensions dims = TextureGetDimensions(textureState.lightingTexture);
    Combine::CombineUniforms combineConsts;
    combineConsts.LowresResolution[0] = 1.0f / dims.width;
    combineConsts.LowresResolution[1] = 1.0f / dims.height;
    offset = SetConstants(combineConsts);
    ResourceTableSetConstantBuffer(combineState.resourceTables[bufferIndex], { GetGraphicsConstantBuffer(), Combine::Table_Batch::CombineUniforms::SLOT, 0, Combine::Table_Batch::CombineUniforms::SIZE, (SizeT)offset });
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
    case GlobalLightType:
    {
        // dealloc observers
        Util::Array<Graphics::GraphicsEntityId>& observers = globalLightAllocator.Get<GlobalLight_CascadeObservers>(lightId);
        for (IndexT i = 0; i < observers.Size(); i++)
        {
            Visibility::ObserverContext::DeregisterEntity(observers[i]);
            Graphics::DestroyEntity(observers[i]);
        }

        globalLightAllocator.Dealloc(lightId);
        break;
    }
    case SpotLightType:
        spotLightAllocator.Dealloc(lightId);
        break;
    case PointLightType:
        spotLightAllocator.Dealloc(lightId);
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
    ShapeRenderer* shapeRenderer = ShapeRenderer::Instance();

    for (int i = 0, n = types.Size(); i < n; ++i)
    {
        switch(types[i])
        {
        case PointLightType:
        {
            Math::mat4 trans = Math::affine(Math::vec3(ranges[ids[i]]), Math::vec3(0), Math::quat(), xyz(pointTrans[ids[i]].position));
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
        }
        break;
        case SpotLightType:
        {

            // setup a perpsective transform with a fixed z near and far and aspect
            Graphics::CameraSettings settings;
            std::array<float, 2> angles = spotLightAllocator.Get<SpotLight_ConeAngles>(ids[i]);

            // get projection
            Math::mat4 proj = spotLightAllocator.Get<SpotLight_ProjectionTransform>(ids[i]);

            // take transform, scale Z with range and move back half the range
            Math::mat4 unscaledTransform = spotTrans[ids[i]];
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
        }
        break;
        case GlobalLightType:
        {

        }
        break;
        }
    }
}
} // namespace Lighting
