#pragma once
//------------------------------------------------------------------------------
/**
	@type CoreGraphics::SubmissionContext

	A submission context encapsulates a body of GPU submission related objects, such as
	command buffers and synchronization semaphores. It may also be used internally to keep
	track of, and release resources such as buffers, images and memory once the cycle submission is done.

	The SubmissionContext contains two methods, one for creating a new command buffer - semaphore
	pair, and one for cycling to the next 'frame' or however you want to see it. Cycling may
	incur a synchronization, and should only be done after the context has been submitted.


	(C) 2019 Individual contributors, see AUTHORS file
*/
#include "ids/id.h"
#include "coregraphics/config.h"
#include "util/fixedarray.h"
#include "ids/idallocator.h"
#include "coregraphics/cmdbuffer.h"

//------------------------------------------------------------------------------
namespace CoreGraphics
{

struct SemaphoreId;
struct FenceId;
ID_24_8_TYPE(SubmissionContextId);

struct SubmissionContextCreateInfo
{
	uint numBuffers : 3; // number of buffers it should keep track of
	CmdBufferCreateInfo cmdInfo; // creation info for the cmd buffers
};

enum
{
	SubmissionContextCmdBuffer,
	SubmissionContextSemaphore,
	SubmissionContextRetiredCmdBuffer,
	SubmissionContextRetiredSemaphore,
	SubmissionContextFence,
	SubmissionContextCurrentIndex,
	SubmissionContextCmdCreateInfo
};

typedef Ids::IdAllocator <
	Util::FixedArray<CmdBufferId>,
	Util::FixedArray<SemaphoreId>,
	Util::FixedArray<Util::Array<CmdBufferId>>,
	Util::FixedArray<Util::Array<SemaphoreId>>,
	Util::FixedArray<FenceId>,
	IndexT,
	CmdBufferCreateInfo
> SubmissionContextAllocator;
extern SubmissionContextAllocator submissionContextAllocator;

/// create new submission context
SubmissionContextId CreateSubmissionContext(const SubmissionContextCreateInfo& info);
/// destroy submission context
void DestroySubmissionContext(const SubmissionContextId id);

/// create new buffer and retire the old, outputs the new buffer and semaphore to be used for recording
void SubmissionContextNewBuffer(const SubmissionContextId id, CmdBufferId& outBuf, SemaphoreId& outSem);

/// get current fence object
const FenceId SubmissionContextGetFence(const SubmissionContextId id);

/// cycle submission context, returns fence to previous cycle
const FenceId SubmissionContextNextCycle(const SubmissionContextId id);
}

