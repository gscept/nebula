//------------------------------------------------------------------------------
//  scriptserver.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "scripting/scriptserver.h"
#include "io/console.h"

namespace Scripting
{
__ImplementClass(Scripting::ScriptServer, 'SCRS', Core::RefCounted);
__ImplementSingleton(Scripting::ScriptServer);

using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
ScriptServer::ScriptServer() :
    isOpen(false),
    debug(true)
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
ScriptServer::~ScriptServer()
{
    n_assert(!this->isOpen);
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
bool
ScriptServer::Open()
{
    n_assert(!this->isOpen);
    this->isOpen = true;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
ScriptServer::Close()
{
    n_assert(this->isOpen);    
    this->isOpen = false;
}


} // namespace Scripting