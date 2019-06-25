//------------------------------------------------------------------------------
//  submissioncontext.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "submissioncontext.h"
#include "coregraphics/cmdbuffer.h"
#include "coregraphics/semaphore.h"
#include "coregraphics/fence.h"
#include <array>

#ifdef CreateSemaphore
#pragma push_macro("CreateSemaphore")
#undef CreateSemaphore
#endif

namespace CoreGraphics
{
SubmissionContextAllocator submissionContextAllocator;
//------------------------------------------------------------------------------
/**
*/
SubmissionContextId
CreateSubmissionContext(const SubmissionContextCreateInfo& info)
{
	SubmissionContextId ret;
	Ids::Id32 id = submissionContextAllocator.Alloc();

	submissionContextAllocator.Get<SubmissionContextCmdBuffer>(id).Resize(info.numBuffers);
	submissionContextAllocator.Get<SubmissionContextSemaphore>(id).Resize(info.numBuffers);
	submissionContextAllocator.Get<SubmissionContextRetiredCmdBuffer>(id).Resize(info.numBuffers);
	submissionContextAllocator.Get<SubmissionContextRetiredSemaphore>(id).Resize(info.numBuffers);
	submissionContextAllocator.Get<SubmissionContextFence>(id).Resize(info.numBuffers);
	submissionContextAllocator.Get<SubmissionContextCmdCreateInfo>(id) = info.cmdInfo;

	// create fences
	for (uint i = 0; i < info.numBuffers; i++)
	{
		FenceId& fence = submissionContextAllocator.Get<SubmissionContextFence>(id)[i];

		FenceCreateInfo fenceInfo{ false };
		fence = CreateFence(fenceInfo);
	}

	submissionContextAllocator.Get<SubmissionContextCurrentIndex>(id) = 0;

	ret.id24 = id;
	ret.id8 = SubmissionContextIdType;
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroySubmissionContext(const SubmissionContextId id)
{
	const Util::FixedArray<CmdBufferId>& bufs = submissionContextAllocator.Get<SubmissionContextCmdBuffer>(id.id24);
	const Util::FixedArray<SemaphoreId>& sems = submissionContextAllocator.Get<SubmissionContextSemaphore>(id.id24);

	// go through buffers and semaphores and clear the ones created
	for (IndexT i = 0; i < bufs.Size(); i++)
	{
		DestroyCmdBuffer(bufs[i]);
		DestroySemaphore(sems[i]);
	}
	submissionContextAllocator.Dealloc(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
void 
SubmissionContextNewBuffer(const SubmissionContextId id, CmdBufferId& outBuf, SemaphoreId& outSem)
{
	const IndexT currentIndex = submissionContextAllocator.Get<SubmissionContextCurrentIndex>(id.id24);
	CmdBufferId oldBuf = submissionContextAllocator.Get<SubmissionContextCmdBuffer>(id.id24)[currentIndex];
	SemaphoreId oldSem = submissionContextAllocator.Get<SubmissionContextSemaphore>(id.id24)[currentIndex];

	// append to retired buffers
	if (oldBuf != CmdBufferId::Invalid())
		submissionContextAllocator.Get<SubmissionContextRetiredCmdBuffer>(id.id24)[currentIndex].Append(outBuf);
	if (oldSem != SemaphoreId::Invalid())
		submissionContextAllocator.Get<SubmissionContextRetiredSemaphore>(id.id24)[currentIndex].Append(outSem);

	// create new buffer and semaphore, we will delete the retired buffers upon next cycle when we come back
	CmdBufferCreateInfo cmdInfo = submissionContextAllocator.Get<SubmissionContextCmdCreateInfo>(id.id24);
	outBuf = CreateCmdBuffer(cmdInfo);
	submissionContextAllocator.Get<SubmissionContextCmdBuffer>(id.id24)[currentIndex] = outBuf;
	SemaphoreCreateInfo semInfo{};
	outSem = CreateSemaphore(semInfo);
	submissionContextAllocator.Get<SubmissionContextSemaphore>(id.id24)[currentIndex] = outSem;
}

//------------------------------------------------------------------------------
/**
*/
const FenceId 
SubmissionContextGetFence(const SubmissionContextId id)
{
	const IndexT currentIndex = submissionContextAllocator.Get<SubmissionContextCurrentIndex>(id.id24);
	return submissionContextAllocator.Get<SubmissionContextFence>(id.id24)[currentIndex];
}

//------------------------------------------------------------------------------
/**
*/
const FenceId
SubmissionContextNextCycle(const SubmissionContextId id)
{
	IndexT currentIndex = submissionContextAllocator.Get<SubmissionContextCurrentIndex>(id.id24);
	FenceId ret = submissionContextAllocator.Get<SubmissionContextFence>(id.id24)[currentIndex];

	// cycle index and update
	currentIndex = (currentIndex + 1) % submissionContextAllocator.Get<SubmissionContextFence>(id.id24).Size();
	submissionContextAllocator.Get<SubmissionContextCurrentIndex>(id.id24) = currentIndex;

	// get next fence and wait for it to finish
	FenceId nextFence = submissionContextAllocator.Get<SubmissionContextFence>(id.id24)[currentIndex];
	bool res = FenceWait(nextFence, UINT_MAX);
	n_assert(res);

	// clean up retired buffers and semaphores
	Util::Array<CmdBufferId>& bufs = submissionContextAllocator.Get<SubmissionContextRetiredCmdBuffer>(id.id24)[currentIndex];
	Util::Array<SemaphoreId>& sems = submissionContextAllocator.Get<SubmissionContextRetiredSemaphore>(id.id24)[currentIndex];

	for (IndexT i = 0; i < bufs.Size(); i++)
	{
		DestroyCmdBuffer(bufs[i]);
		DestroySemaphore(sems[i]);
	}
	bufs.Clear();
	sems.Clear();

	// also destroy current buffers
	const CmdBufferId buf = submissionContextAllocator.Get<SubmissionContextCmdBuffer>(id.id24)[currentIndex];
	const SemaphoreId sem = submissionContextAllocator.Get<SubmissionContextSemaphore>(id.id24)[currentIndex];
	DestroyCmdBuffer(buf);
	DestroySemaphore(sem);

	return ret;
}

} // namespace CoreGraphics

#pragma pop_macro("CreateSemaphore")