//------------------------------------------------------------------------------
// hbaoalgorithm.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "hbaoalgorithm.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/shadersemantics.h"
#include "coregraphics/renderdevice.h"
#include "graphics/camerasettings.h"

using namespace CoreGraphics;
using namespace Graphics;
namespace Algorithms
{

__ImplementClass(Algorithms::HBAOAlgorithm, 'HBAO', Algorithms::Algorithm);
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

	this->internalTargets[0] = ShaderReadWriteTexture::Create();
	this->internalTargets[0]->SetupWithRelativeSize(1.0f, 1.0f, PixelFormat::R16G16F, "HBAO-Internal0");

	this->internalTargets[1] = ShaderReadWriteTexture::Create();
	this->internalTargets[1]->SetupWithRelativeSize(1.0f, 1.0f, PixelFormat::R16G16F, "HBAO-Internal1");

	this->barriers[0] = Barrier::Create();
	this->barriers[0]->SetLeftDependency(Barrier::Dependency::ComputeShader);
	this->barriers[0]->SetRightDependency(Barrier::Dependency::ComputeShader);
	this->barriers[0]->SetDomain(Barrier::Domain::Global);
	this->barriers[0]->AddReadWriteTexture(this->internalTargets[0], Barrier::Access::ShaderWrite, Barrier::Access::ShaderRead);
	this->barriers[0]->Setup();

	this->barriers[1] = Barrier::Create();
	this->barriers[1]->SetLeftDependency(Barrier::Dependency::ComputeShader);
	this->barriers[1]->SetRightDependency(Barrier::Dependency::ComputeShader);
	this->barriers[1]->SetDomain(Barrier::Domain::Global);
	this->barriers[1]->AddReadWriteTexture(this->internalTargets[1], Barrier::Access::ShaderWrite, Barrier::Access::ShaderRead);
	this->barriers[1]->Setup();

	this->barriers[2] = Barrier::Create();
	this->barriers[2]->SetLeftDependency(Barrier::Dependency::ComputeShader);
	this->barriers[2]->SetRightDependency(Barrier::Dependency::ComputeShader);
	this->barriers[2]->SetDomain(Barrier::Domain::Global);
	this->barriers[2]->AddReadWriteTexture(this->readWriteTextures[0], Barrier::Access::ShaderWrite, Barrier::Access::ShaderRead);
	this->barriers[2]->Setup();

	// setup shaders
	this->hbao = CoreGraphics::ShaderServer::Instance()->CreateShaderState("shd:hbao_cs", { NEBULAT_DEFAULT_GROUP });
	this->blur = CoreGraphics::ShaderServer::Instance()->CreateShaderState("shd:hbaoblur_cs", { NEBULAT_DEFAULT_GROUP });

	this->xDirection = CoreGraphics::ShaderServer::Instance()->FeatureStringToMask("Alt0");
	this->yDirection = CoreGraphics::ShaderServer::Instance()->FeatureStringToMask("Alt1");

	this->hbao0Var = this->hbao->GetVariableByName("HBAO0");
	this->hbao1Var = this->hbao->GetVariableByName("HBAO1");
	this->hbaoX = this->blur->GetVariableByName("HBAOX");
	this->hbaoY = this->blur->GetVariableByName("HBAOY");
	
	this->hbaoBlurRGVar = this->blur->GetVariableByName("HBAORG");
	this->hbaoBlurRVar = this->blur->GetVariableByName("HBAOR");

	this->hbaoX->SetTexture(this->internalTargets[1]->GetTexture());
	this->hbaoY->SetTexture(this->internalTargets[0]->GetTexture());

	// assign variables, HBAO0 is read-write
	this->hbao0Var->SetShaderReadWriteTexture(this->internalTargets[0]->GetTexture());
	this->hbao1Var->SetShaderReadWriteTexture(this->internalTargets[1]->GetTexture());
	this->hbaoBlurRGVar->SetShaderReadWriteTexture(this->internalTargets[0]->GetTexture());
	this->hbaoBlurRVar->SetShaderReadWriteTexture(this->readWriteTextures[0]->GetTexture());

