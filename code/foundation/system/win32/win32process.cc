//------------------------------------------------------------------------------
//  win32process.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "system/process.h"
#include "win32process.h"
#include <Tlhelp32.h>

namespace System
{
using namespace Util;
using namespace IO;

ProcessIdAllocator processIdAllocator(0xFFF);

//------------------------------------------------------------------------------
/**
*/
bool
CreateCapturePipe(PHANDLE read, PHANDLE write)
{
    // configure pipe security attributes
    SECURITY_ATTRIBUTES security;
    security.bInheritHandle = TRUE;
    security.lpSecurityDescriptor = 0;
    security.nLength = sizeof(SECURITY_ATTRIBUTES);

    // create a pipe for the child process's stdout
    if (!CreatePipe(read, write, &security, 4096))
    {
        return false;
    }

    // ensure the read handle to the pipe for stdout is not inherited.
    SetHandleInformation((*read), HANDLE_FLAG_INHERIT, 0);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
FindProcess(const IO::URI& uri)
{
    bool exists = false;
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    Util::String procname = uri.LocalPath().ExtractFileName();
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (Process32First(snapshot, &entry))
        while (Process32Next(snapshot, &entry))
            if (procname == entry.szExeFile)
                exists = true;
    CloseHandle(snapshot);
    return exists;
}

//------------------------------------------------------------------------------
/**
*/
ProcessId
StartProcess(const ProcessStartInfo& createInfo)
{
    n_assert(createInfo.exePath.IsValid());

    DWORD creationFlags = 0;
    if (!createInfo.consoleWindow)
    {
        creationFlags |= CREATE_NO_WINDOW;
    }

    Ids::Id32 id = processIdAllocator.Alloc();

    PROCESS_INFORMATION& processInfo = processIdAllocator.Get<Process_LaunchInfo>(id);
    HANDLE& stdoutRead = processIdAllocator.Get<Process_StdoutRead>(id);
    HANDLE& stdoutWrite = processIdAllocator.Get<Process_StdoutWrite>(id);
    HANDLE& stderrRead = processIdAllocator.Get<Process_StderrRead>(id);
    HANDLE& stderrWrite = processIdAllocator.Get<Process_StderrWrite>(id);
    processIdAllocator.Set<Process_StdoutStream>(id, createInfo.outputStream);
    processIdAllocator.Set<Process_StderrStream>(id, createInfo.errorStream);
    Util::String& asyncBuffer = processIdAllocator.Get<Process_AsyncBuffer>(id);
    asyncBuffer.Fill(4096, 0);

    // build a command line
    String cmdLine = createInfo.exePath.LocalPath();
    cmdLine.Append(" ");
    cmdLine.Append(createInfo.args);
    STARTUPINFO startupInfo = { sizeof(STARTUPINFO), 0 };

    // setup some additional startup info for stdout / stderr streaming
    if (createInfo.outputStream.isvalid() || createInfo.errorStream.isvalid())
    {
        startupInfo.dwFlags |= STARTF_USESTDHANDLES;
        startupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        if (createInfo.outputStream.isvalid())
        {
            if (!CreateCapturePipe(&stdoutRead, &stdoutWrite))
            {
                processIdAllocator.Dealloc(id);
                return InvalidProcessId;
            }
            startupInfo.hStdOutput = stdoutWrite;
        }
        else
        {
            startupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        }
        if (createInfo.errorStream.isvalid())
        {
            if (!CreateCapturePipe(&stderrRead, &stderrWrite))
            {
                processIdAllocator.Dealloc(id);
                return InvalidProcessId;
            }
            startupInfo.hStdError = stderrWrite;
        }
        else
        {
            startupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        }
    }

    // create the process, return false if this fails
    if (!CreateProcess(NULL,                        // lpApplicationName
        (LPSTR)cmdLine.AsCharPtr(),                // lpCommandLine
        NULL,                                       // lpProcessAttributes
        NULL,                                       // lpThreadAttributes
        TRUE,                                       // bInheritsHandle
        creationFlags,                              // dwCreationFlags
        NULL,                                       // lpEnvironment
        createInfo.workingDir.LocalPath().AsCharPtr(),   // lpCurrentDirectory
        &startupInfo,                               // lpStartupInfo
        (LPPROCESS_INFORMATION) &processInfo)) // lpProcessInformation
    {
        if (createInfo.outputStream.isvalid())
        {
            CloseHandle(stdoutRead);
            CloseHandle(stdoutWrite);
        }
        if (createInfo.errorStream.isvalid())
        {
            CloseHandle(stderrRead);
            CloseHandle(stderrWrite);
        }
        processIdAllocator.Dealloc(id);
        return InvalidProcessId;
    }

    // check if optional stdout / stderr stream is open, else return with false
    if (createInfo.outputStream.isvalid())
    {
        createInfo.outputStream->SetAccessMode(Stream::WriteAccess);
        if (!createInfo.outputStream->Open())
        {
            CloseHandle(stdoutRead);
            CloseHandle(stdoutWrite);
            processIdAllocator.Dealloc(id);
            return InvalidProcessId;
        }
    }
    if (createInfo.errorStream.isvalid())
    {
        createInfo.errorStream->SetAccessMode(Stream::WriteAccess);
        if (!createInfo.errorStream->Open())
        {
            CloseHandle(stderrRead);
            CloseHandle(stderrWrite);
            processIdAllocator.Dealloc(id);
            return InvalidProcessId;
        }
    }
    return id;
}

//------------------------------------------------------------------------------
/**
*/
void
UpdateProcessStreams(ProcessId processId)
{
    PROCESS_INFORMATION& processInfo = processIdAllocator.Get<Process_LaunchInfo>(processId.id);
    HANDLE& stdoutRead = processIdAllocator.Get<Process_StdoutRead>(processId.id);
    HANDLE& stderrRead = processIdAllocator.Get<Process_StderrWrite>(processId.id);
    DWORD& numAsyncBytesRead = processIdAllocator.Get<Process_NumAsyncBytesRead>(processId.id);
    DWORD& numAsyncBytesAvailable = processIdAllocator.Get<Process_NumAsyncBytesAvailable>(processId.id);

    Ptr<IO::Stream>& stdoutCaptureStream = processIdAllocator.Get<Process_StdoutStream>(processId.id);
    Ptr<IO::Stream>& stderrCaptureStream = processIdAllocator.Get<Process_StderrStream>(processId.id);
    Util::String& asyncBuffer = processIdAllocator.Get<Process_AsyncBuffer>(processId.id);

    if (stdoutCaptureStream.isvalid())
    {
        n_assert(0 != processInfo.hProcess);

        numAsyncBytesRead = 0;
        numAsyncBytesAvailable = 0;
        
        Memory::Clear(&asyncBuffer[0], asyncBuffer.Length());

        // peek to see if there is any data to read
        PeekNamedPipe(stdoutRead, &asyncBuffer[0], asyncBuffer.Length(), &numAsyncBytesRead, &numAsyncBytesAvailable, NULL);

        if (numAsyncBytesRead != 0)
        {
            if (numAsyncBytesAvailable > asyncBuffer.Length())
            {
                while (numAsyncBytesRead >= asyncBuffer.Length())
                {
                    BOOL res = ReadFile(stdoutRead, &asyncBuffer[0], asyncBuffer.Length(), &numAsyncBytesRead, NULL);
                    n_assert(res);
                    stdoutCaptureStream->Write(&asyncBuffer[0], numAsyncBytesRead);
                }
            }
            else
            {
                BOOL res = ReadFile(stdoutRead, &asyncBuffer[0], asyncBuffer.Length(), &numAsyncBytesRead, NULL);
                n_assert(res);
                stdoutCaptureStream->Write(&asyncBuffer[0], numAsyncBytesRead);
            }
        }
    }
    if (stderrCaptureStream.isvalid())
    {
        n_assert(0 != processInfo.hProcess);

        numAsyncBytesRead = 0;
        numAsyncBytesAvailable = 0;
        Memory::Clear(&asyncBuffer[0], asyncBuffer.Length());

        // peek to see if there is any data to read
        PeekNamedPipe(stderrRead, &asyncBuffer[0], asyncBuffer.Length(), &numAsyncBytesRead, &numAsyncBytesAvailable, NULL);

        if (numAsyncBytesRead != 0)
        {
            if (numAsyncBytesAvailable > asyncBuffer.Length())
            {
                while (numAsyncBytesRead >= asyncBuffer.Length())
                {
                    BOOL res = ReadFile(stderrRead, &asyncBuffer[0], asyncBuffer.Length(), &numAsyncBytesRead, NULL);
                    n_assert(res);
                    stderrCaptureStream->Write(&asyncBuffer[0], numAsyncBytesRead);
                }
            }
            else
            {
                BOOL res = ReadFile(stderrRead, &asyncBuffer[0], asyncBuffer.Length(), &numAsyncBytesRead, NULL);
                n_assert(res);
                stderrCaptureStream->Write(&asyncBuffer[0], numAsyncBytesRead);
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
IsProcessRunning(ProcessId processId)
{
    PROCESS_INFORMATION& processInfo = processIdAllocator.Get<Process_LaunchInfo>(processId.id);

    DWORD exitCode = 0;
    GetExitCodeProcess(processInfo.hProcess, &exitCode);
    if (exitCode != STILL_ACTIVE)
    {
        UpdateProcessStreams(processId);

        HANDLE& stdoutRead = processIdAllocator.Get<Process_StdoutRead>(processId.id);
        HANDLE& stdoutWrite = processIdAllocator.Get<Process_StdoutWrite>(processId.id);
        HANDLE& stderrRead = processIdAllocator.Get<Process_StderrRead>(processId.id);
        HANDLE& stderrWrite = processIdAllocator.Get<Process_StderrWrite>(processId.id);
        Ptr<IO::Stream>& stdoutCaptureStream = processIdAllocator.Get<Process_StdoutStream>(processId.id);
        Ptr<IO::Stream>& stderrCaptureStream = processIdAllocator.Get<Process_StderrStream>(processId.id);

        // cleanup
        if (stdoutCaptureStream.isvalid())
        {
            stdoutCaptureStream->Close();
            CloseHandle(stdoutRead);
            CloseHandle(stdoutWrite);
        }
        if (stderrCaptureStream.isvalid())
        {
            stderrCaptureStream->Close();
            CloseHandle(stderrRead);
            CloseHandle(stderrWrite);
        }
        CloseHandle(processInfo.hProcess);

        return false;
    }
    UpdateProcessStreams(processId);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
WaitForProcess(ProcessId processId)
{
    PROCESS_INFORMATION& processInfo = processIdAllocator.Get<Process_LaunchInfo>(processId.id);

    // wait until process exits
    WaitForSingleObject(processInfo.hProcess, INFINITE);
}

} // namespace System
