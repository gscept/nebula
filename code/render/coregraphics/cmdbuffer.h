#pragma once
//------------------------------------------------------------------------------
/**
	CmdBuffer contains general functions related to command buffer management.

	(C)2017-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
namespace CoreGraphics
{

enum CmdBufferUsage
{
	CmdDraw,
	CmdCompute,
	CmdTransfer,		// CPU GPU data transfer calls
	CmdSparse,			// Sparse binding calls
	InvalidCmdUsage,
	NumCmdBufferUsages = InvalidCmdUsage
};

struct CmdBufferCreateInfo
{
	bool subBuffer : 1;		// create buffer to be executed on another command buffer (subBuffer must be 0 on that one)
	bool resetable : 1;		// allow the buffer to be reset
	bool shortlived : 1;	// the buffer won't last long until it's destroyed or reset
	CmdBufferUsage usage;
};

struct CmdBufferBeginInfo
{
	bool submitOnce : 1;
	bool submitDuringPass : 1;
	bool resubmittable : 1;
};

struct CmdBufferClearInfo
{
	bool allowRelease : 1;	// release resources when clearing (don't use if buffer converges in size)
};

ID_24_8_TYPE(CmdBufferId);

/// create new command buffer
const CmdBufferId CreateCmdBuffer(const CmdBufferCreateInfo& info);
/// destroy command buffer
void DestroyCmdBuffer(const CmdBufferId id);

/// begin recording to command buffer
void CmdBufferBeginRecord(const CmdBufferId id, const CmdBufferBeginInfo& info);
/// end recording command buffer, it may be submitted after this is done
void CmdBufferEndRecord(const CmdBufferId id);

/// clear the command buffer to be empty
void CmdBufferClear(const CmdBufferId id, const CmdBufferClearInfo& info);

} // namespace CoreGraphics

