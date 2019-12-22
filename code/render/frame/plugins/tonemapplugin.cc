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

	TextureCreateInfo rtinfo;
	rtinfo.name = "Tonemapping-Downsample2x2"_atm;
	rtinfo.type = Texture2D;
	rtinfo.format = TextureGetPixelFormat(this->textures["Color"]);
	rtinfo.width = 2;
	rtinfo.height = 2;
	rtinfo.usage = TextureUsage::CopyUsage;
	this->downsample2x2 = CreateTexture(rtinfo);

	rtinfo.name = "Tonemapping-Copy";
	rtinfo.width = 1;
	rtinfo.height = 1;
	rtinfo.format = TextureGetPixelFormat(this->textures["Luminance"]);
	rtinfo.usage = TextureUsage::CopyUsage;
	this->copy = CreateTexture(rtinfo);

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

	this->fsq.Setup(1, 1);

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
				  TextureBarrier{ this->downsample2x2, ImageSubresourceInfo{CoreGraphicsImageAspect::ColorBits, 0, 1, 0, 1}, CoreGraphicsImageLayout::ShaderRead, CoreGraphicsImageLayout::TransferDestination, CoreGraphics::BarrierAccess::ShaderRead, CoreGraphics::BarrierAccess::TransferWrite }
			},
			nullptr, 
			"Tonemapping Downscale Begin");

		CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(this->textures["Color"]);
		CoreGraphics::Blit(this->textures["Color"], Math::rectangle<int>(0, 0, dims.width, dims.height), 0, this->downsample2x2, Math::rectangle<int>(0, 0, 2, 2), 0);

		CoreGraphics::BarrierInsert(
			GraphicsQueueType,
			CoreGraphics::BarrierStage::Transfer,
			CoreGraphics::BarrierStage::PixelShader,
			CoreGraphics::BarrierDomain::Global,
			{
				  TextureBarrier{ this->downsample2x2, ImageSubresourceInfo{CoreGraphicsImageAspect::ColorBits, 0, 1, 0, 1}, CoreGraphicsImageLayout::TransferDestination, CoreGraphicsImageLayout::ShaderRead, CoreGraphics::BarrierAccess::TransferWrite, CoreGraphics::BarrierAccess::ShaderRead }
			},
			nullptr,
			"Tonemapping Downscale End");
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
				  TextureBarrier{ this->copy, ImageSubresourceInfo::ColorNoMipNoLayer(),	CoreGraphicsImageLayout::ShaderRead, CoreGraphicsImageLayout::TransferDestination, CoreGraphics::BarrierAccess::ShaderRead, CoreGraphics::BarrierAccess::TransferWrite }
			},
			nullptr,
			"Tonemapping Copy Last Frame Begin");

		CoreGraphics::Copy(this->textures["Luminance"], Math::rectangle<int>(0, 0, 1, 1), this->copy, Math::rectangle<int>(0, 0, 1, 1));

		CoreGraphics::BarrierInsert(
			GraphicsQueueType,
			CoreGraphics::BarrierStage::Transfer,
			CoreGraphics::BarrierStage::PixelShader,
			CoreGraphics::BarrierDomain::Global,
			{
				  TextureBarrier{ this->copy, ImageSubresourceInfo::ColorNoMipNoLayer(), CoreGraphicsImageLayout::TransferDestination, CoreGraphicsImageLayout::ShaderRead, CoreGraphics::BarrierAccess::TransferWrite, CoreGraphics::BarrierAccess::ShaderRead }
			},
			nullptr,
			"Tonemapping Copy Last Frame End");

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
	DestroyTexture(this->downsample2x2);
	DestroyTexture(this->copy);
	DestroyConstantBuffer(this->constants);
	DestroyResourceTable(this->tonemapTable);
	this->fsq.Discard();
}

} // namespace Algorithms
