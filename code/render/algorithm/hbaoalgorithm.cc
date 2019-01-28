//------------------------------------------------------------------------------
// hbaoalgorithm.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "hbaoalgorithm.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/shadersemantics.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/shaderrwtexture.h"
#include "coregraphics/shaderrwbuffer.h"
#include "graphics/graphicsserver.h"
#include "graphics/cameracontext.h"
#include "graphics/view.h"

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
		CoreGraphicsImageLayout::ShaderRead,
		1.0f, 1.0f, 1.0f,
		1, 1,				// layers and mips
		false, true, false
	};

	this->internalTargets[0] = CreateShaderRWTexture(tinfo);
	tinfo.name = "HBAO-Internal1";
	this->internalTargets[1] = CreateShaderRWTexture(tinfo);

	CoreGraphics::BarrierCreateInfo binfo =
	{
		BarrierDomain::Global,
		BarrierStage::ComputeShader,
		BarrierStage::ComputeShader
	};
	ImageSubresourceInfo subres;
	subres.aspect = CoreGraphicsImageAspect::ColorBits;

	// hbao generation barriers
	binfo.shaderRWTextures.Append(std::make_tuple(this->internalTargets[0], subres, CoreGraphicsImageLayout::ShaderRead, CoreGraphicsImageLayout::General, BarrierAccess::ShaderRead, BarrierAccess::ShaderWrite));
	binfo.shaderRWTextures.Append(std::make_tuple(this->internalTargets[1], subres, CoreGraphicsImageLayout::ShaderRead, CoreGraphicsImageLayout::General, BarrierAccess::ShaderRead, BarrierAccess::ShaderRead));
	this->barriers[0] = CreateBarrier(binfo);
	binfo.shaderRWTextures.Clear();

	binfo.shaderRWTextures.Append(std::make_tuple(this->internalTargets[0], subres, CoreGraphicsImageLayout::General, CoreGraphicsImageLayout::General, BarrierAccess::ShaderWrite, BarrierAccess::ShaderRead));
	binfo.shaderRWTextures.Append(std::make_tuple(this->internalTargets[1], subres, CoreGraphicsImageLayout::General, CoreGraphicsImageLayout::General, BarrierAccess::ShaderRead, BarrierAccess::ShaderWrite));
	this->barriers[1] = CreateBarrier(binfo);
	binfo.shaderRWTextures.Clear();

	// hbao blur barriers
	binfo.shaderRWTextures.Append(std::make_tuple(this->internalTargets[1], subres, CoreGraphicsImageLayout::General, CoreGraphicsImageLayout::ShaderRead, BarrierAccess::ShaderWrite, BarrierAccess::ShaderRead));
	binfo.shaderRWTextures.Append(std::make_tuple(this->internalTargets[0], subres, CoreGraphicsImageLayout::General, CoreGraphicsImageLayout::General, BarrierAccess::ShaderRead, BarrierAccess::ShaderWrite));
	this->barriers[2] = CreateBarrier(binfo);
	binfo.shaderRWTextures.Clear();

	binfo.shaderRWTextures.Append(std::make_tuple(this->internalTargets[0], subres, CoreGraphicsImageLayout::General, CoreGraphicsImageLayout::ShaderRead, BarrierAccess::ShaderWrite, BarrierAccess::ShaderRead));
	this->barriers[3] = CreateBarrier(binfo);
	binfo.shaderRWTextures.Clear();

	// setup shaders
	this->hbaoShader = ShaderGet("shd:hbao_cs.fxb");
	this->blurShader = ShaderGet("shd:hbaoblur_cs.fxb");
	this->hbaoTable = ShaderCreateResourceTable(this->hbaoShader, NEBULA_BATCH_GROUP);
	this->blurTableX = ShaderCreateResourceTable(this->blurShader, NEBULA_BATCH_GROUP);
	this->blurTableY = ShaderCreateResourceTable(this->blurShader, NEBULA_BATCH_GROUP);
	this->hbao0 = ShaderGetResourceSlot(this->hbaoShader, "HBAO0");
	this->hbao1 = ShaderGetResourceSlot(this->hbaoShader, "HBAO1");
	this->hbaoC = ShaderGetResourceSlot(this->hbaoShader, "HBAOBlock");
	this->hbaoX = ShaderGetResourceSlot(this->blurShader, "HBAOX");
	this->hbaoY = ShaderGetResourceSlot(this->blurShader, "HBAOY");
	this->hbaoBlurRG = ShaderGetResourceSlot(this->blurShader, "HBAORG");
	this->hbaoBlurR = ShaderGetResourceSlot(this->blurShader, "HBAOR");
	this->blurC = ShaderGetResourceSlot(this->blurShader, "HBAOBlur");

	this->xDirectionHBAO = ShaderGetProgram(this->hbaoShader, ShaderFeatureFromString("Alt0"));
	this->yDirectionHBAO = ShaderGetProgram(this->hbaoShader, ShaderFeatureFromString("Alt1"));
	this->xDirectionBlur = ShaderGetProgram(this->blurShader, ShaderFeatureFromString("Alt0"));
	this->yDirectionBlur = ShaderGetProgram(this->blurShader, ShaderFeatureFromString("Alt1"));

	this->hbaoConstants = ShaderCreateConstantBuffer(this->hbaoShader, "HBAOBlock");
	this->blurConstants = ShaderCreateConstantBuffer(this->blurShader, "HBAOBlur");

	// setup hbao table
	ResourceTableSetShaderRWTexture(this->hbaoTable, { this->internalTargets[0], this->hbao0, 0, CoreGraphics::SamplerId::Invalid() });
	ResourceTableSetShaderRWTexture(this->hbaoTable, { this->internalTargets[1], this->hbao1, 0, CoreGraphics::SamplerId::Invalid() });
	ResourceTableSetConstantBuffer(this->hbaoTable, { this->hbaoConstants, this->hbaoC, 0, false, false, -1, 0 });
	ResourceTableCommitChanges(this->hbaoTable);

	// setup blur table
	ResourceTableSetTexture(this->blurTableX, { this->internalTargets[1], this->hbaoX, 0, CoreGraphics::SamplerId::Invalid() });
	ResourceTableSetShaderRWTexture(this->blurTableX, { this->internalTargets[0], this->hbaoBlurRG, 0, CoreGraphics::SamplerId::Invalid() });

	ResourceTableSetTexture(this->blurTableY, { this->internalTargets[0], this->hbaoY, 0, CoreGraphics::SamplerId::Invalid() });
	ResourceTableSetShaderRWTexture(this->blurTableY, { this->readWriteTextures[0], this->hbaoBlurR, 0, CoreGraphics::SamplerId::Invalid() });

	ResourceTableSetConstantBuffer(this->blurTableX, { this->blurConstants, this->blurC, 0, false, false, -1, 0 });
	ResourceTableSetConstantBuffer(this->blurTableY, { this->blurConstants, this->blurC, 0, false, false, -1, 0 });
	ResourceTableCommitChanges(this->blurTableX);
	ResourceTableCommitChanges(this->blurTableY);

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
	this->uvToViewAVar = ShaderGetConstantBinding(this->hbaoShader, NEBULA_SEMANTIC_UVTOVIEWA);
	this->uvToViewBVar = ShaderGetConstantBinding(this->hbaoShader, NEBULA_SEMANTIC_UVTOVIEWB);
	this->r2Var = ShaderGetConstantBinding(this->hbaoShader, NEBULA_SEMANTIC_R2);
	this->aoResolutionVar = ShaderGetConstantBinding(this->hbaoShader, NEBULA_SEMANTIC_AORESOLUTION);
	this->invAOResolutionVar = ShaderGetConstantBinding(this->hbaoShader, NEBULA_SEMANTIC_INVAORESOLUTION);
	this->strengthVar = ShaderGetConstantBinding(this->hbaoShader, NEBULA_SEMANTIC_STRENGHT);
	this->tanAngleBiasVar = ShaderGetConstantBinding(this->hbaoShader, NEBULA_SEMANTIC_TANANGLEBIAS);

	// setup blur params
	this->powerExponentVar = ShaderGetConstantBinding(this->blurShader, NEBULA_SEMANTIC_POWEREXPONENT);
	this->blurFalloff = ShaderGetConstantBinding(this->blurShader, NEBULA_SEMANTIC_FALLOFF);
	this->blurDepthThreshold = ShaderGetConstantBinding(this->blurShader, NEBULA_SEMANTIC_DEPTHTHRESHOLD);

	// calculate relevant stuff for AO
	this->AddFunction("Prepare", Algorithm::Compute, [this](IndexT)
	{

#if NEBULA_GRAPHICS_DEBUG
		//CoreGraphics::QueueInsertMarker(GraphicsQueueType, Math::float4(0.6f, 0.4f, 0.0f, 1.0f), "HBAO Preparation step");
#endif
		// get camera settings
		const CameraSettings& cameraSettings = CameraContext::GetSettings(Graphics::GraphicsServer::Instance()->GetCurrentView()->GetCamera());

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

		ConstantBufferUpdate(this->hbaoConstants, this->vars.uvToViewA, this->uvToViewAVar);
		ConstantBufferUpdate(this->hbaoConstants, this->vars.uvToViewB, this->uvToViewBVar);

		ConstantBufferUpdate(this->hbaoConstants, this->vars.r2, this->r2Var);
		ConstantBufferUpdate(this->hbaoConstants, this->vars.aoResolution, this->aoResolutionVar);
		ConstantBufferUpdate(this->hbaoConstants, this->vars.invAOResolution, this->invAOResolutionVar);
		ConstantBufferUpdate(this->hbaoConstants, this->vars.strength, this->strengthVar);

		ConstantBufferUpdate(this->blurConstants, this->vars.blurFalloff, this->blurFalloff);
		ConstantBufferUpdate(this->blurConstants, this->vars.blurThreshold, this->blurDepthThreshold);
		ConstantBufferUpdate(this->blurConstants, 1.5f, this->powerExponentVar);
	});

	// calculate HBAO and blur
	this->AddFunction("HBAOAndBlur", Algorithm::Compute, [this](IndexT)
	{
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

		// we are running the SSAO on the graphics queue
#if NEBULA_GRAPHICS_DEBUG
		CoreGraphics::CmdBufBeginMarker(GraphicsQueueType, NEBULA_MARKER_ORANGE, "HBAO");
#endif

		// render AO in X
		CoreGraphics::InsertBarrier(this->barriers[0], GraphicsQueueType); // transition from shader read to general
		CoreGraphics::SetShaderProgram(this->xDirectionHBAO);
		CoreGraphics::SetResourceTable(this->hbaoTable, NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
		CoreGraphics::Compute(numGroupsX1, numGroupsY2, 1);		
		CoreGraphics::InsertBarrier(this->barriers[1], GraphicsQueueType); // transition from shader read to general

		// now do it in Y
		CoreGraphics::InsertBarrier(this->barriers[1], GraphicsQueueType); // transition from shader read to general
		CoreGraphics::SetShaderProgram(this->yDirectionHBAO);
		CoreGraphics::SetResourceTable(this->hbaoTable, NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
		CoreGraphics::Compute(numGroupsY1, numGroupsX2, 1);
		
		// blur in X
		CoreGraphics::InsertBarrier(this->barriers[2], GraphicsQueueType); // transition from shader read to general
		CoreGraphics::SetShaderProgram(this->xDirectionBlur);
		CoreGraphics::SetResourceTable(this->blurTableX, NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
		CoreGraphics::Compute(numGroupsX1, numGroupsY2, 1);

		// blur in Y
		CoreGraphics::InsertBarrier(this->barriers[3], GraphicsQueueType);
		CoreGraphics::SetShaderProgram(this->yDirectionBlur);
		CoreGraphics::SetResourceTable(this->blurTableY, NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
		CoreGraphics::Compute(numGroupsY1, numGroupsX2, 1);

#if NEBULA_GRAPHICS_DEBUG
		CoreGraphics::CmdBufEndMarker(GraphicsQueueType);
#endif
	});
}

//------------------------------------------------------------------------------
/**
*/
void
HBAOAlgorithm::Discard()
{
	Algorithm::Discard();
	CoreGraphics::DestroyShaderRWTexture(internalTargets[0]);
	CoreGraphics::DestroyShaderRWTexture(internalTargets[1]);
	DestroyConstantBuffer(this->hbaoConstants);
	DestroyConstantBuffer(this->blurConstants);
	DestroyResourceTable(this->hbaoTable);
	DestroyResourceTable(this->blurTableX);
	DestroyResourceTable(this->blurTableY);
}

} // namespace Algorithms