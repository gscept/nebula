//------------------------------------------------------------------------------
//  win32applauncher.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "win32applauncher.h"
#include <Tlhelp32.h>

namespace ToolkitUtil
{
using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
Win32AppLauncher::Win32AppLauncher() :   
    lauchedProcessInfo(PROCESS_INFORMATION())
{ 
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
Win32AppLauncher::LaunchWait() const
{
    n_assert(this->exePath.IsValid());

    DWORD creationFlags = 0;
    if (this->noConsoleWindow)
    {
        creationFlags |= CREATE_NO_WINDOW;
    }

    // build a command line
    String cmdLine = this->exePath.LocalPath();
    cmdLine.Append(" ");
    cmdLine.Append(this->args);

    if(this->stdoutCaptureStream.isvalid() || this->stderrCaptureStream.isvalid())
    {
        return this->LaunchWithOutputCapturing(creationFlags, cmdLine);
    }
    else
    {
        return this->LaunchWithoutOutputCapturing(creationFlags, cmdLine);
    }
}

//------------------------------------------------------------------------------
/**
    Launch the application process and returns immediately. The state of the launched
    process can be checked by calling IsRunning().
*/
bool
Win32AppLauncher::Launch() 
{
    n_assert(this->exePath.IsValid());

    DWORD creationFlags = 0;
    if (this->noConsoleWindow)
    {
        creationFlags |= CREATE_NO_WINDOW;
    }

    // build a command line
    String cmdLine = this->exePath.LocalPath();
    cmdLine.Append(" ");
    cmdLine.Append(this->args);

    
    STARTUPINFO startupInfo = { sizeof(STARTUPINFO), 0 };
    
    // setup some additional startup info for stdout / stderr streaming
    if(this->stdoutCaptureStream.isvalid() || this->stderrCaptureStream.isvalid())
    {
        startupInfo.dwFlags |= STARTF_USESTDHANDLES;
        startupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        if(this->stdoutCaptureStream.isvalid())
        {
            if(!this->CreateStdoutCapturePipe((PHANDLE)&this->stdoutAsyncRead, (PHANDLE)&this->stdoutAsyncWrite))
            {
                return false;
            }
            startupInfo.hStdOutput = this->stdoutAsyncWrite;
        }
        else
        {
            startupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        }
        if(this->stderrCaptureStream.isvalid())
        {
            if(!this->CreateStdoutCapturePipe((PHANDLE)&this->stderrAsyncRead, (PHANDLE)&this->stderrAsyncWrite))
            {
                return false;
            }
            startupInfo.hStdError = this->stderrAsyncWrite;   
        }
        else
        {
            startupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        }
    }
    
    // create the process, return false if this fails
    if (!CreateProcess(NULL,                        // lpApplicationName
        (LPSTR) cmdLine.AsCharPtr(),                // lpCommandLine
        NULL,                                       // lpProcessAttributes
        NULL,                                       // lpThreadAttributes
        TRUE,                                       // bInheritsHandle
        creationFlags,                              // dwCreationFlags
        NULL,                                       // lpEnvironment
        this->workingDir.LocalPath().AsCharPtr(),   // lpCurrentDirectory
        &startupInfo,                               // lpStartupInfo
        (LPPROCESS_INFORMATION)&(this->lauchedProcessInfo))) // lpProcessInformation
        {
            if(this->stdoutCaptureStream.isvalid())
            {
                CloseHandle(this->stdoutAsyncRead);
                CloseHandle(this->stdoutAsyncWrite);
            }
            if(this->stderrCaptureStream.isvalid())
            {
                CloseHandle(this->stderrAsyncRead);
                CloseHandle(this->stderrAsyncWrite);
            }
            return false;
        }
    
    // check if optional stdout / stderr stream is open, else return with false
    if(this->stdoutCaptureStream.isvalid())
    {
        this->stdoutCaptureStream->SetAccessMode(Stream::WriteAccess);
        if (!this->stdoutCaptureStream->Open())
        {
            CloseHandle(this->stdoutAsyncRead);
            CloseHandle(this->stdoutAsyncWrite);
            return false;
        }
    }
    if(this->stderrCaptureStream.isvalid())
    {
        this->stderrCaptureStream->SetAccessMode(Stream::WriteAccess);
        if (!this->stderrCaptureStream->Open())
        {
            CloseHandle(this->stderrAsyncRead);
            CloseHandle(this->stderrAsyncWrite);
            return false;
        }
    }

    this->isRunning = true;
    return true;
}

//------------------------------------------------------------------------------
/**
    Gets the state of the application.
*/
bool
Win32AppLauncher::IsRunning()
{
    if(this->isRunning)
    {
        DWORD exitCode = 0;
        GetExitCodeProcess(this->lauchedProcessInfo.hProcess, &exitCode);
        if (exitCode != STILL_ACTIVE)
        {
            this->UpdateStdoutStream();
            // cleanup
            if(this->stdoutCaptureStream.isvalid())
            {
                this->stdoutCaptureStream->Close();
                CloseHandle(this->stdoutAsyncRead);
                CloseHandle(this->stdoutAsyncWrite);
            }
            if(this->stderrCaptureStream.isvalid())
            {
                this->stderrCaptureStream->Close();
                CloseHandle(this->stderrAsyncRead);
                CloseHandle(this->stderrAsyncWrite);
            }
            CloseHandle(this->lauchedProcessInfo.hProcess);
            CloseHandle(this->lauchedProcessInfo.hThread);  

            this->isRunning = false;
        }
        this->UpdateStdoutStream();
    }
    
    return this->isRunning;
}
//------------------------------------------------------------------------------
/**
    Reads all arrived data from stdout since the last call of this method and puts it to
    the stream.
*/
void
Win32AppLauncher::UpdateStdoutStream()
{
    if(this->stdoutCaptureStream.isvalid())
    {
        n_assert(0 != this->stdoutAsyncRead);
        n_assert(0 != this->lauchedProcessInfo.hProcess);

        numAsyncBytesRead = 0;
        numAsyncBytesAvailable = 0;
        Memory::Clear(asyncBuffer, asyncBufferSize);

        // peek to see if there is any data to read
        PeekNamedPipe(this->stdoutAsyncRead, asyncBuffer, asyncBufferSize, &numAsyncBytesRead, &numAsyncBytesAvailable, NULL);

        if (numAsyncBytesRead != 0)
        {
            if (numAsyncBytesAvailable > asyncBufferSize)
            {
                while (numAsyncBytesRead >= asyncBufferSize)
                {
                    ReadFile(this->stdoutAsyncRead, asyncBuffer, asyncBufferSize, &numAsyncBytesRead, NULL);
                    this->stdoutCaptureStream->Write(asyncBuffer, numAsyncBytesRead);
                }
            }
            else
            {
                ReadFile(this->stdoutAsyncRead, asyncBuffer, asyncBufferSize, &numAsyncBytesRead, NULL);
                this->stdoutCaptureStream->Write(asyncBuffer, numAsyncBytesRead);
            }
        }
    }
    if(this->stderrCaptureStream.isvalid())
    {
        n_assert(0 != this->stderrAsyncRead);
        n_assert(0 != this->lauchedProcessInfo.hProcess);

        numAsyncBytesRead = 0;
        numAsyncBytesAvailable = 0;
        Memory::Clear(asyncBuffer, asyncBufferSize);

        // peek to see if there is any data to read
        PeekNamedPipe(this->stderrAsyncRead, asyncBuffer, asyncBufferSize, &numAsyncBytesRead, &numAsyncBytesAvailable, NULL);

        if (numAsyncBytesRead != 0)
        {
            if (numAsyncBytesAvailable > asyncBufferSize)
            {
                while (numAsyncBytesRead >= asyncBufferSize)
                {
                    ReadFile(this->stderrAsyncRead, asyncBuffer, asyncBufferSize, &numAsyncBytesRead, NULL);
                    this->stderrCaptureStream->Write(asyncBuffer, numAsyncBytesRead);
                }
            }
            else
            {
                ReadFile(this->stderrAsyncRead, asyncBuffer, asyncBufferSize, &numAsyncBytesRead, NULL);
                this->stderrCaptureStream->Write(asyncBuffer, numAsyncBytesRead);
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
Win32AppLauncher::LaunchWithoutOutputCapturing(DWORD creationFlags, const String& cmdLine) const
{
    STARTUPINFO startupInfo = { sizeof(STARTUPINFO), 0 };
    PROCESS_INFORMATION processInfo = { 0 };

    if (!CreateProcess(NULL,                            // lpApplicationName
                       (LPSTR) cmdLine.AsCharPtr(),     // lpCommandLine
                       NULL,                            // lpProcessAttributes
                       NULL,                            // lpThreadAttributes
                       FALSE,                           // bInheritsHandle
                       creationFlags,                   // dwCreationFlags
                       NULL,                            // lpEnvironment
                       this->workingDir.LocalPath().AsCharPtr(),    // lpCurrentDirectory
                       &startupInfo,                    // lpStartupInfo
                       &processInfo))                   // lpProcessInformation
    {
        return false;
    }

    // wait until process exits
    WaitForSingleObject(processInfo.hProcess, INFINITE);
    
    // cleanup
    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
Win32AppLauncher::LaunchWithOutputCapturing(DWORD creationFlags, const String& cmdLine) const
{ 
    HANDLE stdoutRead = 0;
    HANDLE stdoutWrite = 0;
    HANDLE stderrRead = 0;
    HANDLE stderrWrite = 0;

    // build startup info
    STARTUPINFO startupInfo = { sizeof(STARTUPINFO), 0 };
    startupInfo.dwFlags |= STARTF_USESTDHANDLES;
    startupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    
    // create stdout output
    if(this->stdoutCaptureStream.isvalid())
    {
        if(!this->CreateStdoutCapturePipe(&stdoutRead, &stdoutWrite))
        {
            return false;
        }
        startupInfo.hStdOutput = stdoutWrite;
    }
    else
    {
        startupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    // create stderr output
	if(this->stderrCaptureStream.isvalid())
    {
        if(!this->CreateStdoutCapturePipe(&stderrRead, &stderrWrite))
        {
            return false;
        }
        startupInfo.hStdError = stderrWrite;
    }
    else
    {
        startupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    }
    
    // create the process
    PROCESS_INFORMATION processInfo = { 0 };

    if (!CreateProcess(NULL,                            // lpApplicationName
                       (LPSTR) cmdLine.AsCharPtr(),     // lpCommandLine
                       NULL,                            // lpProcessAttributes
                       NULL,                            // lpThreadAttributes
                       TRUE,                            // bInheritsHandle
                       creationFlags,                   // dwCreationFlags
                       NULL,                            // lpEnvironment
                       this->workingDir.LocalPath().AsCharPtr(),    // lpCurrentDirectory
                       &startupInfo,                    // lpStartupInfo
                       &processInfo))                   // lpProcessInformation
    {
        // cleanup if failed
        if(this->stdoutCaptureStream.isvalid())
        {
            CloseHandle(stdoutRead);
            CloseHandle(stdoutWrite);
        }
        if(this->stderrCaptureStream.isvalid())
        {
            CloseHandle(stderrRead);
            CloseHandle(stderrWrite);
        }
        return false;
    }
    
    // try to open stdout stream
    if(this->stdoutCaptureStream.isvalid())
    {
        this->stdoutCaptureStream->SetAccessMode(Stream::WriteAccess);
	    if (!this->stdoutCaptureStream->Open())
	    {
		    CloseHandle(stdoutRead);
            CloseHandle(stdoutWrite);
            return false;
	    }
    }

    // try to open stderr stream
    if(this->stderrCaptureStream.isvalid())
    {
        this->stderrCaptureStream->SetAccessMode(Stream::WriteAccess);
        if (!this->stderrCaptureStream->Open())
        {
            CloseHandle(stderrRead);
            CloseHandle(stderrWrite);
            return false;
        }
    }
    
    // capture output
    this->CaptureOutput(stdoutRead, stderrRead, processInfo.hProcess);

    // cleanup
    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);
    if(this->stdoutCaptureStream.isvalid())
    {
        this->stdoutCaptureStream->Close();
        CloseHandle(stdoutRead);
        CloseHandle(stdoutWrite);
    }
    if(this->stderrCaptureStream.isvalid())
    {
        this->stderrCaptureStream->Close();
        CloseHandle(stderrRead);
        CloseHandle(stderrWrite);
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
Win32AppLauncher::CreateStdoutCapturePipe(PHANDLE stdoutRead, PHANDLE stdoutWrite) const
{
    // configure pipe security attributes
    SECURITY_ATTRIBUTES security;
	security.bInheritHandle = TRUE;
    security.lpSecurityDescriptor = 0;
    security.nLength = sizeof(SECURITY_ATTRIBUTES);
    
    // create a pipe for the child process's stdout
    if (!CreatePipe(stdoutRead, stdoutWrite, &security, 4096))
    {
        return false;
    }

    // ensure the read handle to the pipe for stdout is not inherited.
    SetHandleInformation((*stdoutRead), HANDLE_FLAG_INHERIT, 0);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
Win32AppLauncher::CaptureOutput(HANDLE stdoutRead, HANDLE stderrRead, HANDLE childProcess) const
{
    n_assert(0 != childProcess);
    
    DWORD numBytesRead = 0;
    DWORD numBytesAvailable = 0;
    DWORD exitCode = 0;
    const int bufferSize = 4096;
    char buffer[bufferSize];
    Memory::Clear(buffer, bufferSize);

    // read everything from read end of pipe and write to stream ...
    while(true)
    {
        if(this->stdoutCaptureStream.isvalid())
        {
            n_assert(0 != stdoutRead);
            // peek to see if there is any data to read
            PeekNamedPipe(stdoutRead, buffer, bufferSize, &numBytesRead, &numBytesAvailable, NULL);
            
            if (numBytesRead != 0)
            {
                if (numBytesAvailable > bufferSize)
                {
                    while (numBytesRead >= bufferSize)
                    {
                        ReadFile(stdoutRead, buffer, bufferSize, &numBytesRead, NULL);
                        this->stdoutCaptureStream->Write(buffer, numBytesRead);
                    }
                }
                else
                {
                    ReadFile(stdoutRead, buffer, bufferSize, &numBytesRead, NULL);
                    this->stdoutCaptureStream->Write(buffer, numBytesRead);
                }
            }
        }
        if(this->stderrCaptureStream.isvalid())
        {
            n_assert(0 != stderrRead);
            // peek to see if there is any data to read
            PeekNamedPipe(stderrRead, buffer, bufferSize, &numBytesRead, &numBytesAvailable, NULL);

            if (numBytesRead != 0)
            {
                if (numBytesAvailable > bufferSize)
                {
                    while (numBytesRead >= bufferSize)
                    {
                        ReadFile(stderrRead, buffer, bufferSize, &numBytesRead, NULL);
                        this->stderrCaptureStream->Write(buffer, numBytesRead);
                    }
                }
                else
                {
                    ReadFile(stderrRead, buffer, bufferSize, &numBytesRead, NULL);
                    this->stderrCaptureStream->Write(buffer, numBytesRead);
                }
            }
        }

        // ... while the child process is running
        GetExitCodeProcess(childProcess, &exitCode);
        if (exitCode != STILL_ACTIVE)
        {
            break;
        }
    }
}
 
//------------------------------------------------------------------------------
/**
*/
bool
Win32AppLauncher::CheckIfExists()
{
	return CheckIfExists(this->exePath);
}

//------------------------------------------------------------------------------
/**
*/
bool
Win32AppLauncher::CheckIfExists(const IO::URI & program)
{
	bool exists = false;
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);
	Util::String procname = program.LocalPath().ExtractFileName();
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (Process32First(snapshot, &entry))
		while (Process32Next(snapshot, &entry))
			if (procname == entry.szExeFile)
				exists = true;
	CloseHandle(snapshot);
	return exists;
}

} // namespace ToolkitUtil