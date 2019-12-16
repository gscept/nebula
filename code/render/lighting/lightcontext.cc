//------------------------------------------------------------------------------
// lightcontext.cc
// (C) 2017 Individual contributors, see AUTHORS file
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
#include "resources/resourcemanager.h"
#include "visibility/visibilitycontext.h"
#ifndef PUBLIC_BUILD
#include "dynui/im3d/im3dcontext.h"
#include "debug/framescriptinspector.h"
#endif

#include "lights_clustered.h"
#include "lights.h"


// match these in lights.fx
const uint USE_SHADOW_BITFLAG = 0x1;
const uint USE_PROJECTION_TEX_BITFLAG = 0x2;

namespace Lighting
{

LightContext::GenericLightAllocator LightContext::genericLightAllocator;
LightContext::PointLightAllocator LightContext::pointLightAllocator;
LightContext::SpotLightAllocator LightContext::spotLightAllocator;
LightContext::GlobalLightAllocator LightContext::globalLightAllocator;
_ImplementContext(LightContext, LightContext::genericLightAllocator);

struct ShadowCasterState
{
	Graphics::GraphicsEntityId camera;
	Ptr<Graphics::View> view;
};

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

	CoreGraphics::ResourceTableId localLightsResourceTable;
	CoreGraphics::ResourceTableId lightsbatchResourceTable;
	CoreGraphics::ConstantBufferId localLightsConstantBuffer;			// use for all lights
	CoreGraphics::ConstantBufferId localLightsShadowConstantBuffer;		// only use for shadowcasting lights

	CoreGraphics::MeshId spotLightMesh;
	CoreGraphics::MeshId pointLightMesh;
	CoreGraphics::TextureId spotLightDefaultProjection;

	IndexT localLightsSlot, localLightShadowSlot;

	Ptr<Frame::FrameScript> shadowMappingFrameScript;
	CoreGraphics::TextureId spotlightShadowAtlas;
	CoreGraphics::TextureId globalLightShadowMap;
	CoreGraphics::TextureId globalLightShadowMapBlurred0, globalLightShadowMapBlurred1;
	CoreGraphics::BatchGroup::Code spotlightsBatchCode;
	CoreGraphics::BatchGroup::Code globalLightsBatchCode;

	CoreGraphics::ShaderId csmBlurShader;
	CoreGraphics::ShaderProgramId csmBlurXProgram, csmBlurYProgram;
	CoreGraphics::ResourceTableId csmBlurXTable, csmBlurYTable;
	IndexT csmBlurXInputSlot, csmBlurYInputSlot;
	IndexT csmBlurXOutputSlot, csmBlurYOutputSlot;

	ShadowCasterState globalLightShadowView;
	CSMUtil csmUtil;
	Util::Array<ShadowCasterState> spotLightShadowViews;
	Util::Array<ShadowCasterState> pointLightShadowViews;
} lightServerState;

struct
{
	CoreGraphics::ShaderId classificationShader;
	CoreGraphics::ShaderProgramId aabbGenerateProgram;
	CoreGraphics::ShaderProgramId debugProgram;
	CoreGraphics::ShaderRWBufferId clusterAABBBuffer;
	CoreGraphics::ShaderRWBufferId clusterIndexBuffer;
	CoreGraphics::ConstantBufferId clusterLightBuffer;
	CoreGraphics::ConstantBufferId clusterConstants;
	IndexT uniformsSlot;
	IndexT lightListSlot;
	CoreGraphics::ResourceTableId clusterResourceTable;

	uint numThreadsThisFrame;

	CoreGraphics::TextureId clusterDebugTexture;

	enum DepthDivisionMode
	{
		Linear,
		Exponential
	};

	DepthDivisionMode divMode;

	static const SizeT ClusterSubdivsX = 64;
	static const SizeT ClusterSubdivsY = 64;
	static const SizeT ClusterSubdivsZ = 16;
	static const SizeT IndicesPerCluster = 32;

	// these are used to update the light clustering
	LightsClustered::Light lights[2048];

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

