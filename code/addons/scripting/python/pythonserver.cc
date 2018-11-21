//------------------------------------------------------------------------------
//  pythonserver.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#define NOMINMAX
#include "foundation/stdneb.h"
#include "scripting/python/pythonserver.h"
#include "pybind11/embed.h"
#include "Python.h"
#include "PyLogHook.h"
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
    Evaluates a piece of Python code in a string.
*/
bool
PythonServer::Eval(const String& str)
{
    n_assert(this->IsOpen());    
    n_assert(str.IsValid());

    return 0 != PyRun_SimpleString(str.AsCharPtr());
}



} // namespace Scripting