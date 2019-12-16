//------------------------------------------------------------------------------
// bloomplugin.cc
// (C) 2017-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "bloomplugin.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/barrier.h"

using namespace CoreGraphics;
namespace Frame
{
//------------------------------------------------------------------------------
/**
*/
BloomPlugin::BloomPlugin()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
BloomPlugin::~BloomPlugin()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
BloomPlugin::Setup()
{
	FramePlugin::Setup();

	// setup shaders
	this->brightPassShader = ShaderGet("shd:brightpass.fxb");
	this->blurShader = ShaderGet("shd:blur_2d_rgb16f_cs.fxb");
	this->brightPassTable = ShaderCreateResourceTable(this->brightPassShader, NEBULA_BATCH_GROUP);
	this->blurTable = ShaderCreateResourceTable(this->blurShader, NEBULA_BATCH_GROUP);
	
	this->colorSourceSlot = ShaderGetResourceSlot(this->brightPassShader, "ColorSource");
	this->luminanceTextureSlot = ShaderGetResourceSlot(this->brightPassShader, "LuminanceTexture");
	this->inputImageXSlot = ShaderGetResourceSlot(this->blurShader, "InputImageX");
	this->inputImageYSlot = ShaderGetResourceSlot(this->blurShader, "InputImageY");
	this->blurImageXSlot = ShaderGetResourceSlot(this->blurShader, "BlurImageX");
	this->blurImageYSlot = ShaderGetResourceSlot(this->blurShader, "BlurImageY");

	TextureDimensions dims = TextureGetDimensions(this->textures["BloomBuffer"]);
	TextureCreateInfo tinfo;
	tinfo.name = "Bloom-Internal0";
	tinfo.type = Texture2D;
	tinfo.format = CoreGraphics::PixelFormat::R16G16B16A16F;
	tinfo.width = dims.width;
	tinfo.height = dims.height;
	tinfo.usage = TextureUsage::ReadWriteUsage;
	this->internalTargets[0] = CreateTexture(tinfo);

	ResourceTableSetTexture(this->brightPassTable, { this->textures["LightBuffer"], this->colorSourceSlot, 0, CoreGraphics::SamplerId::Invalid() });
	ResourceTableSetTexture(this->brightPassTable, { this->textures["BloomBuffer"], this->luminanceTextureSlot, 0, CoreGraphics::SamplerId::Invalid() , false });
	ResourceTableCommitChanges(this->brightPassTable);

	// bloom buffer goes in, internal target goes out
	ResourceTableSetTexture(this->blurTable, { this->textures["BloomBuffer"], this->inputImageXSlot, 0, CoreGraphics::SamplerId::Invalid() , false });
	ResourceTableSetRWTexture(this->blurTable, { this->internalTargets[0], this->blurImageXSlot, 0, CoreGraphics::SamplerId::Invalid() });

	// internal target goes in, blurred buffer goes out
	ResourceTableSetTexture(this->blurTable, { this->internalTargets[0], this->inputImageYSlot, 0, CoreGraphics::SamplerId::Invalid() });
	ResourceTableSetRWTexture(this->blurTable, { this->textures["BloomBufferBlurred"], this->blurImageYSlot, 0, CoreGraphics::SamplerId::Invalid() });
	ResourceTableCommitChanges(this->blurTable);

	this->blurX = ShaderGetProgram(this->blurShader, ShaderFeatureFromString("Alt0"));
	this->blurY = ShaderGetProgram(this->blurShader, ShaderFeatureFromString("Alt1"));
	this->brightPassProgram = ShaderGetProgram(this->brightPassShader, ShaderFeatureFromString("Alt0"));

	dims = TextureGetDimensions(this->textures["BloomBuffer"]);

	// get size of target texture
	this->fsq.Setup(dims.width, dims.height);

	FramePlugin::AddCallback("Bloom-BrightnessLowpass", [this](IndexT)
		{
#if NEBULA_GRAPHICS_DEBUG
			CoreGraphics::CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_ORANGE, "BrightnessLowpass");
#endif
			CoreGraphics::SetShaderProgram(this->brightPassProgram);
			CoreGraphics::BeginBatch(Frame::FrameBatchType::System);
			this->fsq.ApplyMesh();
			CoreGraphics::SetResourceTable(this->brightPassTable, NEBULA_BATCH_GROUP, CoreGraphics::GraphicsPipeline, nullptr);
			this->fsq.Draw();
			CoreGraphics::EndBatch();
#if NEBULA_GRAPHICS_DEBUG
			CoreGraphics::CommandBufferEndMarker(GraphicsQueueType);
#endif
		});

	FramePlugin::AddCallback("Bloom-Blur", [this, dims](IndexT)
	{
#if NEBULA_GRAPHICS_DEBUG
		CoreGraphics::CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLUE, "BloomBlur");
#endif

#define TILE_WIDTH 320
#define DivAndRoundUp(a, b) (a % b != 0) ? (a / b + 1) : (a / b)

		// calculate execution dimensions
		uint numGroupsX1 = DivAndRoundUp(dims.width, TILE_WIDTH);
		uint numGroupsX2 = dims.width;
		uint numGroupsY1 = DivAndRoundUp(dims.height, TILE_WIDTH);
		uint numGroupsY2 = dims.height;

		CoreGraphics::BarrierInsert(
			GraphicsQueueType,
			CoreGraphics::BarrierStage::ComputeShader,
			CoreGraphics::BarrierStage::ComputeShader,
			CoreGraphics::BarrierDomain::Global,
			{
				  TextureBarrier{ this->internalTargets[0], ImageSubresourceInfo::ColorNoMipNoLayer(), CoreGraphicsImageLayout::ShaderRead, CoreGraphicsImageLayout::General, CoreGraphics::BarrierAccess::ShaderWrite, CoreGraphics::BarrierAccess::ShaderRead }
			},
			nullptr,
			"Bloom Blur Pass #1 Barrier");

		CoreGraphics::SetShaderProgram(this->blurX);
		CoreGraphics::SetResourceTable(this->blurTable, NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
		CoreGraphics::Compute(numGroupsX1, numGroupsY2, 1);

		CoreGraphics::BarrierInsert(
			GraphicsQueueType,
			CoreGraphics::BarrierStage::ComputeShader,
			CoreGraphics::BarrierStage::ComputeShader,
			CoreGraphics::BarrierDomain::Global,
			{
				  TextureBarrier{ this->internalTargets[0], ImageSubresourceInfo::ColorNoMipNoLayer(), CoreGraphicsImageLayout::General, CoreGraphicsImageLayout::ShaderRead, CoreGraphics::BarrierAccess::ShaderRead, CoreGraphics::BarrierAccess::ShaderWrite }
			},
			nullptr,
			"Bloom Blur Pass #2 Barrier");

		CoreGraphics::SetShaderProgram(this->blurY);
		CoreGraphics::SetResourceTable(this->blurTable, NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
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
BloomPlugin::Discard()
{
	DestroyResourceTable(this->brightPassTable);
	DestroyResourceTable(this->blurTable);
	DestroyTexture(this->internalTargets[0]);
	this->fsq.Discard();
}

} // namespace Algorithm