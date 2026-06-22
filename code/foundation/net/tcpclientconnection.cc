//------------------------------------------------------------------------------
//  tcpclientconnection.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "net/tcpclientconnection.h"

namespace Net
{
#if __WIN32__ || __linux__ || __APPLE__
__ImplementClass(Net::TcpClientConnection, 'TPCC', Net::StdTcpClientConnection);
#else
#error "Net::TcpClientConnection not implemented on this platform!"
#endif
}
