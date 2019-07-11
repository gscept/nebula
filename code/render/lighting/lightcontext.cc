//------------------------------------------------------------------------------
// lightcontext.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "lightcontext.h"
#include "graphics/graphicsserver.h"
#include "graphics/view.h"
#include "graphics/cameracontext.h"
#include "renderutil/drawfullscreenquad.h"
#include "csmutil.h"
#include "math/polar.h"
#include "frame/framebatchtype.h"
#include "resources/resourcemanager.h"
#ifndef PUBLIC_BUILD
#include "dynui/im3d/im3dcontext.h"
#endif

#include "lights_cluster_classification.h"
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
} lightServerState;

struct
{
	CoreGraphics::ShaderId classificationShader;
	CoreGraphics::ShaderProgramId classificationProgram;
	CoreGraphics::ShaderRWBufferId clusterIndexBuffer;
	CoreGraphics::ShaderRWBufferId clusterLightBuffer;
	CoreGraphics::ResourceTableId clusterResourceTable;

	enum DepthDivisionMode
	{
		Linear,
		Exponential
	};

	DepthDivisionMode divMode;

	static const SizeT ClusterSubdivsX = 16;
	static const SizeT ClusterSubdivsY = 16;
	static const SizeT ClusterSubdivsZ = 8;
	static const SizeT IndicesPerCluster = 32;

	// these are used to update the light clustering
	alignas(16) LightsClusterClassification::Light lights[2048];

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

	using namespace CoreGraphics;

