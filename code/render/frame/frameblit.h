#pragma once
//------------------------------------------------------------------------------
/**
	A frame blit performs an image copy with optional filtering and image type conversions.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "coregraphics/rendertexture.h"
namespace Frame
{
class FrameBlit : public FrameOp
{
public:
	/// constructor
	FrameBlit();
	/// destructor
	virtual ~FrameBlit();

	/// discard operation
	void Discard();
	/// run operation
	void Run(const IndexT frameIndex);

	CoreGraphics::RenderTextureId from, to;
};

} // namespace Frame2