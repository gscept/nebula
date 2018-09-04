//------------------------------------------------------------------------------
// framecomputealgorithm.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framecomputealgorithm.h"

namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FrameComputeAlgorithm::FrameComputeAlgorithm()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FrameComputeAlgorithm::~FrameComputeAlgorithm()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
FrameComputeAlgorithm::CompiledImpl::Run(const IndexT frameIndex)
{
	this->func(frameIndex);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameComputeAlgorithm::CompiledImpl::Discard()
{
	this->func = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled* 
FrameComputeAlgorithm::AllocCompiled(Memory::ChunkAllocator<BIG_CHUNK>& allocator)
{
	CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
	ret->func = this->alg->GetFunction(this->funcName);
	return ret;
}


} // namespace Frame2