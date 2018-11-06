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
#include "frame/framebatchtype.h"
#include "resources/resourcemanager.h"

namespace Lighting
{

_ImplementContext(LightContext);
LightContext::GenericLightAllocator LightContext::genericLightAllocator;
LightContext::PointLightAllocator LightContext::pointLightAllocator;
LightContext::SpotLightAllocator LightContext::spotLightAllocator;
LightContext::GlobalLightAllocator LightContext::globalLightAllocator;

struct LightServerState
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

	CoreGraphics::ConstantBufferId tickParamsBuffer;		// contains lighting constants
	Graphics::GraphicsEntityId globalLightEntity = Graphics::GraphicsEntityId::Invalid();
	RenderUtil::DrawFullScreenQuad fsq;

	CoreGraphics::ShaderProgramId spotLight;		
	CoreGraphics::ShaderProgramId spotLightShadow;
	CoreGraphics::ShaderProgramId pointLight;
	CoreGraphics::ShaderProgramId pointLightShadow;

	struct LocalLight
	{
		CoreGraphics::ConstantBinding color, posRange, projectionTransform, transform, projectionTexture;
		CoreGraphics::ConstantBinding shadowOffsetScale, shadowConstants, shadowBias, shadowIntensity, shadowTransform, shadowMap;
	} local;

	CoreGraphics::ResourceTableId localLightsResourceTable;
	CoreGraphics::ConstantBufferId localLightsConstantBuffer;			// use for all lights
	CoreGraphics::ConstantBufferId localLightsShadowConstantBuffer;		// only use for shadowcasting lights

	CoreGraphics::MeshId spotLightMesh;
	CoreGraphics::MeshId pointLightMesh;

