//------------------------------------------------------------------------------
//  tcpclientconnection.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "net/tcpclientconnection.h"

namespace Net
{
#if (__WIN32__ || __XBOX360__ || __PS3__ || __linux__)
__ImplementClass(Net::TcpClientConnection, 'TPCC', Net::StdTcpClientConnection);
#elif __WII__
__ImplementClass(Net::TcpClientConnection, 'TPCC', Wii::WiiTcpClientConnection);
#else
#error "Net::TcpClientConnection not implemented on this platform!"
#endif
}