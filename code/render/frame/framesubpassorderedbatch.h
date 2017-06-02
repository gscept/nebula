#pragma once
//------------------------------------------------------------------------------
/**
	A subpass sorted batch renders the same geometry as the ordinary batch, however
	it prioritizes Z-order instead shader, making it potentially detrimental for performance.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "graphics/batchgroup.h"
namespace Frame
{
class FrameSubpassOrderedBatch : public FrameOp
{
	__DeclareClass(FrameSubpassOrderedBatch);
public:
	/// constructor
	FrameSubpassOrderedBatch();
	/// destructor
	virtual ~FrameSubpassOrderedBatch();

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
FrameSubpassOrderedBatch::SetBatchCode(const Graphics::BatchGroup::Code& code)
{
	this->batch = code;
}
} // namespace Frame2