#pragma once
//------------------------------------------------------------------------------
/**
    @class Scripting::ScriptServer

    Server class of the scripting subsystem. The scripting server keeps
    track of all registered class script interfaces and registered
    global script commands. Subclasses of script server know how
    to execute scripts of a specific language.
    
    (C) 2006 Radon Labs
    (C) 2013-2018 Individual contributors, see AUTHORS file

*/
#include "core/refcounted.h"
#include "core/singleton.h"
#include "io/uri.h"
#include "util/variant.h"

//------------------------------------------------------------------------------
namespace Scripting
{


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
    /// evaluate a script statement in a string
    virtual bool Eval(const Util::String& str) { return false; }
protected:	
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

} // namespace Scripting
//------------------------------------------------------------------------------
