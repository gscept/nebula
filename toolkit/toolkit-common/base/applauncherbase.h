#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::AppLauncherBase
    
    Base class for launching an external application.
        
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "util/string.h"
#include "io/uri.h"
#include "io/stream.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class AppLauncherBase
{
public:
    
    /// constructor
    AppLauncherBase();

    /// set the executable path
    void SetExecutable(const IO::URI& uri);
    /// set working directory
    void SetWorkingDirectory(const IO::URI& uri);
    /// set command line arguments
    void SetArguments(const Util::String& args);
    /// do not open a console window for the new process
    void SetNoConsoleWindow(bool b);
    /// set optional stdout capture stream
    void SetStdoutCaptureStream(const Ptr<IO::Stream>& stream);
    /// get optional pointer to stdout capture stream
    const Ptr<IO::Stream>& GetStdoutCaptureStream() const;
    /// set optional stderr capture stream
    void SetStderrCaptureStream(const Ptr<IO::Stream>& stream);
    /// get optional pointer to stderr capture stream
    const Ptr<IO::Stream>& GetStderrCaptureStream() const;

    /// launch application and wait for its termination
    virtual bool LaunchWait() const = 0;
    /// launch application (returns immediately)
    virtual bool Launch() = 0;
    /// Gets the state of application. Call this per frame.
    virtual bool IsRunning() = 0;
    /// Read data from the captured stdout and writes it to the stream.
    virtual void UpdateStdoutStream() = 0;
    /// Detect if an instance of the process is already running
    virtual bool CheckIfExists() = 0;

protected:

    bool noConsoleWindow;
    bool isRunning;
    IO::URI exePath;
    IO::URI workingDir;
    Util::String args;
    Ptr<IO::Stream> stdoutCaptureStream;
    Ptr<IO::Stream> stderrCaptureStream;    
};


//------------------------------------------------------------------------------
/**
*/
inline
AppLauncherBase::AppLauncherBase() :    
    stdoutCaptureStream(nullptr),
    isRunning(false),
    noConsoleWindow(false)
{ 
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline void
AppLauncherBase::SetExecutable(const IO::URI& uri)
{
    this->exePath = uri;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AppLauncherBase::SetWorkingDirectory(const IO::URI& uri)
{
    this->workingDir = uri;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AppLauncherBase::SetArguments(const Util::String& a)
{
    this->args = a;
}


//------------------------------------------------------------------------------
/**
*/
inline void
AppLauncherBase::SetNoConsoleWindow(bool b)
{
    this->noConsoleWindow = b;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AppLauncherBase::SetStdoutCaptureStream(const Ptr<IO::Stream>& stream)
{
    this->stdoutCaptureStream = stream;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<IO::Stream>&
AppLauncherBase::GetStdoutCaptureStream() const
{
    return this->stdoutCaptureStream;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AppLauncherBase::SetStderrCaptureStream(const Ptr<IO::Stream>& stream)
{
    this->stderrCaptureStream = stream;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<IO::Stream>&
AppLauncherBase::GetStderrCaptureStream() const
{
    return this->stderrCaptureStream;
}


} // namespace ToolkitUtil
//------------------------------------------------------------------------------
    
