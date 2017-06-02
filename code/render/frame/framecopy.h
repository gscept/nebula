#pragma once
//------------------------------------------------------------------------------
/**
	Performs an image copy without any filtering or image conversion.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "coregraphics/rendertexture.h"
namespace Frame
{
class FrameCopy : public FrameOp
{
	__DeclareClass(FrameCopy);
public:
	/// constructor
	FrameCopy();
	/// destructor
	virtual ~FrameCopy();

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
FrameCopy::SetFromTexture(const Ptr<CoreGraphics::RenderTexture>& from)
{
	this->from = from;
}

//------------------------------------------------------------------------------
/**
*/
inline void
FrameCopy::SetToTexture(const Ptr<CoreGraphics::RenderTexture>& to)
{
	this->to = to;
}

} // namespace Frame2