//------------------------------------------------------------------------------
//  tcpclient.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "net/tcpclient.h"

namespace Net
{
#if (__WIN32__ || __linux__)
__ImplementClass(Net::TcpClient, 'TCPC', Net::StdTcpClient);
#else
#error "Net::TcpClient not implemented on this platform!"
#endif
}