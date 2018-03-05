#pragma once
//------------------------------------------------------------------------------
/**
	Implements a signal for swapping backbuffers for window targets
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "coregraphics/rendertexture.h"
namespace Frame
{
class FrameSwapbuffers : public Frame::FrameOp
{
	__DeclareClass(FrameSwapbuffers);
public:
	/// constructor
	FrameSwapbuffers();
	/// destructor
	virtual ~FrameSwapbuffers();

	/// discard operation
	void Discard();
	/// run operation
	void Run(const IndexT frameIndex);

	CoreGraphics::RenderTextureId tex;
};

} // namespace Frame2