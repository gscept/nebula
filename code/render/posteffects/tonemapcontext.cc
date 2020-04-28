//------------------------------------------------------------------------------
//  tonemapcontext.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "renderutil/drawfullscreenquad.h"
#include "frame/frameplugin.h"
#include "tonemapcontext.h"
namespace PostEffects
{

_ImplementPluginContext(PostEffects::TonemapContext);

struct
{
	CoreGraphics::TextureId downsample2x2;
	CoreGraphics::TextureId copy;

	CoreGraphics::ShaderId shader;
	CoreGraphics::ResourceTableId tonemapTable;
	IndexT constantsSlot, colorSlot, prevSlot;

	CoreGraphics::ShaderProgramId program;

	CoreGraphics::ConstantBinding timevar;
	CoreGraphics::ConstantBufferId constants;
	RenderUtil::DrawFullScreenQuad fsq;

	CoreGraphics::TextureId colorBuffer;
	CoreGraphics::TextureId averageLumBuffer;
} tonemapState;

//------------------------------------------------------------------------------
/**
*/
TonemapContext::TonemapContext()
{
}

//------------------------------------------------------------------------------
/**
*/
TonemapContext::~TonemapContext()
{
}

//------------------------------------------------------------------------------
/**
*/
void 
TonemapContext::Create()
{
	_CreatePluginContext();

	using namespace CoreGraphics;

	// begin by copying and mipping down to a 2x2 texture
	Frame::AddCallback("Tonemap-Downsample", [](IndexT)
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
					  TextureBarrier{ tonemapState.downsample2x2, ImageSubresourceInfo{CoreGraphics::ImageAspect::ColorBits, 0, 1, 0, 1}, CoreGraphics::ImageLayout::ShaderRead, CoreGraphics::ImageLayout::TransferDestination, CoreGraphics::BarrierAccess::ShaderRead, CoreGraphics::BarrierAccess::TransferWrite }
				},
				nullptr,
				"Tonemapping Downscale Begin");

			CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(tonemapState.colorBuffer);
			CoreGraphics::Blit(tonemapState.colorBuffer, Math::rectangle<int>(0, 0, dims.width, dims.height), 0, tonemapState.downsample2x2, Math::rectangle<int>(0, 0, 2, 2), 0);

