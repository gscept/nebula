#pragma once
//------------------------------------------------------------------------------
/**
	A subpass batch performs batch rendering of geometry.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "frame/framebatchtype.h"
namespace Frame
{
class FrameSubpassBatch : public FrameOp
{
	__DeclareClass(FrameSubpassBatch);
public:
	/// constructor
	FrameSubpassBatch();
	/// destructor
	virtual ~FrameSubpassBatch();

	/// run operation
	void Run(const IndexT frameIndex);

	FrameBatchType::Code batch;
};

} // namespace Frame2