//------------------------------------------------------------------------------
//  posixprocess.cc
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "ids/idpool.h"
#include "system/process.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

namespace Posix
{
struct PosixProcess
{
    IO::URI path;
    IO::URI workingDir;
    Util::String args;
    Ptr<IO::Stream> stdoutCaptureStream;
    Ptr<IO::Stream> stderrCaptureStream;
    int outPipe = -1;
    int errPipe = -1;
    pid_t pid = -1;
    int exitCode = 0;
    bool isRunning = false;
    bool hasExitCode = false;
};

static Ids::IdPool idPool;
static Util::Array<PosixProcess> processes;

//------------------------------------------------------------------------------
/**
    Set a pipe read end to nonblocking so polling process output never stalls.
*/
bool
SetNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    return flags >= 0 && fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

//------------------------------------------------------------------------------
/**
*/
void
ClosePipe(int& fd)
{
    if (fd >= 0)
    {
        close(fd);
        fd = -1;
    }
}

//------------------------------------------------------------------------------
/**
    Report a child-side launch failure to the parent. The descriptor is
    close-on-exec, so a successful exec is reported by EOF instead.
*/
void
ReportLaunchFailure(int fd)
{
    int error = errno;
    write(fd, &error, sizeof(error));
    _exit(127);
}

//------------------------------------------------------------------------------
/**
    Read all currently available data from a capture pipe.
*/
void
UpdateStream(int fd, const Ptr<IO::Stream>& stream)
{
    if (fd < 0 || !stream.isvalid())
    {
        return;
    }

    char buffer[4096];
    while(true)
    {
        ssize_t bytesRead = read(fd, buffer, sizeof(buffer));
        if (bytesRead > 0)
        {
            stream->Write(buffer, (IO::Stream::Size)bytesRead);
        }
        else if (bytesRead < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            break;
        }
        else
        {
            break;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
UpdateStreams(PosixProcess& process)
{
    UpdateStream(process.outPipe, process.stdoutCaptureStream);
    UpdateStream(process.errPipe, process.stderrCaptureStream);
}

//------------------------------------------------------------------------------
/**
*/
void
CloseStreams(PosixProcess& process)
{
    if (process.stdoutCaptureStream.isvalid())
    {
        process.stdoutCaptureStream->Close();
    }

    if (process.stderrCaptureStream.isvalid())
    {
        process.stderrCaptureStream->Close();
    }

    ClosePipe(process.outPipe);
    ClosePipe(process.errPipe);
}

//------------------------------------------------------------------------------
/**
*/
bool
ValidProcessId(System::ProcessId processId)
{
    return processId != System::InvalidProcessId
           && processId.id < processes.Size();
}

//------------------------------------------------------------------------------
/**
*/
System::ProcessId
Launch(PosixProcess process)
{
    n_assert(process.path.IsValid());

    int out[2] = {-1, -1};
    int err[2] = {-1, -1};
    int launchStatus[2] = {-1, -1};

    if (process.stdoutCaptureStream.isvalid() && pipe(out) != 0)
    {
        n_error("[PosixProcess]: Failed to create stdout pipe\n");
        return System::InvalidProcessId;
    }

    if (process.stderrCaptureStream.isvalid() && pipe(err) != 0)
    {
        ClosePipe(out[0]);
        ClosePipe(out[1]);
        n_error("[PosixProcess]: Failed to create stderr pipe\n");
        return System::InvalidProcessId;
    }

    if (pipe(launchStatus) != 0 || fcntl(launchStatus[1], F_SETFD, FD_CLOEXEC) != 0)
    {
        ClosePipe(out[0]);
        ClosePipe(out[1]);
        ClosePipe(err[0]);
        ClosePipe(err[1]);
        ClosePipe(launchStatus[0]);
        ClosePipe(launchStatus[1]);
        n_error("[PosixProcess]: Failed to create launch status pipe\n");
        return System::InvalidProcessId;
    }

    pid_t pid = fork();
    if (pid < 0)
    { // parent
        ClosePipe(out[0]);
        ClosePipe(out[1]);
        ClosePipe(err[0]);
        ClosePipe(err[1]);
        ClosePipe(launchStatus[0]);
        ClosePipe(launchStatus[1]);
        n_error("[PosixProcess]: Failed to fork\n");
        return System::InvalidProcessId;
    }
    if (pid == 0)
    { // child
        close(launchStatus[0]);
        if (process.workingDir.IsValid())
        {
            if (chdir(process.workingDir.LocalPath().AsCharPtr()) != 0)
            {
                ReportLaunchFailure(launchStatus[1]);
            }
        }
        if (process.stdoutCaptureStream.isvalid())
        {
            close(out[0]);
            if (dup2(out[1], STDOUT_FILENO) < 0)
            {
                ReportLaunchFailure(launchStatus[1]);
            }
            close(out[1]);
        }
        if (process.stderrCaptureStream.isvalid())
        {
            close(err[0]);
            if (dup2(err[1], STDERR_FILENO) < 0)
            {
                ReportLaunchFailure(launchStatus[1]);
            }
            close(err[1]);
        }

        Util::Array<Util::String> stringArgs = process.args.Tokenize(" ", '"');
        char** argv = new char*[stringArgs.Size() + 2];
        argv[0] = const_cast<char*>(process.path.LocalPath().AsCharPtr());
        for (IndexT i = 0; i < stringArgs.Size(); i++)
        {
            argv[i + 1] = const_cast<char*>(stringArgs[i].AsCharPtr());
        }
        argv[stringArgs.Size() + 1] = nullptr;
        execvp(argv[0], argv);
        ReportLaunchFailure(launchStatus[1]);
    }

    ClosePipe(launchStatus[1]);
    ClosePipe(out[1]);
    ClosePipe(err[1]);

    int launchError = 0;
    ssize_t launchBytes = -1;
    do
    {
        launchBytes = read(launchStatus[0], &launchError, sizeof(launchError));
    }
    while (launchBytes < 0 && errno == EINTR);

    ClosePipe(launchStatus[0]);

    if (launchBytes > 0)
    {
        ClosePipe(out[0]);
        ClosePipe(err[0]);
        waitpid(pid, nullptr, 0);
        return System::InvalidProcessId;
    }

    process.outPipe = out[0];
    process.errPipe = err[0];
    if ((process.outPipe >= 0 && !SetNonBlocking(process.outPipe)) ||
        (process.errPipe >= 0 && !SetNonBlocking(process.errPipe)))
    {
        CloseStreams(process);
        kill(pid, SIGKILL);
        waitpid(pid, nullptr, 0);
        return System::InvalidProcessId;
    }

    if (process.stdoutCaptureStream.isvalid())
    {
        process.stdoutCaptureStream->SetAccessMode(IO::Stream::WriteAccess);
        if (!process.stdoutCaptureStream->Open())
        {
            CloseStreams(process);
            kill(pid, SIGKILL);
            waitpid(pid, nullptr, 0);
            return System::InvalidProcessId;
        }
    }

    if (process.stderrCaptureStream.isvalid())
    {
        process.stderrCaptureStream->SetAccessMode(IO::Stream::WriteAccess);
        if (!process.stderrCaptureStream->Open())
        {
            CloseStreams(process);
            kill(pid, SIGKILL);
            waitpid(pid, nullptr, 0);
            return System::InvalidProcessId;
        }
    }

    process.pid = pid;
    process.isRunning = true;

    uint32_t id = idPool.Alloc();
    
    while (processes.Size() <= id)
    {
        processes.Append({});
    }

    processes[id] = std::move(process);

    return System::ProcessId(id);
}
} // namespace Posix

//--------------------------------------------------------------------------

namespace System
{
using namespace Posix;

//------------------------------------------------------------------------------
/**
*/
ProcessId
StartProcess(const ProcessStartInfo& createInfo)
{
    PosixProcess process;
    process.path = createInfo.exePath;
    process.workingDir = createInfo.workingDir;
    process.args = createInfo.args;
    process.stdoutCaptureStream = createInfo.outputStream;
    process.stderrCaptureStream = createInfo.errorStream;
    return Launch(std::move(process));
}

//------------------------------------------------------------------------------
/**
*/
void
UpdateProcessStreams(ProcessId processId)
{
    if (!ValidProcessId(processId))
    {
        return;
    }

    PosixProcess& process = processes[processId.id];

    if (process.isRunning || process.hasExitCode)
    {
        UpdateStreams(process);
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
IsProcessRunning(ProcessId processId)
{
    if (!ValidProcessId(processId))
    {
        return false;
    }

    PosixProcess& process = processes[processId.id];

    if (!process.isRunning)
    {
        return false;
    }

    UpdateStreams(process);

    int status = 0;
    pid_t result = waitpid(process.pid, &status, WNOHANG);
    if (result == process.pid)
    {
        process.exitCode = WIFEXITED(status) ? WEXITSTATUS(status) :
            (WIFSIGNALED(status) ? 128 + WTERMSIG(status) : 1);
        process.hasExitCode = true;
        process.isRunning = false;
        UpdateStreams(process);
        CloseStreams(process);
    }

    return process.isRunning;
}

//------------------------------------------------------------------------------
/**
*/
uint
WaitForProcess(ProcessId processId)
{
    if (!ValidProcessId(processId))
    {
        return 0;
    }

    PosixProcess& process = processes[processId.id];

    if (process.isRunning)
    {
        int status = 0;
        pid_t result = 0;
        do
        {
            result = waitpid(process.pid, &status, WNOHANG);
            if (result == 0)
            {
                UpdateStreams(process);
                // allow time for pipes to drain
                usleep(1000);
            }
        }
        while (result == 0 || (result < 0 && errno == EINTR));

        if (result == process.pid)
        {
            process.exitCode = WIFEXITED(status) ? WEXITSTATUS(status) :
                (WIFSIGNALED(status) ? 128 + WTERMSIG(status) : 1);
            process.hasExitCode = true;
            process.isRunning = false;
        }
    }

    UpdateStreams(process);
    CloseStreams(process);
    
    uint result = (uint)process.exitCode;
    
    idPool.Dealloc(processId.id);
    processes[processId.id] = {};
    
    return result;
}
} // namespace System
