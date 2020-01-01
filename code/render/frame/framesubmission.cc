//------------------------------------------------------------------------------
//  framesubmission.cc
//  (C) 2019-2020 Individual contributors, see AUTHORS file
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
	queue(CoreGraphics::InvalidQueueType),
	waitQueue(CoreGraphics::InvalidQueueType),
	resourceResetBarriers(nullptr),
	startOrEnd(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FrameSubmission::~FrameSubmission()
{
	// empty
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
		if (this->resourceResetBarriers && this->resourceResetBarriers->Size() > 0)
		{
			IndexT i;
			for (i = 0; i < this->resourceResetBarriers->Size(); i++)
			{
				// make sure to transition resources back to their original state in preparation for the next frame
				CoreGraphics::BarrierReset((*this->resourceResetBarriers)[i]);
				CoreGraphics::BarrierInsert((*this->resourceResetBarriers)[i], this->queue);
			}			
			CoreGraphics::EndSubmission(this->queue, this->waitQueue, true);
		}
		else
			CoreGraphics::EndSubmission(this->queue, this->waitQueue);
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
	ret->resourceResetBarriers = this->resourceResetBarriers;
	return ret;
}

} // namespace Frame
