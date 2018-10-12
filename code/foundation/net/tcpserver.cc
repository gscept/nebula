//------------------------------------------------------------------------------
//  tcpserver.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "net/tcpserver.h"

namespace Net
{
#if (__WIN32__ || __XBOX360__ || __PS3__ || __linux__)
__ImplementClass(Net::TcpServer, 'TCPS', Net::StdTcpServer);
#elif __WII__
__ImplementClass(Net::TcpServer, 'TCPS', Wii::WiiTcpServer);
#else
#error "Net::TcpServer not implemented on this platform!"
#endif
}