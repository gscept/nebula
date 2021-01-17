#pragma once
//------------------------------------------------------------------------------
/**
    @class Net::Socket
    
    Platform independent wrapper class for the Sockets API.
    
    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#if (__WIN32__)
#include "net/win32/win32socket.h"
namespace Net
{
class Socket : public Win32::Win32Socket
{
    __DeclareClass(Socket);
};
}
#elif __linux__
#include "net/posix/posixsocket.h"
namespace Net
{
class Socket : public Posix::PosixSocket
{
    __DeclareClass(Socket);
};
}
#else
#error "Socket class not implemented on this platform"
#endif
//------------------------------------------------------------------------------
