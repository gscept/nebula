//------------------------------------------------------------------------------
//  gamecontentserver.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "io/gamecontentserver.h"

namespace IO
{
#if (__WIN32__ || __OSX__ || __linux__)
__ImplementClass(IO::GameContentServer, 'IGCS', Base::GameContentServerBase);
#else
#error "IO::GameContentServer not implemented on this platform!"
#endif
__ImplementInterfaceSingleton(IO::GameContentServer);

//------------------------------------------------------------------------------
/**
*/
GameContentServer::GameContentServer()
{
    __ConstructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
GameContentServer::~GameContentServer()
{
    __DestructInterfaceSingleton;
}

} // namespace IO
