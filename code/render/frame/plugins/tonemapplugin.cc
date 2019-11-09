//------------------------------------------------------------------------------
// tonemapplugin.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "tonemapplugin.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/graphicsdevice.h"
#include "framesync/framesynctimer.h"

using namespace CoreGraphics;
namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
TonemapPlugin::TonemapPlugin()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
TonemapPlugin::~TonemapPlugin()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
TonemapPlugin::Setup()
{
	FramePlugin::Setup();
	n_assert(this->renderTextures.Size() == 2);

	RenderTextureCreateInfo rtinfo = 
	{
		"Tonemapping-Downsample2x2",
		Texture2D,
		RenderTextureGetPixelFormat(this->renderTextures[0]),
		ColorAttachment,
		2, 2, 1,
		1, 1,
		false, false, false
	};
	this->downsample2x2 = CreateRenderTexture(rtinfo);

	rtinfo.name = "Tonemapping-Copy";
	rtinfo.width = 1;
	rtinfo.height = 1;
	rtinfo.format = RenderTextureGetPixelFormat(this->renderTextures[1]);
	this->copy = CreateRenderTexture(rtinfo);

	// create shader
	this->shader = ShaderGet("shd:averagelum.fxb");
	this->tonemapTable = ShaderCreateResourceTable(this->shader, NEBULA_BATCH_GROUP);
	this->constants = ShaderCreateConstantBuffer(this->shader, "AverageLumBlock");
	this->timevar = ShaderGetConstantBinding(this->shader, "TimeDiff");
	this->colorSlot = ShaderGetResourceSlot(this->shader, "ColorSource");
	this->prevSlot = ShaderGetResourceSlot(this->shader, "PreviousLum");
	this->constantsSlot = ShaderGetResourceSlot(this->shader, "AverageLumBlock");
	this->program = ShaderGetProgram(this->shader, ShaderFeatureFromString("Alt0"));
	ResourceTableSetConstantBuffer(this->tonemapTable, { this->constants, this->constantsSlot, 0, false, false, -1, 0});
	ResourceTableSetTexture(this->tonemapTable, { this->copy, this->prevSlot, 0, SamplerId::Invalid(), false });
	ResourceTableSetTexture(this->tonemapTable, { this->downsample2x2, this->colorSlot, 0, SamplerId::Invalid(), false });
	ResourceTableCommitChanges(this->tonemapTable);

	this->fsq.Setup(2, 2);

	// begin by copying and mipping down to a 2x2 texture
	FramePlugin::AddCallback("Tonemap-Downsample", [this](IndexT)
	{
#if NEBULA_GRAPHICS_DEBUG
		CoreGraphics::CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_RED, "Tonemapping Downsample");
#endif
		CoreGraphics::BarrierInsert(
			GraphicsQueueType,
			CoreGraphics::BarrierStage::PixelShader,
			CoreGraphics::BarrierStage::Transfer,
			CoreGraphics::BarrierDomain::Global,
			{ 
				  RenderTextureBarrier{ this->downsample2x2, ImageSubresourceInfo{CoreGraphicsImageAspect::ColorBits, 0, 1, 0, 1}, CoreGraphicsImageLayout::ShaderRead, CoreGraphicsImageLayout::TransferDestination, CoreGraphics::BarrierAccess::ShaderRead, CoreGraphics::BarrierAccess::TransferWrite }
			},
			nullptr, 
			nullptr, 
			"Tonemapping Downscale Before Barrier");
		CoreGraphics::Blit(this->renderTextures[0], Math::rectangle<int>(0, 0, 512, 512), 0, this->downsample2x2, Math::rectangle<int>(0, 0, 2, 2), 0);

		CoreGraphics::BarrierInsert(
			GraphicsQueueType,
			CoreGraphics::BarrierStage::Transfer,
			CoreGraphics::BarrierStage::PixelShader,
			CoreGraphics::BarrierDomain::Global,
			{
				  RenderTextureBarrier{ this->downsample2x2, ImageSubresourceInfo{CoreGraphicsImageAspect::ColorBits, 0, 1, 0, 1}, CoreGraphicsImageLayout::TransferDestination, CoreGraphicsImageLayout::ShaderRead, CoreGraphics::BarrierAccess::TransferWrite, CoreGraphics::BarrierAccess::ShaderRead }
			},
			nullptr,
			nullptr,
			"Tonemapping Downscale After Barrier");
#if NEBULA_GRAPHICS_DEBUG
		CoreGraphics::CommandBufferEndMarker(GraphicsQueueType);
#endif
	});

	// this pass calculates tonemapping from 2x2 cluster down to single pixel, called from the script
	FramePlugin::AddCallback("Tonemap-AverageLum", [this](IndexT)
	{
#if NEBULA_GRAPHICS_DEBUG
		CoreGraphics::CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLUE, "Tonemapping Average Luminance");
#endif

		Timing::Time time = FrameSync::FrameSyncTimer::Instance()->GetFrameTime();
		CoreGraphics::SetShaderProgram(this->program);
		CoreGraphics::BeginBatch(Frame::FrameBatchType::System);
		this->fsq.ApplyMesh();
		ConstantBufferUpdate(this->constants, (float)time, this->timevar);
		CoreGraphics::SetResourceTable(this->tonemapTable, NEBULA_BATCH_GROUP, CoreGraphics::GraphicsPipeline, nullptr);
		this->fsq.Draw();
		CoreGraphics::EndBatch();

#if NEBULA_GRAPHICS_DEBUG
		CoreGraphics::CommandBufferEndMarker(GraphicsQueueType);
#endif
	});

	// last pass, copy from render target to copy
	FramePlugin::AddCallback("Tonemap-Copy", [this](IndexT)
	{
#if NEBULA_GRAPHICS_DEBUG
		CoreGraphics::CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_RED, "Tonemapping Copy Previous Frame");
#endif
		CoreGraphics::BarrierInsert(
			GraphicsQueueType,
			CoreGraphics::BarrierStage::PixelShader,
			CoreGraphics::BarrierStage::Transfer,
			CoreGraphics::BarrierDomain::Global,
			{
				  RenderTextureBarrier{ this->copy, ImageSubresourceInfo::ColorNoMipNoLayer(),	CoreGraphicsImageLayout::ShaderRead, CoreGraphicsImageLayout::TransferDestination, CoreGraphics::BarrierAccess::ShaderRead, CoreGraphics::BarrierAccess::TransferWrite }
			},
			nullptr,
			nullptr,
			"Tonemapping Copy Last Frame Before Barrier");

		CoreGraphics::Copy(this->renderTextures[1], Math::rectangle<int>(0, 0, 1, 1), this->copy, Math::rectangle<int>(0, 0, 1, 1));

		CoreGraphics::BarrierInsert(
			GraphicsQueueType,
			CoreGraphics::BarrierStage::Transfer,
			CoreGraphics::BarrierStage::PixelShader,
			CoreGraphics::BarrierDomain::Global,
			{
				  RenderTextureBarrier{ this->copy, ImageSubresourceInfo::ColorNoMipNoLayer(), CoreGraphicsImageLayout::TransferDestination, CoreGraphicsImageLayout::ShaderRead, CoreGraphics::BarrierAccess::TransferWrite, CoreGraphics::BarrierAccess::ShaderRead }
			},
			nullptr,
			nullptr,
			"Tonemapping Copy Last Frame After Barrier");

#if NEBULA_GRAPHICS_DEBUG
		CoreGraphics::CommandBufferEndMarker(GraphicsQueueType);
#endif
	});
}

//------------------------------------------------------------------------------
/**
*/
void
TonemapPlugin::Discard()
{
	FramePlugin::Discard();
	DestroyRenderTexture(this->downsample2x2);
	DestroyRenderTexture(this->copy);
	DestroyConstantBuffer(this->constants);
	DestroyResourceTable(this->tonemapTable);
	this->fsq.Discard();
}

} // namespace Algorithms
