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
	CoreGraphics::ShaderStateId tonemapShader;
	CoreGraphics::ShaderProgramId program;

	CoreGraphics::ShaderConstantId timevar;
	CoreGraphics::ShaderConstantId colorvar;
	CoreGraphics::ShaderConstantId prevvar;
	RenderUtil::DrawFullScreenQuad fsq;
};

} // namespace Algorithms
