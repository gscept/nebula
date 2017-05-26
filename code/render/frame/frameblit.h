#pragma once
//------------------------------------------------------------------------------
/**
	A frame blit performs an image copy with optional filtering and image type conversions.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "coregraphics/rendertexture.h"
namespace Frame2
{
class FrameBlit : public FrameOp
{
	__DeclareClass(FrameBlit);
public:
	/// constructor
	FrameBlit();
	/// destructor
	virtual ~FrameBlit();

	/// set texture to copy from
	void SetFromTexture(const Ptr<CoreGraphics::RenderTexture>& from);
	/// set texture to copy to
	void SetToTexture(const Ptr<CoreGraphics::RenderTexture>& to);

	/// discard operation
	void Discard();
	/// run operation
	void Run(const IndexT frameIndex);
private:
	Ptr<CoreGraphics::RenderTexture> from;
	Ptr<CoreGraphics::RenderTexture> to;
};

//------------------------------------------------------------------------------
/**
*/
inline void
FrameBlit::SetFromTexture(const Ptr<CoreGraphics::RenderTexture>& from)
{
	this->from = from;
}

//------------------------------------------------------------------------------
/**
*/
inline void
FrameBlit::SetToTexture(const Ptr<CoreGraphics::RenderTexture>& to)
{
	this->to = to;
}

} // namespace Frame2