//------------------------------------------------------------------------------
// lightcontext.cc
// (C) 2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "frame/plugins/frameplugin.h"
#include "lightcontext.h"
#include "graphics/graphicsserver.h"
#include "graphics/view.h"
#include "graphics/cameracontext.h"
#include "renderutil/drawfullscreenquad.h"
#include "csmutil.h"
#include "math/polar.h"
#include "frame/framebatchtype.h"
#include "frame/framesubpassbatch.h"
#include "resources/resourceserver.h"
#include "visibility/visibilitycontext.h"
#include "clustering/clustercontext.h"
#ifndef PUBLIC_BUILD
#include "dynui/im3d/im3dcontext.h"
#include "debug/framescriptinspector.h"
#endif

#include "lights_cluster_cull.h"
#include "lights.h"

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
Util::HashTable<Graphics::GraphicsEntityId, Graphics::ContextEntityId, 6, 1> LightContext::shadowCasterSliceMap;
_ImplementContext(LightContext, LightContext::genericLightAllocator);

struct
{
	CoreGraphics::ShaderId lightShader;

	// compute shader implementation (todo)
	CoreGraphics::ShaderProgramId lightsClassification;		// pre-pass for compute
	CoreGraphics::ShaderProgramId lightsCompute;			// compute shader for lights

	// pixel shader versions
	CoreGraphics::ShaderProgramId globalLight;
	CoreGraphics::ShaderProgramId globalLightShadow;
	struct GlobalLight
	{
		CoreGraphics::ConstantBinding dirWorldSpace, dir, color, backlightColor, ambientColor, backlightFactor, shadowMatrix, shadowHandle;
	} global;

	Graphics::GraphicsEntityId globalLightEntity = Graphics::GraphicsEntityId::Invalid();
	RenderUtil::DrawFullScreenQuad fsq;

	CoreGraphics::ShaderProgramId spotLight;		
	CoreGraphics::ShaderProgramId pointLight;

	struct LocalLight
	{
		CoreGraphics::ConstantBinding color, posRange, projectionTransform, transform, projectionTexture, flags;
		CoreGraphics::ConstantBinding shadowOffsetScale, shadowConstants, shadowBias, shadowIntensity, shadowTransform, shadowMap;
	} local;

	Util::Array<Graphics::GraphicsEntityId> spotLightEntities;
	Util::RingBuffer<Graphics::GraphicsEntityId> shadowcastingLocalLights;

	CoreGraphics::ResourceTableId localLightsResourceTable;
	CoreGraphics::ResourceTableId lightsbatchResourceTable;
	CoreGraphics::ConstantBufferId localLightsConstantBuffer;			// use for all lights
	CoreGraphics::ConstantBufferId localLightsShadowConstantBuffer;		// only use for shadowcasting lights

	CoreGraphics::MeshId spotLightMesh;
	CoreGraphics::MeshId pointLightMesh;
	CoreGraphics::TextureId spotLightDefaultProjection;

	IndexT localLightsSlot, localLightShadowSlot;

	Ptr<Frame::FrameScript> shadowMappingFrameScript;
	alignas(16) Shared::ShadowMatrixBlock shadowMatrixUniforms;
	CoreGraphics::TextureId localLightShadows;
	CoreGraphics::TextureId globalLightShadowMap;
	CoreGraphics::TextureId globalLightShadowMapBlurred0, globalLightShadowMapBlurred1;
	CoreGraphics::BatchGroup::Code spotlightsBatchCode;
	CoreGraphics::BatchGroup::Code globalLightsBatchCode;

	CoreGraphics::ShaderId csmBlurShader;
	CoreGraphics::ShaderProgramId csmBlurXProgram, csmBlurYProgram;
	CoreGraphics::ResourceTableId csmBlurXTable, csmBlurYTable;
	IndexT csmBlurXInputSlot, csmBlurYInputSlot;
	IndexT csmBlurXOutputSlot, csmBlurYOutputSlot;

	CSMUtil csmUtil;
} lightServerState;

struct
{
	CoreGraphics::ShaderId classificationShader;
	CoreGraphics::ShaderProgramId cullProgram;
	CoreGraphics::ShaderProgramId debugProgram;
	CoreGraphics::ShaderProgramId lightingProgram;
	CoreGraphics::ShaderRWBufferId clusterLightIndexLists;
	Util::FixedArray<CoreGraphics::ShaderRWBufferId> clusterLightsList;
	CoreGraphics::ConstantBufferId clusterPointLights;
	CoreGraphics::ConstantBufferId clusterSpotLights;

	IndexT clusterUniformsSlot;
	IndexT lightCullUniformsSlot;
	IndexT lightingUniformsSlot;
	IndexT lightListSlot;
	IndexT lightShadingTextureSlot;
	IndexT lightShadingDebugTextureSlot;
	Util::FixedArray<CoreGraphics::ResourceTableId> clusterResourceTables;

	uint numThreadsThisFrame;

#ifdef CLUSTERED_LIGHTING_DEBUG
	CoreGraphics::TextureId clusterDebugTexture;
#endif
	CoreGraphics::TextureId clusterLightingTexture;

	// these are used to update the light clustering
	LightsClusterCull::PointLight pointLights[1024];
	LightsClusterCull::SpotLight spotLights[1024];
	LightsClusterCull::SpotLightProjectionExtension spotLightProjection[256];
	LightsClusterCull::SpotLightShadowExtension spotLightShadow[16];

} clusterState;

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
	_CreateContext();

	__bundle.OnPrepareView = LightContext::OnPrepareView;
	__bundle.OnUpdateViewResources = LightContext::UpdateViewDependentResources;
	__bundle.OnWorkFinished = LightContext::RunFrameScriptJobs;
	__bundle.StageBits = &LightContext::__state.currentStage;
#ifndef PUBLIC_BUILD
	__bundle.OnRenderDebug = LightContext::OnRenderDebug;