	__bundle.OnBeforeFrame = nullptr;
	__bundle.OnWaitForWork = nullptr;
	__bundle.OnBeforeView = LightContext::OnBeforeView;
	__bundle.OnAfterView = nullptr;
	__bundle.OnAfterFrame = nullptr;
	__bundle.StageBits = &LightContext::__state.currentStage;
#ifndef PUBLIC_BUILD
	__bundle.OnRenderDebug = LightContext::OnRenderDebug;
#endif
	Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

	// called from main script
	Frame::FramePlugin::AddCallback("LightContext - Update Shadowmaps", [](IndexT frame) // trigger update
		{
			// run the script
			//CoreGraphics::BarrierInsert(lightServerState.globalLightShadowBufferBarrier[0], GraphicsQueueType);
			lightServerState.shadowMappingFrameScript->Run(frame);
#ifndef PUBLIC_BUILD
			Debug::FrameScriptInspector::Run(lightServerState.shadowMappingFrameScript);
#endif
			//CoreGraphics::BarrierInsert(lightServerState.globalLightShadowBufferBarrier[1], GraphicsQueueType);
		});

	// register shadow mapping algorithms
	Frame::FramePlugin::AddCallback("LightContext - Spotlight Shadows", [](IndexT frame) // graphics
		{
			IndexT i;
			for (i = 0; i < lightServerState.spotLightShadowViews.Size(); i++)
			{

			}
		});
	Frame::FramePlugin::AddCallback("LightContext - Spotlight Blur", [](IndexT frame) // compute
		{
		});

	Frame::FramePlugin::AddCallback("LightContext - Sun Shadows", [](IndexT frame) // graphics
		{
			if (lightServerState.globalLightEntity != Graphics::GraphicsEntityId::Invalid())
			{
				// draw it!
				Frame::FrameSubpassBatch::DrawBatch(lightServerState.globalLightsBatchCode, lightServerState.globalLightEntity, 4);
			}
		});
	Frame::FramePlugin::AddCallback("LightContext - Sun Blur", [](IndexT frame) // compute
		{
			LightContext::BlurGlobalShadowMap();
		});

	Frame::FramePlugin::AddCallback("LightContext - Clusted Process", [](IndexT frame)
		{
			LightContext::UpdateClustersAndCull();
		});

