#pragma once
//------------------------------------------------------------------------------
/**
    @class Win32::Win32IpAddress

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

    @copyright
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "io/uri.h"

//------------------------------------------------------------------------------
namespace Win32
{
class Win32IpAddress
{
public:
    /// default constructor
    Win32IpAddress();
    /// copy constructor
    Win32IpAddress(const Win32IpAddress& rhs);
    /// construct from URI
    Win32IpAddress(const IO::URI& uri);
    /// construct from host name and port number
    Win32IpAddress(const Util::String& hostName, ushort portNumber);
    /// equality operator
    bool operator==(const Win32IpAddress& rhs) const;
    /// less-then operator
    bool operator<(const Win32IpAddress& rhs) const;
    /// greater-then operator
    bool operator>(const Win32IpAddress& rhs) const;
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

private:
    friend class Win32Socket;

    /// construct from sockaddr_in struct
    Win32IpAddress(const sockaddr_in& addr);
    /// set sockaddr_in directly
    void SetSockAddr(const sockaddr_in& addr);
    /// get sockaddr_in field
    const sockaddr_in& GetSockAddr() const;
    /// perform address resolution, understands special host names
    static bool GetHostByName(const Util::String& hostName, in_addr& outAddr);
    /// return true if an address is an internet address (not class A,B,C)
    static bool IsInetAddr(const in_addr* addr);

    Util::String hostName;
    Util::String addrAsString;
    sockaddr_in addr;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
Win32IpAddress::operator==(const Win32IpAddress& rhs) const
{
    return ((this->addr.sin_addr.S_un.S_addr == rhs.addr.sin_addr.S_un.S_addr) &&
            (this->addr.sin_port == rhs.addr.sin_port));
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Win32IpAddress::operator<(const Win32IpAddress& rhs) const
{
    if (this->addr.sin_addr.S_un.S_addr == rhs.addr.sin_addr.S_un.S_addr)
    {
        return this->addr.sin_port < rhs.addr.sin_port;
    }
    else
    {
        return this->addr.sin_addr.S_un.S_addr < rhs.addr.sin_addr.S_un.S_addr;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Win32IpAddress::operator>(const Win32IpAddress& rhs) const
{
    if (this->addr.sin_addr.S_un.S_addr == rhs.addr.sin_addr.S_un.S_addr)
    {
        return this->addr.sin_port > rhs.addr.sin_port;
    }
    else
    {
        return this->addr.sin_addr.S_un.S_addr > rhs.addr.sin_addr.S_un.S_addr;
    }
}

} // namespace Win32
//------------------------------------------------------------------------------
