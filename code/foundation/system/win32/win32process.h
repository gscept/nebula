#pragma once
//------------------------------------------------------------------------------
/**
    @class System::Process
    
    Win32 implementation of System::Process.
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "util/string.h"
#include "io/uri.h"
#include "io/stream.h"
#include "ids/idallocator.h"

namespace System
{

enum
{
    Process_LaunchInfo = 0,
    Process_StdoutRead,
    Process_StdoutWrite,
    Process_StderrRead,
    Process_StderrWrite,
    Process_NumAsyncBytesRead,
    Process_NumAsyncBytesAvailable,
    Process_StdoutStream,
    Process_StderrStream,
    Process_AsyncBuffer
};

typedef Ids::IdAllocator<
    PROCESS_INFORMATION,
    HANDLE,
    HANDLE,
    HANDLE,
    HANDLE,
    DWORD,
    DWORD,
    Ptr<IO::Stream>,
    Ptr<IO::Stream>,
    Util::String
> ProcessIdAllocator;
extern ProcessIdAllocator processIdAllocator;


} // namespace Win32
//------------------------------------------------------------------------------
