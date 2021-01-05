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


    (C) 2019-2020 Individual contributors, see AUTHORS file
*/
#include "ids/id.h"
#include "coregraphics/config.h"
#include "util/fixedarray.h"
#include "ids/idallocator.h"
#include "coregraphics/commandbuffer.h"
#include "coregraphics/memory.h"

//------------------------------------------------------------------------------
namespace Resources
{
    struct ResourceId;
}

namespace CoreGraphics
{

struct TextureId;
struct BufferId;
struct SemaphoreId;
struct FenceId;
ID_24_8_TYPE(SubmissionContextId);

struct SubmissionContextCreateInfo
{
    CommandBufferCreateInfo cmdInfo;    // creation info for the cmd buffers
    uint numBuffers : 3;                // number of buffers it should keep track of
    bool useFence : 1;                  // set if a fence should be used when we cycle  
#if NEBULA_GRAPHICS_DEBUG
    Util::String name;
#endif
};

/// create new submission context
SubmissionContextId CreateSubmissionContext(const SubmissionContextCreateInfo& info);
/// destroy submission context
void DestroySubmissionContext(const SubmissionContextId id);

/// create new buffer and retire the old, outputs the new buffer and semaphore to be used for recording
void SubmissionContextNewBuffer(const SubmissionContextId id, CommandBufferId& outBuf);
/// get current buffer
CommandBufferId SubmissionContextGetCmdBuffer(const SubmissionContextId id);

/// add resource for release
void SubmissionContextFreeResource(const CoreGraphics::SubmissionContextId id, const Resources::ResourceId res);
/// add image for deletion
void SubmissionContextFreeTexture(const CoreGraphics::SubmissionContextId id, CoreGraphics::TextureId tex);
/// add a buffer deletion
void SubmissionContextFreeBuffer(const CoreGraphics::SubmissionContextId id, CoreGraphics::BufferId buf);
/// add command buffer for deletion
void SubmissionContextFreeCommandBuffer(const CoreGraphics::SubmissionContextId id, const CoreGraphics::CommandBufferId cmd);
/// add command buffer for reset
void SubmissionContextClearCommandBuffer(const CoreGraphics::SubmissionContextId id, const CoreGraphics::CommandBufferId cmd);
/// add a memory alloc for freeing
void SubmissionContextFreeMemory(const CoreGraphics::SubmissionContextId id, const CoreGraphics::Alloc& alloc);
/// add void* to memory free upon completion
void SubmissionContextFreeHostMemory(const SubmissionContextId id, void* buf);

/// get current fence object
const FenceId SubmissionContextGetFence(const SubmissionContextId id);

/// cycle submission context, returns fence to previous cycle
const FenceId SubmissionContextNextCycle(const SubmissionContextId id, const std::function<void(uint64 index)>& sync);
/// poll submission context for completion, run function to check status
void SubmissionContextPoll(const SubmissionContextId id);
}

