//------------------------------------------------------------------------------
// lightcontext.cc
// (C) 2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "frame/frameplugin.h"
#include "lightcontext.h"
#include "graphics/graphicsserver.h"
#include "graphics/view.h"
#include "graphics/cameracontext.h"
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

#include "lights_cluster.h"
#include "lights.h"
#include "combine.h"

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
	Graphics::GraphicsEntityId globalLightEntity = Graphics::GraphicsEntityId::Invalid();

	Util::Array<Graphics::GraphicsEntityId> spotLightEntities;
	Util::RingBuffer<Graphics::GraphicsEntityId> shadowcastingLocalLights;

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

	CSMUtil csmUtil;
} lightServerState;

struct
{
	CoreGraphics::ShaderId classificationShader;
	CoreGraphics::ShaderProgramId cullProgram;
	CoreGraphics::ShaderProgramId debugProgram;
	CoreGraphics::ShaderProgramId renderProgram;
	CoreGraphics::ShaderRWBufferId clusterLightIndexLists;
	Util::FixedArray<CoreGraphics::ShaderRWBufferId> stagingClusterLightsList;
	CoreGraphics::ShaderRWBufferId clusterLightsList;
	CoreGraphics::ConstantBufferId clusterPointLights;
	CoreGraphics::ConstantBufferId clusterSpotLights;

	IndexT clusterUniformsSlot;
	IndexT lightingUniformsSlot;
	IndexT lightListSlot;
	IndexT lightShadingTextureSlot;
	IndexT lightShadingDebugTextureSlot;
	Util::FixedArray<CoreGraphics::ResourceTableId> resourceTables;

	uint numThreadsThisFrame;

#ifdef CLUSTERED_LIGHTING_DEBUG
	CoreGraphics::TextureId clusterDebugTexture;
#endif
	CoreGraphics::TextureId lightingTexture;

	// these are used to update the light clustering
	LightsCluster::PointLight pointLights[1024];
	LightsCluster::SpotLight spotLights[1024];
	LightsCluster::SpotLightProjectionExtension spotLightProjection[256];
	LightsCluster::SpotLightShadowExtension spotLightShadow[16];

} clusterState;

struct
{
	CoreGraphics::ShaderId combineShader;
	CoreGraphics::ShaderProgramId combineProgram;
	Util::FixedArray<CoreGraphics::ResourceTableId> resourceTables;
	IndexT combineUniforms;
	IndexT fogTextureSlot;
	IndexT reflectionsTextureSlot;
	IndexT lightingTextureSlot;
} combineState;

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

#if NEBULA_ENABLE_MT_DRAW
	__bundle.OnWorkFinished = LightContext::RunFrameScriptJobs;
#endif

	__bundle.StageBits = &LightContext::__state.currentStage;
#ifndef PUBLIC_BUILD
	__bundle.OnRenderDebug = LightContext::OnRenderDebug;
