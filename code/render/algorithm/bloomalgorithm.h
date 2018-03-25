#pragma once
//------------------------------------------------------------------------------
/**
	Implements a three-phase bloom algorithm
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "algorithm.h"
#include "coregraphics/shader.h"
#include "renderutil/drawfullscreenquad.h"
#include "coregraphics/barrier.h"
namespace Algorithms
{
class BloomAlgorithm : public Algorithm
{
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

	CoreGraphics::BarrierId barriers[1];
	CoreGraphics::ShaderRWTextureId internalTargets[1];
	CoreGraphics::ShaderConstantId brightPassColor, brightPassLuminance, blurInputX, blurInputY, blurOutputX, blurOutputY;
	CoreGraphics::ShaderProgramId brightPassProgram;
	CoreGraphics::ShaderProgramId blurX, blurY;
	CoreGraphics::ShaderId brightPassShader, blurShader;
	CoreGraphics::ShaderStateId brightPass, blur;
	
	RenderUtil::DrawFullScreenQuad fsq;
};
} // namespace Algorithm