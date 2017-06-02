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

	/// set texture
	void SetTexture(const Ptr<CoreGraphics::RenderTexture>& tex);

	/// discard operation
	void Discard();
	/// run operation
	void Run(const IndexT frameIndex);
private:
	Ptr<CoreGraphics::RenderTexture> tex;
};

//------------------------------------------------------------------------------
/**
*/
inline void
FrameSwapbuffers::SetTexture(const Ptr<CoreGraphics::RenderTexture>& tex)
{
	this->tex = tex;
}

} // namespace Frame2