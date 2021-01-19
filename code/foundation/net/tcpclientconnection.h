#pragma once
//------------------------------------------------------------------------------
/**
    @class Net::TcpClientConnection

    See StdTcpClientConnection for details!

    @copyright
    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#if (__WIN32__ || __OSX__ || __APPLE__ || __linux__)
#include "net/tcp/stdtcpclientconnection.h"
namespace Net
{
class TcpClientConnection : public StdTcpClientConnection
{
    __DeclareClass(TcpClientConnection);
};
}
#else
#error "Net::TcpClientConnection not implemented on this platform!"
#endif
//------------------------------------------------------------------------------
