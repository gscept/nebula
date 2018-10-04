//------------------------------------------------------------------------------
//  ogl4shadowserver.cc
//  (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "lighting/ogl4/ogl4shadowserver.h"
#include "frame/frameserver.h"
#include "coregraphics/transformdevice.h"
#include "models/visresolver.h"
#include "graphics/modelentity.h"
#include "coregraphics/shaderserver.h"     
#include "graphics/spotlightentity.h"
#include "materials/materialserver.h"
#include "math/polar.h"
#include "lighting/lightserver.h"
#include "math/bbox.h"
#include "math/vector.h"
#include "graphics/view.h"
#include "graphics/graphicsserver.h"
#include "graphics/view.h"
#include "graphics/graphicsserver.h"
#include "coregraphics/base/rendertargetbase.h"
#include "coregraphics/renderdevice.h"
#include "framesync/framesynctimer.h"

#define DivAndRoundUp(a, b) (a % b != 0) ? (a / b + 1) : (a / b)
namespace Lighting
{
__ImplementClass(Lighting::OGL4ShadowServer, 'SM5S', ShadowServerBase);

using namespace Math;
using namespace Util;
using namespace Frame;
using namespace Resources;
using namespace CoreGraphics;
using namespace Graphics;
using namespace Models;
using namespace Materials;
using namespace Base;
using namespace FrameSync;

//------------------------------------------------------------------------------
/**
*/
OGL4ShadowServer::OGL4ShadowServer()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
OGL4ShadowServer::~OGL4ShadowServer()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShadowServer::Open()
{
	// call parent class
	ShadowServerBase::Open();

	// load the ShadowBuffer frame shader
	const Ptr<FrameServer>& frameServer = FrameServer::Instance();
	const Ptr<MaterialServer> materialServer = MaterialServer::Instance();

	// setup the shadow buffer render target, this is a single
	// render target which contains the shadow buffer data for
	// all shadow casting light sources
	SizeT rtWidth = 512;									// hard code each light to have a 512x512 shadow map
	SizeT rtHeight = 512;
	SizeT resolveWidth = rtWidth * ShadowLightsPerColumn;
	SizeT resolveHeight = rtHeight * ShadowLightsPerRow;
	PixelFormat::Code pixelFormat = PixelFormat::G32R32F;	// VSM shadow mapping uses two channels

	// create a shadow buffer for up to MaxNumShadowLights local light sources
	this->spotLightShadowBufferAtlas = RenderTarget::Create();
	this->spotLightShadowBufferAtlas->SetWidth(rtWidth);
	this->spotLightShadowBufferAtlas->SetHeight(rtHeight);
	this->spotLightShadowBufferAtlas->SetAntiAliasQuality(AntiAliasQuality::None);
	this->spotLightShadowBufferAtlas->SetColorBufferFormat(pixelFormat);
	this->spotLightShadowBufferAtlas->SetResolveTextureResourceId(ResourceId("SpotLightShadowBufferAtlas"));
	this->spotLightShadowBufferAtlas->SetResolveTextureWidth(resolveWidth);
	this->spotLightShadowBufferAtlas->SetResolveTextureHeight(resolveHeight);
	this->spotLightShadowBufferAtlas->Setup();

	// create shadow map for spot lights
	this->spotLightShadowMap1 = RenderTarget::Create();
	this->spotLightShadowMap1->SetWidth(rtWidth);
	this->spotLightShadowMap1->SetHeight(rtHeight);
	this->spotLightShadowMap1->SetAntiAliasQuality(AntiAliasQuality::None);
	this->spotLightShadowMap1->SetColorBufferFormat(pixelFormat);
	this->spotLightShadowMap1->SetResolveTextureResourceId(ResourceId("SpotLightShadowMap1"));
	this->spotLightShadowMap1->Setup();

	// create secondary shadow map for spot lights after filtering
	this->spotLightShadowMap2 = RenderTarget::Create();
	this->spotLightShadowMap2->SetWidth(rtWidth);
	this->spotLightShadowMap2->SetHeight(rtHeight);
	this->spotLightShadowMap2->SetAntiAliasQuality(AntiAliasQuality::None);
	this->spotLightShadowMap2->SetColorBufferFormat(pixelFormat);
	this->spotLightShadowMap2->SetResolveTextureResourceId(ResourceId("SpotLightShadowMap2"));
	this->spotLightShadowMap2->Setup();

	// create spotlight batch
	this->spotLightBatch = FrameBatch::Create();
	this->spotLightBatch->SetType(CoreGraphics::FrameBatchType::FromString("Geometry"));
	this->spotLightBatch->SetBatchGroup(BatchGroup::FromName("SpotLightShadow"));

	// create spotlight pass
	this->spotLightPass = FramePass::Create();
	this->spotLightPass->SetRenderTarget(this->spotLightShadowMap1);
	this->spotLightPass->SetClearColor(float4(1000, 1000, 0, 0));
	this->spotLightPass->SetClearFlags(RenderTarget::ClearColor);
	this->spotLightPass->SetName("SpotLightShadows");
	this->spotLightPass->AddBatch(this->spotLightBatch);

	// create SAT shaders
	this->satXShader = ShaderServer::Instance()->GetShader("shd:box3taphori")->CreateState({ NEBULA_DEFAULT_GROUP });
	this->satXShader->GetVariableByName("SourceBuffer")->SetTexture(this->spotLightShadowMap1->GetResolveTexture());
	this->satYShader = ShaderServer::Instance()->GetShader("shd:box3tapvert")->CreateState({ NEBULA_DEFAULT_GROUP });
	this->satYShader->GetVariableByName("SourceBuffer")->SetTexture(this->spotLightShadowMap2->GetResolveTexture());

	// setup sat passes
	this->spotLightVertPass = FramePostEffect::Create();
	this->spotLightVertPass->SetName("SATVert");
	this->spotLightVertPass->SetRenderTarget(this->spotLightShadowMap2);
	this->spotLightVertPass->SetShaderState(this->satXShader);
	this->spotLightVertPass->Setup();

	this->spotLightHoriPass = FramePostEffect::Create();
	this->spotLightHoriPass->SetName("SATHori");
	this->spotLightHoriPass->SetRenderTarget(this->spotLightShadowBufferAtlas);
	this->spotLightHoriPass->SetShaderState(this->satYShader);
	this->spotLightHoriPass->Setup();

#if NEBULA_ENABLE_PROFILING
	{
		// add debug timer for batch
		Util::String name;
		name.Format("SpotLightBatch");
		this->spotLightBatch->SetBatchDebugTimer(name);

		name.Format("SpotLightDrawPass");
		this->spotLightPass->SetFramePassDebugTimer(name);

		name.Format("SpotLightVertSATPass");
		this->spotLightVertPass->SetFramePostEffectDebugTimer(name);

		name.Format("SpotLightHoriSATPass");
		this->spotLightHoriPass->SetFramePostEffectDebugTimer(name);
	}
#endif

    // create render cubes
    IndexT i;
	for (i = 0; i < MaxNumShadowPointLights; i++)
    {
        this->pointLightShadowCubes[i] = RenderTargetCube::Create();
        this->pointLightShadowCubes[i]->SetWidth(rtWidth);
        this->pointLightShadowCubes[i]->SetHeight(rtHeight);
        this->pointLightShadowCubes[i]->SetColorBufferFormat(pixelFormat);
        this->pointLightShadowCubes[i]->SetResolveTextureResourceId(ResourceId("PointLightShadowCube" + String::FromInt(i)));
		this->pointLightShadowCubes[i]->SetLayered(true);
        this->pointLightShadowCubes[i]->Setup();
    }

	this->pointLightShadowFilterCube = RenderTargetCube::Create();
	this->pointLightShadowFilterCube->SetWidth(rtWidth);
	this->pointLightShadowFilterCube->SetHeight(rtHeight);
	this->pointLightShadowFilterCube->SetColorBufferFormat(pixelFormat);
	this->pointLightShadowFilterCube->SetResolveTextureResourceId(ResourceId("PointLightShadowCubeFilter"));
	this->pointLightShadowFilterCube->SetLayered(true);
	this->pointLightShadowFilterCube->Setup();

    // create batch for pointlights
    this->pointLightBatch = FrameBatch::Create();
    this->pointLightBatch->SetType(CoreGraphics::FrameBatchType::FromString("Geometry"));
    this->pointLightBatch->SetBatchGroup(BatchGroup::FromName("PointLightShadow"));
	this->pointLightBatch->SetForceInstancing(true, 6);

    // create pass for pointlights
    this->pointLightPass = FramePass::Create();
	this->pointLightPass->SetClearFlags(RenderTarget::ClearColor);
    this->pointLightPass->SetName("PointLightShadowPass");
	this->pointLightPass->SetClearColor(float4(1000, 1000, 0, 0));
    this->pointLightPass->AddBatch(this->pointLightBatch);
	this->shadowShader = ShaderServer::Instance()->GetShader("shd:shadow")->CreateState({ NEBULA_DEFAULT_GROUP });
	this->pointLightPosVar = this->shadowShader->GetVariableByName("LightCenter");

	// load shaders for point light blur
	this->pointLightBlur = ShaderServer::Instance()->GetShader("shd:blur_cube_rg32f_cs")->CreateState({ NEBULA_DEFAULT_GROUP });
	this->xBlurMask = ShaderServer::Instance()->FeatureStringToMask("Alt0");
	this->yBlurMask = ShaderServer::Instance()->FeatureStringToMask("Alt1");
	//this->pointLightBlurReadLinear = this->pointLightBlur->GetVariableByName("ReadImageLinear");
	//this->pointLightBlurReadPoint = this->pointLightBlur->GetVariableByName("ReadImagePoint");
	this->pointLightBlurWrite = this->pointLightBlur->GetVariableByName("WriteImage");

#if NEBULA_ENABLE_PROFILING
    {
        // add debug timer
        Util::String name;
        name.Format("PointLightBatch");
        this->pointLightBatch->SetBatchDebugTimer(name);

        name.Format("PointLightPass");
        this->pointLightPass->SetFramePassDebugTimer(name);
    }	
#endif

    //rtWidth /= 2;
    //rtHeight /= 2;
    const SizeT CSMCascadeTileSize = 1024;
    SizeT csmWidth = CSMCascadeTileSize * SplitsPerRow;
    SizeT csmHeight = CSMCascadeTileSize * SplitsPerColumn;

	// setup hot buffer
	this->globalLightShadowBuffer = RenderTarget::Create();
	this->globalLightShadowBuffer->SetWidth(csmWidth);
	this->globalLightShadowBuffer->SetHeight(csmHeight);
	this->globalLightShadowBuffer->SetAntiAliasQuality(AntiAliasQuality::None);
	this->globalLightShadowBuffer->SetColorBufferFormat(pixelFormat);
	this->globalLightShadowBuffer->SetResolveTextureResourceId(ResourceId("GlobalLightShadowBuffer"));
	this->globalLightShadowBuffer->Setup();	

	// setup other buffer
	this->globalLightShadowBufferFinal = RenderTarget::Create();
	this->globalLightShadowBufferFinal->SetWidth(csmWidth);
	this->globalLightShadowBufferFinal->SetHeight(csmHeight);
	this->globalLightShadowBufferFinal->SetAntiAliasQuality(AntiAliasQuality::None);
	this->globalLightShadowBufferFinal->SetColorBufferFormat(pixelFormat);
	this->globalLightShadowBufferFinal->SetResolveTextureResourceId(ResourceId("GlobalLightShadowBufferFinal"));
	this->globalLightShadowBufferFinal->Setup();	

	// create batch
	this->globalLightShadowBatch = FrameBatch::Create();
	this->globalLightShadowBatch->SetType(CoreGraphics::FrameBatchType::FromString("Geometry"));
	this->globalLightShadowBatch->SetBatchGroup(BatchGroup::FromName("GlobalShadow"));
    this->globalLightShadowBatch->SetForceInstancing(true, 4);

	// create pass
	this->globalLightHotPass = FramePass::Create();
	this->globalLightHotPass->SetRenderTarget(this->globalLightShadowBuffer);
	this->globalLightHotPass->SetClearColor(float4(1000, 1000, 0, 0));
	this->globalLightHotPass->SetClearFlags(RenderTarget::ClearColor);
	this->globalLightHotPass->SetName("GlobalLightHotPass");
	this->globalLightHotPass->AddBatch(this->globalLightShadowBatch);

	// get blur shader
	this->blurShader = ShaderServer::Instance()->GetShader("shd:csmblur")->CreateState({ NEBULA_DEFAULT_GROUP });

	// create blur pass
	this->globalLightBlurPass = FramePostEffect::Create();
	this->globalLightBlurPass->SetName("GlobalLightBlurPass");
	this->globalLightBlurPass->SetRenderTarget(this->globalLightShadowBufferFinal);
	this->globalLightBlurPass->SetShaderState(this->blurShader);
	this->globalLightBlurPass->Setup();
	this->blurShader->GetVariableByName("SourceMap")->SetTexture(this->globalLightShadowBuffer->GetResolveTexture());

#if NEBULA_ENABLE_PROFILING
	{
        // add debug timer for batch
        Util::String name;
        name.Format("GlobalLightShadowBatch");
        this->globalLightShadowBatch->SetBatchDebugTimer(name);

		// add debug timer for hot pass
		name.Format("GlobalLightShadowHotPass");
		this->globalLightHotPass->SetFramePassDebugTimer(name);

        // add debug timer for blur
        name.Format("GlobalLightShadowBlur");
        this->globalLightBlurPass->SetFramePostEffectDebugTimer(name);
	}	
#endif

	// create resolve rect array
	Util::Array<Math::rectangle<int> > rectArray;

	// render shadow casters for each view volume split
    IndexT cascadeIndex;
	for (cascadeIndex = 0; cascadeIndex < CSMUtil::NumCascades; cascadeIndex++)
	{
		// prepare shadow buffer render target for rendering and 
		// patch shadow buffer render target into the frame shader
		Math::rectangle<int> resolveRect;
		resolveRect.left   = (cascadeIndex % SplitsPerRow) * CSMCascadeTileSize;
		resolveRect.right  = resolveRect.left + CSMCascadeTileSize;
 		resolveRect.top    = (cascadeIndex / SplitsPerColumn) * CSMCascadeTileSize;
 		resolveRect.bottom = resolveRect.top + CSMCascadeTileSize;

		rectArray.Append(resolveRect);
	}
	this->globalLightShadowBuffer->SetResolveRectArray(rectArray);

	this->csmUtil.SetTextureWidth(csmWidth / SplitsPerRow);
	this->csmUtil.SetClampingMethod(CSMUtil::AABB);
	this->csmUtil.SetFittingMethod(CSMUtil::Scene);

#if NEBULA_ENABLE_PROFILING
	this->globalShadow = Debug::DebugTimer::Create();
	String name("ShadowServer_GlobalShadow");
	this->globalShadow->Setup(name, "Shadowmapping");

	this->spotLightShadow = Debug::DebugTimer::Create();
	name.Format("ShadowServer_SpotLightShadow");
	this->spotLightShadow->Setup(name, "Shadowmapping");

	this->pointLightShadow = Debug::DebugTimer::Create();
	name.Format("ShadowServer_PointLightShadow");
	this->pointLightShadow->Setup(name, "Shadowmapping");
#endif
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShadowServer::Close()
{
	// cleanup spotlights
	this->spotLightShadowMap1->Discard();
	this->spotLightShadowMap1 = 0;
	this->spotLightShadowMap2->Discard();
	this->spotLightShadowMap2 = 0;
    this->spotLightShadowBufferAtlas->Discard();
    this->spotLightShadowBufferAtlas = 0;
	this->satXShader = 0;
	this->satYShader = 0;
	this->spotLightPass->Discard();
	this->spotLightPass = 0;
	this->spotLightBatch = 0;
	this->spotLightVertPass->Discard();
	this->spotLightVertPass = 0;
	this->spotLightHoriPass->Discard();
	this->spotLightHoriPass = 0;
    
    IndexT i;
    for (i = 0; i < MaxNumShadowPointLights; i++)
    {
        this->pointLightShadowCubes[i]->Discard();
        this->pointLightShadowCubes[i] = 0;
    }	
	this->pointLightShadowFilterCube->Discard();
	this->pointLightShadowFilterCube = 0;
	this->pointLightBlurReadLinear = 0;
	this->pointLightBlurReadPoint = 0;
	this->pointLightBlurWrite = 0;

    // discard point light pass
    this->pointLightPass->Discard();
    this->pointLightPass = 0;
    this->pointLightBatch = 0;
	this->pointLightBlur = 0;

	// cleanup CSM
	this->globalLightShadowBuffer->Discard();
	this->globalLightShadowBuffer = 0;
	this->globalLightShadowBufferFinal->Discard();
	this->globalLightShadowBufferFinal = 0;

	this->globalLightHotPass->Discard();
	this->globalLightHotPass = 0;
	this->globalLightBlurPass->Discard();
	this->globalLightBlurPass = 0;
	this->globalLightShadowBatch = 0;				// we dont need to discard the batch, the pass does this for us
	this->blurShader = 0;

    this->blurShader = 0;

    // call parent class
    ShadowServerBase::Close();

	_discard_timer(globalShadow);
	_discard_timer(pointLightShadow);
	_discard_timer(spotLightShadow);
}

//------------------------------------------------------------------------------
/**
	This method updates the  shadow buffer
*/
void
OGL4ShadowServer::UpdateShadowBuffers()
{
	n_assert(this->inBeginFrame);
	n_assert(!this->inBeginAttach);

	// update local lights shadow buffer
	_start_timer(spotLightShadow);
	if (this->spotLightEntities.Size() > 0)
	{
		this->UpdateSpotLightShadowBuffers();
	}
	_stop_timer(spotLightShadow);

	_start_timer(pointLightShadow);
	if (this->pointLightEntities.Size() > 0)
	{
		this->UpdatePointLightShadowBuffers();
	}
	_stop_timer(pointLightShadow);

	// update global light cascaded shadow buffers
	_start_timer(globalShadow);
	if (this->globalLightEntity.isvalid())
	{
		// prepare the global buffer by updating visibility
		this->PrepareGlobalShadowBuffer();
		this->UpdateHotGlobalShadowBuffer();
	}
	_stop_timer(globalShadow);
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShadowServer::UpdateSpotLightShadowBuffers()
{
	const Ptr<VisResolver>& visResolver = VisResolver::Instance();
	const Ptr<TransformDevice>& transDev = TransformDevice::Instance();
	const Ptr<RenderDevice>& renderDev = RenderDevice::Instance();
	const Ptr<FrameSyncTimer>& timer = FrameSyncTimer::Instance();
    IndexT frameIndex = timer->GetFrameIndex();

	// for each shadow casting light...
	SizeT numLights = this->spotLightEntities.Size();
	if (numLights > MaxNumShadowSpotLights)
	{
		numLights = MaxNumShadowSpotLights;
	}

	IndexT lightIndex;
	for (lightIndex = 0; lightIndex < numLights; lightIndex++)
	{
		// render shadow casters in current light volume to shadow buffer
		const Ptr<SpotLightEntity>& lightEntity = this->spotLightEntities[lightIndex];

		// perform visibility resolve for current light
		visResolver->BeginResolve(lightEntity->GetTransform());
		const Array<Ptr<GraphicsEntity> >& visLinks = lightEntity->GetLinks(GraphicsEntity::LightLink);
		if (visLinks.Size() == 0) continue;
		IndexT linkIndex;
		for (linkIndex = 0; linkIndex < visLinks.Size(); linkIndex++)
		{
			const Ptr<GraphicsEntity>& curEntity = visLinks[linkIndex];
			n_assert(GraphicsEntityType::Model == curEntity->GetType());
			if (curEntity->IsA(ModelEntity::RTTI) && curEntity->GetCastsShadows())
			{
				const Ptr<ModelEntity>& modelEntity = curEntity.downcast<ModelEntity>();
                visResolver->AttachVisibleModelInstance(frameIndex, modelEntity->GetModelInstance(), false);
			}
		}
		visResolver->EndResolve();

        // prepare shadow buffer render target for rendering and 
        // patch current shadow buffer render target into the frame shader
        Math::rectangle<int> resolveRect;
        resolveRect.left   = (lightIndex % ShadowLightsPerRow) * this->spotLightShadowBufferAtlas->GetWidth();
		resolveRect.right = resolveRect.left + this->spotLightShadowBufferAtlas->GetWidth();
		resolveRect.top = (lightIndex / ShadowLightsPerColumn) * this->spotLightShadowBufferAtlas->GetHeight();
		resolveRect.bottom = resolveRect.top + this->spotLightShadowBufferAtlas->GetHeight();

		// render into shadow map
		matrix44 viewProj = matrix44::multiply(lightEntity->GetShadowInvTransform(), lightEntity->GetShadowProjTransform());
		transDev->ApplyViewMatrixArray(&viewProj, 1);
        this->spotLightPass->Render(frameIndex);

		// render first SAT pass
        this->spotLightVertPass->Render(frameIndex);

		// render second SAT pass
		this->spotLightShadowBufferAtlas->SetResolveRect(resolveRect);
        this->spotLightHoriPass->Render(frameIndex);

        // patch shadow buffer and shadow buffer uv offset into the light source  
        // uvOffset.xy is offset
        // uvOffset.zw is modulate
        // also moves projection space coords into uv space
		float shadowBufferHoriPixelSize = 1.0f / this->spotLightShadowBufferAtlas->GetResolveTextureWidth();
		float shadowBufferVertPixelSize = 1.0f / this->spotLightShadowBufferAtlas->GetResolveTextureHeight();
        float borderPaddingX = float(ShadowAtlasBorderPixels / ShadowLightsPerRow) / ShadowLightsPerRow;
        float borderPaddingY = float(ShadowAtlasBorderPixels / ShadowLightsPerColumn) / ShadowLightsPerColumn;
        Math::float4 uvOffset;
        uvOffset.x() = float(lightIndex%ShadowLightsPerRow) / float(ShadowLightsPerRow);
        uvOffset.y() = float(lightIndex/ShadowLightsPerColumn) / float(ShadowLightsPerColumn);
        uvOffset.z() = (1.0f - shadowBufferHoriPixelSize) / float(ShadowLightsPerRow);
        uvOffset.w() = (1.0f - shadowBufferVertPixelSize) / float(ShadowLightsPerColumn);
        lightEntity->SetShadowBufferUvOffsetAndScale(uvOffset);
	}	
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShadowServer::UpdatePointLightShadowBuffers()
{
	const Ptr<VisResolver>& visResolver = VisResolver::Instance();
	const Ptr<TransformDevice>& transDev = TransformDevice::Instance();
	const Ptr<RenderDevice>& renderDev = RenderDevice::Instance();
	const Ptr<FrameSyncTimer>& timer = FrameSyncTimer::Instance();
	IndexT frameIndex = timer->GetFrameIndex();

	// for each shadow casting light...
	SizeT numLights = this->pointLightEntities.Size();
	if (numLights > MaxNumShadowPointLights)
	{
		numLights = MaxNumShadowPointLights;
	}

	IndexT lightIndex;
	for (lightIndex = 0; lightIndex < numLights; lightIndex++)
	{
		// render shadow casters in current light volume to shadow buffer
		const Ptr<PointLightEntity>& lightEntity = this->pointLightEntities[lightIndex];

		// perform visibility resolve for current light
		visResolver->BeginResolve(lightEntity->GetTransform());
		const Array<Ptr<GraphicsEntity> >& visLinks = lightEntity->GetLinks(GraphicsEntity::LightLink);
		if (visLinks.Size() == 0) continue;
		IndexT linkIndex;
		for (linkIndex = 0; linkIndex < visLinks.Size(); linkIndex++)
		{
			const Ptr<GraphicsEntity>& curEntity = visLinks[linkIndex];
			n_assert(GraphicsEntityType::Model == curEntity->GetType());
			if (curEntity->IsA(ModelEntity::RTTI) && curEntity->GetCastsShadows())
			{
				const Ptr<ModelEntity>& modelEntity = curEntity.downcast<ModelEntity>();
				visResolver->AttachVisibleModelInstance(frameIndex, modelEntity->GetModelInstance(), false);
			}
		}
		visResolver->EndResolve();

		// generate view projection matrix
		matrix44 proj = matrix44::perspfovrh(n_deg2rad(90.0f), 1, 0.1f, 100.0f);
		//viewProj.set_position(0);

		// generate matrices
		matrix44 views[6];
		const float4 lightPos = lightEntity->GetTransform().get_position();

		views[0] = matrix44::lookatrh(vector(0), vector(1, 0, 0), -vector::upvec());
		views[0].set_position(lightPos);
		views[0] = matrix44::multiply(matrix44::inverse(views[0]), proj);

		views[1] = matrix44::lookatrh(vector(0), vector(-1, 0, 0), -vector::upvec());
		views[1].set_position(lightPos);
		views[1] = matrix44::multiply(matrix44::inverse(views[1]), proj);

		views[2] = matrix44::lookatrh(vector(0), vector(0, 1, 0), vector(0, 0, 1));
		views[2].set_position(lightPos);
		views[2] = matrix44::multiply(matrix44::inverse(views[2]), proj);

		views[3] = matrix44::lookatrh(vector(0), vector(0, -1, 0), vector(0, 0, -1));
		views[3].set_position(lightPos);
		views[3] = matrix44::multiply(matrix44::inverse(views[3]), proj);

		views[4] = matrix44::lookatrh(vector(0), vector(0, 0, 1), -vector::upvec());
		views[4].set_position(lightPos);
		views[4] = matrix44::multiply(matrix44::inverse(views[4]), proj);

		views[5] = matrix44::lookatrh(vector(0), vector(0, 0, -1), -vector::upvec());
		views[5].set_position(lightPos);
		views[5] = matrix44::multiply(matrix44::inverse(views[5]), proj);

		// send to transform device
		transDev->ApplyViewMatrixArray(views, 6);

		// get texture
		const Ptr<CoreGraphics::RenderTargetCube>& cube = this->pointLightShadowCubes[lightIndex];

		// setup pass and render
		this->pointLightPosVar->SetFloat4(lightPos);
		this->pointLightPass->SetRenderTargetCube(cube);
		this->pointLightPass->Render(frameIndex);

		// blur
#define TILE_WIDTH 320

		// calculate execution dimensions
		uint numGroupsX1 = DivAndRoundUp(512, TILE_WIDTH);
		uint numGroupsX2 = 512;
		uint numGroupsY1 = DivAndRoundUp(512, TILE_WIDTH);
		uint numGroupsY2 = 512;

		// blur in X, also commit prior to this call
		this->pointLightBlur->SelectActiveVariation(this->xBlurMask);
		this->pointLightBlur->Apply();

		// update shader for first pass
		this->pointLightBlurWrite->SetTexture(cube->GetResolveTexture());

		// commit and compute
		this->pointLightBlur->Commit();
		renderDev->Compute(numGroupsX1, numGroupsY2, 6, RenderDevice::RenderTargetAccessBarrierBits);

		// blur in Y
		this->pointLightBlur->SelectActiveVariation(this->yBlurMask);
		this->pointLightBlur->Apply();

		// commit and compute
		this->pointLightBlur->Commit();
		renderDev->Compute(numGroupsY1, numGroupsX2, 6, RenderDevice::ImageAccessBarrierBits);

		// set texture in light
		lightEntity->SetShadowCube(cube);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShadowServer::PrepareGlobalShadowBuffer()
{
	// get timer, visibility resolver and transform device
	const Ptr<TransformDevice>& transDev = TransformDevice::Instance();

	// update CSM util
	this->csmUtil.SetCameraEntity(this->cameraEntity);
	this->csmUtil.SetGlobalLight(this->globalLightEntity);
	Math::bbox box = GraphicsServer::Instance()->GetDefaultView()->GetStage()->GetGlobalBoundingBox();

	// add extension to bounding box, this ensures our ENTIRE level gets into a cascade (hopefully), but clamp the box to not be bigger than 1000, 1000, 1000
	vector extents = box.extents() + vector(10, 10, 10);
	extents = float4::clamp(extents, vector(-500), vector(500));

	box.set(point(0), extents);
	this->csmUtil.SetShadowBox(box);
	this->csmUtil.Compute();

	// update view matrix array
	matrix44 splitMatrices[CSMUtil::NumCascades];

	// render shadow casters for each view volume split
	IndexT cascadeIndex;
	for (cascadeIndex = 0; cascadeIndex < CSMUtil::NumCascades; cascadeIndex++)
	{
		splitMatrices[cascadeIndex] = this->csmUtil.GetCascadeViewProjection(cascadeIndex);
	}

	// set transforms in device
	transDev->ApplyViewMatrixArray(splitMatrices, CSMUtil::NumCascades);
}

//------------------------------------------------------------------------------
/**
    Update the parallel-split-shadow-map shadow buffers for the
    global light source.
*/
void
OGL4ShadowServer::UpdateHotGlobalShadowBuffer()
{
    n_assert(this->globalLightEntity.isvalid());
	const Ptr<VisResolver>& visResolver = VisResolver::Instance();
	const Ptr<FrameSyncTimer>& timer = FrameSyncTimer::Instance();
    IndexT frameIndex = timer->GetFrameIndex();

	// get the first view projection matrix which will resolve all visible shadow casting entities
	matrix44 cascadeViewProjection = this->csmUtil.GetCascadeViewProjection(0);

	// perform visibility resolve, directional lights don't really have a position
	// thus we're feeding an identity matrix as camera transform
	visResolver->BeginResolve(cascadeViewProjection);
	const Array<Ptr<GraphicsEntity>>& visLinks = this->globalLightEntity->GetLinks(GraphicsEntity::LightLink);
	IndexT linkIndex;
	for (linkIndex = 0; linkIndex < visLinks.Size(); linkIndex++)
	{
		const Ptr<GraphicsEntity>& curEntity = visLinks[linkIndex];
		n_assert(GraphicsEntityType::Model == curEntity->GetType());

		if (curEntity->IsA(ModelEntity::RTTI) && curEntity->GetCastsShadows())
		{
			const Ptr<ModelEntity>& modelEntity = curEntity.downcast<ModelEntity>();
            visResolver->AttachVisibleModelInstance(frameIndex, modelEntity->GetModelInstance(), false);
		}
	}
	visResolver->EndResolve();

	// render batch
    this->globalLightHotPass->Render(frameIndex);

	// render blur
    this->globalLightBlurPass->Render(frameIndex);
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShadowServer::SortLights()
{
	// @todo: sort without dictionary 
	Util::Dictionary<float, Ptr<SpotLightEntity> > sortedArray;
	sortedArray.BeginBulkAdd();
	IndexT i;
	for (i = 0; i < this->spotLightEntities.Size(); ++i)
	{
		const matrix44& lightTrans = this->spotLightEntities[i]->GetTransform();                                                                                                            
		vector vec2Poi = (float4)this->pointOfInterest - lightTrans.get_position();
		float distance = vec2Poi.length();
		float range = lightTrans.get_zaxis().length();
		float attenuation = n_saturate(1.0f - (distance / range));
		if (this->spotLightEntities[i]->IsA(SpotLightEntity::RTTI))
		{
			// consider spotlight frustum            
			if (!matrix44::ispointinside(this->pointOfInterest, this->spotLightEntities[i]->GetInvLightProjTransform()))
			{ 
				attenuation = 0.0f;
			}
		}
		attenuation = 1.0f - attenuation;
		sortedArray.Add(attenuation, this->spotLightEntities[i]);
	}
	sortedArray.EndBulkAdd();     

	// patch positions if maxnumshadowlights  = 1
	if (sortedArray.Size() > MaxNumShadowSpotLights)
	{   
		if (MaxNumShadowSpotLights == 1)
		{   
			IndexT lastShadowCastingLight = MaxNumShadowSpotLights - 1;
			const Ptr<AbstractLightEntity>& lightEntity = sortedArray.ValueAtIndex(lastShadowCastingLight).upcast<AbstractLightEntity>();
			const matrix44& shadowLightTransform = lightEntity->GetTransform();
			float range0 = shadowLightTransform.get_zaxis().length();
			vector vecPoiLight = shadowLightTransform.get_position() - this->pointOfInterest;
			vector normedPoiLight = float4::normalize(vecPoiLight);

			point interpolatedPos = shadowLightTransform.get_position();
			polar lightDir(shadowLightTransform.get_zaxis());
			float interpolatedXRot = lightDir.theta;
			float interpolatedYRot = lightDir.rho;
			SizeT numLightsInRange = 0;
			IndexT i;
			for (i = MaxNumShadowSpotLights; i < sortedArray.Size(); ++i)
			{
				const Ptr<AbstractLightEntity>& curLightEntity = sortedArray.ValueAtIndex(i).upcast<AbstractLightEntity>();
				const matrix44& curTransform = curLightEntity->GetTransform();            
				vector distVec = shadowLightTransform.get_position() - curTransform.get_position();
				float range1 = curTransform.get_zaxis().length();
				bool lerpLightTransform = (distVec.length() - range0 - range1 < 0);
				if (curLightEntity->IsA(SpotLightEntity::RTTI))
				{
					// consider spotlight frustum            
					if (!matrix44::ispointinside(shadowLightTransform.get_position(), curLightEntity->GetInvLightProjTransform()))
					{
						lerpLightTransform = false;
					}
				}
				if (lerpLightTransform)
				{                      
					float dotPoi = float4::dot3(normedPoiLight, float4::normalize(distVec));
					if (dotPoi > 0)
					{                
						// project poi - shadowlight to shadowlight next light vector
						float len0 = distVec.length();
						float projLen = float4::dot3(vecPoiLight, distVec);
						projLen /= (len0 * len0);
						// do a smoothstep
						float lerpFactor = n_smoothstep(0, 1, projLen);
						interpolatedPos = float4::lerp(interpolatedPos, curTransform.get_position(), lerpFactor);                                                      

						polar curDir(curTransform.get_zaxis());
						interpolatedXRot = n_lerp(interpolatedXRot, curDir.theta, lerpFactor);
						interpolatedYRot = n_lerp(interpolatedYRot, curDir.rho, lerpFactor);
					}                
				}            
			}       
			interpolatedPos.w() = 1.0f;
			matrix44 rotM = matrix44::rotationyawpitchroll(interpolatedYRot, interpolatedXRot - (N_PI * 0.5f), 0.0);
			matrix44 interpolatedTransform = matrix44::scaling(shadowLightTransform.get_xaxis().length(),
				shadowLightTransform.get_yaxis().length(),
				shadowLightTransform.get_zaxis().length());        
			interpolatedTransform = matrix44::multiply(interpolatedTransform, rotM);
			interpolatedTransform.set_position(interpolatedPos);
			lightEntity->SetShadowTransform(interpolatedTransform);
		}
		// more than one casting lights allowed, use shadow fading
		else
		{
			IndexT lastShadowCastingLight = MaxNumShadowSpotLights - 1;
			IndexT firstNotAllowedShadowCastingLight = MaxNumShadowSpotLights;
			// fade out shadow of casting light depended on next shadowcasting light
			float att0 = 1 - sortedArray.KeyAtIndex(lastShadowCastingLight);
			float att1 = 1 - sortedArray.KeyAtIndex(firstNotAllowedShadowCastingLight);

			float shadowIntensity = n_saturate(4.0f * att0);
			sortedArray.ValueAtIndex(lastShadowCastingLight)->SetShadowIntensity(shadowIntensity);                                                                                                                
		}
	}
	// assign sorted array
	this->spotLightEntities = sortedArray.ValuesAsArray();
}

} // namespace Lighting