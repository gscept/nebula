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
public:
	/// constructor
	FrameCopy();
	/// destructor
	virtual ~FrameCopy();

	/// discard operation
	void Discard();
	/// run operation
	void Run(const IndexT frameIndex);

	CoreGraphics::RenderTextureId from, to;
};

} // namespace Frame2