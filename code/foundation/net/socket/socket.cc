//------------------------------------------------------------------------------
//  socket.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "net/socket/socket.h"

namespace Net
{
#if (__WIN32__)
__ImplementClass(Net::Socket, 'SOCK', Win32::Win32Socket);
#elif __linux__
__ImplementClass(Net::Socket, 'SOCK', Posix::PosixSocket);
#else
#error "Socket class not implemented on this platform!"
#endif
}
