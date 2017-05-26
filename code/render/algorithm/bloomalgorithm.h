#pragma once
//------------------------------------------------------------------------------
/**
	Implements a three-phase bloom algorithm
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "algorithm.h"
#include "coregraphics/shaderstate.h"
#include "renderutil/drawfullscreenquad.h"
namespace Algorithms
{
class BloomAlgorithm : public Algorithm
{
	__DeclareClass(BloomAlgorithm);
public:
	/// constructor
	BloomAlgorithm ();
	/// destructor
	virtual ~BloomAlgorithm ();

	/// setup algorithm
	void Setup();
	/// discard algorithm
	void Discard();
private:

	Ptr<CoreGraphics::Barrier> barriers[1];
	Ptr<CoreGraphics::ShaderReadWriteTexture> internalTargets[1];
	Ptr<CoreGraphics::ShaderVariable> brightPassColor, brightPassLuminance, blurInputX, blurInputY, blurOutputX, blurOutputY;
	CoreGraphics::ShaderFeature::Mask blurX, blurY;
	Ptr<CoreGraphics::ShaderState> brightPassShader;
	Ptr<CoreGraphics::ShaderState> blurShader;
	RenderUtil::DrawFullScreenQuad fsq;
};
__RegisterClass(BloomAlgorithm);
} // namespace Algorithm