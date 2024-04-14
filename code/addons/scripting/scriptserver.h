#pragma once
//------------------------------------------------------------------------------
/**
    @class Scripting::ScriptServer

    Server class of the scripting subsystem. The scripting server keeps
    track of all registered class script interfaces and registered
    global script commands. Subclasses of script server know how
    to execute scripts of a specific language.
    
    @copyright
    (C) 2006 Radon Labs
    (C) 2013-2020 Individual contributors, see AUTHORS file

*/
#include "core/refcounted.h"
#include "core/singleton.h"
#include "io/uri.h"
#include "util/variant.h"
#include "util/delegate.h"

//------------------------------------------------------------------------------
namespace Scripting
{

using ScriptModuleInit = Util::Delegate<void()>;

class ScriptServer : public Core::RefCounted
{
    __DeclareClass(ScriptServer);
    __DeclareSingleton(ScriptServer);
public:
    /// constructor
    ScriptServer();
    /// destructor
    virtual ~ScriptServer();
    /// open the script server
    virtual bool Open();
    /// close the script server
    virtual void Close();
    /// return true if open
    bool IsOpen() const;
    /// set debugging with the HTTP interface
    void SetDebug(const bool b);    
    /// add module search path
    virtual void AddModulePath(const IO::URI & folder) { n_assert(false); }
    /// evaluate a script statement in a string
    virtual bool Eval(const Util::String& str) { return false; }
    /// evaluate script in file
    virtual bool EvalFile(const IO::URI& file) { return false; }

    static void RegisterModuleInit(const ScriptModuleInit& init);
protected:  
    static Util::Array<ScriptModuleInit> initFuncs;
    bool isOpen;
    bool debug;
};

//------------------------------------------------------------------------------
/**
*/
inline
bool
ScriptServer::IsOpen() const
{
    return this->isOpen;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
ScriptServer::SetDebug(const bool b)
{
    this->debug = b;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
ScriptServer::RegisterModuleInit(const ScriptModuleInit& init)
{
    initFuncs.Append(init);
}

} // namespace Scripting
//------------------------------------------------------------------------------
