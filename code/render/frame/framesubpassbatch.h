#pragma once
//------------------------------------------------------------------------------
/**
	A subpass batch performs batch rendering of geometry.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "graphics/batchgroup.h"
namespace Frame2
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
	void SetBatchCode(const Graphics::BatchGroup::Code& code);

	/// run operation
	void Run(const IndexT frameIndex);

private:
	Graphics::BatchGroup::Code batch;
};

//------------------------------------------------------------------------------
/**
*/
inline void
FrameSubpassBatch::SetBatchCode(const Graphics::BatchGroup::Code& code)
{
	this->batch = code;
}

} // namespace Frame2