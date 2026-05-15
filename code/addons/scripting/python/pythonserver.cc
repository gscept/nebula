//------------------------------------------------------------------------------
//  pythonserver.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#define NOMINMAX
#include "foundation/stdneb.h"
#include "scripting/python/pythonserver.h"
#include "nanobind/nanobind.h"
#include "conversion.h"
#include "PyLogHook.h"
#include "io/ioserver.h"
#include "io/textreader.h"

using namespace IO;

namespace Scripting
{
__ImplementClass(Scripting::PythonServer, 'PYSS', Scripting::ScriptServer);
__ImplementSingleton(Scripting::PythonServer);

using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
PythonServer::PythonServer() 
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
PythonServer::~PythonServer()
{
    if (this->IsOpen())
    {
        this->Close();
    }
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
bool
PythonServer::Open()
{
    Threading::CriticalScope lock(&this->pythonLock);

    //FIXME fugly as f...
    static Util::String linebuffer;
    static Util::String errorbuffer;
    n_assert(!this->IsOpen());    
    if (ScriptServer::Open())
    {
        Python::RegisterNebulaModules();

        for (const Scripting::ScriptModuleInit& init : initFuncs)
        {
            init();
        }
        if (!this->pythonInitialized)
        {
            Py_Initialize();
            this->pythonInitialized = true;
        }
        
        nanobind::detail::init(nullptr);
        tyti::pylog::redirect_stdout([](const char* msg) 
        {            
            // collect until we get a newline
            if (Util::String::StrChr(msg, '\n'))
            {
                n_printf("%s%s", linebuffer.AsCharPtr(), msg);
                linebuffer.Clear();                
            }
            else
            {
                linebuffer.Append(msg);
            }            
        });
        tyti::pylog::redirect_stderr([](const char* msg) 
        {
            // collect until we get a newline
            if (Util::String::StrChr(msg, '\n'))
            {
                n_warning("%s%s", errorbuffer.AsCharPtr(), msg);
                errorbuffer.Clear();
            }
            else
            {
                errorbuffer.Append(msg);
            }
        });
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
PythonServer::Close()
{
    Threading::CriticalScope lock(&this->pythonLock);

    n_assert(this->IsOpen());    
    
    // this will unregister all commands
    ScriptServer::Close();

    // NOTE: Do NOT call Py_Finalize() here.
    // Python cannot be safely re-initialized after finalization in the same process.
    // During runtime module reloads, Close() can be called multiple times, and
    // Py_Finalize() would corrupt interpreter state for subsequent Py_Initialize() calls.
    // Only finalize at true process shutdown.
    // Py_Finalize();
}


//------------------------------------------------------------------------------
/**
    add module path
*/
void
PythonServer::AddModulePath(const IO::URI & folder)
{
    n_assert(this->IsOpen());
    Util::String exec;
    exec.Format("import sys\nsys.path.insert(0,\"%s\")\n", folder.LocalPath().AsCharPtr());
    this->Eval(exec);
}

//------------------------------------------------------------------------------
/**
    Evaluates a piece of Python code in a string.
*/
bool
PythonServer::Eval(const String& str)
{
    Threading::CriticalScope lock(&this->pythonLock);

    n_assert(this->IsOpen());    
    if (!str.IsValid())
    {
        return false;
    }

    if (!Py_IsInitialized())
    {
        n_warning("PythonServer::Eval(): Python runtime is not initialized");
        return false;
    }

    PyGILState_STATE gilState = PyGILState_Ensure();
    int runResult = PyRun_SimpleStringFlags(str.AsCharPtr(), nullptr);
    if (runResult != 0 && PyErr_Occurred())
    {
        PyErr_Print();
    }
    PyGILState_Release(gilState);

    return runResult == 0;
}


//------------------------------------------------------------------------------
/**
    Evaluate script in file
*/
bool
PythonServer::EvalFile(const IO::URI& file)
{
    n_assert(this->IsOpen());
    auto stream = IO::IoServer::Instance()->CreateStream(file);
    Ptr<TextReader> reader = IO::TextReader::Create();
    reader->SetStream(stream);
    if (reader->Open())
    {
        Util::String str = reader->ReadAll();
        reader->Close();
        return this->Eval(str);
    }
    return false;
}



} // namespace Scripting