#endif
	Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

	// called from main script
	Frame::FramePlugin::AddCallback("LightContext - Update Shadowmaps", [](IndexT frame) // trigger update
		{
			// run the script
			N_SCOPE(ShadowMapExecute, Render);
			lightServerState.shadowMappingFrameScript->Run(frame);
#ifndef PUBLIC_BUILD
			//Debug::FrameScriptInspector::Run(lightServerState.shadowMappingFrameScript);
#endif
		});

	// register shadow mapping algorithms
	Frame::FramePlugin::AddCallback("LightContext - Spotlight Shadows", [](IndexT frame) // graphics
		{
			IndexT i;
			for (i = 0; i < lightServerState.shadowcastingLocalLights.Size(); i++)
			{
				// draw it!
				Frame::FrameSubpassBatch::DrawBatch(lightServerState.spotlightsBatchCode, lightServerState.shadowcastingLocalLights[i], 1, i);
			}
		});
	Frame::FramePlugin::AddCallback("LightContext - Spotlight Blur", [](IndexT frame) // compute
		{
		});

	Frame::FramePlugin::AddCallback("LightContext - Sun Shadows", [](IndexT frame) // graphics
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
					Frame::FrameSubpassBatch::DrawBatch(lightServerState.globalLightsBatchCode, observers[i], 1, i);
				}				
			}
		});
	Frame::FramePlugin::AddCallback("LightContext - Sun Blur", [](IndexT frame) // compute
		{
			LightContext::BlurGlobalShadowMap();
		});

	Frame::FramePlugin::AddCallback("LightContext - Cull and Classify", [](IndexT frame)
		{
			LightContext::CullAndClassify();
		});

	Frame::FramePlugin::AddCallback("LightContext - Deferred Cluster", [](IndexT frame)
		{
			LightContext::ComputeLighting();
		});

	// create shadow mapping frame script
	lightServerState.shadowMappingFrameScript = Frame::FrameServer::Instance()->LoadFrameScript("shadowmap_framescript", "frame:vkshadowmap.json"_uri);
	lightServerState.shadowMappingFrameScript->Build();
	lightServerState.spotlightsBatchCode = CoreGraphics::BatchGroup::FromName("SpotLightShadow");
	lightServerState.globalLightsBatchCode = CoreGraphics::BatchGroup::FromName("GlobalShadow");
	lightServerState.globalLightShadowMap = lightServerState.shadowMappingFrameScript->GetTexture("SunShadow");
	lightServerState.globalLightShadowMapBlurred0 = lightServerState.shadowMappingFrameScript->GetTexture("SunShadowFiltered0");
	lightServerState.globalLightShadowMapBlurred1 = lightServerState.shadowMappingFrameScript->GetTexture("SunShadowFiltered1");
	lightServerState.localLightShadows = lightServerState.shadowMappingFrameScript->GetTexture("LocalLightShadow");

	using namespace CoreGraphics;

	lightServerState.csmBlurShader = ShaderGet("shd:csmblur.fxb");
	lightServerState.csmBlurXProgram = ShaderGetProgram(lightServerState.csmBlurShader, ShaderFeatureFromString("Alt0"));
	lightServerState.csmBlurYProgram = ShaderGetProgram(lightServerState.csmBlurShader, ShaderFeatureFromString("Alt1"));
	lightServerState.csmBlurXInputSlot = ShaderGetResourceSlot(lightServerState.csmBlurShader, "InputImageX");
	lightServerState.csmBlurYInputSlot = ShaderGetResourceSlot(lightServerState.csmBlurShader, "InputImageY");
	lightServerState.csmBlurXOutputSlot = ShaderGetResourceSlot(lightServerState.csmBlurShader, "BlurImageX");
	lightServerState.csmBlurYOutputSlot = ShaderGetResourceSlot(lightServerState.csmBlurShader, "BlurImageY");
	lightServerState.csmBlurXTable = ShaderCreateResourceTable(lightServerState.csmBlurShader, NEBULA_BATCH_GROUP);
	lightServerState.csmBlurYTable = ShaderCreateResourceTable(lightServerState.csmBlurShader, NEBULA_BATCH_GROUP);
	ResourceTableSetTexture(lightServerState.csmBlurXTable, { lightServerState.globalLightShadowMap, lightServerState.csmBlurXInputSlot, 0, CoreGraphics::SamplerId::Invalid(), false }); // ping
	ResourceTableSetRWTexture(lightServerState.csmBlurXTable, { lightServerState.globalLightShadowMapBlurred0, lightServerState.csmBlurXOutputSlot, 0, CoreGraphics::SamplerId::Invalid() }); // pong
	ResourceTableSetTexture(lightServerState.csmBlurYTable, { lightServerState.globalLightShadowMapBlurred0, lightServerState.csmBlurYInputSlot, 0, CoreGraphics::SamplerId::Invalid() }); // ping
	ResourceTableSetRWTexture(lightServerState.csmBlurYTable, { lightServerState.globalLightShadowMapBlurred1, lightServerState.csmBlurYOutputSlot, 0, CoreGraphics::SamplerId::Invalid() }); // pong
	ResourceTableCommitChanges(lightServerState.csmBlurXTable);
	ResourceTableCommitChanges(lightServerState.csmBlurYTable);

	lightServerState.lightShader = ShaderServer::Instance()->GetShader("shd:lights.fxb");
	lightServerState.globalLight = ShaderGetProgram(lightServerState.lightShader, CoreGraphics::ShaderFeatureFromString("Global"));
	lightServerState.global.dirWorldSpace = ShaderGetConstantBinding(lightServerState.lightShader, "GlobalLightDirWorldspace");
	lightServerState.global.dir = ShaderGetConstantBinding(lightServerState.lightShader, "GlobalLightDir");
	lightServerState.global.color = ShaderGetConstantBinding(lightServerState.lightShader, "GlobalLightColor");
	lightServerState.global.backlightColor = ShaderGetConstantBinding(lightServerState.lightShader, "GlobalBackLightColor");
	lightServerState.global.ambientColor = ShaderGetConstantBinding(lightServerState.lightShader, "GlobalAmbientLightColor");
	lightServerState.global.backlightFactor = ShaderGetConstantBinding(lightServerState.lightShader, "GlobalBackLightOffset");
	lightServerState.global.shadowMatrix = ShaderGetConstantBinding(lightServerState.lightShader, "CSMShadowMatrix");
	lightServerState.global.shadowHandle = ShaderGetConstantBinding(lightServerState.lightShader, "GlobalLightShadowBuffer");

	lightServerState.pointLight = ShaderGetProgram(lightServerState.lightShader, CoreGraphics::ShaderFeatureFromString("Point"));
	lightServerState.spotLight = ShaderGetProgram(lightServerState.lightShader, CoreGraphics::ShaderFeatureFromString("Spot"));
	lightServerState.local.flags = ShaderGetConstantBinding(lightServerState.lightShader, "Flags");
	lightServerState.local.color = ShaderGetConstantBinding(lightServerState.lightShader, "LightColor");
	lightServerState.local.posRange = ShaderGetConstantBinding(lightServerState.lightShader, "LightPosRange");
	lightServerState.local.projectionTransform = ShaderGetConstantBinding(lightServerState.lightShader, "LightProjTransform");
	lightServerState.local.transform = ShaderGetConstantBinding(lightServerState.lightShader, "LightTransform");
	lightServerState.local.projectionTexture = ShaderGetConstantBinding(lightServerState.lightShader, "ProjectionTexture");

	lightServerState.local.shadowOffsetScale = ShaderGetConstantBinding(lightServerState.lightShader, "ShadowOffsetScale");
	lightServerState.local.shadowConstants = ShaderGetConstantBinding(lightServerState.lightShader, "ShadowConstants");
	lightServerState.local.shadowBias = ShaderGetConstantBinding(lightServerState.lightShader, "ShadowBias");
	lightServerState.local.shadowIntensity = ShaderGetConstantBinding(lightServerState.lightShader, "ShadowIntensity");
	lightServerState.local.shadowTransform = ShaderGetConstantBinding(lightServerState.lightShader, "ShadowProjTransform");
	lightServerState.local.shadowMap = ShaderGetConstantBinding(lightServerState.lightShader, "ShadowMap");

	// setup buffers, copy tick params from shader server (but only update global light stuff)
	lightServerState.localLightsResourceTable = ShaderCreateResourceTable(lightServerState.lightShader, NEBULA_INSTANCE_GROUP);
	lightServerState.localLightsConstantBuffer = CoreGraphics::GetGraphicsConstantBuffer(MainThreadConstantBuffer);
	lightServerState.localLightsShadowConstantBuffer = CoreGraphics::GetGraphicsConstantBuffer(MainThreadConstantBuffer);
	lightServerState.localLightsSlot = ShaderGetResourceSlot(lightServerState.lightShader, "LocalLightBlock");
	lightServerState.localLightShadowSlot = ShaderGetResourceSlot(lightServerState.lightShader, "LocalLightShadowBlock");
	ResourceTableSetConstantBuffer(lightServerState.localLightsResourceTable, { lightServerState.localLightsConstantBuffer, lightServerState.localLightsSlot, 0, true, false, sizeof(Lights::LocalLightBlock), 0});
	ResourceTableSetConstantBuffer(lightServerState.localLightsResourceTable, { lightServerState.localLightsShadowConstantBuffer, lightServerState.localLightShadowSlot, 0, true, false, sizeof(Lights::LocalLightShadowBlock), 0 });
	ResourceTableCommitChanges(lightServerState.localLightsResourceTable);

	lightServerState.spotLightMesh = Resources::CreateResource("msh:system/spotlightshape.nvx2", "system", nullptr, nullptr, true);
	lightServerState.pointLightMesh = Resources::CreateResource("msh:system/pointlightshape.nvx2", "system", nullptr, nullptr, true);
	lightServerState.spotLightDefaultProjection = Resources::CreateResource("tex:system/spotlight.dds", "system", nullptr, nullptr, true);

	// setup batch table (contains all the samplers and such)
	lightServerState.lightsbatchResourceTable = ShaderCreateResourceTable(lightServerState.lightShader, NEBULA_BATCH_GROUP);

	DisplayMode mode = WindowGetDisplayMode(DisplayDevice::Instance()->GetCurrentWindow());
	lightServerState.fsq.Setup(mode.GetWidth(), mode.GetHeight());

	ShaderRWBufferCreateInfo rwbInfo =
	{
		"LightIndexListsBuffer",
		sizeof(LightsClusterCull::LightIndexLists),
		1,
		BufferUpdateMode::DeviceWriteable,
		false
	};
	clusterState.clusterLightIndexLists = CreateShaderRWBuffer(rwbInfo);

	clusterState.classificationShader = ShaderServer::Instance()->GetShader("shd:lights_cluster_cull.fxb");
	IndexT lightIndexListsSlot = ShaderGetResourceSlot(clusterState.classificationShader, "LightIndexLists");
	IndexT lightsListSlot = ShaderGetResourceSlot(clusterState.classificationShader, "LightLists");
	IndexT clusterAABBSlot = ShaderGetResourceSlot(clusterState.classificationShader, "ClusterAABBs");