			CoreGraphics::BarrierInsert(
				GraphicsQueueType,
				CoreGraphics::BarrierStage::Transfer,
				CoreGraphics::BarrierStage::PixelShader,
				CoreGraphics::BarrierDomain::Global,
				{
					  TextureBarrier{ tonemapState.downsample2x2, ImageSubresourceInfo{CoreGraphics::ImageAspect::ColorBits, 0, 1, 0, 1}, CoreGraphics::ImageLayout::TransferDestination, CoreGraphics::ImageLayout::ShaderRead, CoreGraphics::BarrierAccess::TransferWrite, CoreGraphics::BarrierAccess::ShaderRead }
				},
				nullptr,
				"Tonemapping Downscale End");
#if NEBULA_GRAPHICS_DEBUG
			CoreGraphics::CommandBufferEndMarker(GraphicsQueueType);
#endif
		});

	// this pass calculates tonemapping from 2x2 cluster down to single pixel, called from the script
	Frame::AddCallback("Tonemap-AverageLum", [](IndexT)
		{
#if NEBULA_GRAPHICS_DEBUG
			CoreGraphics::CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLUE, "Tonemapping Average Luminance");
#endif

			Timing::Time time = FrameSync::FrameSyncTimer::Instance()->GetFrameTime();
			CoreGraphics::SetShaderProgram(tonemapState.program);
			CoreGraphics::BeginBatch(Frame::FrameBatchType::System);
			tonemapState.fsq.ApplyMesh();
			ConstantBufferUpdate(tonemapState.constants, (float)time, tonemapState.timevar);
			CoreGraphics::SetResourceTable(tonemapState.tonemapTable, NEBULA_BATCH_GROUP, CoreGraphics::GraphicsPipeline, nullptr);
			tonemapState.fsq.Draw();
			CoreGraphics::EndBatch();

#if NEBULA_GRAPHICS_DEBUG
			CoreGraphics::CommandBufferEndMarker(GraphicsQueueType);
#endif
		});

	// last pass, copy from render target to copy
	Frame::AddCallback("Tonemap-Copy", [](IndexT)
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
					  TextureBarrier{ tonemapState.copy, ImageSubresourceInfo::ColorNoMipNoLayer(),	CoreGraphics::ImageLayout::ShaderRead, CoreGraphics::ImageLayout::TransferDestination, CoreGraphics::BarrierAccess::ShaderRead, CoreGraphics::BarrierAccess::TransferWrite }
				},
				nullptr,
				"Tonemapping Copy Last Frame Begin");

			CoreGraphics::Copy(tonemapState.averageLumBuffer, Math::rectangle<int>(0, 0, 1, 1), tonemapState.copy, Math::rectangle<int>(0, 0, 1, 1));

			CoreGraphics::BarrierInsert(
				GraphicsQueueType,
				CoreGraphics::BarrierStage::Transfer,
				CoreGraphics::BarrierStage::PixelShader,
				CoreGraphics::BarrierDomain::Global,
				{
					  TextureBarrier{ tonemapState.copy, ImageSubresourceInfo::ColorNoMipNoLayer(), CoreGraphics::ImageLayout::TransferDestination, CoreGraphics::ImageLayout::ShaderRead, CoreGraphics::BarrierAccess::TransferWrite, CoreGraphics::BarrierAccess::ShaderRead }
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
TonemapContext::Discard()
{
	using namespace CoreGraphics;
	DestroyTexture(tonemapState.downsample2x2);
	DestroyTexture(tonemapState.copy);
	DestroyConstantBuffer(tonemapState.constants);
	DestroyResourceTable(tonemapState.tonemapTable);
	tonemapState.fsq.Discard();
}

//------------------------------------------------------------------------------
/**
*/
void 
TonemapContext::Setup(const Ptr<Frame::FrameScript>& script)
{
	using namespace CoreGraphics;
	tonemapState.colorBuffer = script->GetTexture("LightBuffer");
	tonemapState.averageLumBuffer = script->GetTexture("AverageLumBuffer");

	TextureCreateInfo rtinfo;
	rtinfo.name = "Tonemapping-Downsample2x2"_atm;
	rtinfo.type = Texture2D;
	rtinfo.format = TextureGetPixelFormat(tonemapState.colorBuffer);
	rtinfo.width = 2;
	rtinfo.height = 2;
	rtinfo.usage = TextureUsage::CopyUsage;
	tonemapState.downsample2x2 = CreateTexture(rtinfo);

	rtinfo.name = "Tonemapping-Copy";
	rtinfo.width = 1;
	rtinfo.height = 1;
	rtinfo.format = TextureGetPixelFormat(tonemapState.averageLumBuffer);
	rtinfo.usage = TextureUsage::CopyUsage;
	tonemapState.copy = CreateTexture(rtinfo);

	// create shader
	tonemapState.shader = ShaderGet("shd:averagelum.fxb");
	tonemapState.tonemapTable = ShaderCreateResourceTable(tonemapState.shader, NEBULA_BATCH_GROUP);
	tonemapState.constants = ShaderCreateConstantBuffer(tonemapState.shader, "AverageLumBlock");
	tonemapState.timevar = ShaderGetConstantBinding(tonemapState.shader, "TimeDiff");
	tonemapState.colorSlot = ShaderGetResourceSlot(tonemapState.shader, "ColorSource");
	tonemapState.prevSlot = ShaderGetResourceSlot(tonemapState.shader, "PreviousLum");
	tonemapState.constantsSlot = ShaderGetResourceSlot(tonemapState.shader, "AverageLumBlock");
	tonemapState.program = ShaderGetProgram(tonemapState.shader, ShaderFeatureFromString("Alt0"));
	ResourceTableSetConstantBuffer(tonemapState.tonemapTable, { tonemapState.constants, tonemapState.constantsSlot, 0, false, false, -1, 0 });
	ResourceTableSetTexture(tonemapState.tonemapTable, { tonemapState.copy, tonemapState.prevSlot, 0, SamplerId::Invalid(), false });
	ResourceTableSetTexture(tonemapState.tonemapTable, { tonemapState.downsample2x2, tonemapState.colorSlot, 0, SamplerId::Invalid(), false });
	ResourceTableCommitChanges(tonemapState.tonemapTable);

	tonemapState.fsq.Setup(1, 1);

}
} // namespace PostEffects
