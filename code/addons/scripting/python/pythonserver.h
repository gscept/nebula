#pragma once
//------------------------------------------------------------------------------
/**
    @class Scripting::PythonServer
  
    Python backend for the Nebula scripting subsystem.

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
#include "scripting/scriptserver.h"
#include "util/string.h"

//------------------------------------------------------------------------------
namespace Scripting
{
class PythonServer : public ScriptServer
{
    __DeclareClass(PythonServer);
    __DeclareSingleton(PythonServer);
public:
    /// constructor
    PythonServer();
    /// destructor
    virtual ~PythonServer();
    /// open the script server
    bool Open();
    /// close the script server
    void Close();    
    /// add module search path
    void AddModulePath(const IO::URI & folder);
    /// evaluate a script statement in a string
    bool Eval(const Util::String& str); 
    /// evaluate script in file
    bool EvalFile(const IO::URI& file);
private:

};

} // namespace Scripting
//------------------------------------------------------------------------------
