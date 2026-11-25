//------------------------------------------------------------------------------
//  win32stacktrace.cc
//  (C) 2015-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "debug/win32/win32stacktrace.h"
#include <windows.h>
#include <dbghelp.h>
#include <intrin.h>
#include <cstdio>
#include <cstring>
#include <mutex>

#pragma comment(lib, "dbghelp.lib")

namespace
{

const int MaxFrames = 64;
const int MaxLineLength = 1024;
const int MaxSymbolNameLength = 1024;

//------------------------------------------------------------------------------
/**
    Writes straight to the stderr handle, bypassing the C runtime.
*/
void
WriteRaw(const char* text)
{
    HANDLE handle = GetStdHandle(STD_ERROR_HANDLE);
    if (handle == NULL || handle == INVALID_HANDLE_VALUE)
        return;
    DWORD written;
    WriteFile(handle, text, (DWORD)strlen(text), &written, NULL);
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
    int numAddresses = RtlCaptureStackBackTrace(0, MaxFrames, addresses, NULL);

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

    // dbghelp needs the module list, load it on the first trace and refresh it after that
    HANDLE process = GetCurrentProcess();
    static bool symbolsLoaded = false;
    if (!symbolsLoaded)
    {
        SymSetOptions(SymGetOptions() | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS);
        symbolsLoaded = SymInitialize(process, NULL, TRUE) != FALSE;
    }
    else
    {
        SymRefreshModuleList(process);
    }

    int numFrames = 0;
    for (int i = first + skip; i < numAddresses; i++)
    {
        // A return address points after the call instruction, step back into it
        DWORD64 address = (DWORD64)addresses[i] - 1;

        char moduleName[MAX_PATH] = "??";
        HMODULE module = NULL;
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)address, &module))
        {
            char path[MAX_PATH];
            DWORD length = GetModuleFileNameA(module, path, MAX_PATH);
            if (length > 0 && length < MAX_PATH)
            {
                const char* slash = strrchr(path, '\\');
                snprintf(moduleName, sizeof(moduleName), "%s", slash != nullptr ? slash + 1 : path);
            }
        }

        alignas(SYMBOL_INFO) char symbolBuffer[sizeof(SYMBOL_INFO) + MaxSymbolNameLength];
        SYMBOL_INFO* symbol = (SYMBOL_INFO*)symbolBuffer;
        memset(symbol, 0, sizeof(SYMBOL_INFO));
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MaxSymbolNameLength;
        DWORD64 symbolOffset = 0;
        bool hasSymbol = symbolsLoaded && SymFromAddr(process, address, &symbolOffset, symbol) != FALSE;

        IMAGEHLP_LINE64 line = {};
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        DWORD lineOffset = 0;
        bool hasLine = hasSymbol && SymGetLineFromAddr64(process, address, &lineOffset, &line) != FALSE;

        // The module offset lets the frame be resolved later against the pdb, the symbol offset can't
        unsigned long long moduleOffset = (unsigned long long)(address - (DWORD64)module);
        if (hasLine)
            snprintf(lines[numFrames], MaxLineLength, "#%-2d %s at %s:%lu (%s)\n", numFrames, symbol->Name, line.FileName, (unsigned long)line.LineNumber, moduleName);
        else if (hasSymbol)
            snprintf(lines[numFrames], MaxLineLength, "#%-2d %s+0x%llx (%s+0x%llx)\n", numFrames, symbol->Name, (unsigned long long)symbolOffset, moduleName, moduleOffset);
        else
            snprintf(lines[numFrames], MaxLineLength, "#%-2d %s+0x%llx\n", numFrames, moduleName, moduleOffset);
        numFrames++;
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
        // dbghelp isn't thread safe
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

namespace Win32
{

//------------------------------------------------------------------------------
/**
*/
__declspec(noinline) const Util::Array<Util::String>
Win32StackTrace::GenerateStackTrace()
{
    return CollectStackTrace(_ReturnAddress(), 0, true);
}

//------------------------------------------------------------------------------
/**
*/
__declspec(noinline) void
Win32StackTrace::PrintStackTrace(int skip)
{
    const Util::Array<Util::String> trace = CollectStackTrace(_ReturnAddress(), skip, false);
    for (IndexT i = 0; i < trace.Size(); i++)
    {
        n_printf("%s", trace[i].AsCharPtr());
    }
}

} // namespace Win32
