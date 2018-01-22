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

	/// set batch
	void SetBatchCode(const FrameBatchType::Code& code);

	/// run operation
	void Run(const IndexT frameIndex);

private:
	FrameBatchType::Code batch;
};

//------------------------------------------------------------------------------
/**
*/
inline void
FrameSubpassBatch::SetBatchCode(const FrameBatchType::Code& code)
{
	this->batch = code;
}

} // namespace Frame2