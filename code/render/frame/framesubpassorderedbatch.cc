//------------------------------------------------------------------------------
// framesubpasssortedbatch.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "framesubpassorderedbatch.h"

namespace Frame
{

__ImplementClass(Frame::FrameSubpassOrderedBatch, 'FSSB', Frame::FrameOp);
//------------------------------------------------------------------------------
/**
*/
FrameSubpassOrderedBatch::FrameSubpassOrderedBatch()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FrameSubpassOrderedBatch::~FrameSubpassOrderedBatch()
{
	// empty
}


//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassOrderedBatch::Run(const IndexT frameIndex)
{

}

} // namespace Frame2