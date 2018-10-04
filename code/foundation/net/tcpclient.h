#pragma once
//------------------------------------------------------------------------------
/**
    @class Net::TcpClient

    See StdTcpClient for details.

    (C) 2009 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#if (__WIN32__ || __XBOX360__ || __PS3__ || __OSX__ || __APPLE__ || __linux__)
#include "net/tcp/stdtcpclient.h"
namespace Net
{
class TcpClient : public StdTcpClient
{
    __DeclareClass(TcpClient);
};
}
#elif __WII__
// only an empty stub on the Wii
#include "core/refcounted.h"
namespace Net
{
class TcpClient : public Core::RefCounted
{
    __DeclareClass(TcpClient);
};
}
#else
#error "Net::TcpClient not implemented on this platform!"
#endif
//------------------------------------------------------------------------------