	lightServerState.lightShader = ShaderServer::Instance()->GetShader("shd:lights.fxb");
	lightServerState.globalLight = ShaderGetProgram(lightServerState.lightShader, CoreGraphics::ShaderFeatureFromString("Global"));
	lightServerState.globalLightShadow = ShaderGetProgram(lightServerState.lightShader, CoreGraphics::ShaderFeatureFromString("Global|Alt0"));
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
		clusterState.ClusterSubdivsX * clusterState.ClusterSubdivsY * clusterState.ClusterSubdivsZ * clusterState.IndicesPerCluster * sizeof(IndexT),
		1,
		false
	};
	clusterState.clusterIndexBuffer = CreateShaderRWBuffer(rwbInfo);

	ShaderRWBufferCreateInfo rwb2Info = 
	{
		"LightClusterInputBuffer",
		2048 * sizeof(LightsClusterClassification::Light),
		1,
		false
	};
	clusterState.clusterLightBuffer = CreateShaderRWBuffer(rwb2Info);

	clusterState.classificationShader = ShaderServer::Instance()->GetShader("shd:lights_cluster_classification.fxb");
	clusterState.classificationProgram = ShaderGetProgram(clusterState.classificationShader, 0);
	clusterState.clusterResourceTable = ShaderCreateResourceTable(clusterState.classificationShader, NEBULA_BATCH_GROUP);

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

	SetGlobalLightDirection(cid, direction);
	globalLightAllocator.Get<GlobalLightBacklight>(lid) = backlight;
	globalLightAllocator.Get<GlobalLightAmbient>(lid) = ambient;
	globalLightAllocator.Get<GlobalLightBacklightOffset>(lid) = backlightFactor;

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
	pointLightAllocator.Get<PointLightDynamicOffsets>(pli).Resize(2);
	genericLightAllocator.Get<TypedLightId>(cid.id) = pli;

	// set initial state
	pointLightAllocator.Get<PointLightDynamicOffsets>(pli)[0] = 0;
	pointLightAllocator.Get<PointLightDynamicOffsets>(pli)[1] = 0;
	pointLightAllocator.Get<PointLightProjectionTexture>(pli) = projection;

	// allocate shadow buffer slice if we cast shadows
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::SetupSpotLight(const Graphics::GraphicsEntityId id, 
	const Math::float4& color, 
	const float intensity, 
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
	spotLightAllocator.Get<SpotLightDynamicOffsets>(sli).Resize(2);
	genericLightAllocator.Get<TypedLightId>(cid.id) = sli;

	// do this after we assign the typed light id
	SetSpotLightTransform(cid, transform);

	// set initial state
	spotLightAllocator.Get<SpotLightDynamicOffsets>(sli)[0] = 0;
	spotLightAllocator.Get<SpotLightDynamicOffsets>(sli)[1] = 0;
	spotLightAllocator.Get<SpotLightProjectionTexture>(sli) = projection == CoreGraphics::TextureId::Invalid() ? lightServerState.spotLightDefaultProjection : projection;
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
		return globalLightAllocator.Get<GlobalLightTransform>(lightId);
		break;
	case SpotLightType:
		return spotLightAllocator.Get<SpotLightTransform>(lightId);
		break;
	case PointLightType:
		return pointLightAllocator.Get<PointLightTransform>(lightId);
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
        SetGlobalLightDirection(cid, trans.get_yaxis());        
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
void
LightContext::SetSpotLightTransform(const Graphics::ContextEntityId id, const Math::matrix44& transform)
{
	// todo, update projection and invviewprojection!!!!
	auto lid = genericLightAllocator.Get<TypedLightId>(id.id);
	spotLightAllocator.Get<SpotLightTransform>(lid) = transform;

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
	spotLightAllocator.Get<SpotLightProjection>(lid) = Math::matrix44::persprh(widthAtNearZ, heightAtNearZ, nearZ, farZ);
	spotLightAllocator.Get<SpotLightInvViewProjection>(lid) = Math::matrix44::multiply(Math::matrix44::inverse(transform), spotLightAllocator.Get<SpotLightProjection>(lid));
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::SetPointLightTransform(const Graphics::ContextEntityId id, const Math::matrix44& transform)
{
	auto lid = genericLightAllocator.Get<TypedLightId>(id.id);
	pointLightAllocator.Get<PointLightTransform>(lid) = transform;
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::SetGlobalLightDirection(const Graphics::ContextEntityId id, const Math::vector& direction)
{
	auto lid = genericLightAllocator.Get<TypedLightId>(id.id);
	globalLightAllocator.Get<GlobalLightDirection>(lid) = direction;
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
	(genericLightAllocator.Get<Color>(cid.id) * genericLightAllocator.Get<Intensity>(cid.id)).store(params.GlobalLightColor);
	Math::matrix44::transform(globalLightAllocator.Get<GlobalLightDirection>(globalLightId), viewTransform).store(params.GlobalLightDir);
	globalLightAllocator.Get<GlobalLightDirection>(globalLightId).store(params.GlobalLightDirWorldspace);
	globalLightAllocator.Get<GlobalLightBacklight>(globalLightId).store(params.GlobalBackLightColor);
	params.GlobalBackLightOffset = globalLightAllocator.Get<GlobalLightBacklightOffset>(globalLightId);
	globalLightAllocator.Get<GlobalLightAmbient>(globalLightId).store(params.GlobalAmbientLightColor);

	// go through and update local lights
	const Util::Array<LightType>& types = genericLightAllocator.GetArray<Type>();
	const Util::Array<Math::float4>& color = genericLightAllocator.GetArray<Color>();
	const Util::Array<float>& intensity = genericLightAllocator.GetArray<Intensity>();
	const Util::Array<bool>& castShadow = genericLightAllocator.GetArray<ShadowCaster>();
	const Util::Array<Ids::Id32>& typeIds = genericLightAllocator.GetArray<TypedLightId>();

	IndexT i;
	for (i = 0; i < types.Size(); i++)
	{
		switch (types[i])
		{

		case PointLightType:
		{
			auto trans = pointLightAllocator.Get<PointLightTransform>(typeIds[i]);
			auto tex = pointLightAllocator.Get<PointLightProjectionTexture>(typeIds[i]);

			uint flags = 0;

			Math::float4 posAndRange = Math::matrix44::transform(trans.get_position(), viewTransform);
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
			uint offset = CoreGraphics::AllocateGraphicsConstantBufferMemory(MainThreadConstantBuffer, sizeof(Lights::LocalLightBlock));
			alignas(16) Lights::LocalLightBlock block;
			(color[i] * intensity[i]).store(block.LightColor);
			trans.store(block.LightTransform);
			posAndRange.store(block.LightPosRange);
			block.Flags = flags;

			// update buffer
			pointLightAllocator.Get<PointLightDynamicOffsets>(typeIds[i])[0] = offset;
			ConstantBufferUpdate(lightServerState.localLightsConstantBuffer, block, offset);

		}
		break;

		case SpotLightType:
		{
			auto trans = spotLightAllocator.Get<SpotLightTransform>(typeIds[i]);
			auto tex = spotLightAllocator.Get<SpotLightProjectionTexture>(typeIds[i]);
			auto invViewProj = spotLightAllocator.Get<SpotLightInvViewProjection>(typeIds[i]);

			uint flags = USE_PROJECTION_TEX_BITFLAG;

			Math::float4 posAndRange = Math::matrix44::transform(trans.get_position(), viewTransform);
			posAndRange.w() = 1 / trans.get_zaxis().length();

			// update shadow data
			if (castShadow[i])
			{
				flags |= USE_SHADOW_BITFLAG;
			}

			// update projection transform
			Math::matrix44 fromViewToLightProj = Math::matrix44::multiply(invViewTransform, invViewProj);

			// allocate memory
			uint offset = CoreGraphics::AllocateGraphicsConstantBufferMemory(MainThreadConstantBuffer, sizeof(Lights::LocalLightBlock));
			alignas(16) Lights::LocalLightBlock block;
			(color[i] * intensity[i]).store(block.LightColor);
			trans.store(block.LightTransform);
			posAndRange.store(block.LightPosRange);
			fromViewToLightProj.store(block.LightProjTransform);
			block.ProjectionTexture = TextureGetBindlessHandle(tex);
			block.Flags = flags;

			// update buffer
			spotLightAllocator.Get<SpotLightDynamicOffsets>(typeIds[i])[0] = offset;
			ConstantBufferUpdate(lightServerState.localLightsConstantBuffer, block, offset);
		}
		break;

		}

		// update cluster state
		clusterState.lights[i].type = types[i];
		
		switch (types[i])
		{
			case PointLightType:
			{
				auto trans = pointLightAllocator.Get<PointLightTransform>(typeIds[i]);
				Math::float4 posAndRange = Math::matrix44::transform(trans.get_position(), viewTransform);
				posAndRange.w() = 1 / trans.get_zaxis().length();
				posAndRange.storeu(clusterState.lights[i].position);
			}
			break;

			case SpotLightType:
			{
				auto trans = spotLightAllocator.Get<SpotLightTransform>(typeIds[i]);
				Math::float4 posAndRange = Math::matrix44::transform(trans.get_position(), viewTransform);
				posAndRange.w() = 1 / trans.get_zaxis().length();
				posAndRange.storeu(clusterState.lights[i].position);
				trans.get_zaxis().store3(clusterState.lights[i].forward);
			}
			break;
		}
	}

	// update lights buffer which will be used for the light culling pass
	ShaderRWBufferUpdate(clusterState.clusterLightBuffer, clusterState.lights, sizeof(LightsClusterClassification::Light) * types.Size());	
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::UpdateLightClassification()
{
	CoreGraphics::SetShaderProgram(clusterState.classificationProgram);
	CoreGraphics::SetResourceTable(clusterState.clusterResourceTable, NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
	CoreGraphics::Compute(15, 15, 7); // we have 16 x 16 x 8 cells, so the indices naturally become 15, 15, 7
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
		if (genericLightAllocator.Get<ShadowCaster>(cid.id))
			CoreGraphics::SetShaderProgram(lightServerState.globalLightShadow);
		else
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
	const Util::Array<Util::FixedArray<uint>>& pdynamicOffsets = pointLightAllocator.GetArray<PointLightDynamicOffsets>();

	IndexT i;
	for (i = 0; i < pdynamicOffsets.Size(); i++)
	{
		// setup resources
		CoreGraphics::SetResourceTable(lightServerState.localLightsResourceTable, NEBULA_INSTANCE_GROUP, CoreGraphics::GraphicsPipeline, pdynamicOffsets[i]);

		// draw point light!
		CoreGraphics::Draw();
	}

	// first bind pointlight mesh
	CoreGraphics::MeshBind(lightServerState.spotLightMesh, 0);

	// set shader program and bind pipeline
	CoreGraphics::SetShaderProgram(lightServerState.spotLight);
	CoreGraphics::SetGraphicsPipeline();

	const Util::Array<Util::FixedArray<uint>>& sdynamicOffsets = spotLightAllocator.GetArray<SpotLightDynamicOffsets>();
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
LightContext::UpdateGlobalShadowMap()
{
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::UpdateSpotShadows()
{
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::UpdatePointShadows()
{
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
    auto const& pointTrans = pointLightAllocator.GetArray<PointLightTransform>();
    auto const& spotTrans = spotLightAllocator.GetArray< SpotLightTransform>();
    auto const& spotProj = spotLightAllocator.GetArray< SpotLightProjection>();
    auto const& spotInvProj = spotLightAllocator.GetArray< SpotLightInvViewProjection>();
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
                col.set_w(0.5f);
                Im3d::Im3dContext::DrawSphere(trans, col, Im3d::CheckDepth | Im3d::Solid);
            }            
        }
        break;
        case SpotLightType:
        {
            //FIXME            
            /*
            Math::matrix44 unscaledTransform = spotTrans[ids[i]];
            unscaledTransform.set_xaxis(Math::float4::normalize(unscaledTransform.get_xaxis()));
            unscaledTransform.set_yaxis(Math::float4::normalize(unscaledTransform.get_yaxis()));
            unscaledTransform.set_zaxis(Math::float4::normalize(unscaledTransform.get_zaxis()));
            Math::matrix44 frustum = Math::matrix44::multiply(spotInvProj[ids[i]], unscaledTransform);
            Math::float4 col = colours[i];
            Im3d::Im3dContext::DrawBox(frustum, col);
            */            
        }
        break;
        }
    }
}
} // namespace Lighting
