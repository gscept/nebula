//------------------------------------------------------------------------------
//  tcpserver.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "net/tcpserver.h"

namespace Net
{
#if (__WIN32__ || __linux__)
__ImplementClass(Net::TcpServer, 'TCPS', Net::StdTcpServer);
#else
#error "Net::TcpServer not implemented on this platform!"
#endif
}