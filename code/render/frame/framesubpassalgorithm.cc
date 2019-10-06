//------------------------------------------------------------------------------
// framesubpassalgorithm.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framesubpassalgorithm.h"

namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FrameSubpassAlgorithm::FrameSubpassAlgorithm()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FrameSubpassAlgorithm::~FrameSubpassAlgorithm()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassAlgorithm::Setup()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassAlgorithm::Discard()
{
	FrameOp::Discard();

	this->func = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled* 
FrameSubpassAlgorithm::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
{
	CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
	ret->func = this->func;
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassAlgorithm::CompiledImpl::Run(const IndexT frameIndex)
{
	this->func(frameIndex);
}

} // namespace Frame2