//------------------------------------------------------------------------------
// hbaoalgorithm.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "hbaoalgorithm.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/shadersemantics.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/shaderrwtexture.h"
#include "coregraphics/shaderrwbuffer.h"
//#include "graphics/camerasettings.h"

using namespace CoreGraphics;
using namespace Graphics;
namespace Algorithms
{

//------------------------------------------------------------------------------
/**
*/
HBAOAlgorithm::HBAOAlgorithm()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
HBAOAlgorithm::~HBAOAlgorithm()
{
	// empty
}

#define MAX_RADIUS_PIXELS 0.5f
#define DivAndRoundUp(a, b) (a % b != 0) ? (a / b + 1) : (a / b)
//------------------------------------------------------------------------------
/**
*/
void
HBAOAlgorithm::Setup()
{
	Algorithm::Setup();
	n_assert(this->renderTextures.Size() == 1);
	n_assert(this->readWriteTextures.Size() == 1);

	CoreGraphics::ShaderRWTextureCreateInfo tinfo =
	{
		"HBAO-Internal0",
		Texture2D,
		PixelFormat::R16G16F,
		0, 0, 0,			// hard dimensions
		1, 1,				// layers and mips
		1.0f, 1.0f, 1.0f,	// scaling
		false, false, true
	};

	this->internalTargets[0] = CreateShaderRWTexture(tinfo);
	tinfo.name = "HBAO-Internal1";
	this->internalTargets[1] = CreateShaderRWTexture(tinfo);

	CoreGraphics::BarrierCreateInfo binfo =
	{
		BarrierDomain::Global,
		BarrierDependency::ComputeShader,
		BarrierDependency::ComputeShader
	};
	binfo.shaderRWTextures.Append(std::make_tuple(this->internalTargets[0], BarrierAccess::ShaderWrite, BarrierAccess::ShaderRead));
	this->barriers[0] = CreateBarrier(binfo);

	binfo.shaderRWTextures.Clear();
	binfo.shaderRWTextures.Append(std::make_tuple(this->internalTargets[1], BarrierAccess::ShaderWrite, BarrierAccess::ShaderRead));
	this->barriers[1] = CreateBarrier(binfo);

	binfo.shaderRWTextures.Clear();
	binfo.shaderRWTextures.Append(std::make_tuple(this->readWriteTextures[0], BarrierAccess::ShaderWrite, BarrierAccess::ShaderRead));
	this->barriers[2] = CreateBarrier(binfo);

	// setup shaders
	this->hbaoShader = ShaderGet("shd:hbao_cs");
	this->blurShader = ShaderGet("shd:hbaoblur_cs");
	this->hbao = ShaderCreateState(this->hbaoShader, { NEBULAT_BATCH_GROUP }, false);
	this->blur = ShaderCreateState(this->blurShader, { NEBULAT_BATCH_GROUP }, false);

	this->xDirectionHBAO = ShaderGetProgram(this->hbaoShader, ShaderFeatureFromString("Alt0"));
	this->yDirectionHBAO = ShaderGetProgram(this->hbaoShader, ShaderFeatureFromString("Alt1"));
	this->xDirectionBlur = ShaderGetProgram(this->blurShader, ShaderFeatureFromString("Alt0"));
	this->yDirectionBlur = ShaderGetProgram(this->blurShader, ShaderFeatureFromString("Alt1"));

	this->hbao0Var = ShaderStateGetConstant(this->hbao, "HBAO0");
	this->hbao1Var = ShaderStateGetConstant(this->hbao, "HBAO1");
	this->hbaoX = ShaderStateGetConstant(this->blur, "HBAOX");
	this->hbaoY = ShaderStateGetConstant(this->blur, "HBAOY");
	
	this->hbaoBlurRGVar = ShaderStateGetConstant(this->blur, "HBAORG");
	this->hbaoBlurRVar = ShaderStateGetConstant(this->blur, "HBAOR");
	
	ShaderResourceSetReadWriteTexture(this->hbaoX, this->blur, this->internalTargets[1]);
	ShaderResourceSetReadWriteTexture(this->hbaoY, this->blur, this->internalTargets[0]);

	// assign variables, HBAO0 is read-write
	ShaderResourceSetReadWriteTexture(this->hbao0Var, this->hbao, this->internalTargets[0]);
	ShaderResourceSetReadWriteTexture(this->hbao1Var, this->hbao, this->internalTargets[1]);
	ShaderResourceSetReadWriteTexture(this->hbaoBlurRGVar, this->blur, this->internalTargets[0]);
	ShaderResourceSetReadWriteTexture(this->hbaoBlurRVar, this->blur, this->readWriteTextures[0]);

	TextureDimensions dims = RenderTextureGetDimensions(this->renderTextures[0]);
	this->vars.fullWidth = (float)dims.width;
	this->vars.fullHeight = (float)dims.height;
	this->vars.radius = 12.0f;
	this->vars.downsample = 1.0f;
	this->vars.sceneScale = 1.0f;

	this->vars.maxRadiusPixels = MAX_RADIUS_PIXELS * Math::n_min(this->vars.fullWidth, this->vars.fullHeight);
	this->vars.tanAngleBias = tanf(Math::n_deg2rad(10.0));
	this->vars.strength = 2.0f;

	// setup hbao params
	this->uvToViewAVar = ShaderStateGetConstant(this->hbao, NEBULA3_SEMANTIC_UVTOVIEWA);
	this->uvToViewBVar = ShaderStateGetConstant(this->hbao, NEBULA3_SEMANTIC_UVTOVIEWB);
	this->r2Var = ShaderStateGetConstant(this->hbao, NEBULA3_SEMANTIC_R2);
	this->aoResolutionVar = ShaderStateGetConstant(this->hbao, NEBULA3_SEMANTIC_AORESOLUTION);
	this->invAOResolutionVar = ShaderStateGetConstant(this->hbao, NEBULA3_SEMANTIC_INVAORESOLUTION);
	this->strengthVar = ShaderStateGetConstant(this->hbao, NEBULA3_SEMANTIC_STRENGHT);
	this->tanAngleBiasVar = ShaderStateGetConstant(this->hbao, NEBULA3_SEMANTIC_TANANGLEBIAS);

	// setup blur params
	this->powerExponentVar = ShaderStateGetConstant(this->blur, NEBULA3_SEMANTIC_POWEREXPONENT);
	this->blurFalloff = ShaderStateGetConstant(this->blur, NEBULA3_SEMANTIC_FALLOFF);
	this->blurDepthThreshold = ShaderStateGetConstant(this->blur, NEBULA3_SEMANTIC_DEPTHTHRESHOLD);

	// calculate relevant stuff for AO
	this->AddFunction("Prepare", Algorithm::Compute, [this](IndexT)
	{
		// get camera settings
		const CameraSettings& cameraSettings = CameraGetSettings(Graphics::GraphicsServer::Instance()->GetCurrentView()->GetCamera());

		this->vars.width = this->vars.fullWidth / this->vars.downsample;
		this->vars.height = this->vars.fullHeight / this->vars.downsample;

		this->vars.nearZ = cameraSettings.GetZNear() + 0.1f;
		this->vars.farZ = cameraSettings.GetZFar();

		vars.r = this->vars.radius * 4.0f / 100.0f;
		vars.r2 = vars.r * vars.r;
		vars.negInvR2 = -1.0f / vars.r2;

		vars.aoResolution.x() = this->vars.width;
		vars.aoResolution.y() = this->vars.height;
		vars.invAOResolution.x() = 1.0f / this->vars.width;
		vars.invAOResolution.y() = 1.0f / this->vars.height;

		float fov = cameraSettings.GetFov();
		vars.focalLength.x() = 1.0f / tanf(fov * 0.5f) * (this->vars.fullHeight / this->vars.fullWidth);
		vars.focalLength.y() = 1.0f / tanf(fov * 0.5f);

		Math::float2 invFocalLength;
		invFocalLength.x() = 1 / vars.focalLength.x();
		invFocalLength.y() = 1 / vars.focalLength.y();

		vars.uvToViewA.x() = 2.0f * invFocalLength.x();
		vars.uvToViewA.y() = -2.0f * invFocalLength.y();
		vars.uvToViewB.x() = -1.0f * invFocalLength.x();
		vars.uvToViewB.y() = 1.0f * invFocalLength.y();

#ifndef INV_LN2
#define INV_LN2 1.44269504f
#endif

#ifndef SQRT_LN2
#define SQRT_LN2 0.832554611f
#endif

#define BLUR_RADIUS 33
#define BLUR_SHARPNESS 8.0f

		float blurSigma = (BLUR_RADIUS + 1) * 0.5f;
		vars.blurFalloff = INV_LN2 / (2.0f * blurSigma * blurSigma);
		vars.blurThreshold = 2.0f * SQRT_LN2 * (this->vars.sceneScale / BLUR_SHARPNESS);

		// actually set variables
		ShaderConstantSet(this->uvToViewAVar, this->hbao, this->vars.uvToViewA);
		ShaderConstantSet(this->uvToViewBVar, this->hbao, this->vars.uvToViewB);

		ShaderConstantSet(this->r2Var, this->hbao, this->vars.r2);
		ShaderConstantSet(this->aoResolutionVar, this->hbao, this->vars.aoResolution);
		ShaderConstantSet(this->invAOResolutionVar, this->hbao, this->vars.invAOResolution);
		ShaderConstantSet(this->strengthVar, this->hbao, this->vars.strength);
	});

	// calculate HBAO and blur
	this->AddFunction("HBAOAndBlur", Algorithm::Compute, [this](IndexT)
	{
		RenderDevice* renderDevice = RenderDevice::Instance();
		ShaderServer* shaderServer = ShaderServer::Instance();

#define TILE_WIDTH 320

		// get final dimensions
		SizeT width = this->vars.width;
		SizeT height = this->vars.height;

		// calculate execution dimensions
		uint numGroupsX1 = DivAndRoundUp(width, TILE_WIDTH);
		uint numGroupsX2 = width;
		uint numGroupsY1 = DivAndRoundUp(height, TILE_WIDTH);
		uint numGroupsY2 = height;

		// render AO in X
		ShaderProgramBind(this->xDirectionHBAO);
		ShaderStateApply(this->hbao);
		renderDevice->Compute(numGroupsX1, numGroupsY2, 1);

		renderDevice->InsertBarrier(this->barriers[0]);

		ShaderProgramBind(this->yDirectionHBAO);
		ShaderStateApply(this->hbao);
		renderDevice->Compute(numGroupsY1, numGroupsX2, 1);

		renderDevice->InsertBarrier(this->barriers[1]);

		ShaderProgramBind(this->xDirectionBlur);
		ShaderStateApply(this->blur);
		renderDevice->Compute(numGroupsX1, numGroupsY2, 1);

		renderDevice->InsertBarrier(this->barriers[0]);

		ShaderProgramBind(this->yDirectionBlur);
		ShaderStateApply(this->blur);
		renderDevice->Compute(numGroupsY1, numGroupsX2, 1);

		//renderDevice->InsertBarrier(this->barriers[2]);
	});
}

//------------------------------------------------------------------------------
/**
*/
void
HBAOAlgorithm::Discard()
{
	Algorithm::Discard();
	ShaderDestroyState(this->hbao);
	ShaderDestroyState(this->blur);
}

} // namespace Algorithms