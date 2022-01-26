//------------------------------------------------------------------------------
//  pythonserver.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#define NOMINMAX
#include "foundation/stdneb.h"
#include "scripting/python/pythonserver.h"
#include "pybind11/embed.h"
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
    //FIXME fugly as f...
    static Util::String linebuffer;
    static Util::String errorbuffer;
    n_assert(!this->IsOpen());    
    if (ScriptServer::Open())
    {
        
        Py_Initialize();        

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
    n_assert(this->IsOpen());    
    
    // this will unregister all commands
    ScriptServer::Close();

    // close python
    Py_Finalize();
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
    exec.Format("sys.path.insert(0,\"%s\")\n", folder.LocalPath().AsCharPtr());
    this->Eval(exec);
}

//------------------------------------------------------------------------------
/**
    Evaluates a piece of Python code in a string.
*/
bool
PythonServer::Eval(const String& str)
{
    n_assert(this->IsOpen());    
    if (!str.IsValid())
    {
        return false;
    }

    return 0 != PyRun_SimpleString(str.AsCharPtr());
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
