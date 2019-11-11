#pragma once
//------------------------------------------------------------------------------
/**
	Implements a three-phase bloom algorithm
	
	(C) 2017-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameplugin.h"
#include "coregraphics/shader.h"
#include "coregraphics/resourcetable.h"
#include "renderutil/drawfullscreenquad.h"
#include "coregraphics/barrier.h"
namespace Frame
{
class BloomPlugin : public FramePlugin
{
public:
	/// constructor
	BloomPlugin();
	/// destructor
	virtual ~BloomPlugin();

	/// setup algorithm
	void Setup();
	/// discard algorithm
	void Discard();
private:

	CoreGraphics::ShaderRWTextureId internalTargets[1];
	CoreGraphics::ShaderProgramId brightPassProgram;
	CoreGraphics::ShaderProgramId blurX, blurY;
	CoreGraphics::ShaderId brightPassShader, blurShader;

	CoreGraphics::ResourceTableId brightPassTable, blurTable;
	IndexT colorSourceSlot, luminanceTextureSlot, inputImageXSlot, inputImageYSlot, blurImageXSlot, blurImageYSlot;
	
	RenderUtil::DrawFullScreenQuad fsq;
};
} // namespace Algorithm