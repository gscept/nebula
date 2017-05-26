#pragma once
//------------------------------------------------------------------------------
/**
	Implements tonemapping as a script algorithm
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "algorithm.h"
#include "coregraphics/rendertexture.h"
#include "coregraphics/shaderstate.h"
#include "renderutil/drawfullscreenquad.h"
namespace Algorithms
{
class TonemapAlgorithm : public Algorithm
{
	__DeclareClass(TonemapAlgorithm);
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

	Ptr<CoreGraphics::RenderTexture> downsample2x2;
	Ptr<CoreGraphics::RenderTexture> copy;
	Ptr<CoreGraphics::ShaderState> shader;
	Ptr<CoreGraphics::ShaderVariable> timevar;
	Ptr<CoreGraphics::ShaderVariable> colorvar;
	Ptr<CoreGraphics::ShaderVariable> prevvar;
	RenderUtil::DrawFullScreenQuad fsq;
};
__RegisterClass(TonemapAlgorithm);

} // namespace Algorithms
