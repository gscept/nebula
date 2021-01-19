#pragma once
//------------------------------------------------------------------------------
/**
    @class Net::TcpClient

    See StdTcpClient for details.

    @copyright
    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#if (__WIN32__ || __OSX__ || __APPLE__ || __linux__)
#include "net/tcp/stdtcpclient.h"
namespace Net
{
class TcpClient : public StdTcpClient
{
    __DeclareClass(TcpClient);
};
}
#else
#error "Net::TcpClient not implemented on this platform!"
#endif
//------------------------------------------------------------------------------

