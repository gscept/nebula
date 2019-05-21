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
	this->func = this->alg->GetFunction(this->funcName);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassAlgorithm::Discard()
{
	FrameOp::Discard();

	this->func = nullptr;
	this->alg = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled* 
FrameSubpassAlgorithm::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
{
	CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
	ret->func = this->alg->GetFunction(this->funcName);
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