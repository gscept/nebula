#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::Win32Process
    
    Launch an external Win32 application.
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "util/string.h"
#include "io/uri.h"
#include "io/stream.h"
#include "system/base/processbase.h"

//------------------------------------------------------------------------------
namespace Win32
{
class Win32Process : public Base::Process
{
public:
    
    /// constructor
    Win32Process();
   
    /// launch application and wait for its termination
    bool LaunchWait() const;
    /// launch application (returns immediately)
    bool Launch();
    /// Gets the state of application. Call this per frame.
    bool IsRunning();
    /// Read data from the captured stdout and writes it to the stream.
    void UpdateStdoutStream();
    /// Detect if an instance of the process is already running
    bool CheckIfExists();
    /// Detect if an instance of a given process is running
    static bool CheckIfExists(const IO::URI & program);

private:
    /// launch without stdout capturing
    bool LaunchWithoutOutputCapturing(DWORD creationFlags, const Util::String& cmdLine) const;
    /// launch with stdout capturing
    bool LaunchWithOutputCapturing(DWORD creationFlags, const Util::String& cmdLine) const;
    /// create pipe for optional stdout capturing
    bool CreateStdoutCapturePipe(PHANDLE stdoutRead, PHANDLE stdoutWrite) const;
    /// capture data from output
    void CaptureOutput(HANDLE stdoutRead, HANDLE stderrRead, HANDLE childProcess) const;

    PROCESS_INFORMATION lauchedProcessInfo;
    HANDLE stdoutAsyncRead;
    HANDLE stdoutAsyncWrite;
    HANDLE stderrAsyncRead;
    HANDLE stderrAsyncWrite;
    
    DWORD numAsyncBytesRead;
    DWORD numAsyncBytesAvailable;
    static const int asyncBufferSize = 4096;
    char asyncBuffer[asyncBufferSize];

};

} // namespace Win32
//------------------------------------------------------------------------------
    