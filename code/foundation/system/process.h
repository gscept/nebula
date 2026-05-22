#pragma once
//------------------------------------------------------------------------------
/**
    @class System::Process

    Launch an external application.
    
    (C) 2026 Individual contributors, see AUTHORS file
*/
#include "ids/id.h"
namespace System
{
ID_24_8_TYPE(ProcessId);

struct ProcessStartInfo
{
    IO::URI exePath;
    IO::URI workingDir;
    Util::String args;
    bool consoleWindow;
    Ptr<IO::Stream> outputStream, errorStream;
};

/// Find a process using URI
bool FindProcess(const IO::URI& uri);
/// Create a process
ProcessId StartProcess(const ProcessStartInfo& createInfo);
/// Update streams
void UpdateProcessStreams(ProcessId processId);
/// Returns true if process is running (poll)
bool IsProcessRunning(ProcessId processId);
/// Wait for process, then frees it
void WaitForProcess(ProcessId processId);

} // namespace System
