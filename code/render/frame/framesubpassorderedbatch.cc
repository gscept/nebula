//------------------------------------------------------------------------------
// framesubpasssortedbatch.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
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
FrameSubpassOrderedBatch::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
{
	CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
	ret->batch = this->batch;
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassOrderedBatch::CompiledImpl::Run(const IndexT frameIndex, const IndexT bufferIndex)
{

}

} // namespace Frame2
