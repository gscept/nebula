//------------------------------------------------------------------------------
// framesubpasssortedbatch.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framesubpassorderedbatch.h"

namespace Frame
{

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
FrameOp::Compiled*
FrameSubpassOrderedBatch::AllocCompiled(Memory::ChunkAllocator<BIG_CHUNK>& allocator)
{
	CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
	ret->batch = this->batch;
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassOrderedBatch::CompiledImpl::Run(const IndexT frameIndex)
{

}

} // namespace Frame2