//------------------------------------------------------------------------------
// framesubpassalgorithm.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "framesubpassalgorithm.h"

namespace Frame
{

__ImplementClass(Frame::FrameSubpassAlgorithm, 'FSUA', Frame::FrameOp);
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
	this->alg = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassAlgorithm::Run(const IndexT frameIndex)
{
	this->func(frameIndex);
}

} // namespace Frame2