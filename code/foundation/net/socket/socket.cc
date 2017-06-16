//------------------------------------------------------------------------------
//  socket.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "net/socket/socket.h"

namespace Net
{
#if (__WIN32__ || __XBOX360__)
__ImplementClass(Net::Socket, 'SOCK', Win360::Win360Socket);
#elif __WII__
// there is no class on Wii
#elif __PS3__
__ImplementClass(Net::Socket, 'SOCK', PS3::PS3Socket);
#elif __linux__
__ImplementClass(Net::Socket, 'SOCK', Posix::PosixSocket);
#else
#error "Socket class not implemented on this platform!"
#endif
}