	IndexT localLightsSlot, localLightShadowSlot;
} lightServerState;

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
	__bundle.OnBeforeFrame = nullptr;
	__bundle.OnWaitForWork = nullptr;
	__bundle.OnBeforeView = LightContext::OnBeforeView;
	__bundle.OnAfterView = nullptr;
	__bundle.OnAfterFrame = nullptr;
	__bundle.StageBits = &LightContext::__state.currentStage;
	Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle);

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
	lightServerState.pointLightShadow = ShaderGetProgram(lightServerState.lightShader, CoreGraphics::ShaderFeatureFromString("Point|Alt0"));
	lightServerState.spotLight = ShaderGetProgram(lightServerState.lightShader, CoreGraphics::ShaderFeatureFromString("Spot"));
	lightServerState.spotLightShadow = ShaderGetProgram(lightServerState.lightShader, CoreGraphics::ShaderFeatureFromString("Spot|Alt0"));
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
	lightServerState.tickParamsBuffer = ShaderServer::Instance()->GetTickParams();
	lightServerState.localLightsResourceTable = ShaderCreateResourceTable(lightServerState.lightShader, NEBULA_INSTANCE_GROUP);
	lightServerState.localLightsConstantBuffer = CreateConstantBuffer({ true, lightServerState.lightShader, "LocalLightBlock", 0, 1 });
	lightServerState.localLightsShadowConstantBuffer = CreateConstantBuffer({ true, lightServerState.lightShader, "LocalLightShadowBlock", 0, 1 });
	lightServerState.localLightsSlot = ShaderGetResourceSlot(lightServerState.lightShader, "LocalLightBlock");
	lightServerState.localLightShadowSlot = ShaderGetResourceSlot(lightServerState.lightShader, "LocalLightShadowBlock");
	ResourceTableSetConstantBuffer(lightServerState.localLightsResourceTable, { lightServerState.localLightsConstantBuffer, lightServerState.localLightsSlot, 0, true, false, -1, 0});
	ResourceTableSetConstantBuffer(lightServerState.localLightsResourceTable, { lightServerState.localLightsShadowConstantBuffer, lightServerState.localLightShadowSlot, 0, true, false, -1, 0 });
	ResourceTableCommitChanges(lightServerState.localLightsResourceTable);

	lightServerState.spotLightMesh = Resources::CreateResource("msh:system/spotlightshape.nvx2", "system", nullptr, nullptr, true);
	lightServerState.pointLightMesh = Resources::CreateResource("msh:system/pointlightshape.nvx2", "system", nullptr, nullptr, true);

	DisplayMode mode = WindowGetDisplayMode(DisplayDevice::Instance()->GetCurrentWindow());
	lightServerState.fsq.Setup(mode.GetWidth(), mode.GetHeight());

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

	auto lid = globalLightAllocator.AllocObject();
	globalLightAllocator.Get<GlobalLightDirection>(lid) = direction;
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
LightContext::SetupPointLight(const Graphics::GraphicsEntityId id, const Math::float4& color, const float intensity, const Math::matrix44& transform, const float range, bool castShadows)
{
	n_assert(id != Graphics::GraphicsEntityId::Invalid());
	const Graphics::ContextEntityId cid = GetContextId(id);
	genericLightAllocator.Get<Type>(cid.id) = PointLightType;
	genericLightAllocator.Get<Color>(cid.id) = color;
	genericLightAllocator.Get<Intensity>(cid.id) = intensity;
	genericLightAllocator.Get<ShadowCaster>(cid.id) = castShadows;

	const Math::matrix44 scaleMatrix = Math::matrix44::scaling(range, range, range);
	auto pli = pointLightAllocator.AllocObject();
	pointLightAllocator.Get<PointLightTransform>(pli) = Math::matrix44::multiply(scaleMatrix, transform);
	pointLightAllocator.Get<PointLightDynamicOffsets>(pli).Resize(2);
	genericLightAllocator.Get<TypedLightId>(cid.id) = pli;

	// allocate constant buffer
	bool changes = false;
	uint offset, slice;
	if (ConstantBufferAllocateInstance(lightServerState.localLightsConstantBuffer, offset, slice))
	{
		ResourceTableSetConstantBuffer(lightServerState.localLightsResourceTable, { lightServerState.localLightsConstantBuffer, lightServerState.localLightsSlot, 0, true, false, -1, 0 });
		changes = true;
	}
	pointLightAllocator.Get<PointLightConstantBufferSet>(pli) = { offset, slice };
	pointLightAllocator.Get<PointLightDynamicOffsets>(pli)[0] = offset;

	// allocate shadow buffer slice if we cast shadows
	offset = 0;
	if (castShadows)
	{
		if (ConstantBufferAllocateInstance(lightServerState.localLightsShadowConstantBuffer, offset, slice))
		{
			ResourceTableSetConstantBuffer(lightServerState.localLightsResourceTable, { lightServerState.localLightsShadowConstantBuffer, lightServerState.localLightShadowSlot, 0, true, false, -1, 0 });
			changes = true;
		}
		pointLightAllocator.Get<PointLightShadowConstantBufferSet>(pli) = { offset, slice };
	}
	else
	{
		pointLightAllocator.Get<PointLightShadowConstantBufferSet>(pli) = { UINT_MAX, UINT_MAX };
	}
	pointLightAllocator.Get<PointLightDynamicOffsets>(pli)[1] = offset;
	
	if (changes)
		ResourceTableCommitChanges(lightServerState.localLightsResourceTable);
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::SetupSpotLight(const Graphics::GraphicsEntityId id, const Math::float4& color, const float intensity, const Math::matrix44& transform, const float coneAngle, bool castShadows)
{
	n_assert(id != Graphics::GraphicsEntityId::Invalid());
	const Graphics::ContextEntityId cid = GetContextId(id);
	genericLightAllocator.Get<Type>(cid.id) = SpotLightType;
	genericLightAllocator.Get<Color>(cid.id) = color;
	genericLightAllocator.Get<Intensity>(cid.id) = intensity;
	genericLightAllocator.Get<ShadowCaster>(cid.id) = castShadows;

	auto sli = spotLightAllocator.AllocObject();
	spotLightAllocator.Get<SpotLightTransform>(sli) = transform;
	spotLightAllocator.Get<SpotLightConeAngle>(sli) = coneAngle;
	spotLightAllocator.Get<SpotLightDynamicOffsets>(sli).Resize(2);
	genericLightAllocator.Get<TypedLightId>(cid.id) = sli;

	// allocate constant buffer slice
	bool changes = false;
	uint offset, slice;
	if (ConstantBufferAllocateInstance(lightServerState.localLightsConstantBuffer, offset, slice))
	{
		ResourceTableSetConstantBuffer(lightServerState.localLightsResourceTable, { lightServerState.localLightsConstantBuffer, lightServerState.localLightsSlot, 0, true, false, -1, 0 });
		changes = true;
	}
	spotLightAllocator.Get<SpotLightConstantBufferSet>(sli) = { offset, slice };
	spotLightAllocator.Get<SpotLightDynamicOffsets>(sli)[0] = offset;
	offset = 0;

	// allocate constant buffer for shadows if it casts them
	if (castShadows)
	{
		if (ConstantBufferAllocateInstance(lightServerState.localLightsShadowConstantBuffer, offset, slice))
		{
			ResourceTableSetConstantBuffer(lightServerState.localLightsResourceTable, { lightServerState.localLightsShadowConstantBuffer, lightServerState.localLightShadowSlot, 0, true, false, -1, 0 });
			changes = true;
		}
		spotLightAllocator.Get<SpotLightShadowConstantBufferSet>(sli) = { offset, slice };
	}
	else
	{
		spotLightAllocator.Get<SpotLightShadowConstantBufferSet>(sli) = { UINT_MAX, UINT_MAX };
	}
	spotLightAllocator.Get<SpotLightDynamicOffsets>(sli)[1] = offset;

	if (changes)
		ResourceTableCommitChanges(lightServerState.localLightsResourceTable);
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
LightContext::SetSpotLightTransform(const Graphics::GraphicsEntityId id, const Math::matrix44& transform)
{
	const Graphics::ContextEntityId cid = GetContextId(id);
#if NEBULA_DEBUG
	n_assert(genericLightAllocator.Get<Type>(cid.id) == SpotLightType);
#endif

	auto lid = genericLightAllocator.Get<TypedLightId>(cid.id);
	spotLightAllocator.Get<SpotLightTransform>(lid) = transform;
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::SetPointLightTransform(const Graphics::GraphicsEntityId id, const Math::matrix44& transform)
{
	const Graphics::ContextEntityId cid = GetContextId(id);
#if NEBULA_DEBUG
	n_assert(genericLightAllocator.Get<Type>(cid.id) == PointLightType);
#endif

	auto lid = genericLightAllocator.Get<TypedLightId>(cid.id);
	pointLightAllocator.Get<PointLightTransform>(lid) = transform;
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::SetGlobalLightDirection(const Graphics::GraphicsEntityId id, const Math::vector& direction)
{
	const Graphics::ContextEntityId cid = GetContextId(id);
#if NEBULA_DEBUG
	n_assert(genericLightAllocator.Get<Type>(cid.id) == GlobalLightType);
#endif

	auto lid = genericLightAllocator.Get<TypedLightId>(cid.id);
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

	// update constant buffer
	Ids::Id32 globalLightId = genericLightAllocator.Get<TypedLightId>(cid.id);
	ConstantBufferUpdate(lightServerState.tickParamsBuffer, genericLightAllocator.Get<Color>(cid.id) * genericLightAllocator.Get<Intensity>(cid.id), lightServerState.global.color);
	ConstantBufferUpdate(lightServerState.tickParamsBuffer, Math::matrix44::transform(globalLightAllocator.Get<GlobalLightDirection>(globalLightId), viewTransform), lightServerState.global.dir);
	ConstantBufferUpdate(lightServerState.tickParamsBuffer, globalLightAllocator.Get<GlobalLightDirection>(globalLightId), lightServerState.global.dirWorldSpace);
	ConstantBufferUpdate(lightServerState.tickParamsBuffer, globalLightAllocator.Get<GlobalLightBacklight>(globalLightId), lightServerState.global.backlightColor);
	ConstantBufferUpdate(lightServerState.tickParamsBuffer, globalLightAllocator.Get<GlobalLightBacklightOffset>(globalLightId), lightServerState.global.backlightFactor);
	ConstantBufferUpdate(lightServerState.tickParamsBuffer, globalLightAllocator.Get<GlobalLightAmbient>(globalLightId), lightServerState.global.ambientColor);

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
			auto cboLight = pointLightAllocator.Get<PointLightConstantBufferSet>(typeIds[i]);
			auto trans = pointLightAllocator.Get<PointLightTransform>(typeIds[i]);
			ConstantBufferUpdateInstance(lightServerState.localLightsConstantBuffer, color[i] * intensity[i], cboLight.slice, lightServerState.local.color);
			ConstantBufferUpdateInstance(lightServerState.localLightsConstantBuffer, trans, cboLight.slice, lightServerState.local.transform);

			Math::float4 posAndRange = Math::matrix44::transform(trans.get_position(), viewTransform);
			posAndRange.w() = 1 / trans.get_zaxis().length();
			ConstantBufferUpdateInstance(lightServerState.localLightsConstantBuffer, posAndRange, cboLight.slice, lightServerState.local.posRange);

			if (castShadow[i])
			{
				auto cboShadow = pointLightAllocator.Get<PointLightShadowConstantBufferSet>(typeIds[i]);
			}
		}
		break;

		case SpotLightType:
		{
			auto cboLight = spotLightAllocator.Get<SpotLightConstantBufferSet>(typeIds[i]);
			auto trans = spotLightAllocator.Get<SpotLightTransform>(typeIds[i]);
			ConstantBufferUpdateInstance(lightServerState.localLightsConstantBuffer, color[i] * intensity[i], cboLight.slice, lightServerState.local.color);
			ConstantBufferUpdateInstance(lightServerState.localLightsConstantBuffer, trans, cboLight.slice, lightServerState.local.transform);

			if (castShadow[i])
			{
				auto cboShadow = spotLightAllocator.Get<SpotLightShadowConstantBufferSet>(typeIds[i]);
			}
		}
		break;

		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
LightContext::RenderLights()
{
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
		CoreGraphics::Draw();
	}

	// first bind pointlight mesh
	CoreGraphics::MeshBind(lightServerState.pointLightMesh, 0);

	// draw local lights
	const Util::Array<ConstantBufferSet>& lightCbo = pointLightAllocator.GetArray<PointLightConstantBufferSet>();
	const Util::Array<ConstantBufferSet>& shadowCbo = pointLightAllocator.GetArray<PointLightShadowConstantBufferSet>();
	const Util::Array<Util::FixedArray<uint>>& dynamicOffsets = pointLightAllocator.GetArray<PointLightDynamicOffsets>();
	IndexT i;
	for (i = 0; i < lightCbo.Size(); i++)
	{
		// set shader program and bind pipeline
		CoreGraphics::SetShaderProgram(lightServerState.pointLight);
		CoreGraphics::SetGraphicsPipeline();

		// setup resources
		CoreGraphics::SetResourceTable(lightServerState.localLightsResourceTable, NEBULA_INSTANCE_GROUP, CoreGraphics::GraphicsPipeline, dynamicOffsets[i]);

		// draw light!
		CoreGraphics::Draw();
	}
	CoreGraphics::EndBatch();
}

//------------------------------------------------------------------------------
/**
*/

void LightContext::UpdateGlobalShadowMap()
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
	return genericLightAllocator.AllocObject();
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::Dealloc(Graphics::ContextEntityId id)
{
	genericLightAllocator.DeallocObject(id.id);
}

} // namespace Lighting