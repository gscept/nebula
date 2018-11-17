#pragma once
//------------------------------------------------------------------------------
/**
    @class Scripting::PythonServer
  
    Python backend for the Nebula scripting subsystem.
        
    (C) 2018 Individual contributors, see AUTHORS file
*/
#include "scripting/scriptserver.h"
#include "util/string.h"
#include "Python.h"

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
    virtual bool Open();
    /// close the script server
    virtual void Close();
    /// register a command with the script server
    virtual void RegisterCommand(const Util::String& name, const Ptr<Command>& cmd);
    /// unregister a command from the script server
    virtual void UnregisterCommand(const Util::String& name);        
    /// evaluate a script statement in a string
    virtual bool Eval(const Util::String& str);
	/// evaluate a script statement in a string with a single integer argument
	virtual bool EvalWithParameter(const Util::String& str, const Util::String& entry, uint parm);
    /// evaluate a script file
    virtual bool EvalScript(const IO::URI& uri);
	/// check if script contains a function, will assign nil to the name before (might remove global function)
	virtual bool ScriptHasFunction(const Util::String& script, const Util::String& func);
	/// creates a table for storing functions for faster calling
	virtual void CreateFunctionTable(const Util::String& name);
	/// store a function in table for faster calling
	virtual bool RegisterFunction(const Util::String& script, const Util::String& func, const Util::String& table, unsigned int entry, const Util::String & itemName = "object");
	/// remove a function from the table
	virtual bool UnregisterFunction(const Util::String& table, unsigned int entry);	

private:
    
};

} // namespace Scripting
//------------------------------------------------------------------------------