	// create shadow mapping frame script
	lightServerState.shadowMappingFrameScript = Frame::FrameServer::Instance()->LoadFrameScript("shadowmap_framescript", "frame:vkshadowmap.json"_uri);
	lightServerState.shadowMappingFrameScript->Build();
	lightServerState.spotlightsBatchCode = CoreGraphics::BatchGroup::FromName("SpotLightShadow");
	lightServerState.globalLightsBatchCode = CoreGraphics::BatchGroup::FromName("GlobalShadow");
	lightServerState.globalLightShadowMap = lightServerState.shadowMappingFrameScript->GetColorTexture("GlobalLightShadow");
	lightServerState.globalLightShadowMapBlurred0 = lightServerState.shadowMappingFrameScript->GetReadWriteTexture("GlobalLightShadowFiltered0");
	lightServerState.globalLightShadowMapBlurred1 = lightServerState.shadowMappingFrameScript->GetReadWriteTexture("GlobalLightShadowFiltered1");
	lightServerState.spotlightShadowAtlas = lightServerState.shadowMappingFrameScript->GetColorTexture("SpotLightShadowAtlas");

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
		"LightClusterIndexBuffer",
		clusterState.ClusterSubdivsX * clusterState.ClusterSubdivsY * clusterState.ClusterSubdivsZ * sizeof(LightsClustered::LightTileList),
		1,
		false
	};
	clusterState.clusterIndexBuffer = CreateShaderRWBuffer(rwbInfo);

	ShaderRWBufferCreateInfo rwb3Info =
	{
		"LightsClusterAABBBuffer",
		64*64*16*sizeof(LightsClustered::ClusterAABB),
		1,
		false
	};
	clusterState.clusterAABBBuffer = CreateShaderRWBuffer(rwb3Info);

	ShaderRWTextureCreateInfo debugTexInfo =
	{
		"LightClsuterDebugTexture",
		TextureType::Texture2D,
		PixelFormat::R16G16B16A16F,
		CoreGraphicsImageLayout::ShaderRead,
		1.0f, 1.0f, 1.0f, 1, 1, false, true, false
	};
	clusterState.clusterDebugTexture = CreateShaderRWTexture(debugTexInfo);

	clusterState.classificationShader = ShaderServer::Instance()->GetShader("shd:lights_clustered.fxb");
	IndexT indexBufferSlot = ShaderGetResourceSlot(clusterState.classificationShader, "LightIndexList");
	IndexT clusterAABBSlot = ShaderGetResourceSlot(clusterState.classificationShader, "ClusterAABBs");
	IndexT clusterDebugSlot = ShaderGetResourceSlot(clusterState.classificationShader, "DebugOutput");
	clusterState.lightListSlot = ShaderGetResourceSlot(clusterState.classificationShader, "LightList");
	clusterState.uniformsSlot = ShaderGetResourceSlot(clusterState.classificationShader, "Uniforms");

	clusterState.aabbGenerateProgram = ShaderGetProgram(clusterState.classificationShader, ShaderServer::Instance()->FeatureStringToMask("AABBAndCull"));
	clusterState.debugProgram = ShaderGetProgram(clusterState.classificationShader, ShaderServer::Instance()->FeatureStringToMask("ClusterDebug"));
	clusterState.clusterResourceTable = ShaderCreateResourceTable(clusterState.classificationShader, NEBULA_BATCH_GROUP);

	// update resource table
	ResourceTableSetShaderRWBuffer(clusterState.clusterResourceTable, { clusterState.clusterIndexBuffer, indexBufferSlot, 0, false, false, -1, 0 });
	ResourceTableSetShaderRWBuffer(clusterState.clusterResourceTable, { clusterState.clusterAABBBuffer, clusterAABBSlot, 0, false, false, -1, 0 });
	ResourceTableSetShaderRWTexture(clusterState.clusterResourceTable, { clusterState.clusterDebugTexture, clusterDebugSlot, 0, CoreGraphics::SamplerId::Invalid() });
	ResourceTableSetConstantBuffer(clusterState.clusterResourceTable, { CoreGraphics::GetComputeConstantBuffer(MainThreadConstantBuffer), clusterState.lightListSlot, 0, false, false, sizeof(LightsClustered::LightList), 0 });
	ResourceTableSetConstantBuffer(clusterState.clusterResourceTable, { CoreGraphics::GetComputeConstantBuffer(MainThreadConstantBuffer), clusterState.uniformsSlot, 0, false, false, sizeof(LightsClustered::Uniforms), 0});
	ResourceTableCommitChanges(clusterState.clusterResourceTable);

	_CreateContext();
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::SetupGlobalLight(const Graphics::GraphicsEntityId id, const Math::float4& color, const float intensity, const Math::float4& ambient, const Math::float4& backlight, const float backlightFactor, const Math::vector& direction, bool castShadows)
{
	n_assert(id != Graphics::GraphicsEntityId::Invalid());
	n_assert(lightServerState.globalLightEntity == Graphics::GraphicsEntityId::Invalid());

	const Graphics::ContextEntityId cid = GetContextId(id);
	genericLightAllocator.Get<Type>(cid.id) = GlobalLightType;
	genericLightAllocator.Get<Color>(cid.id) = color;
	genericLightAllocator.Get<Intensity>(cid.id) = intensity;
	genericLightAllocator.Get<ShadowCaster>(cid.id) = castShadows;

	auto lid = globalLightAllocator.Alloc();

	Math::matrix44 mat = Math::matrix44::lookatrh(Math::point(0.0f), -direction, Math::vector::upvec());
	
	SetGlobalLightTransform(cid, mat);
	globalLightAllocator.Get<GlobalLight_Backlight>(lid) = backlight;
	globalLightAllocator.Get<GlobalLight_Ambient>(lid) = ambient;
	globalLightAllocator.Get<GlobalLight_BacklightOffset>(lid) = backlightFactor;

	if (castShadows)
	{
		// register observer as a light
		Visibility::ObserverContext::RegisterEntity(id);
		Visibility::ObserverContext::Setup(id, Visibility::VisibilityEntityType::Light);
	}

	genericLightAllocator.Get<TypedLightId>(cid.id) = lid;
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

	const Math::matrix44 scaleMatrix = Math::matrix44::scaling(range, range, range);
	auto pli = pointLightAllocator.Alloc();

	SetPointLightTransform(cid, Math::matrix44::multiply(scaleMatrix, transform));
	pointLightAllocator.Get<PointLight_DynamicOffsets>(pli).Resize(2);
	genericLightAllocator.Get<TypedLightId>(cid.id) = pli;

	// set initial state
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
	bool castShadows, 
	const CoreGraphics::TextureId projection)
{
	n_assert(id != Graphics::GraphicsEntityId::Invalid());
	const Graphics::ContextEntityId cid = GetContextId(id);
	genericLightAllocator.Get<Type>(cid.id) = SpotLightType;
	genericLightAllocator.Get<Color>(cid.id) = color;
	genericLightAllocator.Get<Intensity>(cid.id) = intensity;
	genericLightAllocator.Get<ShadowCaster>(cid.id) = castShadows;

	auto sli = spotLightAllocator.Alloc();
	spotLightAllocator.Get<SpotLight_DynamicOffsets>(sli).Resize(2);
	genericLightAllocator.Get<TypedLightId>(cid.id) = sli;

	// do this after we assign the typed light id
	SetSpotLightTransform(cid, transform);

	// set initial state
	spotLightAllocator.Get<SpotLight_DynamicOffsets>(sli)[0] = 0;
	spotLightAllocator.Get<SpotLight_DynamicOffsets>(sli)[1] = 0;
	spotLightAllocator.Get<SpotLight_ProjectionTexture>(sli) = projection;
	std::array<float, 2> angles = { innerConeAngle, outerConeAngle };
	spotLightAllocator.Get<SpotLight_ConeAngles>(sli) = angles;
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
LightContext::GetViewProjTransform(const Graphics::GraphicsEntityId id)
{
	const Graphics::ContextEntityId cid = GetContextId(id);
	LightType type = genericLightAllocator.Get<Type>(cid.id);
	Ids::Id32 lightId = genericLightAllocator.Get<TypedLightId>(cid.id);

	switch (type)
	{
	case GlobalLightType:
		return globalLightAllocator.Get<GlobalLight_ViewProjTransform>(lightId);
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
LightContext::SetSpotLightTransform(const Graphics::ContextEntityId id, const Math::matrix44& transform)
{
	// todo, update projection and invviewprojection!!!!
	auto lid = genericLightAllocator.Get<TypedLightId>(id.id);
	spotLightAllocator.Get<SpotLight_Transform>(lid) = transform;

	// compute the spot light's perspective projection matrix from
	// its transform matrix (spot direction is along -z, and goes
	// throught the rectangle formed by the x and y components
	// at the end of -z
	float widthAtFarZ = transform.getrow0().length() * 2.0f;
	float heightAtFarZ = transform.getrow1().length() * 2.0f;
	float nearZ = 0.001f; // put the near plane at 0.001cm 
	float farZ = transform.getrow2().length();
	n_assert(farZ > 0.0f);
	if (nearZ >= farZ)
	{
		nearZ = farZ / 2.0f;
	}
	float widthAtNearZ = (widthAtFarZ / farZ) * nearZ;
	float heightAtNearZ = (heightAtFarZ / farZ) * nearZ;
	spotLightAllocator.Get<SpotLight_Projection>(lid) = Math::matrix44::persprh(widthAtNearZ, heightAtNearZ, nearZ, farZ);
	spotLightAllocator.Get<SpotLight_InvViewProjection>(lid) = Math::matrix44::multiply(Math::matrix44::inverse(transform), spotLightAllocator.Get<SpotLight_Projection>(lid));
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
	globalLightAllocator.Get<GlobalLight_Direction>(lid) = -transform.get_zaxis();
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
LightContext::OnBeforeView(const Ptr<Graphics::View>& view, const IndexT frameIndex, const Timing::Time frameTime)
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
	Math::float4::storeu(globalLightAllocator.Get<GlobalLight_Direction>(globalLightId), params.GlobalLightDirWorldspace);
	Math::float4::storeu(globalLightAllocator.Get<GlobalLight_Backlight>(globalLightId), params.GlobalBackLightColor);
	Math::float4::storeu(globalLightAllocator.Get<GlobalLight_Ambient>(globalLightId), params.GlobalAmbientLightColor);
	Math::float4::storeu(globalLightAllocator.Get<GlobalLight_Direction>(globalLightId), params.GlobalLightDir);
	params.GlobalBackLightOffset = globalLightAllocator.Get<GlobalLight_BacklightOffset>(globalLightId);

	uint flags = 0;

	if (genericLightAllocator.Get<ShadowCaster>(cid.id))
	{
		lightServerState.csmUtil.SetCameraEntity(view->GetCamera());
		lightServerState.csmUtil.SetGlobalLight(lightServerState.globalLightEntity);
		lightServerState.csmUtil.SetShadowBox(Math::bbox(Math::point(0), Math::vector(500)));
		lightServerState.csmUtil.Compute(view->GetCamera(), lightServerState.globalLightEntity);
		SetGlobalLightViewProjTransform(cid, lightServerState.csmUtil.GetShadowView());

		// update camera
		CoreGraphics::TransformDevice* transDev = CoreGraphics::TransformDevice::Instance();

		// update csm matrices
		alignas(16) Shared::ShadowMatrixBlock block;
		IndexT i;
		for (i = 0; i < CSMUtil::NumCascades; i++)
		{
			Math::matrix44::store(lightServerState.csmUtil.GetCascadeViewProjection(i), block.ViewMatrixArray[i]);
		}
		transDev->ApplyCSMMatrices(block);

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
		params.GlobalLightShadowBuffer = CoreGraphics::ShaderRWTextureGetBindlessHandle(lightServerState.globalLightShadowMapBlurred1);
		Math::matrix44::store(lightServerState.csmUtil.GetShadowView(), params.CSMShadowMatrix);

		flags |= USE_SHADOW_BITFLAG;
	}
	params.GlobalLightFlags = flags;
	params.GlobalLightShadowBias = 0.000001f;																			 
	params.GlobalLightShadowIntensity = 1.0f;

	// go through and update local lights
	const Util::Array<LightType>& types    = genericLightAllocator.GetArray<Type>();
	const Util::Array<Math::float4>& color = genericLightAllocator.GetArray<Color>();
	const Util::Array<float>& intensity    = genericLightAllocator.GetArray<Intensity>();
	const Util::Array<bool>& castShadow    = genericLightAllocator.GetArray<ShadowCaster>();
	const Util::Array<Ids::Id32>& typeIds  = genericLightAllocator.GetArray<TypedLightId>();

	IndexT i;
	for (i = 0; i < types.Size(); i++)
	{
		switch (types[i])
		{

		case PointLightType:
		{
			auto trans = pointLightAllocator.Get<PointLight_Transform>(typeIds[i]);
			auto tex = pointLightAllocator.Get<PointLight_ProjectionTexture>(typeIds[i]);

			uint flags = 0;

			Math::float4 posAndRange = trans.get_position();
			posAndRange.w() = 1 / trans.get_zaxis().length();

			// update shadow stuff
			if (castShadow[i])
			{
				flags |= USE_SHADOW_BITFLAG;
			}

			// bind projection
			if (tex != TextureId::Invalid())
			{
				flags |= USE_PROJECTION_TEX_BITFLAG;
			}

			// allocate memory
			alignas(16) Lights::LocalLightBlock block;
			Math::float4::storeu(color[i] * intensity[i], block.LightColor);
			Math::float4::storeu(posAndRange, block.LightPosRange);
			Math::matrix44::storeu(trans, block.LightTransform);
			block.ProjectionTexture = tex != TextureId::Invalid() ? TextureGetBindlessHandle(tex) : 0;
			block.Flags = flags;

			// update constants
			uint offset = CoreGraphics::SetGraphicsConstants(MainThreadConstantBuffer, block);

			// update buffer
			pointLightAllocator.Get<PointLight_DynamicOffsets>(typeIds[i])[0] = offset;
		}
		break;

		case SpotLightType:
		{
			auto trans = spotLightAllocator.Get<SpotLight_Transform>(typeIds[i]);
			auto tex = spotLightAllocator.Get<SpotLight_ProjectionTexture>(typeIds[i]);
			auto invViewProj = spotLightAllocator.Get<SpotLight_InvViewProjection>(typeIds[i]);
			auto angles = spotLightAllocator.Get<SpotLight_ConeAngles>(typeIds[i]);

			uint flags = 0;

			Math::float4 posAndRange = trans.get_position();
			posAndRange.w() = 1 / trans.get_zaxis().length();
			Math::float4 forward = Math::float4::normalize(trans.get_zaxis());

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

			// update projection transform
			Math::matrix44 fromViewToLightProj = invViewProj;

			// allocate memory
			alignas(16) Lights::LocalLightBlock block;
			Math::float4::storeu(color[i] * intensity[i], block.LightColor);
			Math::float4::storeu(posAndRange, block.LightPosRange);
			Math::matrix44::storeu(trans, block.LightTransform);
			Math::matrix44::storeu(fromViewToLightProj, block.LightProjTransform);
			Math::float4::storeu(forward, block.LightForward);
			block.LightCutoff[0] = angles[0];
			block.LightCutoff[1] = angles[1];
			block.ProjectionTexture = tex != TextureId::Invalid() ? TextureGetBindlessHandle(tex) : 0;
			block.Flags = flags;

			// update constants
			uint offset = CoreGraphics::SetGraphicsConstants(MainThreadConstantBuffer, block);

			// update buffer
			spotLightAllocator.Get<SpotLight_DynamicOffsets>(typeIds[i])[0] = offset;
		}
		break;

		}

		// update cluster state
		clusterState.lights[i].type = types[i];
		
		switch (types[i])
		{
			case PointLightType:
			{
				auto trans = pointLightAllocator.Get<PointLight_Transform>(typeIds[i]);
				Math::float4 posAndRange = Math::matrix44::transform(trans.get_position(), viewTransform);
				posAndRange.w() = 1 / trans.get_zaxis().length();
				Math::float4::storeu(posAndRange, clusterState.lights[i].position);
				clusterState.lights[i].radius = posAndRange.w();
			}
			break;

			case SpotLightType:
			{
				auto trans = spotLightAllocator.Get<SpotLight_Transform>(typeIds[i]);
				Math::float4 posAndRange = Math::matrix44::transform(trans.get_position(), viewTransform);
				posAndRange.w() = 1 / trans.get_zaxis().length();
				Math::float4::storeu(posAndRange, clusterState.lights[i].position);
				Math::float4::store3u(trans.get_zaxis(), clusterState.lights[i].forward);
				clusterState.lights[i].radius = posAndRange.w();
			}
			break;
		}
	}

	// update lights buffer which will be used for the light culling pass
	//ShaderRWBufferUpdate(clusterState.clusterLightBuffer, clusterState.lights, sizeof(LightsClustered::Light) * types.Size());	

	// get dimensions of output texture
	TextureDimensions dims = ShaderRWTextureGetDimensions(clusterState.clusterDebugTexture);

	Graphics::GraphicsEntityId cam = Graphics::GraphicsServer::Instance()->GetCurrentView()->GetCamera();
	CoreGraphics::DisplayMode displayMode = CoreGraphics::WindowGetDisplayMode(CoreGraphics::DisplayDevice::Instance()->GetCurrentWindow());
	const Graphics::CameraSettings settings = Graphics::CameraContext::GetSettings(cam);

	uint clusterDimensionX = Math::n_ceil(displayMode.GetWidth() / clusterState.ClusterSubdivsX);
	uint clusterDimensionY = Math::n_ceil(displayMode.GetHeight() / clusterState.ClusterSubdivsY);

	float sd = 2.0f * settings.GetFocalLength().y() / clusterDimensionY;
	float logDist = 1.0f / Math::n_log(1.0f + sd);
	float logDepth = Math::n_log(settings.GetZFar() / settings.GetZNear());
	uint clusterDimensionZ = (unsigned int)Math::n_floor(logDepth * logDist);

	uint numLights = types.Size();
	LightsClustered::Uniforms uniforms;
	uniforms.NumInputLights = numLights;

	uniforms.ZPerspectiveCorrection = 1.0f + sd;
	uniforms.InvFramebufferDimensions[0] = 1.0f / displayMode.GetWidth();
	uniforms.InvFramebufferDimensions[1] = 1.0f / displayMode.GetHeight();
	uniforms.NumCells[0] = clusterDimensionX;
	uniforms.NumCells[1] = clusterDimensionY;
	uniforms.NumCells[2] = clusterDimensionZ;
	uniforms.BlockSize[0] = clusterState.ClusterSubdivsX;
	uniforms.BlockSize[1] = clusterState.ClusterSubdivsY;
	uniforms.Fov = settings.GetFov();

	clusterState.numThreadsThisFrame = Math::n_ceil((clusterDimensionX * clusterDimensionY * clusterDimensionZ) / 1024.0f);

	uint offset = SetComputeConstants(MainThreadConstantBuffer, uniforms);
	ResourceTableSetConstantBuffer(clusterState.clusterResourceTable, { GetComputeConstantBuffer(MainThreadConstantBuffer), clusterState.uniformsSlot, 0, false, false, sizeof(LightsClustered::Uniforms), (SizeT)offset });

	LightsClustered::LightList list;
	memcpy(list.Lights, clusterState.lights, sizeof(LightsClustered::Light)* types.Size());
	offset = SetComputeConstants(MainThreadConstantBuffer, list);
	ResourceTableSetConstantBuffer(clusterState.clusterResourceTable, { GetComputeConstantBuffer(MainThreadConstantBuffer), clusterState.lightListSlot, 0, false, false, (SizeT)sizeof(LightsClustered::Light) * types.Size(), (SizeT)offset });

}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::UpdateClustersAndCull()
{
	// update constants
	using namespace CoreGraphics;

	ResourceTableCommitChanges(clusterState.clusterResourceTable);

	// begin command buffer work
	CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLUE, "Cluster AABB Generation and Lights culling");

	// make sure to sync so we don't read from data that is being written...
	BarrierInsert(GraphicsQueueType,
		BarrierStage::ComputeShader,
		BarrierStage::ComputeShader,
		BarrierDomain::Global,
		nullptr,
		{
			BufferBarrier
			{
				clusterState.clusterAABBBuffer,
				BarrierAccess::ShaderRead,
				BarrierAccess::ShaderWrite,
				0, -1
			}
		},
		nullptr, "AABB begin barrier");

	SetShaderProgram(clusterState.aabbGenerateProgram);
	SetResourceTable(clusterState.clusterResourceTable, NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);

	// we subdivide the scene into 64x64x16 clusters, and the thread size is 64, so run a 1x64x16 job to update all aabbs
	Compute(clusterState.numThreadsThisFrame, 1, 1);

	// make sure to sync so we don't read from data that is being written...
	BarrierInsert(GraphicsQueueType,
		BarrierStage::ComputeShader,
		BarrierStage::ComputeShader,
		BarrierDomain::Global,
		nullptr,
		{
			BufferBarrier
			{
				clusterState.clusterAABBBuffer,
				BarrierAccess::ShaderWrite,
				BarrierAccess::ShaderRead,
				0, -1
			}
		},
		nullptr, "AABB finish barrier");

	CommandBufferEndMarker(GraphicsQueueType);

	CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLUE, "Cluster DEBUG");

	SetShaderProgram(clusterState.debugProgram);
	SetResourceTable(clusterState.clusterResourceTable, NEBULA_BATCH_GROUP, ComputePipeline, nullptr);
	
	// perform debug output
	TextureDimensions dims = ShaderRWTextureGetDimensions(clusterState.clusterDebugTexture);
	Compute(Math::n_divandroundup(dims.width, 256), dims.height, 1);

	CommandBufferEndMarker(GraphicsQueueType);
	/*

	// bind the cull shader
	CoreGraphics::SetShaderProgram(clusterState.lightCullProgram);

	// now, run through all previously created AABBs and cull the lights
	CoreGraphics::Compute(15, 15, 7); // we have 16 x 16 x 8 cells, so the indices naturally become 15, 15, 7
	*/
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
			/*
			{
				RenderTextureBarrier
				{
					lightServerState.globalLightShadowMap,
					ImageSubresourceInfo::ColorNoMip(4),
					CoreGraphicsImageLayout::ColorRenderTexture,
					CoreGraphicsImageLayout::ShaderRead,
					BarrierAccess::ColorAttachmentWrite,
					BarrierAccess::ShaderRead
				}
			},*/
			nullptr, nullptr,
			{
				RWTextureBarrier
				{
					lightServerState.globalLightShadowMapBlurred0,
					ImageSubresourceInfo::ColorNoMip(4),
					CoreGraphicsImageLayout::ShaderRead,
					CoreGraphicsImageLayout::General,
					BarrierAccess::ShaderRead,
					BarrierAccess::ShaderWrite
				},
				RWTextureBarrier
				{
					lightServerState.globalLightShadowMapBlurred1,
					ImageSubresourceInfo::ColorNoMip(4),
					CoreGraphicsImageLayout::ShaderRead,
					CoreGraphicsImageLayout::General,
					BarrierAccess::ShaderRead,
					BarrierAccess::ShaderWrite
				}
			},
			"CSM Blur Init");

		TextureDimensions dims = ShaderRWTextureGetDimensions(lightServerState.globalLightShadowMapBlurred0);
		SetShaderProgram(lightServerState.csmBlurXProgram);
		SetResourceTable(lightServerState.csmBlurXTable, NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
		Compute(Math::n_divandroundup(dims.width, 320), dims.height, 4);

		BarrierInsert(GraphicsQueueType,
			BarrierStage::ComputeShader,
			BarrierStage::ComputeShader,
			BarrierDomain::Global,
			nullptr, nullptr,
			{
				RWTextureBarrier
				{
					lightServerState.globalLightShadowMapBlurred0,
					ImageSubresourceInfo::ColorNoMip(4),
					CoreGraphicsImageLayout::General,
					CoreGraphicsImageLayout::ShaderRead,
					BarrierAccess::ShaderWrite,
					BarrierAccess::ShaderRead
				},
			},
			"CSM Blur X Finish");
		SetShaderProgram(lightServerState.csmBlurYProgram);
		SetResourceTable(lightServerState.csmBlurYTable, NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
		Compute(Math::n_divandroundup(dims.height, 320), dims.width, 4);

		BarrierInsert(GraphicsQueueType,
			BarrierStage::ComputeShader,
			BarrierStage::PixelShader,
			BarrierDomain::Global,
			nullptr, nullptr,
			{
				RWTextureBarrier
				{
					lightServerState.globalLightShadowMapBlurred1,
					ImageSubresourceInfo::ColorNoMip(4),
					CoreGraphicsImageLayout::General,
					CoreGraphicsImageLayout::ShaderRead,
					BarrierAccess::ShaderWrite,
					BarrierAccess::ShaderRead
				},
			},
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
	genericLightAllocator.Dealloc(id.id);
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::OnRenderDebug(uint32_t flags)
{
    auto const& types = genericLightAllocator.GetArray<Type>();    
    auto const& colours = genericLightAllocator.GetArray<Color>();
    auto const& ids = genericLightAllocator.GetArray<TypedLightId>();
    auto const& pointTrans = pointLightAllocator.GetArray<PointLight_Transform>();
    auto const& spotTrans = spotLightAllocator.GetArray< SpotLight_Transform>();
    auto const& spotProj = spotLightAllocator.GetArray< SpotLight_Projection>();
    auto const& spotInvProj = spotLightAllocator.GetArray< SpotLight_InvViewProjection>();
    for (int i = 0, n = types.Size(); i < n; ++i)
    {
        switch(types[i])
        {
        case PointLightType:
        {
            Math::matrix44 const& trans = pointTrans[ids[i]];
            Math::float4 col = colours[i];            
            Im3d::Im3dContext::DrawSphere(trans, col, Im3d::CheckDepth|Im3d::Wireframe);
            //FIXME define debug flags somewhere
            if (flags & Im3d::Solid)
            {
                col.w() = 0.5f;
                Im3d::Im3dContext::DrawSphere(trans, col, Im3d::CheckDepth | Im3d::Solid);
            }            
        }
        break;
        case SpotLightType:
        {
            //FIXME            
            Math::matrix44 unscaledTransform = spotTrans[ids[i]];
            unscaledTransform.set_xaxis(Math::float4::normalize(unscaledTransform.get_xaxis()));
            unscaledTransform.set_yaxis(Math::float4::normalize(unscaledTransform.get_yaxis()));
            unscaledTransform.set_zaxis(Math::float4::normalize(unscaledTransform.get_zaxis()));
            Math::matrix44 frustum = Math::matrix44::multiply(spotInvProj[ids[i]], unscaledTransform);
            Math::float4 col = colours[i];
            Im3d::Im3dContext::DrawBox(frustum, col);
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
