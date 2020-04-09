#pragma once
//------------------------------------------------------------------------------
/**
    @class Net::TcpServer

    Front-end wrapper class for StdTcpServer, see StdTcpServer for details!

    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#if (__WIN32__ || __OSX__ || __APPLE__ || __linux__)
#include "net/tcp/stdtcpserver.h"
namespace Net
{
class TcpServer : public StdTcpServer
{
    __DeclareClass(TcpServer);
};
}
#else
#error "Net::TcpServer not implemented on this platform!"
#endif
//------------------------------------------------------------------------------
