#pragma once
//------------------------------------------------------------------------------
/**
	Implements tonemapping as a script algorithm
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "algorithm.h"
#include "coregraphics/rendertexture.h"
#include "coregraphics/shader.h"
#include "coregraphics/resourcetable.h"
#include "renderutil/drawfullscreenquad.h"
namespace Algorithms
{
class TonemapAlgorithm : public Algorithm
{
public:
	/// constructor
	TonemapAlgorithm();
	/// destructor
	virtual ~TonemapAlgorithm();

	/// setup algorithm
	void Setup();
	/// discard algorithm
	void Discard();

private:

	CoreGraphics::RenderTextureId downsample2x2;
	CoreGraphics::RenderTextureId copy;

	CoreGraphics::ShaderId shader;
	CoreGraphics::ResourceTableId tonemapTable;
	IndexT constantsSlot, colorSlot, prevSlot;

	CoreGraphics::ShaderProgramId program;

	CoreGraphics::ConstantBinding timevar;
	CoreGraphics::ConstantBufferId constants;
	RenderUtil::DrawFullScreenQuad fsq;
};

} // namespace Algorithms
