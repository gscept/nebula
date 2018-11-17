//------------------------------------------------------------------------------
//  pythonserver.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "scripting/python/pythonserver.h"

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
    n_assert(!this->IsOpen());    
    if (ScriptServer::Open())
    {
        Py_Initialize();

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