#endif
	Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

	// called from main script
	Frame::AddCallback("LightContext - Update Shadowmaps", [](IndexT frame) // trigger update
		{
			// run the script
			N_SCOPE(ShadowMapExecute, Render);
			lightServerState.shadowMappingFrameScript->Run(frame);
#ifndef PUBLIC_BUILD
			//Debug::FrameScriptInspector::Run(lightServerState.shadowMappingFrameScript);
#endif
		});

	// register shadow mapping algorithms
	Frame::AddCallback("LightContext - Spotlight Shadows", [](IndexT frame) // graphics
		{
			IndexT i;
			for (i = 0; i < lightServerState.shadowcastingLocalLights.Size(); i++)
			{
				// draw it!
				Frame::FrameSubpassBatch::DrawBatch(lightServerState.spotlightsBatchCode, lightServerState.shadowcastingLocalLights[i], 1, i);
			}
		});
	Frame::AddCallback("LightContext - Spotlight Blur", [](IndexT frame) // compute
		{
		});

	Frame::AddCallback("LightContext - Sun Shadows", [](IndexT frame) // graphics
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
	Frame::AddCallback("LightContext - Sun Blur", [](IndexT frame) // compute
		{
			LightContext::BlurGlobalShadowMap();
		});

	Frame::AddCallback("LightContext - Cull and Classify", [](IndexT frame)
		{
			LightContext::CullAndClassify();
		});

	Frame::AddCallback("LightContext - Deferred Cluster", [](IndexT frame)
		{
			LightContext::ComputeLighting();
		});

	Frame::AddCallback("LightContext - Combine", [](IndexT frame)
		{
			LightContext::CombineLighting();
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
	IndexT blurXInputSlot = ShaderGetResourceSlot(lightServerState.csmBlurShader, "InputImageX");
	IndexT blurYInputSlot = ShaderGetResourceSlot(lightServerState.csmBlurShader, "InputImageY");
	IndexT blurXOutputSlot = ShaderGetResourceSlot(lightServerState.csmBlurShader, "BlurImageX");
	IndexT blurYOutputSlot = ShaderGetResourceSlot(lightServerState.csmBlurShader, "BlurImageY");
	lightServerState.csmBlurXTable = ShaderCreateResourceTable(lightServerState.csmBlurShader, NEBULA_BATCH_GROUP);
	lightServerState.csmBlurYTable = ShaderCreateResourceTable(lightServerState.csmBlurShader, NEBULA_BATCH_GROUP);
	ResourceTableSetTexture(lightServerState.csmBlurXTable, { lightServerState.globalLightShadowMap, blurXInputSlot, 0, CoreGraphics::SamplerId::Invalid(), false }); // ping
	ResourceTableSetRWTexture(lightServerState.csmBlurXTable, { lightServerState.globalLightShadowMapBlurred0, blurXOutputSlot, 0, CoreGraphics::SamplerId::Invalid() }); // pong
	ResourceTableSetTexture(lightServerState.csmBlurYTable, { lightServerState.globalLightShadowMapBlurred0, blurYInputSlot, 0, CoreGraphics::SamplerId::Invalid() }); // ping
	ResourceTableSetRWTexture(lightServerState.csmBlurYTable, { lightServerState.globalLightShadowMapBlurred1, blurYOutputSlot, 0, CoreGraphics::SamplerId::Invalid() }); // pong
	ResourceTableCommitChanges(lightServerState.csmBlurXTable);
	ResourceTableCommitChanges(lightServerState.csmBlurYTable);

	clusterState.classificationShader = ShaderServer::Instance()->GetShader("shd:lights_cluster.fxb");
	IndexT lightIndexListsSlot = ShaderGetResourceSlot(clusterState.classificationShader, "LightIndexLists");
	IndexT lightsListSlot = ShaderGetResourceSlot(clusterState.classificationShader, "LightLists");
	IndexT clusterAABBSlot = ShaderGetResourceSlot(clusterState.classificationShader, "ClusterAABBs");
#ifdef CLUSTERED_LIGHTING_DEBUG
	clusterState.lightShadingDebugTextureSlot = ShaderGetResourceSlot(clusterState.classificationShader, "DebugOutput");
#endif
	clusterState.lightShadingTextureSlot = ShaderGetResourceSlot(clusterState.classificationShader, "Lighting");
	clusterState.clusterUniformsSlot = ShaderGetResourceSlot(clusterState.classificationShader, "ClusterUniforms");
	clusterState.lightingUniformsSlot = ShaderGetResourceSlot(clusterState.classificationShader, "LightUniforms");

	clusterState.cullProgram = ShaderGetProgram(clusterState.classificationShader, ShaderServer::Instance()->FeatureStringToMask("Cull"));
	clusterState.renderProgram = ShaderGetProgram(clusterState.classificationShader, ShaderServer::Instance()->FeatureStringToMask("Render"));
#ifdef CLUSTERED_LIGHTING_DEBUG
	clusterState.debugProgram = ShaderGetProgram(clusterState.classificationShader, ShaderServer::Instance()->FeatureStringToMask("Debug"));
#endif

	ShaderRWBufferCreateInfo rwbInfo =
	{
		"LightIndexListsBuffer",
		sizeof(LightsCluster::LightIndexLists),
		BufferUpdateMode::DeviceWriteable,
		false
	};
	clusterState.clusterLightIndexLists = CreateShaderRWBuffer(rwbInfo);

	rwbInfo.name = "LightLists";
	rwbInfo.size = sizeof(LightsCluster::LightLists);
	clusterState.clusterLightsList = CreateShaderRWBuffer(rwbInfo);

	rwbInfo.mode = BufferUpdateMode::HostWriteable;
	clusterState.resourceTables.Resize(CoreGraphics::GetNumBufferedFrames());
	clusterState.stagingClusterLightsList.Resize(CoreGraphics::GetNumBufferedFrames());

	for (IndexT i = 0; i < clusterState.resourceTables.Size(); i++)
	{
		clusterState.resourceTables[i] = ShaderCreateResourceTable(clusterState.classificationShader, NEBULA_BATCH_GROUP);
		clusterState.stagingClusterLightsList[i] = CreateShaderRWBuffer(rwbInfo);

		// update resource table
		ResourceTableSetRWBuffer(clusterState.resourceTables[i], { Clustering::ClusterContext::GetClusterBuffer(), clusterAABBSlot, 0, false, false, NEBULA_WHOLE_BUFFER_SIZE, 0 });
		ResourceTableSetRWBuffer(clusterState.resourceTables[i], { clusterState.clusterLightIndexLists, lightIndexListsSlot, 0, false, false, NEBULA_WHOLE_BUFFER_SIZE, 0 });
		ResourceTableSetRWBuffer(clusterState.resourceTables[i], { clusterState.clusterLightsList, lightsListSlot, 0, false, false, NEBULA_WHOLE_BUFFER_SIZE, 0 });
		ResourceTableSetConstantBuffer(clusterState.resourceTables[i], { CoreGraphics::GetComputeConstantBuffer(MainThreadConstantBuffer), clusterState.clusterUniformsSlot, 0, false, false, sizeof(LightsCluster::ClusterUniforms), 0 });
		ResourceTableSetConstantBuffer(clusterState.resourceTables[i], { CoreGraphics::GetComputeConstantBuffer(MainThreadConstantBuffer), clusterState.lightingUniformsSlot, 0, false, false, sizeof(LightsCluster::LightUniforms), 0 });
		ResourceTableSetConstantBuffer(clusterState.resourceTables[i], { Clustering::ClusterContext::GetConstantBuffer(), clusterState.clusterUniformsSlot, 0, false, false, sizeof(ClusterGenerate::ClusterUniforms), 0 });
		ResourceTableCommitChanges(clusterState.resourceTables[i]);
	}

	// setup combine
	combineState.combineShader = ShaderServer::Instance()->GetShader("shd:combine.fxb");
	combineState.combineProgram = ShaderGetProgram(combineState.combineShader, ShaderServer::Instance()->FeatureStringToMask("Combine"));
	combineState.resourceTables.Resize(CoreGraphics::GetNumBufferedFrames());

	combineState.fogTextureSlot = ShaderGetResourceSlot(combineState.combineShader, "Fog");
	combineState.reflectionsTextureSlot = ShaderGetResourceSlot(combineState.combineShader, "Reflections");
	combineState.lightingTextureSlot = ShaderGetResourceSlot(combineState.combineShader, "Lighting");
	combineState.combineUniforms = ShaderGetResourceSlot(combineState.combineShader, "CombineUniforms");

	for (IndexT i = 0; i < clusterState.resourceTables.Size(); i++)
	{
		combineState.resourceTables[i] = ShaderCreateResourceTable(combineState.combineShader, NEBULA_BATCH_GROUP);
		ResourceTableSetConstantBuffer(combineState.resourceTables[i], { CoreGraphics::GetComputeConstantBuffer(MainThreadConstantBuffer), combineState.combineUniforms, 0, false, false, sizeof(Combine::CombineUniforms), 0 });
		ResourceTableCommitChanges(combineState.resourceTables[i]);
	}

	// allow 16 shadow casting local lights
	lightServerState.shadowcastingLocalLights.SetCapacity(16);
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::Discard()
{
	lightServerState.shadowMappingFrameScript->Discard();
	Graphics::GraphicsServer::Instance()->UnregisterGraphicsContext(&__bundle);
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::SetupGlobalLight(const Graphics::GraphicsEntityId id, const Math::vec3& color, const float intensity, const Math::vec3& ambient, const Math::vec3& backlight, const float backlightFactor, const Math::vector& direction, bool castShadows)
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

	Math::mat4 mat = lookatrh(Math::point(0.0f), Math::point(0.0f) - direction, Math::vector::upvec());
	
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

	// allocate shadow buffer slice if we cast shadows
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
		angles[0] = outerConeAngle - Math::n_deg2rad(0.1f);

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
			lightServerState.csmUtil.GetCascadeViewProjection(i).store(lightServerState.shadowMatrixUniforms.CSMViewMatrix[i]);
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
				Math::mat4 projection = spotLightAllocator.Get<SpotLight_ProjectionTransform>(typeIds[i]);
				Math::mat4 view = spotLightAllocator.Get<SpotLight_Transform>(typeIds[i]);
				Math::mat4 viewProjection = inverse(view) * projection;
				Graphics::GraphicsEntityId observer = spotLightAllocator.Get<SpotLight_Observer>(typeIds[i]);
				Graphics::ContextEntityId ctxId = shadowCasterSliceMap[observer];
				shadowCasterAllocator.Get<ShadowCaster_Transform>(ctxId.id) = viewProjection;

				lightServerState.shadowcastingLocalLights.Add(observer);
				viewProjection.store(lightServerState.shadowMatrixUniforms.LightViewMatrix[shadowCasterCount++]);
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
const CoreGraphics::TextureId LightContext::GetLightingTexture()
{
	return clusterState.lightingTexture;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ShaderRWBufferId 
LightContext::GetLightIndexBuffer()
{
	return clusterState.clusterLightIndexLists;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ShaderRWBufferId 
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
LightContext::SetGlobalLightTransform(const Graphics::ContextEntityId id, const Math::mat4& transform)
{
	auto lid = genericLightAllocator.Get<TypedLightId>(id.id);
	globalLightAllocator.Get<GlobalLight_Direction>(lid) = -normalize(xyz(transform.z_axis));
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
	Math::mat4 viewTransform = Graphics::CameraContext::GetTransform(view->GetCamera());
	Math::mat4 invViewTransform = inverse(viewTransform);

	// update constant buffer
	Ids::Id32 globalLightId = genericLightAllocator.Get<TypedLightId>(cid.id);
	Shared::PerTickParams& params = ShaderServer::Instance()->GetTickParams();
	(genericLightAllocator.Get<Color>(cid.id) * genericLightAllocator.Get<Intensity>(cid.id)).store(params.GlobalLightColor);
	globalLightAllocator.Get<GlobalLight_Direction>(globalLightId).store(params.GlobalLightDirWorldspace);
	globalLightAllocator.Get<GlobalLight_Backlight>(globalLightId).store(params.GlobalBackLightColor);
	globalLightAllocator.Get<GlobalLight_Ambient>(globalLightId).store(params.GlobalAmbientLightColor);
	Math::vec4 viewSpaceLightDir = viewTransform * Math::vec4(globalLightAllocator.Get<GlobalLight_Direction>(globalLightId), 0.0f);
	normalize(viewSpaceLightDir).store3(params.GlobalLightDir);
	params.GlobalBackLightOffset = globalLightAllocator.Get<GlobalLight_BacklightOffset>(globalLightId);

	// apply shadow uniforms
	CoreGraphics::TransformDevice::Instance()->ApplyShadowSettings(lightServerState.shadowMatrixUniforms);

	uint flags = 0;

	if (genericLightAllocator.Get<ShadowCaster>(cid.id))
	{

#if __DX12__
		Math::mat4 textureScale = Math::scaling(0.5f, -0.5f, 1.0f);
#elif __VULKAN__
		Math::mat4 textureScale = Math::scaling(0.5f, 0.5f, 1.0f);
#endif
		Math::mat4 textureTranslation = Math::translation(0.5f, 0.5f, 0);
		const Math::mat4* transforms = lightServerState.csmUtil.GetCascadeProjectionTransforms();
		Math::vec4 cascadeScales[CSMUtil::NumCascades];
		Math::vec4 cascadeOffsets[CSMUtil::NumCascades];

		for (IndexT splitIndex = 0; splitIndex < CSMUtil::NumCascades; ++splitIndex)
		{
			Math::mat4 shadowTexture = transforms[splitIndex] * (textureScale * textureTranslation);
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

		memcpy(params.CascadeOffset, cascadeOffsets, sizeof(Math::vec4) * CSMUtil::NumCascades);
		memcpy(params.CascadeScale, cascadeScales, sizeof(Math::vec4) * CSMUtil::NumCascades);
		memcpy(params.CascadeDistances, lightServerState.csmUtil.GetCascadeDistances(), sizeof(float) * CSMUtil::NumCascades);
		params.MinBorderPadding = 1.0f / 1024.0f;
		params.MaxBorderPadding = (1024.0f - 1.0f) / 1024.0f;
		params.ShadowPartitionSize = 1.0f;
		params.GlobalLightShadowBuffer = CoreGraphics::TextureGetBindlessHandle(lightServerState.globalLightShadowMapBlurred1);
		(invViewTransform * lightServerState.csmUtil.GetShadowView()).store(params.CSMShadowMatrix);

		flags |= USE_SHADOW_BITFLAG;
	}
	params.GlobalLightFlags = flags;
	params.GlobalLightShadowBias = 0.000001f;																			 
	params.GlobalLightShadowIntensity = 1.0f;

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
				auto& spotLight = clusterState.spotLights[numSpotLights];
				Math::mat4 shadowProj;
				if (tex != TextureId::Invalid() || castShadow[i])
				{
					Graphics::GraphicsEntityId observer = spotLightAllocator.Get<SpotLight_Observer>(typeIds[i]);
					Graphics::ContextEntityId ctxId = shadowCasterSliceMap[observer];
					shadowProj = invViewTransform * shadowCasterAllocator.Get<ShadowCaster_Transform>(ctxId.id);
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
					
					shadowProj.store(shadow.projection);
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
					shadowProj.store(projection.projection);
					projection.projectionTexture = CoreGraphics::TextureGetBindlessHandle(tex);
					numSpotLightsProjection++;
				}

				Math::mat4 viewSpace = trans * viewTransform;
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
				spotLight.angleSinCos[0] = Math::n_sin(angles[1]);
				spotLight.angleSinCos[1] = Math::n_cos(angles[1]);
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
		LightsCluster::LightLists lightList;
		Memory::CopyElements(clusterState.pointLights, lightList.PointLights, numPointLights);
		Memory::CopyElements(clusterState.spotLights, lightList.SpotLights, numSpotLights);
		Memory::CopyElements(clusterState.spotLightProjection, lightList.SpotLightProjection, numSpotLightsProjection);
		Memory::CopyElements(clusterState.spotLightShadow, lightList.SpotLightShadow, numSpotLightShadows);
		CoreGraphics::ShaderRWBufferUpdate(clusterState.stagingClusterLightsList[bufferIndex], &lightList, sizeof(LightsCluster::LightLists));
		CoreGraphics::ShaderRWBufferFlush(clusterState.stagingClusterLightsList[bufferIndex]);
	}

	// a little ugly, but since the view can change the script, this has to adopt
	const CoreGraphics::TextureId lightingTex = view->GetFrameScript()->GetTexture("LightBuffer");
	clusterState.lightingTexture = lightingTex;
	ResourceTableSetRWTexture(clusterState.resourceTables[bufferIndex], { lightingTex, clusterState.lightShadingTextureSlot, 0, CoreGraphics::SamplerId::Invalid() });

#ifdef CLUSTERED_LIGHTING_DEBUG
	const CoreGraphics::TextureId debugTex = view->GetFrameScript()->GetTexture("LightDebugBuffer");
	clusterState.clusterDebugTexture = debugTex;
	ResourceTableSetRWTexture(clusterState.resourceTables[bufferIndex], { clusterState.clusterDebugTexture, clusterState.lightShadingDebugTextureSlot, 0, CoreGraphics::SamplerId::Invalid() });
#endif

	const CoreGraphics::TextureId ssaoTex = view->GetFrameScript()->GetTexture("SSAOBuffer");
	LightsCluster::LightUniforms consts;
	consts.NumSpotLights = numSpotLights;
	consts.NumPointLights = numPointLights;
	consts.NumClusters = Clustering::ClusterContext::GetNumClusters();
	consts.SSAOBuffer = CoreGraphics::TextureGetBindlessHandle(ssaoTex);
	IndexT offset = SetComputeConstants(MainThreadConstantBuffer, consts);
	ResourceTableSetConstantBuffer(clusterState.resourceTables[bufferIndex], { GetComputeConstantBuffer(MainThreadConstantBuffer), clusterState.lightingUniformsSlot, 0, false, false, sizeof(LightsCluster::LightUniforms), (SizeT)offset });
	ResourceTableCommitChanges(clusterState.resourceTables[bufferIndex]);

	const CoreGraphics::TextureId fogTex = view->GetFrameScript()->GetTexture("VolumetricFogBuffer0");
	const CoreGraphics::TextureId reflectionTex = view->GetFrameScript()->GetTexture("ReflectionBuffer");

	TextureDimensions dims = TextureGetDimensions(lightingTex);
	Combine::CombineUniforms combineConsts;
	combineConsts.LowresResolution[0] = 1.0f / dims.width;
	combineConsts.LowresResolution[1] = 1.0f / dims.height;
	offset = SetComputeConstants(MainThreadConstantBuffer, combineConsts);
	ResourceTableSetConstantBuffer(combineState.resourceTables[bufferIndex], { GetComputeConstantBuffer(MainThreadConstantBuffer), combineState.combineUniforms, 0, false, false, sizeof(Combine::CombineUniforms), (SizeT)offset });
	ResourceTableSetRWTexture(combineState.resourceTables[bufferIndex], { lightingTex, combineState.lightingTextureSlot, 0, CoreGraphics::SamplerId::Invalid() });
	ResourceTableSetTexture(combineState.resourceTables[bufferIndex], { fogTex, combineState.fogTextureSlot, 0, CoreGraphics::SamplerId::Invalid() });
	ResourceTableSetTexture(combineState.resourceTables[bufferIndex], { reflectionTex, combineState.reflectionsTextureSlot, 0, CoreGraphics::SamplerId::Invalid() });
	ResourceTableCommitChanges(combineState.resourceTables[bufferIndex]);
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

	const IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();

	// copy data from staging buffer to shader buffer
	BarrierInsert(ComputeQueueType,
		BarrierStage::ComputeShader,
		BarrierStage::Transfer,
		BarrierDomain::Global,
		nullptr,
		{
			BufferBarrier
			{
				clusterState.clusterLightsList,
				BarrierAccess::ShaderRead,
				BarrierAccess::TransferWrite,
				0, NEBULA_WHOLE_BUFFER_SIZE
			},
		}, "Lights data upload");
	Copy(ComputeQueueType, clusterState.stagingClusterLightsList[bufferIndex], 0, clusterState.clusterLightsList, 0, sizeof(LightsCluster::LightLists));
	BarrierInsert(ComputeQueueType,
		BarrierStage::Transfer,
		BarrierStage::ComputeShader,
		BarrierDomain::Global,
		nullptr,
		{
			BufferBarrier
			{
				clusterState.clusterLightsList,
				BarrierAccess::TransferWrite,
				BarrierAccess::ShaderRead,
				0, NEBULA_WHOLE_BUFFER_SIZE
			},
		}, "Lights data upload");

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
				0, NEBULA_WHOLE_BUFFER_SIZE
			},
		}, "Light cluster culling begin");

	SetShaderProgram(clusterState.cullProgram, ComputeQueueType);
	SetResourceTable(clusterState.resourceTables[bufferIndex], NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr, ComputeQueueType);

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
				0, NEBULA_WHOLE_BUFFER_SIZE
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
	TextureDimensions dims = TextureGetDimensions(clusterState.lightingTexture);
	const IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();

#ifdef CLUSTERED_LIGHTING_DEBUG
	CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLUE, "Cluster DEBUG");

	SetShaderProgram(clusterState.debugProgram, GraphicsQueueType);
	SetResourceTable(clusterState.resourceTables[bufferIndex], NEBULA_BATCH_GROUP, ComputePipeline, nullptr, GraphicsQueueType);

	// perform debug output
	Compute(Math::n_divandroundup(dims.width, 64), dims.height, 1, GraphicsQueueType);

	CommandBufferEndMarker(GraphicsQueueType);
#endif

	CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLUE, "Clustered Shading");

	SetShaderProgram(clusterState.renderProgram, GraphicsQueueType);
	SetResourceTable(clusterState.resourceTables[bufferIndex], NEBULA_BATCH_GROUP, ComputePipeline, nullptr, GraphicsQueueType);

	// perform debug output
	dims = TextureGetDimensions(clusterState.lightingTexture);
	Compute(Math::n_divandroundup(dims.width, 64), dims.height, 1, GraphicsQueueType);

	CommandBufferEndMarker(GraphicsQueueType);
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::CombineLighting()
{
	using namespace CoreGraphics;
	const IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();
	CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLUE, "Combine Lighting");

	SetShaderProgram(combineState.combineProgram, GraphicsQueueType);
	SetResourceTable(combineState.resourceTables[bufferIndex], NEBULA_BATCH_GROUP, ComputePipeline, nullptr, GraphicsQueueType);

	// perform debug output
	TextureDimensions dims = TextureGetDimensions(clusterState.lightingTexture);
	Compute(Math::n_divandroundup(dims.width, 64), dims.height, 1, GraphicsQueueType);

	CommandBufferEndMarker(GraphicsQueueType);
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
            Math::mat4 const& trans = pointTrans[ids[i]];
            Math::vec4 col = Math::vec4(colors[i], 1.0f);
			CoreGraphics::RenderShape shape;
			shape.SetupSimpleShape(RenderShape::Sphere, RenderShape::RenderFlag(RenderShape::CheckDepth|RenderShape::Wireframe), trans, col);
			shapeRenderer->AddShape(shape);
            if (flags & Im3d::Solid)
            {
                col.w = 0.5f;
				shape.SetupSimpleShape(RenderShape::Sphere, RenderShape::RenderFlag(RenderShape::CheckDepth), trans, col);
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
			Math::mat4 frustum = proj * unscaledTransform;
			
            Math::vec4 col = Math::vec4(colors[i], 1.0f);

			RenderShape shape;
			shape.SetupSimpleShape(
				RenderShape::Box, 
				RenderShape::RenderFlag(RenderShape::CheckDepth | RenderShape::Wireframe), 
				frustum, 
				col);
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
