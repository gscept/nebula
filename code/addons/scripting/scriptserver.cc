//------------------------------------------------------------------------------
//  scriptserver.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "scripting/scriptserver.h"
#include "io/console.h"
#include "http/httpserverproxy.h"

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
#if __NEBULA_HTTP__
    if (this->debug)
    {
	    // create handler for http debug requests
	    this->pageHandler = Debug::ScriptingPageHandler::Create();
	    Http::HttpServerProxy::Instance()->AttachRequestHandler(this->pageHandler.cast<Http::HttpRequestHandler>());
    }
#endif
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