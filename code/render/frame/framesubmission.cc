//------------------------------------------------------------------------------
//  framesubmission.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "framesubmission.h"
#include "coregraphics/graphicsdevice.h"
namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FrameSubmission::FrameSubmission() :
	waitQueue(InvalidQueueType),
	endOfFrameBarrier(nullptr)
{
}

//------------------------------------------------------------------------------
/**
*/
FrameSubmission::~FrameSubmission()
{
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubmission::CompiledImpl::Run(const IndexT frameIndex)
{
	switch (this->startOrEnd)
	{
	case 0:
		CoreGraphics::BeginSubmission(this->queue, this->waitQueue);
		break;
	case 1:
		// I will admit, this is a little hacky, and in the future we might put this in its own command...
		if (this->endOfFrameBarrier && *this->endOfFrameBarrier != CoreGraphics::BarrierId::Invalid())
		{
			// make sure to transition resources back to their original state in preparation for the next frame
			CoreGraphics::BarrierReset(*this->endOfFrameBarrier);
			CoreGraphics::BarrierInsert(*this->endOfFrameBarrier, this->queue);
			CoreGraphics::EndSubmission(this->queue, true);
		}
		else
			CoreGraphics::EndSubmission(this->queue);
		break;
	}
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled* 
FrameSubmission::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
{
	CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
	ret->queue = this->queue;
	ret->waitQueue = this->waitQueue;
	ret->startOrEnd = this->startOrEnd;
	ret->endOfFrameBarrier = this->endOfFrameBarrier;
	return ret;
}

} // namespace Frame
