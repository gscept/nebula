#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::GameContentServer
    
    The game content server initializes access to game content on console
    platforms. The GameContentServer must be created by the main thread
    before the first IoServer is created.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#if (__WIN32__ || __XBOX360__ || __WII__ || __OSX__ || __linux__)
#include "io/base/gamecontentserverbase.h"
namespace IO
{
class GameContentServer : public Base::GameContentServerBase
{
    __DeclareClass(GameContentServer);
    __DeclareInterfaceSingleton(GameContentServer);
public:
    /// constructor
    GameContentServer();
    /// destructor
    virtual ~GameContentServer();
};
}
#elif __PS3__
#include "io/ps3/ps3gamecontentserver.h"
namespace IO
{
class GameContentServer : public PS3::PS3GameContentServer
{
    __DeclareClass(GameContentServer);
    __DeclareInterfaceSingleton(GameContentServer);
public:
    /// constructor
    GameContentServer();
    /// destructor
    virtual ~GameContentServer();
};
}
#else
#error "IO::GameContentServer not implemented on this platform!"
#endif

