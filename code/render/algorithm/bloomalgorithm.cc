//------------------------------------------------------------------------------
// bloomalgorithm.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "bloomalgorithm.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/shaderrwtexture.h"
#include "coregraphics/barrier.h"

using namespace CoreGraphics;
namespace Algorithms
{
//------------------------------------------------------------------------------
/**
*/
BloomAlgorithm::BloomAlgorithm()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
BloomAlgorithm::~BloomAlgorithm()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
BloomAlgorithm::Setup()
{
	Algorithm::Setup();
	n_assert(this->renderTextures.Size() == 3);
	n_assert(this->readWriteTextures.Size() == 1);

	// setup shaders
	this->brightPassShader = ShaderGet("shd:brightpass.fxb");
	this->blurShader = ShaderGet("shd:blur_2d_rgb16f_cs.fxb");
	this->brightPassTable = ShaderCreateResourceTable(this->brightPassShader, NEBULAT_BATCH_GROUP);
	this->blurTable = ShaderCreateResourceTable(this->blurShader, NEBULAT_BATCH_GROUP);
	
	this->colorSourceSlot = ShaderGetResourceSlot(this->brightPassShader, "ColorSource");
	this->luminanceTextureSlot = ShaderGetResourceSlot(this->brightPassShader, "LuminanceTexture");
	this->inputImageXSlot = ShaderGetResourceSlot(this->blurShader, "InputImageX");
	this->inputImageYSlot = ShaderGetResourceSlot(this->blurShader, "InputImageY");
	this->blurImageXSlot = ShaderGetResourceSlot(this->blurShader, "BlurImageX");
	this->blurImageYSlot = ShaderGetResourceSlot(this->blurShader, "BlurImageY");

	TextureDimensions dims = ShaderRWTextureGetDimensions(this->readWriteTextures[0]);
	ShaderRWTextureCreateInfo tinfo = 
	{
		"Bloom-Internal0",
		Texture2D,
		CoreGraphics::PixelFormat::R16G16B16A16F,
		dims.width, dims.height, dims.depth,
		1, 1,
		false, false
	};
	this->internalTargets[0] = CreateShaderRWTexture(tinfo);

	CoreGraphics::BarrierCreateInfo binfo =
	{
		BarrierDomain::Global,
		BarrierStage::ComputeShader,
		BarrierStage::ComputeShader
	};
	ImageSubresourceInfo subres;
	subres.aspect = ImageAspect::ColorBits;
	
	binfo.shaderRWTextures.Append(std::make_tuple(this->internalTargets[0], subres, ImageLayout::General, ImageLayout::General, BarrierAccess::ShaderWrite, BarrierAccess::ShaderRead));
	this->barriers[0] = CreateBarrier(binfo);

	ResourceTableSetTexture(this->brightPassTable, { this->renderTextures[0], this->colorSourceSlot, 0, CoreGraphics::SamplerId::Invalid() , false });
	ResourceTableSetTexture(this->brightPassTable, { this->renderTextures[1], this->luminanceTextureSlot, 0, CoreGraphics::SamplerId::Invalid() , false });
	ResourceTableCommitChanges(this->brightPassTable);

	ResourceTableSetTexture(this->blurTable, { this->renderTextures[2], this->inputImageXSlot, 0, CoreGraphics::SamplerId::Invalid() , false });
	ResourceTableSetTexture(this->blurTable, { this->internalTargets[0], this->inputImageYSlot, 0, CoreGraphics::SamplerId::Invalid() });
	ResourceTableSetShaderRWTexture(this->blurTable, { this->internalTargets[0], this->blurImageXSlot, 0, CoreGraphics::SamplerId::Invalid() });
	ResourceTableSetShaderRWTexture(this->blurTable, { this->readWriteTextures[0], this->blurImageYSlot, 0, CoreGraphics::SamplerId::Invalid() });
	ResourceTableCommitChanges(this->blurTable);

	this->blurX = ShaderGetProgram(this->blurShader, ShaderFeatureFromString("Alt0"));
	this->blurY = ShaderGetProgram(this->blurShader, ShaderFeatureFromString("Alt1"));
	this->brightPassProgram = ShaderGetProgram(this->brightPassShader, ShaderFeatureFromString("Alt0"));

	dims = ShaderRWTextureGetDimensions(this->readWriteTextures[0]);

	// get size of target texture
	this->fsq.Setup(dims.width, dims.height);

	this->AddFunction("BrightnessLowpass", Algorithm::Graphics, [this](IndexT)
	{
		CoreGraphics::SetShaderProgram(this->brightPassProgram);
		CoreGraphics::BeginBatch(Frame::FrameBatchType::System);
		this->fsq.ApplyMesh();
		CoreGraphics::SetResourceTable(this->brightPassTable, NEBULAT_BATCH_GROUP, CoreGraphics::GraphicsPipeline, nullptr);
		this->fsq.Draw();
		CoreGraphics::EndBatch();
	});

	this->AddFunction("BrightnessBlur", Algorithm::Compute, [this, dims](IndexT)
	{

#define TILE_WIDTH 320
#define DivAndRoundUp(a, b) (a % b != 0) ? (a / b + 1) : (a / b)

		// calculate execution dimensions
		uint numGroupsX1 = DivAndRoundUp(dims.width, TILE_WIDTH);
		uint numGroupsX2 = dims.width;
		uint numGroupsY1 = DivAndRoundUp(dims.height, TILE_WIDTH);
		uint numGroupsY2 = dims.height;

		CoreGraphics::SetShaderProgram(this->blurX);
		CoreGraphics::SetResourceTable(this->blurTable, NEBULAT_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
		CoreGraphics::Compute(numGroupsX1, numGroupsY2, 1);

		// insert barrier between passes
		CoreGraphics::InsertBarrier(this->barriers[0], ComputeQueueType);

		CoreGraphics::SetShaderProgram(this->blurY);
		CoreGraphics::SetResourceTable(this->blurTable, NEBULAT_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
		CoreGraphics::Compute(numGroupsY1, numGroupsX2, 1);

	});
}

//------------------------------------------------------------------------------
/**
*/
void
BloomAlgorithm::Discard()
{
	DestroyResourceTable(this->brightPassTable);
	DestroyResourceTable(this->blurTable);
	DestroyBarrier(this->barriers[0]);
	DestroyShaderRWTexture(this->internalTargets[0]);
	this->fsq.Discard();
}

} // namespace Algorithm