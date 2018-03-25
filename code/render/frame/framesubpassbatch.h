#pragma once
//------------------------------------------------------------------------------
/**
	A subpass batch performs batch rendering of geometry.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "coregraphics/batchgroup.h"
namespace Frame
{
class FrameSubpassBatch : public FrameOp
{
public:
	/// constructor
	FrameSubpassBatch();
	/// destructor
	virtual ~FrameSubpassBatch();

	/// run operation
	void Run(const IndexT frameIndex);

	CoreGraphics::BatchGroup::Code batch;
};

} // namespace Frame2