#ifdef CLUSTERED_LIGHTING_DEBUG
	clusterState.lightShadingDebugTextureSlot = ShaderGetResourceSlot(clusterState.classificationShader, "DebugOutput");
#endif
	clusterState.lightShadingTextureSlot = ShaderGetResourceSlot(clusterState.classificationShader, "Lighting");
	clusterState.clusterUniformsSlot = ShaderGetResourceSlot(clusterState.classificationShader, "ClusterUniforms");
	clusterState.lightCullUniformsSlot = ShaderGetResourceSlot(clusterState.classificationShader, "LightCullUniforms");
	clusterState.lightingUniformsSlot = ShaderGetResourceSlot(clusterState.classificationShader, "LightConstants");

	clusterState.cullProgram = ShaderGetProgram(clusterState.classificationShader, ShaderServer::Instance()->FeatureStringToMask("Cull"));
#ifdef CLUSTERED_LIGHTING_DEBUG
	clusterState.debugProgram = ShaderGetProgram(clusterState.classificationShader, ShaderServer::Instance()->FeatureStringToMask("ClusterDebug"));
#endif
	clusterState.lightingProgram = ShaderGetProgram(clusterState.classificationShader, ShaderServer::Instance()->FeatureStringToMask("Lighting"));

	clusterState.clusterResourceTables.Resize(CoreGraphics::GetNumBufferedFrames());

	rwbInfo.name = "LightLists";
	rwbInfo.size = sizeof(LightsClusterCull::LightLists);
	rwbInfo.mode = BufferUpdateMode::HostWriteable;
	clusterState.clusterLightsList.Resize(CoreGraphics::GetNumBufferedFrames());

	for (IndexT i = 0; i < clusterState.clusterResourceTables.Size(); i++)
	{
		clusterState.clusterResourceTables[i] = ShaderCreateResourceTable(clusterState.classificationShader, NEBULA_BATCH_GROUP);
		clusterState.clusterLightsList[i] = CreateShaderRWBuffer(rwbInfo);

		// update resource table
		ResourceTableSetRWBuffer(clusterState.clusterResourceTables[i], { clusterState.clusterLightIndexLists, lightIndexListsSlot, 0, false, false, -1, 0 });
		ResourceTableSetRWBuffer(clusterState.clusterResourceTables[i], { Clustering::ClusterContext::GetClusterBuffer(), clusterAABBSlot, 0, false, false, -1, 0 });
		ResourceTableSetRWBuffer(clusterState.clusterResourceTables[i], { clusterState.clusterLightsList[i], lightsListSlot, 0, false, false, -1, 0 });
		ResourceTableSetConstantBuffer(clusterState.clusterResourceTables[i], { CoreGraphics::GetComputeConstantBuffer(MainThreadConstantBuffer), clusterState.clusterUniformsSlot, 0, false, false, sizeof(LightsClusterCull::ClusterUniforms), 0 });
		ResourceTableSetConstantBuffer(clusterState.clusterResourceTables[i], { CoreGraphics::GetComputeConstantBuffer(MainThreadConstantBuffer), clusterState.lightCullUniformsSlot, 0, false, false, sizeof(LightsClusterCull::LightCullUniforms), 0 });
		ResourceTableCommitChanges(clusterState.clusterResourceTables[i]);
	}

	// allow 16 shadow casting local lights
	lightServerState.shadowcastingLocalLights.SetCapacity(16);

	_CreateContext();
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::Discard()
{
	lightServerState.fsq.Discard();
	Frame::FrameServer::Instance()->UnloadFrameScript("shadowmap_framescript");
	Graphics::GraphicsServer::Instance()->UnregisterGraphicsContext(&__bundle);
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::SetupGlobalLight(const Graphics::GraphicsEntityId id, const Math::float4& color, const float intensity, const Math::float4& ambient, const Math::float4& backlight, const float backlightFactor, const Math::vector& direction, bool castShadows)
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

	Math::matrix44 mat = Math::matrix44::lookatrh(Math::point(0.0f), -direction, Math::vector::upvec());
	
	SetGlobalLightTransform(cid, mat);
	globalLightAllocator.Get<GlobalLight_Backlight>(lid) = backlight;
	globalLightAllocator.Get<GlobalLight_Ambient>(lid) = ambient;
	globalLightAllocator.Get<GlobalLight_BacklightOffset>(lid) = backlightFactor;

	if (castShadows)
	{
		// create new graphics entity for each view
		for (IndexT i = 0; i < CSMUtil::NumCascades; i++)
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
	const Math::float4& color, 
	const float intensity, 
	const Math::matrix44& transform, 
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

	// allocate shadow buffer slice if we cast shadows
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::SetupSpotLight(const Graphics::GraphicsEntityId id, 
	const Math::float4& color, 
	const float intensity, 
	const float innerConeAngle,
	const float outerConeAngle,
	const Math::matrix44& transform,
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
		angles[0] = outerConeAngle - Math::n_deg2rad(0.1f);

	// construct projection from angle and range
	const float zNear = 0.1f;
	const float zFar = range;

	// use a fixed aspect of 1
	Math::matrix44 proj = Math::matrix44::perspfov(angles[1] * 2.0f, 1.0f, zNear, zFar);
	proj.setrow1(Math::float4::multiply(proj.getrow1(), Math::float4(-1)));

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
LightContext::SetColor(const Graphics::GraphicsEntityId id, const Math::float4& color)
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
const Math::matrix44
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
		return Math::matrix44::identity();
		break;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::SetTransform(const Graphics::GraphicsEntityId id, const Math::matrix44& trans)
{
    const Graphics::ContextEntityId cid = GetContextId(id);
    LightType type = genericLightAllocator.Get<Type>(cid.id);    

    switch (type)
    {
    case GlobalLightType:
        SetGlobalLightTransform(cid, trans);
        break;
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
const Math::matrix44 
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
		inner = outer - Math::n_deg2rad(0.1f);
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

	/// setup global light visibility
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
		for (i = 0; i < CSMUtil::NumCascades; i++)
		{
			Math::matrix44::store(lightServerState.csmUtil.GetCascadeViewProjection(i), lightServerState.shadowMatrixUniforms.CSMViewMatrix[i]);
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
				Graphics::CameraSettings settings;
				std::array<float, 2> angles = spotLightAllocator.Get<SpotLight_ConeAngles>(typeIds[i]);

				// setup a perpsective transform with a fixed z near and far and aspect
				Math::matrix44 projection = spotLightAllocator.Get<SpotLight_ProjectionTransform>(typeIds[i]);
				Math::matrix44 view = spotLightAllocator.Get<SpotLight_Transform>(typeIds[i]);
				Math::matrix44 viewProjection = Math::matrix44::multiply(Math::matrix44::inverse(view), projection);
				Graphics::GraphicsEntityId observer = spotLightAllocator.Get<SpotLight_Observer>(typeIds[i]);
				Graphics::ContextEntityId ctxId = shadowCasterSliceMap[observer];
				shadowCasterAllocator.Get<ShadowCaster_Transform>(ctxId.id) = viewProjection;

				lightServerState.shadowcastingLocalLights.Add(observer);
				Math::matrix44::storeu(viewProjection, lightServerState.shadowMatrixUniforms.LightViewMatrix[shadowCasterCount++]);
			}

			case PointLightType:
			{

			}
			}
		}

		// we reached our shadow caster max
		if (shadowCasterCount == 16)
			break;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::SetSpotLightTransform(const Graphics::ContextEntityId id, const Math::matrix44& transform)
{
	// todo, update projection and invviewprojection!!!!
	auto lid = genericLightAllocator.Get<TypedLightId>(id.id);
	spotLightAllocator.Get<SpotLight_Transform>(lid) = transform;
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::SetPointLightTransform(const Graphics::ContextEntityId id, const Math::matrix44& transform)
{
	auto lid = genericLightAllocator.Get<TypedLightId>(id.id);
	pointLightAllocator.Get<PointLight_Transform>(lid) = transform;
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::SetGlobalLightTransform(const Graphics::ContextEntityId id, const Math::matrix44& transform)
{
	auto lid = genericLightAllocator.Get<TypedLightId>(id.id);
	globalLightAllocator.Get<GlobalLight_Direction>(lid) = -Math::float4::normalize(Math::vector(transform.get_zaxis()));
	globalLightAllocator.Get<GlobalLight_Transform>(lid) = transform;
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::SetGlobalLightViewProjTransform(const Graphics::ContextEntityId id, const Math::matrix44& transform)
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
	const Graphics::ContextEntityId cid = GetContextId(lightServerState.globalLightEntity);
	using namespace CoreGraphics;

	// get camera view
	Math::matrix44 viewTransform = Graphics::CameraContext::GetTransform(view->GetCamera());
	Math::matrix44 invViewTransform = Math::matrix44::inverse(viewTransform);

	// update constant buffer
	Ids::Id32 globalLightId = genericLightAllocator.Get<TypedLightId>(cid.id);
	Shared::PerTickParams& params = ShaderServer::Instance()->GetTickParams();
	Math::float4::storeu(genericLightAllocator.Get<Color>(cid.id) * genericLightAllocator.Get<Intensity>(cid.id), params.GlobalLightColor);
	Math::float4::store3u(globalLightAllocator.Get<GlobalLight_Direction>(globalLightId), params.GlobalLightDirWorldspace);
	Math::float4::storeu(globalLightAllocator.Get<GlobalLight_Backlight>(globalLightId), params.GlobalBackLightColor);
	Math::float4::storeu(globalLightAllocator.Get<GlobalLight_Ambient>(globalLightId), params.GlobalAmbientLightColor);
	Math::float4 viewSpaceLightDir = Math::matrix44::transform3(Math::vector(globalLightAllocator.Get<GlobalLight_Direction>(globalLightId)), invViewTransform);
	Math::float4::store3u(Math::float4::normalize(viewSpaceLightDir), params.GlobalLightDir);
	params.GlobalBackLightOffset = globalLightAllocator.Get<GlobalLight_BacklightOffset>(globalLightId);

	// apply shadow uniforms
	CoreGraphics::TransformDevice::Instance()->ApplyShadowSettings(lightServerState.shadowMatrixUniforms);

	uint flags = 0;

	if (genericLightAllocator.Get<ShadowCaster>(cid.id))
	{

#if __DX11__
		Math::matrix44 textureScale = Math::matrix44::scaling(0.5f, -0.5f, 1.0f);
#elif (__OGL4__ || __VULKAN__)
		Math::matrix44 textureScale = Math::matrix44::scaling(0.5f, 0.5f, 1.0f);
#endif
		Math::matrix44 textureTranslation = Math::matrix44::translation(0.5f, 0.5f, 0);
		const Math::matrix44* transforms = lightServerState.csmUtil.GetCascadeProjectionTransforms();
		Math::float4 cascadeScales[CSMUtil::NumCascades];
		Math::float4 cascadeOffsets[CSMUtil::NumCascades];

		for (IndexT splitIndex = 0; splitIndex < CSMUtil::NumCascades; ++splitIndex)
		{
			Math::matrix44 shadowTexture = Math::matrix44::multiply(
				transforms[splitIndex],
				Math::matrix44::multiply(textureScale, textureTranslation));
			Math::float4 scale;
			scale.x() = shadowTexture.getrow0().x();
			scale.y() = shadowTexture.getrow1().y();
			scale.z() = shadowTexture.getrow2().z();
			scale.w() = 1;
			Math::float4 offset = shadowTexture.getrow3();
			offset.w() = 0;
			cascadeOffsets[splitIndex] = offset;
			cascadeScales[splitIndex] = scale;
		}

		memcpy(params.CascadeOffset, cascadeOffsets, sizeof(Math::float4) * CSMUtil::NumCascades);
		memcpy(params.CascadeScale, cascadeScales, sizeof(Math::float4) * CSMUtil::NumCascades);
		memcpy(params.CascadeDistances, lightServerState.csmUtil.GetCascadeDistances(), sizeof(float) * CSMUtil::NumCascades);
		params.MinBorderPadding = 1.0f / 1024.0f;
		params.MaxBorderPadding = (1024.0f - 1.0f) / 1024.0f;
		params.ShadowPartitionSize = 1.0f;
		params.GlobalLightShadowBuffer = CoreGraphics::TextureGetBindlessHandle(lightServerState.globalLightShadowMapBlurred1);
		Math::matrix44::store(lightServerState.csmUtil.GetShadowView(), params.CSMShadowMatrix);

		flags |= USE_SHADOW_BITFLAG;
	}
	params.GlobalLightFlags = flags;
	params.GlobalLightShadowBias = 0.000001f;																			 
	params.GlobalLightShadowIntensity = 1.0f;

	// go through and update local lights
	const Util::Array<LightType>& types		= genericLightAllocator.GetArray<Type>();
	const Util::Array<Math::float4>& color	= genericLightAllocator.GetArray<Color>();
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
				auto trans = pointLightAllocator.Get<PointLight_Transform>(typeIds[i]);
				auto tex = pointLightAllocator.Get<PointLight_ProjectionTexture>(typeIds[i]);
				auto& pointLight = clusterState.pointLights[numPointLights];

				uint flags = 0;

				// update shadow data
				if (castShadow[i])
				{
					flags |= USE_SHADOW_BITFLAG;
				}

				// check if we should use projection
				if (tex != TextureId::Invalid())
				{
					flags |= USE_PROJECTION_TEX_BITFLAG;
				}

				Math::float4 posAndRange = Math::float4::transform(trans.get_position(), viewTransform);
				posAndRange.w() = range[i];
				Math::float4::storeu(posAndRange, pointLight.position);
				Math::float4::store3u(color[i] * intensity[i], pointLight.color);
				pointLight.flags = flags;
				numPointLights++;
			}
			break;

			case SpotLightType:
			{
				auto trans = spotLightAllocator.Get<SpotLight_Transform>(typeIds[i]);
				auto tex = spotLightAllocator.Get<SpotLight_ProjectionTexture>(typeIds[i]);
				auto angles = spotLightAllocator.Get<SpotLight_ConeAngles>(typeIds[i]);
				auto& spotLight = clusterState.spotLights[numSpotLights];
				Math::matrix44 shadowProj;
				if (tex != TextureId::Invalid() || castShadow[i])
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
					flags |= USE_SHADOW_BITFLAG;
					spotLight.shadowExtension = numSpotLightShadows;
					auto& shadow = clusterState.spotLightShadow[numSpotLightShadows];
					
					Math::matrix44::storeu(shadowProj, shadow.projection);
					shadow.shadowMap = CoreGraphics::TextureGetBindlessHandle(lightServerState.localLightShadows);
					shadow.shadowIntensity = 1.0f;
					shadow.shadowSlice = numShadowLights;
					numSpotLightShadows++;
					numShadowLights++;
				}

				// check if we should use projection
				if (tex != TextureId::Invalid() && numSpotLightsProjection < 256)
				{
					flags |= USE_PROJECTION_TEX_BITFLAG;
					spotLight.projectionExtension = numSpotLightsProjection;
					auto& projection = clusterState.spotLightProjection[numSpotLightsProjection];
					Math::matrix44::storeu(shadowProj, projection.projection);
					projection.projectionTexture = CoreGraphics::TextureGetBindlessHandle(tex);
					numSpotLightsProjection++;
				}

				Math::matrix44 viewSpace = Math::matrix44::multiply(trans, viewTransform);
				Math::float4 posAndRange = viewSpace.get_position();
				posAndRange.w() = range[i];

				Math::float4 forward = Math::float4::normalize(viewSpace.get_zaxis());
				if (angles[0] == angles[1])
					forward.w() = 1.0f;
				else
					forward.w() = 1.0f / (angles[1] - angles[0]);

				Math::float4::storeu(posAndRange, spotLight.position);
				Math::float4::storeu(forward, spotLight.forward);
				Math::float4::store3u(color[i] * intensity[i], spotLight.color);
				
				// calculate sine and cosine
				spotLight.angleSinCos[0] = Math::n_sin(angles[1]);
				spotLight.angleSinCos[1] = Math::n_cos(angles[1]);
				spotLight.flags = flags;
				numSpotLights++;
			}
			break;
		}
	}

	Graphics::GraphicsEntityId cam = view->GetCamera();
	CoreGraphics::DisplayMode displayMode = CoreGraphics::WindowGetDisplayMode(CoreGraphics::DisplayDevice::Instance()->GetCurrentWindow());
	const Graphics::CameraSettings settings = Graphics::CameraContext::GetSettings(cam);

	LightsClusterCull::LightCullUniforms uniforms;
	uniforms.NumSpotLights = numSpotLights;
	uniforms.NumPointLights = numPointLights;
	uniforms.NumClusters = Clustering::ClusterContext::GetNumClusters();

	IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();

	uint offset = SetComputeConstants(MainThreadConstantBuffer, uniforms);
	ResourceTableSetConstantBuffer(clusterState.clusterResourceTables[bufferIndex], { GetComputeConstantBuffer(MainThreadConstantBuffer), clusterState.lightCullUniformsSlot, 0, false, false, sizeof(LightsClusterCull::LightCullUniforms), (SizeT)offset });

	// use the same uniforms used to divide the clusters
	ClusterGenerate::ClusterUniforms clusterUniforms = Clustering::ClusterContext::GetUniforms();
	offset = SetComputeConstants(MainThreadConstantBuffer, clusterUniforms);
	ResourceTableSetConstantBuffer(clusterState.clusterResourceTables[bufferIndex], { GetComputeConstantBuffer(MainThreadConstantBuffer), clusterState.clusterUniformsSlot, 0, false, false, sizeof(ClusterGenerate::ClusterUniforms), (SizeT)offset });

	// update list of point lights
	if (numPointLights > 0 || numSpotLights > 0)
	{
		LightsClusterCull::LightLists lightList;
		Memory::CopyElements(clusterState.pointLights, lightList.PointLights, numPointLights);
		Memory::CopyElements(clusterState.spotLights, lightList.SpotLights, numSpotLights);
		Memory::CopyElements(clusterState.spotLightProjection, lightList.SpotLightProjection, numSpotLightsProjection);
		Memory::CopyElements(clusterState.spotLightShadow, lightList.SpotLightShadow, numSpotLightShadows);
		CoreGraphics::ShaderRWBufferUpdate(clusterState.clusterLightsList[bufferIndex], &lightList, sizeof(LightsClusterCull::LightLists));
	}

	// a little ugly, but since the view can change the script, this has to adopt
	const CoreGraphics::TextureId shadingTex = view->GetFrameScript()->GetTexture("LightBuffer");
	clusterState.clusterLightingTexture = shadingTex;
	ResourceTableSetRWTexture(clusterState.clusterResourceTables[bufferIndex], { clusterState.clusterLightingTexture, clusterState.lightShadingTextureSlot, 0, CoreGraphics::SamplerId::Invalid() });

#ifdef CLUSTERED_LIGHTING_DEBUG
	const CoreGraphics::TextureId debugTex = view->GetFrameScript()->GetTexture("LightDebugBuffer");
	clusterState.clusterDebugTexture = debugTex;
	ResourceTableSetRWTexture(clusterState.clusterResourceTables[bufferIndex], { clusterState.clusterDebugTexture, clusterState.lightShadingDebugTextureSlot, 0, CoreGraphics::SamplerId::Invalid() });
#endif

	const CoreGraphics::TextureId ssaoTex = view->GetFrameScript()->GetTexture("SSAOBuffer");
	LightsClusterCull::LightConstants consts;
	consts.SSAOBuffer = CoreGraphics::TextureGetBindlessHandle(ssaoTex);
	offset = SetComputeConstants(MainThreadConstantBuffer, consts);
	ResourceTableSetConstantBuffer(clusterState.clusterResourceTables[bufferIndex], { GetComputeConstantBuffer(MainThreadConstantBuffer), clusterState.lightingUniformsSlot, 0, false, false, sizeof(LightsClusterCull::LightConstants), (SizeT)offset });
	ResourceTableCommitChanges(clusterState.clusterResourceTables[bufferIndex]);
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::RunFrameScriptJobs(const Graphics::FrameContext& ctx)
{
	N_SCOPE(ShadowMapRecord, Render);

	// run jobs for shadow frame script after all constants are updated
	lightServerState.shadowMappingFrameScript->RunJobs(ctx.frameIndex);
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::CullAndClassify()
{
	// update constants
	using namespace CoreGraphics;

	// begin command buffer work
	CommandBufferBeginMarker(ComputeQueueType, NEBULA_MARKER_BLUE, "Light cluster culling");

	// make sure to sync so we don't read from data that is being written...
	BarrierInsert(ComputeQueueType,
		BarrierStage::ComputeShader,
		BarrierStage::ComputeShader,
		BarrierDomain::Global,
		nullptr,
		{
			BufferBarrier
			{
				clusterState.clusterLightIndexLists,
				BarrierAccess::ShaderRead,
				BarrierAccess::ShaderWrite,
				0, -1
			},
		}, "Light cluster culling begin");

	SetShaderProgram(clusterState.cullProgram, ComputeQueueType);
	SetResourceTable(clusterState.clusterResourceTables[CoreGraphics::GetBufferedFrameIndex()], NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr, ComputeQueueType);

	// run chunks of 1024 threads at a time
	std::array<SizeT, 3> dimensions = Clustering::ClusterContext::GetClusterDimensions();
	Compute(Math::n_ceil((dimensions[0] * dimensions[1] * dimensions[2]) / 64.0f), 1, 1, ComputeQueueType);

	// make sure to sync so we don't read from data that is being written...
	BarrierInsert(ComputeQueueType,
		BarrierStage::ComputeShader,
		BarrierStage::ComputeShader,
		BarrierDomain::Global,
		nullptr,
		{
			BufferBarrier
			{
				clusterState.clusterLightIndexLists,
				BarrierAccess::ShaderWrite,
				BarrierAccess::ShaderRead,
				0, -1
			},
		}, "Light cluster culling end");

	CommandBufferEndMarker(ComputeQueueType);
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::ComputeLighting()
{
	using namespace CoreGraphics;
	TextureDimensions dims = TextureGetDimensions(clusterState.clusterLightingTexture);

#ifdef CLUSTERED_LIGHTING_DEBUG
	CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLUE, "Cluster DEBUG");

	SetShaderProgram(clusterState.debugProgram, GraphicsQueueType);
	SetResourceTable(clusterState.clusterResourceTables[CoreGraphics::GetBufferedFrameIndex()], NEBULA_BATCH_GROUP, ComputePipeline, nullptr, GraphicsQueueType);

	// perform debug output
	Compute(Math::n_divandroundup(dims.width, 64), dims.height, 1, GraphicsQueueType);

	CommandBufferEndMarker(GraphicsQueueType);
#endif

	CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLUE, "Clustered Shading");

	SetShaderProgram(clusterState.lightingProgram, GraphicsQueueType);
	SetResourceTable(clusterState.clusterResourceTables[CoreGraphics::GetBufferedFrameIndex()], NEBULA_BATCH_GROUP, ComputePipeline, nullptr, GraphicsQueueType);

	// perform debug output
	dims = TextureGetDimensions(clusterState.clusterLightingTexture);
	Compute(Math::n_divandroundup(dims.width, 64), dims.height, 1, GraphicsQueueType);

	CommandBufferEndMarker(GraphicsQueueType);
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::RenderLights()
{
	// bind the batch-local resource table here so it will propagate to all light shaders
	CoreGraphics::SetResourceTable(lightServerState.lightsbatchResourceTable, NEBULA_BATCH_GROUP, CoreGraphics::GraphicsPipeline, nullptr);

	// begin batch
	CoreGraphics::BeginBatch(Frame::FrameBatchType::Lights);

	const Graphics::ContextEntityId cid = GetContextId(lightServerState.globalLightEntity);
	if (cid != Graphics::ContextEntityId::Invalid())
	{
		// select shader
		CoreGraphics::SetShaderProgram(lightServerState.globalLight);

		// draw
		lightServerState.fsq.ApplyMesh();
		CoreGraphics::SetResourceTable(lightServerState.lightsbatchResourceTable, NEBULA_BATCH_GROUP, CoreGraphics::GraphicsPipeline, nullptr);
		CoreGraphics::Draw();
	}

	// first bind pointlight mesh
	CoreGraphics::MeshBind(lightServerState.pointLightMesh, 0);

	// set shader program and bind pipeline
	CoreGraphics::SetShaderProgram(lightServerState.pointLight);
	CoreGraphics::SetGraphicsPipeline();

	// draw local lights
	const Util::Array<Util::FixedArray<uint>>& pdynamicOffsets = pointLightAllocator.GetArray<PointLight_DynamicOffsets>();

	IndexT i;
	for (i = 0; i < pdynamicOffsets.Size(); i++)
	{
		// setup resources
		CoreGraphics::SetResourceTable(lightServerState.localLightsResourceTable, NEBULA_INSTANCE_GROUP, CoreGraphics::GraphicsPipeline, pdynamicOffsets[i]);

		// draw point light!
		CoreGraphics::Draw();
	}

	// bind the same mesh as for pointlights
	CoreGraphics::MeshBind(lightServerState.pointLightMesh, 0);

	// set shader program and bind pipeline
	CoreGraphics::SetShaderProgram(lightServerState.spotLight);
	CoreGraphics::SetGraphicsPipeline();

	const Util::Array<Util::FixedArray<uint>>& sdynamicOffsets = spotLightAllocator.GetArray<SpotLight_DynamicOffsets>();
	for (i = 0; i < sdynamicOffsets.Size(); i++)
	{
		// setup resources
		CoreGraphics::SetResourceTable(lightServerState.localLightsResourceTable, NEBULA_INSTANCE_GROUP, CoreGraphics::GraphicsPipeline, sdynamicOffsets[i]);

		// draw point light!
		CoreGraphics::Draw();
	}
	CoreGraphics::EndBatch();
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::BlurGlobalShadowMap()
{
	using namespace CoreGraphics;
	if (lightServerState.globalLightEntity != Graphics::GraphicsEntityId::Invalid())
	{
		CoreGraphics::CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLUE, "CSM Blur");
		BarrierInsert(GraphicsQueueType,
			BarrierStage::PixelShader,
			BarrierStage::ComputeShader,
			BarrierDomain::Global,
			{
				TextureBarrier
				{
					lightServerState.globalLightShadowMapBlurred0,
					ImageSubresourceInfo::ColorNoMip(4),
					CoreGraphics::ImageLayout::ShaderRead,
					CoreGraphics::ImageLayout::General,
					BarrierAccess::ShaderRead,
					BarrierAccess::ShaderWrite
				},
				TextureBarrier
				{
					lightServerState.globalLightShadowMapBlurred1,
					ImageSubresourceInfo::ColorNoMip(4),
					CoreGraphics::ImageLayout::ShaderRead,
					CoreGraphics::ImageLayout::General,
					BarrierAccess::ShaderRead,
					BarrierAccess::ShaderWrite
				}
			},
			nullptr,
			"CSM Blur Init");

		TextureDimensions dims = TextureGetDimensions(lightServerState.globalLightShadowMapBlurred0);
		SetShaderProgram(lightServerState.csmBlurXProgram);
		SetResourceTable(lightServerState.csmBlurXTable, NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
		Compute(Math::n_divandroundup(dims.width, 320), dims.height, 4);

		BarrierInsert(GraphicsQueueType,
			BarrierStage::ComputeShader,
			BarrierStage::ComputeShader,
			BarrierDomain::Global,
			{
				TextureBarrier
				{
					lightServerState.globalLightShadowMapBlurred0,
					ImageSubresourceInfo::ColorNoMip(4),
					CoreGraphics::ImageLayout::General,
					CoreGraphics::ImageLayout::ShaderRead,
					BarrierAccess::ShaderWrite,
					BarrierAccess::ShaderRead
				},
			}, 
			nullptr,
			"CSM Blur X Finish");
		SetShaderProgram(lightServerState.csmBlurYProgram);
		SetResourceTable(lightServerState.csmBlurYTable, NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
		Compute(Math::n_divandroundup(dims.height, 320), dims.width, 4);

		BarrierInsert(GraphicsQueueType,
			BarrierStage::ComputeShader,
			BarrierStage::PixelShader,
			BarrierDomain::Global,
			{
				TextureBarrier
				{
					lightServerState.globalLightShadowMapBlurred1,
					ImageSubresourceInfo::ColorNoMip(4),
					CoreGraphics::ImageLayout::General,
					CoreGraphics::ImageLayout::ShaderRead,
					BarrierAccess::ShaderWrite,
					BarrierAccess::ShaderRead
				},
			},
			nullptr,
			"CSM Blur Y Finish");

		CoreGraphics::CommandBufferEndMarker(GraphicsQueueType);
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
    auto const& types = genericLightAllocator.GetArray<Type>();    
    auto const& colors = genericLightAllocator.GetArray<Color>();
	auto const& ranges = genericLightAllocator.GetArray<Range>();
    auto const& ids = genericLightAllocator.GetArray<TypedLightId>();
    auto const& pointTrans = pointLightAllocator.GetArray<PointLight_Transform>();
    auto const& spotTrans = spotLightAllocator.GetArray<SpotLight_Transform>();
    for (int i = 0, n = types.Size(); i < n; ++i)
    {
        switch(types[i])
        {
        case PointLightType:
        {
            Math::matrix44 const& trans = pointTrans[ids[i]];
            Math::float4 col = colors[i];
			CoreGraphics::RenderShape shape;
			shape.SetupSimpleShape(Threading::Thread::GetMyThreadId(), CoreGraphics::RenderShape::Sphere, CoreGraphics::RenderShape::RenderFlag(CoreGraphics::RenderShape::CheckDepth|CoreGraphics::RenderShape::Wireframe), trans, col);
			CoreGraphics::ShapeRenderer::Instance()->AddShape(shape);
            if (flags & Im3d::Solid)
            {
                col.w() = 0.5f;
				shape.SetupSimpleShape(Threading::Thread::GetMyThreadId(), CoreGraphics::RenderShape::Sphere, CoreGraphics::RenderShape::RenderFlag(CoreGraphics::RenderShape::CheckDepth), trans, col);
				CoreGraphics::ShapeRenderer::Instance()->AddShape(shape);
            }            
        }
        break;
        case SpotLightType:
        {

			// setup a perpsective transform with a fixed z near and far and aspect
			Graphics::CameraSettings settings;
			std::array<float, 2> angles = spotLightAllocator.Get<SpotLight_ConeAngles>(ids[i]);

			// get projection
			Math::matrix44 proj = spotLightAllocator.Get<SpotLight_ProjectionTransform>(ids[i]);

            // take transform, scale Z with range and move back half the range
            Math::matrix44 unscaledTransform = spotTrans[ids[i]];
			Math::float4 pos = unscaledTransform.get_position() - unscaledTransform.get_zaxis() * ranges[i] * 0.5f;
			unscaledTransform.set_position(Math::float4(0));
			unscaledTransform.set_xaxis(unscaledTransform.get_xaxis() * ranges[i]);
			unscaledTransform.set_yaxis(unscaledTransform.get_yaxis() * ranges[i]);
			unscaledTransform.set_zaxis(unscaledTransform.get_zaxis() * ranges[i]);
			unscaledTransform.scale(Math::float4(2.0f)); // rescale because box should be in [-1, 1] and not [-0.5, 0.5]
			unscaledTransform.set_position(pos);

			// removed skewedness because we are actually just interested in transforming points
			proj.setrow3(Math::float4(0, 0, 0, 1));

			// we want the points to first get projected, then transformed v * Projection * Transform;
			Math::matrix44 frustum = Math::matrix44::multiply(proj, unscaledTransform);
			
            Math::float4 col = colors[i];

			CoreGraphics::RenderShape shape;
			shape.SetupSimpleShape(
				Threading::Thread::GetMyThreadId(), 
				CoreGraphics::RenderShape::Box, 
				CoreGraphics::RenderShape::RenderFlag(CoreGraphics::RenderShape::CheckDepth | CoreGraphics::RenderShape::Wireframe), 
				frustum, 
				col);
			CoreGraphics::ShapeRenderer::Instance()->AddShape(shape);
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
