//------------------------------------------------------------------------------
//  win360ipaddress.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "net/win360/win360ipaddress.h"
#include "net/win360/win360socket.h"

namespace Win360
{
using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
Win360IpAddress::Win360IpAddress()
{
    if (!Win360Socket::NetworkInitialized)
    {
        Win360Socket::InitNetwork();
    }
    Memory::Clear(&this->addr, sizeof(this->addr));
    this->addr.sin_family = AF_INET;
}

//------------------------------------------------------------------------------
/**
*/
Win360IpAddress::Win360IpAddress(const Win360IpAddress& rhs) :
    hostName(rhs.hostName),
    addrAsString(rhs.addrAsString),
    addr(rhs.addr)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Win360IpAddress::Win360IpAddress(const String& hostName_, ushort portNumber_)
{
    Memory::Clear(&this->addr, sizeof(this->addr));
    this->addr.sin_family = AF_INET;
    this->SetHostName(hostName_);
    this->SetPort(portNumber_);
}

//------------------------------------------------------------------------------
/**
*/
Win360IpAddress::Win360IpAddress(const URI& uri)
{
    Memory::Clear(&this->addr, sizeof(this->addr));
    this->addr.sin_family = AF_INET;
    this->ExtractFromUri(uri);
}

//------------------------------------------------------------------------------
/**
*/
Win360IpAddress::Win360IpAddress(const sockaddr_in& sa)
{
    Memory::Clear(&this->addr, sizeof(this->addr));
    this->addr.sin_family = AF_INET;
    this->SetSockAddr(sa);
}

//------------------------------------------------------------------------------
/**
    Set the address directly from a sockaddr_in struct. This will
    set the host name to the string representation of the 
    host address.
*/
void
Win360IpAddress::SetSockAddr(const sockaddr_in& sa)
{
    this->addr = sa;
    this->hostName.Format("%d.%d.%d.%d",
        sa.sin_addr.S_un.S_un_b.s_b1,
        sa.sin_addr.S_un.S_un_b.s_b2,
        sa.sin_addr.S_un.S_un_b.s_b3,
        sa.sin_addr.S_un.S_un_b.s_b4);
    this->addrAsString = this->hostName;
}

//------------------------------------------------------------------------------
/**
    Get the sockaddr_in struct, which has either been set directly
    with SetSockAddr() or indirectly through host name, port number
    or from an URI.
*/
const sockaddr_in&
Win360IpAddress::GetSockAddr() const
{
    return this->addr;
}

//------------------------------------------------------------------------------
/**
    Extract the host name and optionally the port number from the provided
    URI. If no port number is set in the URI, the current port number
    will be left as is. If the host name is empty, it will be set to
    "localhost".
*/
void
Win360IpAddress::ExtractFromUri(const URI& uri)
{
    if (uri.Host().IsValid())
    {
        this->SetHostName(uri.Host());
    }
    else
    {
        this->SetHostName("localhost");
    }
    if (uri.Port().IsValid())
    {
        this->SetPort((ushort)uri.Port().AsInt());
    }
}

//------------------------------------------------------------------------------
/**
    Set the port number. Will be translated to network byte order internally.
*/
void
Win360IpAddress::SetPort(ushort port)
{
    this->addr.sin_port = htons(port);
}

//------------------------------------------------------------------------------
/**
    Get the port number in host byte order.
*/
ushort
Win360IpAddress::GetPort() const
{
    return ntohs(this->addr.sin_port);
}

//------------------------------------------------------------------------------
/**
    Set the host name, and immediately convert it to an ip address. This 
    accepts the special hostnames "any", "broadcast", "localhost", "self" 
    and "inetself". The result ip address can be returned in string form
    with the method GetAddrAsString().
*/
void
Win360IpAddress::SetHostName(const String& n)
{
    n_assert(n.IsValid());
    this->hostName = n;
    this->GetHostByName(n, this->addr.sin_addr);
    this->addrAsString.Format("%d.%d.%d.%d",
        this->addr.sin_addr.S_un.S_un_b.s_b1,
        this->addr.sin_addr.S_un.S_un_b.s_b2,
        this->addr.sin_addr.S_un.S_un_b.s_b3,
        this->addr.sin_addr.S_un.S_un_b.s_b4);
}

//------------------------------------------------------------------------------
/**
    Get the host name.
*/
const String&
Win360IpAddress::GetHostName() const
{
    return this->hostName;
}

//------------------------------------------------------------------------------
/**
    Return the in address as string.
*/
const String&
Win360IpAddress::GetHostAddr() const
{
    return this->addrAsString;
}

//------------------------------------------------------------------------------
/**
    This resolves a host name into a IPv4 ip address. The ip address is
    returned in network byte order in the hostAddress argument. The return value
    indicates whether the operation was successful. The following special hostnames 
    can be defined:

    - "any"         resolves to INADDR_ANY (0.0.0.0)
    - "broadcast"   resolves to INADDR_BROADCAST (255.255.255.255)
    - "localhost"   resolves to 127.0.0.1
    - "self"        (NOT IMPLEMENTED ON XBOX360) resolves to the first address of this host
    - "inetself"    (NOT IMPLEMENTED ON XBOX360) resolves to the first address which is not a LAN address

    An empty host name is invalid. A hostname can also be an address string
    of the form xxx.yyy.zzz.www.

    NOTE: resolving host names and host addresses is not supported
    on the Xbox360, this basically means that an Xbox360 devkit can function
    as a server, but not as a client (this is fine for most debugging purposes).
*/
bool
Win360IpAddress::GetHostByName(const Util::String& hostName, in_addr& outAddr)
{
    n_assert(hostName.IsValid());
    outAddr.S_un.S_addr = 0;

    if ("any" == hostName)
    {
        outAddr.S_un.S_addr = htonl(INADDR_ANY);
        return true;
    }
    else if ("broadcast" == hostName)
    {
        outAddr.S_un.S_addr = htonl(INADDR_BROADCAST);
        return true;
    }
    else if (("self" == hostName) || ("inetself" == hostName))
    {
        #if __WIN32__
            // get the machine's host name
            char localHostName[512];
            int err = gethostname(localHostName, sizeof(localHostName));
            if (SOCKET_ERROR == err)
            {
                return false;
            }

            // resolve own host name
            struct hostent* he = gethostbyname(localHostName);
            if (0 == he)
            {
                // could not resolve own host name
                return false;
            }

            // initialize with the default address 
            const in_addr* inAddr = (const in_addr *) he->h_addr;
            if (hostName == "inetself")
            {
                // if internet address requested, scan list of ip addresses
                // for a non-Class A,B or C network address
                int i;
                for (i = 0; (0 != he->h_addr_list[i]); i++)
                {
                    if (IsInetAddr((const in_addr *)he->h_addr_list[i]))
                    {
                        inAddr = (in_addr *)he->h_addr_list[i];
                        break;
                    }
                }
            }
            outAddr = *inAddr;
            return true;
        #else // __XBOX360__
            n_error("Win360IpAddress::GetHostByName(): self and inetself not implemented on Xbox360!");
            return false;
        #endif
    }
    else if (hostName.CheckValidCharSet(".0123456789"))
    {
        // a numeric address...
        outAddr.S_un.S_addr = inet_addr(hostName.AsCharPtr());
        return true;
    }
    else
    {
        #if __WIN32__
            // the default case: do a DNS name lookup
            struct hostent* he = gethostbyname(hostName.AsCharPtr());
            if (0 == he)
            {
                // could not resolve host name!
                return false;
            }
            outAddr = *((in_addr*)he->h_addr);
            return true;
        #else // __XBOX360__
            n_error("Win360IpAddress::GetHostByName(): DNS name lookups not supported on Xbox360!");
            return false;
        #endif
    }
}

//------------------------------------------------------------------------------
/**
    This method checks if the provided address is an "internet" address,
    not a LAN address (not a class A, B or C network address).
*/
bool
Win360IpAddress::IsInetAddr(const in_addr* addr)
{
    // generate address string from addr
    String addrString;
    addrString.Format("%d.%d.%d.%d", 
        addr->S_un.S_un_b.s_b1, 
        addr->S_un.S_un_b.s_b2, 
        addr->S_un.S_un_b.s_b3, 
        addr->S_un.S_un_b.s_b4); 

    // tokenize string into its members
    Array<String> tokens = addrString.Tokenize(".");
    n_assert(tokens.Size() == 4);
    int b1 = tokens[0].AsInt();
    int b2 = tokens[1].AsInt();
    int b3 = tokens[2].AsInt();
    if ((b1 == 10) && (b2 >= 0) && (b2 <= 254))
    {
        // Class A net
        return false;
    }
    else if ((b1 == 172) && (b2 >= 16) && (b2 <= 31))
    {
        // Class B net
        return false;
    }
    else if ((b1 == 192) && (b2 == 168) && (b3 >= 0) && (b3 <= 254))
    {
        // Class C net
        return false;
    }
    else if (b1 < 224)
    {
        // unknown other local net type
        return false;
    }
    // an internet address
    return true;
}

} // namespace Win360
