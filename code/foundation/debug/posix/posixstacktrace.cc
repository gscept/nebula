//------------------------------------------------------------------------------
//  posixstacktrace.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "debug/posix/posixstacktrace.h"
#if __linux__
#include <execinfo.h>
#include <dlfcn.h>
#include <link.h>
#include <spawn.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <cxxabi.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#endif

#if __linux__
namespace
{

const int MaxFrames = 64;
const int MaxLineLength = 1024;

struct StackFrame
{
    uintptr_t lookup = 0;               // address relative to its module, as addr2line expects
    const char* module = nullptr;
    const char* symbol = nullptr;       // nearest exported symbol, if any
    uintptr_t symbolOffset = 0;
    char location[MaxLineLength] = {};  // "function at file:line" from addr2line
};

//------------------------------------------------------------------------------
/**
    Writes straight to the stderr file descriptor, bypassing stdio.
*/
void
WriteRaw(const char* text)
{
    size_t length = strlen(text);
    while (length > 0)
    {
        ssize_t written = write(STDERR_FILENO, text, length);
        if (written < 0)
        {
            if (errno == EINTR)
                continue;
            return;
        }
        text += written;
        length -= written;
    }
}

//------------------------------------------------------------------------------
/**
    Runs addr2line once for all frames from the same module and stores the
    results in the frames. Leaves a frame's location empty if it can't be resolved.
*/
void
ResolveModuleFrames(StackFrame* frames, int numFrames, int first)
{
    const char* module = frames[first].module;
    char addresses[MaxFrames][32];
    int indices[MaxFrames];
    char* argv[MaxFrames + 8];
    int argc = 0;
    argv[argc++] = const_cast<char*>("addr2line");
    argv[argc++] = const_cast<char*>("-f");
    argv[argc++] = const_cast<char*>("-C");
    argv[argc++] = const_cast<char*>("-p");
    argv[argc++] = const_cast<char*>("-e");
    argv[argc++] = const_cast<char*>(module);
    int numAddresses = 0;
    for (int i = first; i < numFrames; i++)
    {
        if (frames[i].module != nullptr && strcmp(frames[i].module, module) == 0)
        {
            snprintf(addresses[numAddresses], sizeof(addresses[numAddresses]), "0x%zx", (size_t)frames[i].lookup);
            argv[argc++] = addresses[numAddresses];
            indices[numAddresses++] = i;
        }
    }
    argv[argc] = nullptr;

    // Keep addr2line offline, so a crash never waits on debuginfod downloads
    const int MaxEnv = 1024;
    char* envp[MaxEnv];
    int envc = 0;
    for (char** env = environ; *env != nullptr && envc < MaxEnv - 1; env++)
    {
        if (strncmp(*env, "DEBUGINFOD_URLS=", 16) != 0)
            envp[envc++] = *env;
    }
    envp[envc] = nullptr;

    int fds[2];
    if (pipe2(fds, O_CLOEXEC) != 0)
        return;
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, fds[1], STDOUT_FILENO);
    posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);
    pid_t pid;
    int res = posix_spawnp(&pid, "addr2line", &actions, nullptr, argv, envp);
    posix_spawn_file_actions_destroy(&actions);
    close(fds[1]);
    if (res != 0)
    {
        close(fds[0]);
        return;
    }

    // With -p and without -i, addr2line prints exactly one line per address
    FILE* output = fdopen(fds[0], "r");
    for (int i = 0; i < numAddresses; i++)
    {
        char* location = frames[indices[i]].location;
        if (fgets(location, sizeof(frames[indices[i]].location), output) == nullptr)
            break;
        location[strcspn(location, "\n")] = '\0';
        if (strncmp(location, "??", 2) == 0)
            location[0] = '\0';
    }
    fclose(output);
    waitpid(pid, nullptr, 0);
}

