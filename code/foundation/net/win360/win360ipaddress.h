#pragma once
//------------------------------------------------------------------------------
/**
    @class Win360::Win360IpAddress

    NOTE: Socket network communication on the Xbox360 is only provided
    for debugging and development purposes. For actual multiplayer and
    Xbox Live related stuff, use the Xbox-specific add-on modules!
    (which don't exist yet, ha).

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

    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "io/uri.h"

//------------------------------------------------------------------------------
namespace Win360
{
class Win360IpAddress
{
public:
    /// default constructor
    Win360IpAddress();
    /// copy constructor
    Win360IpAddress(const Win360IpAddress& rhs);
    /// construct from URI
    Win360IpAddress(const IO::URI& uri);
    /// construct from host name and port number
    Win360IpAddress(const Util::String& hostName, ushort portNumber);
    /// equality operator
    bool operator==(const Win360IpAddress& rhs) const;
    /// less-then operator
    bool operator<(const Win360IpAddress& rhs) const;
    /// greater-then operator
    bool operator>(const Win360IpAddress& rhs) const;
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
    friend class Win360Socket;

    /// construct from sockaddr_in struct
    Win360IpAddress(const sockaddr_in& addr);
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
Win360IpAddress::operator==(const Win360IpAddress& rhs) const
{
    return ((this->addr.sin_addr.S_un.S_addr == rhs.addr.sin_addr.S_un.S_addr) &&
            (this->addr.sin_port == rhs.addr.sin_port));
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Win360IpAddress::operator<(const Win360IpAddress& rhs) const
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
Win360IpAddress::operator>(const Win360IpAddress& rhs) const
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

} // namespace Win360
//------------------------------------------------------------------------------
