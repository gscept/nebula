#pragma once
//------------------------------------------------------------------------------
/**
    CmdBuffer contains general functions related to command buffer management.

    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "coregraphics/config.h"
namespace CoreGraphics
{

enum CommandBufferUsage
{
    CommandGfx,
    CommandCompute,
    CommandTransfer,        // CPU GPU data transfer calls
    CommandSparse,          // Sparse binding calls
    InvalidCommandUsage,
    NumCommandBufferUsages = InvalidCommandUsage
};

//------------------------------------------------------------------------------
/**
*/
ID_24_8_TYPE(CommandBufferPoolId);

struct CommandBufferPoolCreateInfo
{
    CoreGraphics::QueueType queue;
    bool resetable : 1;     // allow the buffer to be reset
    bool shortlived : 1;    // the buffer won't last long until it's destroyed or reset
};

/// create new command buffer pool
const CommandBufferPoolId CreateCommandBufferPool(const CommandBufferPoolCreateInfo& info);
/// destroy command buffer pool
void DestroyCommandBufferPool(const CommandBufferPoolId pool);

struct CommandBufferCreateInfo
{
    bool subBuffer : 1;     // create buffer to be executed on another command buffer (subBuffer must be 0 on the other buffer)
    CommandBufferPoolId pool;
};

struct CommandBufferBeginInfo
{
    bool submitOnce : 1;
    bool submitDuringPass : 1;
    bool resubmittable : 1;
};

struct CommandBufferClearInfo
{
    bool allowRelease : 1;  // release resources when clearing (don't use if buffer converges in size)
};

ID_24_8_TYPE(CommandBufferId);

/// create new command buffer
const CommandBufferId CreateCommandBuffer(const CommandBufferCreateInfo& info);
/// destroy command buffer
void DestroyCommandBuffer(const CommandBufferId id);

/// begin recording to command buffer
void CommandBufferBeginRecord(const CommandBufferId id, const CommandBufferBeginInfo& info);
/// end recording command buffer, it may be submitted after this is done
void CommandBufferEndRecord(const CommandBufferId id);

/// clear the command buffer to be empty
void CommandBufferClear(const CommandBufferId id, const CommandBufferClearInfo& info);



} // namespace CoreGraphics

