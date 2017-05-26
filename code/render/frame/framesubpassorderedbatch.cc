//------------------------------------------------------------------------------
// framesubpasssortedbatch.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "framesubpassorderedbatch.h"

namespace Frame2
{

__ImplementClass(Frame2::FrameSubpassOrderedBatch, 'FSSB', Frame2::FrameOp);
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