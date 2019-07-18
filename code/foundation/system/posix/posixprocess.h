#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::PosixAppLauncher

    Launch an external application using fork/execve

    (C) 2013 Individual contributors, see AUTHORS file
*/
#include "util/string.h"
#include "io/uri.h"
#include "io/stream.h"
#include "system/base/processbase.h"

//------------------------------------------------------------------------------
namespace Posix
{
class PosixProcess : public Base::Process
{
public:

    /// constructor
    PosixProcess();

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
    /// cleanup all pipes
    void CleanUp() const;

    int inPipe;
    int outPipe;
    int errPipe;
    pid_t pid;
};

} // namespace Posix
//------------------------------------------------------------------------------
