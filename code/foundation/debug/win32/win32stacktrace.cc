//------------------------------------------------------------------------------
//  win32stacktrace.cc
//  (C) 2015-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "debug/win32/win32stacktrace.h"
#include "stackwalker/StackWalker.h"

class StackWalkerToString: public StackWalker
{
public: 

    StackWalkerToString(int options) : StackWalker(options) {};
    const Util::Array<Util::String>& GetTrace()
    {
        return this->trace;
    }

protected:
    virtual void OnOutput(LPCSTR szText)
    {
        this->trace.Append(szText);     
    }
    virtual void OnSymInit(LPCSTR szSearchPath, DWORD symOptions, LPCSTR szUserName){}
    virtual void OnLoadModule(LPCSTR img, LPCSTR mod, DWORD64 baseAddr, DWORD size, DWORD result, LPCSTR symType, LPCSTR pdbName, ULONGLONG fileVersion){}
    //virtual void OnCallstackEntry(CallstackEntryType eType, CallstackEntry &entry);
    virtual void OnDbgHelpErr(LPCSTR szFuncName, DWORD gle, DWORD64 addr){}
    Util::Array<Util::String> trace;
};


//------------------------------------------------------------------------------
/**
*/
const Util::Array<Util::String>
Win32::Win32StackTrace::GenerateStackTrace()
{
    StackWalkerToString sw(StackWalker::RetrieveVerbose & StackWalker::SymBuildPath);
    sw.ShowCallstack();
    return sw.GetTrace();
}

//------------------------------------------------------------------------------
/**
*/
void
Win32::Win32StackTrace::PrintStackTrace(int skiprows)
{
    const Util::Array<Util::String>& trace = Win32::Win32StackTrace::GenerateStackTrace();  
    for (int i = Math::max(0,skiprows-1); i < trace.Size(); i++)
    {
        n_printf(trace[i].AsCharPtr());
    }
}