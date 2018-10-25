#pragma once
#ifndef POSIX_POSIXIPADDRESS_H
#define POSIX_POSIXIPADDRESS_H
//------------------------------------------------------------------------------
/**
    @class Posix::PosixIpAddress

    Represents an IP address, consisting of a IPv4 host address
    and a port number. Performs automatic name lookup on the host name.
    Can extract address information from an URI and    
    automatically converts host names to addresses, and offers the special 
    hostnames "localhost", "any", "broadcast", "self" and "inetself" where:

    - "localhost" will translate to 127.0.0.1
    - "any" will translate to INADDR_ANY, which is 0.0.0.0
    - "broadcast" will translate to INADDR_BROADCAST, which is 255.255.255.255
    - "self" will translate to the first valid tcp/ip address for this host
      (there may be more then one address bound to the host)
    - "inetself" will translate to the first host address which is not 
      a LAN address (which is not a class A, B, or C network) if none such
      exists the address will fall back to "self"

    (C) 2006 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file    
*/
#include "core/types.h"
#include "io/uri.h"
#include <sys/socket.h>
#include <netinet/in.h>

//------------------------------------------------------------------------------
namespace Posix
{
class PosixIpAddress
{
public:
    /// default constructor
    PosixIpAddress();
    /// copy constructor
    PosixIpAddress(const PosixIpAddress& rhs);
    /// construct from URI
    PosixIpAddress(const IO::URI& uri);
    /// construct from host name and port number
    PosixIpAddress(const Util::String& hostName, ushort portNumber);
    /// equality operator
    bool operator==(const PosixIpAddress& rhs) const;
    /// less-then operator
    bool operator<(const PosixIpAddress& rhs) const;
    /// greater-then operator
    bool operator>(const PosixIpAddress& rhs) const;
    /// extract host name and port number from URI
    void ExtractFromUri(const IO::URI& uri);
    /// set host name
    void SetHostName(const Util::String& hostName);
    /// get host name
    const Util::String& GetHostName() const;
    /// set port number
    void SetPort(ushort port);
    /// get port number
    ushort GetPort() const;
    /// get the ip address resulting from the host name as string
    const Util::String& GetHostAddr() const;

protected:
    friend class PosixSocket;

    /// set sockaddr_in directly
    void SetSockAddr(const sockaddr& addr);
    /// get sockaddr_in field
    const sockaddr& GetSockAddr() const;
    /// perform address resolution, understands special host names
    static bool GetHostByName(const Util::String& hostName, sockaddr& outAddr);
    /// return true if an address is an internet address (not class A,B,C)
    static bool IsInetAddr(const in_addr* addr);

    Util::String hostName;
    Util::String addrAsString;
    sockaddr addr;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
PosixIpAddress::operator==(const PosixIpAddress& rhs) const
{
    n_assert(this->addr.sa_family == AF_INET);
    n_assert(this->addr.sa_family == rhs.addr.sa_family);
    sockaddr_in * myAddr = (sockaddr_in *)(&(this->addr));
    sockaddr_in * otherAddr = (sockaddr_in *)(&(rhs.addr));
    return ((myAddr->sin_addr.s_addr == otherAddr->sin_addr.s_addr) &&
            (myAddr->sin_port == otherAddr->sin_port));
}

//------------------------------------------------------------------------------
/**
*/
inline bool
PosixIpAddress::operator<(const PosixIpAddress& rhs) const
{
    n_assert(this->addr.sa_family == AF_INET);
    n_assert(this->addr.sa_family == rhs.addr.sa_family);
    sockaddr_in * myAddr = (sockaddr_in *)(&(this->addr));
    sockaddr_in * otherAddr = (sockaddr_in *)(&(rhs.addr));
    if (myAddr->sin_addr.s_addr == otherAddr->sin_addr.s_addr)
    {
        return myAddr->sin_port < otherAddr->sin_port;
    }
    else
    {
        return myAddr->sin_addr.s_addr < otherAddr->sin_addr.s_addr;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline bool
PosixIpAddress::operator>(const PosixIpAddress& rhs) const
{
    n_assert(this->addr.sa_family == AF_INET);
    n_assert(this->addr.sa_family == rhs.addr.sa_family);
    sockaddr_in * myAddr = (sockaddr_in *)(&(this->addr));
    sockaddr_in * otherAddr = (sockaddr_in *)(&(rhs.addr));
    if (myAddr->sin_addr.s_addr == otherAddr->sin_addr.s_addr)
    {
        return myAddr->sin_port > otherAddr->sin_port;
    }
    else
    {
        return myAddr->sin_addr.s_addr > otherAddr->sin_addr.s_addr;
    }
}

} // namespace Posix
//------------------------------------------------------------------------------
#endif
