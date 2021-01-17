#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::GameContentServer
    
    The game content server initializes access to game content on console
    platforms. The GameContentServer must be created by the main thread
    before the first IoServer is created.
    
    @copyright
    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#if (__WIN32__ || __OSX__ || __linux__)
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
#else
#error "IO::GameContentServer not implemented on this platform!"
#endif