//------------------------------------------------------------------------------
/**
    Resolves the stack of the calling thread into lines. The trace starts at
    the frame that returns to caller, leaving out skip more frames after it.
    Returns the number of lines.
*/
int
WalkStack(void* caller, int skip, char (*lines)[MaxLineLength])
{
    void* addresses[MaxFrames];
    int numAddresses = backtrace(addresses, MaxFrames);

    // Searching for the return address instead of counting frames keeps this
    // right when the compiler inlines or tail-calls the functions in between
    int first = 0;
    for (int i = 0; i < numAddresses; i++)
    {
        if (addresses[i] == caller)
        {
            first = i;
            break;
        }
    }

    char exePath[4096];
    ssize_t exePathLen = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    exePath[exePathLen > 0 ? exePathLen : 0] = '\0';

    static StackFrame frames[MaxFrames];
    int numFrames = 0;
    for (int i = first + skip; i < numAddresses; i++)
    {
        StackFrame& frame = frames[numFrames++];
        frame = StackFrame();

        // A return address points after the call instruction, step back into it
        uintptr_t address = (uintptr_t)addresses[i] - 1;
        Dl_info info;
        link_map* map = nullptr;
        if (dladdr1((void*)address, &info, (void**)&map, RTLD_DL_LINKMAP) != 0 && map != nullptr)
        {
            frame.module = map->l_name[0] != '\0' ? map->l_name : exePath;
            frame.lookup = address - map->l_addr;
            frame.symbol = info.dli_sname;
            frame.symbolOffset = address - (uintptr_t)info.dli_saddr;
        }
        else
        {
            frame.lookup = address;
        }
    }

    for (int i = 0; i < numFrames; i++)
    {
        if (frames[i].module == nullptr || frames[i].location[0] != '\0')
            continue;
        bool resolved = false;
        for (int j = 0; j < i && !resolved; j++)
            resolved = frames[j].module != nullptr && strcmp(frames[j].module, frames[i].module) == 0;
        if (!resolved)
            ResolveModuleFrames(frames, numFrames, i);
    }

    for (int i = 0; i < numFrames; i++)
    {
        const StackFrame& frame = frames[i];
        const char* moduleName = frame.module != nullptr ? frame.module : "??";
        const char* slash = strrchr(moduleName, '/');
        if (slash != nullptr)
            moduleName = slash + 1;

        if (frame.location[0] != '\0')
        {
            snprintf(lines[i], MaxLineLength, "#%-2d %s (%s)\n", i, frame.location, moduleName);
        }
        else if (frame.symbol != nullptr)
        {
            int status = 0;
            char* demangled = abi::__cxa_demangle(frame.symbol, nullptr, nullptr, &status);
            // The module offset lets addr2line resolve the frame later, the symbol offset can't
            snprintf(lines[i], MaxLineLength, "#%-2d %s+0x%zx (%s+0x%zx)\n", i, status == 0 ? demangled : frame.symbol, (size_t)frame.symbolOffset, moduleName, (size_t)frame.lookup);
            free(demangled);
        }
        else
        {
            snprintf(lines[i], MaxLineLength, "#%-2d %s+0x%zx\n", i, moduleName, (size_t)frame.lookup);
        }
    }
    return numFrames;
}

//------------------------------------------------------------------------------
/**
    Shared by GenerateStackTrace and PrintStackTrace.
*/
Util::Array<Util::String>
CollectStackTrace(void* caller, int skip, bool writeRaw)
{
    Util::Array<Util::String> trace;

    // An error raised while tracing would come back here, skip the nested trace
    static thread_local bool collecting = false;
    if (collecting)
        return trace;
    collecting = true;
    {
        static std::mutex lock;
        std::lock_guard<std::mutex> guard(lock);
        static char lines[MaxFrames][MaxLineLength];
        int numLines = WalkStack(caller, skip, lines);

        // Out before anything allocates, in case the caller's handling of the trace fails
        if (writeRaw)
        {
            WriteRaw("*** STACK TRACE ***\n");
            for (int i = 0; i < numLines; i++)
                WriteRaw(lines[i]);
        }

        trace.Reserve(numLines);
        for (int i = 0; i < numLines; i++)
            trace.Append(Util::String(lines[i]));
    }
    collecting = false;
    return trace;
}

} // namespace
#endif

namespace Posix
{

//------------------------------------------------------------------------------
/**
*/
__attribute__((noinline)) const Util::Array<Util::String>
PosixStackTrace::GenerateStackTrace()
{
#if __linux__
    return CollectStackTrace(__builtin_return_address(0), 0, true);
#else
    return Util::Array<Util::String>();
#endif
}

//------------------------------------------------------------------------------
/**
*/
__attribute__((noinline)) void
PosixStackTrace::PrintStackTrace(int skip)
{
#if __linux__
    const Util::Array<Util::String> trace = CollectStackTrace(__builtin_return_address(0), skip, false);
    for (IndexT i = 0; i < trace.Size(); i++)
    {
        n_printf("%s", trace[i].AsCharPtr());
    }
#endif
}

} // namespace Posix