	this->vars.fullWidth = (float)this->renderTextures[0]->GetTexture()->GetWidth();
	this->vars.fullHeight = (float)this->renderTextures[0]->GetTexture()->GetHeight();
	this->vars.radius = 12.0f;
	this->vars.downsample = 1.0f;
	this->vars.sceneScale = 1.0f;

	this->vars.maxRadiusPixels = MAX_RADIUS_PIXELS * Math::n_min(this->vars.fullWidth, this->vars.fullHeight);
	this->vars.tanAngleBias = tanf(Math::n_deg2rad(10.0));
	this->vars.strength = 2.0f;

	//this->depthTextureVar = this->hbao->GetVariableByName("DepthBuffer");
	this->uvToViewAVar = this->hbao->GetVariableByName(NEBULA3_SEMANTIC_UVTOVIEWA);
	this->uvToViewBVar = this->hbao->GetVariableByName(NEBULA3_SEMANTIC_UVTOVIEWB);
	this->r2Var = this->hbao->GetVariableByName(NEBULA3_SEMANTIC_R2);
	this->aoResolutionVar = this->hbao->GetVariableByName(NEBULA3_SEMANTIC_AORESOLUTION);
	this->invAOResolutionVar = this->hbao->GetVariableByName(NEBULA3_SEMANTIC_INVAORESOLUTION);
	this->strengthVar = this->hbao->GetVariableByName(NEBULA3_SEMANTIC_STRENGHT);
	this->tanAngleBiasVar = this->hbao->GetVariableByName(NEBULA3_SEMANTIC_TANANGLEBIAS);
	this->powerExponentVar = this->blur->GetVariableByName(NEBULA3_SEMANTIC_POWEREXPONENT);
	this->blurFalloff = this->blur->GetVariableByName(NEBULA3_SEMANTIC_FALLOFF);
	this->blurDepthThreshold = this->blur->GetVariableByName(NEBULA3_SEMANTIC_DEPTHTHRESHOLD);

	// calculate relevant stuff for AO
	this->AddFunction("Prepare", Algorithm::Compute, [this](IndexT)
	{
		// get camera settings
		const CameraSettings& cameraSettings = Graphics::GraphicsServer::Instance()->GetCurrentView()->GetCameraEntity()->GetCameraSettings();

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
		this->uvToViewAVar->SetFloat2(this->vars.uvToViewA);
		this->uvToViewBVar->SetFloat2(this->vars.uvToViewB);

		this->r2Var->SetFloat(this->vars.r2);
		this->aoResolutionVar->SetFloat2(this->vars.aoResolution);
		this->invAOResolutionVar->SetFloat2(this->vars.invAOResolution);
		this->strengthVar->SetFloat(this->vars.strength);
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
		this->hbao->SelectActiveVariation(this->xDirection);
		this->hbao->Apply();
		this->hbao->Commit();
		renderDevice->Compute(numGroupsX1, numGroupsY2, 1);

		//renderDevice->InsertBarrier(this->barriers[0]);

		this->hbao->SelectActiveVariation(this->yDirection);
		this->hbao->Apply();
		renderDevice->Compute(numGroupsY1, numGroupsX2, 1);

		//renderDevice->InsertBarrier(this->barriers[1]);

		this->blur->SelectActiveVariation(this->xDirection);
		this->blur->Apply();
		this->blur->Commit();
		renderDevice->Compute(numGroupsX1, numGroupsY2, 1);

		//renderDevice->InsertBarrier(this->barriers[0]);

		this->blur->SelectActiveVariation(this->yDirection);
		this->blur->Apply();
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
	this->hbao->Discard();
	this->blur->Discard();
	this->hbao0Var = nullptr;
	this->hbao1Var = nullptr;
	this->hbaoBlurRGVar = nullptr;
	this->hbaoBlurRVar = nullptr;
	this->hbaoX = nullptr;
	this->hbaoY = nullptr;
	this->uvToViewAVar = nullptr;
	this->uvToViewBVar = nullptr;
	this->aoResolutionVar = nullptr;
	this->invAOResolutionVar = nullptr;
	this->strengthVar = nullptr;
	this->tanAngleBiasVar = nullptr;
	this->powerExponentVar = nullptr;
	this->blurFalloff = nullptr;
	this->blurDepthThreshold = nullptr;
}

} // namespace Algorithms