//------------------------------------------------------------------------------
// ssaoplugin.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "ssaoplugin.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/shadersemantics.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/shaderrwbuffer.h"
#include "graphics/graphicsserver.h"
#include "graphics/cameracontext.h"
#include "graphics/view.h"

#include "hbao_cs.h"
#include "hbaoblur_cs.h"

using namespace CoreGraphics;
using namespace Graphics;
namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
SSAOPlugin::SSAOPlugin()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
SSAOPlugin::~SSAOPlugin()
{
	// empty
}

#define MAX_RADIUS_PIXELS 0.5f
//------------------------------------------------------------------------------
/**
*/
void
SSAOPlugin::Setup()
{
	FramePlugin::Setup();

	CoreGraphics::TextureCreateInfo tinfo; 
	tinfo.name = "HBAO-Internal0"_atm;
	tinfo.tag = "system"_atm;
	tinfo.type = Texture2D;
	tinfo.format = PixelFormat::R16G16F;
	tinfo.windowRelative = true;
	tinfo.usage = TextureUsage::ReadWriteUsage;

	this->internalTargets[0] = CreateTexture(tinfo);
	tinfo.name = "HBAO-Internal1";
	this->internalTargets[1] = CreateTexture(tinfo);

	CoreGraphics::BarrierCreateInfo binfo =
	{
		""_atm,
		BarrierDomain::Global,
		BarrierStage::ComputeShader,
		BarrierStage::ComputeShader
	};
	ImageSubresourceInfo subres;
	subres.aspect = CoreGraphics::ImageAspect::ColorBits;

	// hbao generation barriers
	binfo.name = "HBAO Initial transition";
	binfo.textures.Append(TextureBarrier{ this->internalTargets[0], subres, CoreGraphics::ImageLayout::ShaderRead, CoreGraphics::ImageLayout::General, BarrierAccess::ShaderRead, BarrierAccess::ShaderWrite });
	binfo.textures.Append(TextureBarrier{ this->internalTargets[1], subres, CoreGraphics::ImageLayout::ShaderRead, CoreGraphics::ImageLayout::General, BarrierAccess::ShaderRead, BarrierAccess::ShaderWrite });
	this->barriers[0] = CreateBarrier(binfo);
	binfo.textures.Clear();

	binfo.name = "HBAO Transition Pass 0 -> 1";
	binfo.textures.Append(TextureBarrier{this->internalTargets[0], subres, CoreGraphics::ImageLayout::General, CoreGraphics::ImageLayout::General, BarrierAccess::ShaderWrite, BarrierAccess::ShaderRead});
	binfo.textures.Append(TextureBarrier{this->internalTargets[1], subres, CoreGraphics::ImageLayout::General, CoreGraphics::ImageLayout::General, BarrierAccess::ShaderRead, BarrierAccess::ShaderWrite});
	this->barriers[1] = CreateBarrier(binfo);
	binfo.textures.Clear();

	// hbao blur barriers
	binfo.name = "HBAO Transition to Blur";
	binfo.textures.Append(TextureBarrier{ this->internalTargets[1], subres, CoreGraphics::ImageLayout::General, CoreGraphics::ImageLayout::ShaderRead, BarrierAccess::ShaderWrite, BarrierAccess::ShaderRead });
	binfo.textures.Append(TextureBarrier{ this->internalTargets[0], subres, CoreGraphics::ImageLayout::General, CoreGraphics::ImageLayout::General, BarrierAccess::ShaderRead, BarrierAccess::ShaderWrite });
	this->barriers[2] = CreateBarrier(binfo);
	binfo.textures.Clear();

	binfo.name = "HBAO Transition to Blur Pass 0 -> 1";
	binfo.textures.Append(TextureBarrier{this->internalTargets[0], subres, CoreGraphics::ImageLayout::General, CoreGraphics::ImageLayout::ShaderRead, BarrierAccess::ShaderWrite, BarrierAccess::ShaderRead});
	binfo.textures.Append(TextureBarrier{this->internalTargets[1], subres, CoreGraphics::ImageLayout::ShaderRead, CoreGraphics::ImageLayout::General, BarrierAccess::ShaderRead, BarrierAccess::ShaderWrite});
	this->barriers[3] = CreateBarrier(binfo);
	binfo.textures.Clear();

	// setup shaders
	this->hbaoShader = ShaderGet("shd:hbao_cs.fxb");
	this->blurShader = ShaderGet("shd:hbaoblur_cs.fxb");
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

	this->hbaoConstants = CoreGraphics::GetGraphicsConstantBuffer(MainThreadConstantBuffer);
	this->blurConstants = CoreGraphics::GetGraphicsConstantBuffer(MainThreadConstantBuffer);

	SizeT numBuffers = CoreGraphics::GetNumBufferedFrames();
	this->hbaoTable.Resize(numBuffers);
	this->blurTableX.Resize(numBuffers);
	this->blurTableY.Resize(numBuffers);
	IndexT i;
	for (i = 0; i < numBuffers; i++)
	{
		this->hbaoTable[i] = ShaderCreateResourceTable(this->hbaoShader, NEBULA_BATCH_GROUP);
		this->blurTableX[i] = ShaderCreateResourceTable(this->blurShader, NEBULA_BATCH_GROUP);
		this->blurTableY[i] = ShaderCreateResourceTable(this->blurShader, NEBULA_BATCH_GROUP);

		// setup hbao table
		ResourceTableSetRWTexture(this->hbaoTable[i], { this->internalTargets[0], this->hbao0, 0, CoreGraphics::SamplerId::Invalid() });
		ResourceTableSetRWTexture(this->hbaoTable[i], { this->internalTargets[1], this->hbao1, 0, CoreGraphics::SamplerId::Invalid() });
		ResourceTableCommitChanges(this->hbaoTable[i]);

		// setup blur table
		ResourceTableSetTexture(this->blurTableX[i], { this->internalTargets[1], this->hbaoX, 0, CoreGraphics::SamplerId::Invalid() });
		ResourceTableSetRWTexture(this->blurTableX[i], { this->internalTargets[0], this->hbaoBlurRG, 0, CoreGraphics::SamplerId::Invalid() });
		ResourceTableSetTexture(this->blurTableY[i], { this->internalTargets[0], this->hbaoY, 0, CoreGraphics::SamplerId::Invalid() });
		ResourceTableSetRWTexture(this->blurTableY[i], { this->textures["SSAO"], this->hbaoBlurR, 0, CoreGraphics::SamplerId::Invalid() });
		ResourceTableCommitChanges(this->blurTableX[i]);
		ResourceTableCommitChanges(this->blurTableY[i]);
	}

	TextureDimensions dims = TextureGetDimensions(this->textures["SSAO"]);
	this->vars.fullWidth = (float)dims.width;
	this->vars.fullHeight = (float)dims.height;
	this->vars.radius = 12.0f;
	this->vars.downsample = 1.0f;
	this->vars.sceneScale = 1.0f;

	this->vars.maxRadiusPixels = MAX_RADIUS_PIXELS * Math::n_min(this->vars.fullWidth, this->vars.fullHeight);
	this->vars.tanAngleBias = tanf(Math::n_deg2rad(15.0));
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
	FramePlugin::AddCallback("HBAO-Prepare", [this](IndexT)
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

		HbaoCs::HBAOBlock hbaoBlock;
		hbaoBlock.AOResolution[0] = this->vars.aoResolution.x();
		hbaoBlock.AOResolution[1] = this->vars.aoResolution.y();
		hbaoBlock.InvAOResolution[0] = this->vars.invAOResolution.x();
		hbaoBlock.InvAOResolution[1] = this->vars.invAOResolution.y();
		hbaoBlock.R2 = this->vars.r2;
		hbaoBlock.Strength = this->vars.strength;
		hbaoBlock.TanAngleBias = this->vars.tanAngleBias;
		hbaoBlock.UVToViewA[0] = this->vars.uvToViewA.x();
		hbaoBlock.UVToViewA[1] = this->vars.uvToViewA.y();
		hbaoBlock.UVToViewB[0] = this->vars.uvToViewB.x();
		hbaoBlock.UVToViewB[1] = this->vars.uvToViewB.y();
		uint hbaoOffset = CoreGraphics::SetGraphicsConstants(MainThreadConstantBuffer, hbaoBlock);

		IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();

		ResourceTableSetConstantBuffer(this->hbaoTable[bufferIndex], { this->hbaoConstants, this->hbaoC, 0, false, false, sizeof(HbaoCs::HBAOBlock), (SizeT)hbaoOffset });
		ResourceTableCommitChanges(this->hbaoTable[bufferIndex]);

		HbaoblurCs::HBAOBlur blurBlock;
		blurBlock.BlurFalloff = this->vars.blurFalloff;
		blurBlock.BlurDepthThreshold = this->vars.blurThreshold;
		blurBlock.PowerExponent = 1.5f;
		uint blurOffset = CoreGraphics::SetGraphicsConstants(MainThreadConstantBuffer, blurBlock);

		ResourceTableSetConstantBuffer(this->blurTableX[bufferIndex], { this->blurConstants, this->blurC, 0, false, false, sizeof(HbaoblurCs::HBAOBlur), (SizeT)blurOffset });
		ResourceTableSetConstantBuffer(this->blurTableY[bufferIndex], { this->blurConstants, this->blurC, 0, false, false, sizeof(HbaoblurCs::HBAOBlur), (SizeT)blurOffset });
		ResourceTableCommitChanges(this->blurTableX[bufferIndex]);
		ResourceTableCommitChanges(this->blurTableY[bufferIndex]);
	});

	// calculate HBAO and blur
	FramePlugin::AddCallback("HBAO-Run", [this](IndexT)
	{
		ShaderServer* shaderServer = ShaderServer::Instance();

#define TILE_WIDTH 320

		// get final dimensions
		SizeT width = this->vars.width;
		SizeT height = this->vars.height;

		// calculate execution dimensions
		uint numGroupsX1 = Math::n_divandroundup(width, TILE_WIDTH);
		uint numGroupsX2 = width;
		uint numGroupsY1 = Math::n_divandroundup(height, TILE_WIDTH);
		uint numGroupsY2 = height;

		IndexT bufferIndex = CoreGraphics::GetBufferedFrameIndex();

		// we are running the SSAO on the graphics queue
#if NEBULA_GRAPHICS_DEBUG
		CoreGraphics::CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLUE, "HBAO");
#endif

		// render AO in X
		CoreGraphics::SetShaderProgram(this->xDirectionHBAO);
		CoreGraphics::SetResourceTable(this->hbaoTable[bufferIndex], NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
		CoreGraphics::BarrierInsert(this->barriers[0], GraphicsQueueType); // transition from shader read to general
		CoreGraphics::Compute(numGroupsX1, numGroupsY2, 1);

		// now do it in Y
		CoreGraphics::SetShaderProgram(this->yDirectionHBAO);
		CoreGraphics::SetResourceTable(this->hbaoTable[bufferIndex], NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
		CoreGraphics::BarrierInsert(this->barriers[1], GraphicsQueueType); // transition from shader read to general
		CoreGraphics::Compute(numGroupsY1, numGroupsX2, 1);
		
		// blur in X
		CoreGraphics::SetShaderProgram(this->xDirectionBlur);
		CoreGraphics::SetResourceTable(this->blurTableX[bufferIndex], NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
		CoreGraphics::BarrierInsert(this->barriers[2], GraphicsQueueType); // transition from shader read to general
		CoreGraphics::Compute(numGroupsX1, numGroupsY2, 1);

		// blur in Y
		CoreGraphics::SetShaderProgram(this->yDirectionBlur);
		CoreGraphics::SetResourceTable(this->blurTableY[bufferIndex], NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
		CoreGraphics::BarrierInsert(this->barriers[3], GraphicsQueueType);
		CoreGraphics::Compute(numGroupsY1, numGroupsX2, 1);

#if NEBULA_GRAPHICS_DEBUG
		CoreGraphics::CommandBufferEndMarker(GraphicsQueueType);
#endif
	});
}

//------------------------------------------------------------------------------
/**
*/
void
SSAOPlugin::Discard()
{
	FramePlugin::Discard();
	CoreGraphics::DestroyTexture(internalTargets[0]);
	CoreGraphics::DestroyTexture(internalTargets[1]);
	IndexT i;
	for (i = 0; i < this->hbaoTable.Size(); i++)
	{
		DestroyResourceTable(this->hbaoTable[i]);
		DestroyResourceTable(this->blurTableX[i]);
		DestroyResourceTable(this->blurTableY[i]);
	}
	
}

} // namespace Algorithms