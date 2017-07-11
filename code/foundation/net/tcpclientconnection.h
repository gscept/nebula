#pragma once
//------------------------------------------------------------------------------
/**
    @class Net::TcpClientConnection

    See StdTcpClientConnection for details!

    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#if (__WIN32__ || __XBOX360__ || __PS3__ || __OSX__ || __APPLE__ || __linux__)
#include "net/tcp/stdtcpclientconnection.h"
namespace Net
{
class TcpClientConnection : public StdTcpClientConnection
{
    __DeclareClass(TcpClientConnection);
};
}
#elif __WII__
#include "net/wii/wiitcpclientconnection.h"
namespace Net
{
class TcpClientConnection : public Wii::WiiTcpClientConnection
{
    __DeclareClass(TcpClientConnection);
};
}
#else
#error "Net::TcpClientConnection not implemented on this platform!"
#endif
//------------------------------------------------------------------------------
