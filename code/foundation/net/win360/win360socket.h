#pragma once
//------------------------------------------------------------------------------
/**
    @class Win360::Win360Socket

    NOTE: Socket network communication on the Xbox360 is only provided
    for debugging and development purposes. For actual multiplayer and
    Xbox Live related stuff, use the Xbox-specific add-on modules!

    A lowlevel socket wrapper class. This class is primarily a
    platform abstraction class, not an "end-user-class". Usually a Nebula3 
    application doesn't use this class directly but instead uses the higher
    level classes like TcpServer, TclClient, etc...

    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "net/socket/ipaddress.h"

namespace Net
{
class Socket;
}

//------------------------------------------------------------------------------
namespace Win360
{
class Win360Socket : public Core::RefCounted
{
    __DeclareClass(Win360Socket);
public:
    /// protocol types
    enum Protocol
    {
        TCP,            // a reliable TCP connection
        UDP,            // unreliable UDP connection
    };

    // return values
    enum Result
    {
        Error = 0,      // an error occured, call GetErrorCode() or GetErrorString() for more info
        Success,        // everything ok
        WouldBlock,     // operation on non-blocking socket is not complete
        Closed,         // connection has been gracefully closed
    };

    /// error codes
    enum ErrorCode
    {
        ErrorNone,                   // no error occured
        ErrorUnknown,                // an "unknown" error occured     
        ErrorInterrupted,            // WSAEINTR
        ErrorPermissionDenied,       // WSAEACCESS
        ErrorBadAddress,             // WSAEFAULT
        ErrorInvalidArgument,        // WSAEINVAL
        ErrorTooManyOpenFiles,       // WSAEMFILE
        ErrorWouldBlock,             // WSAEWOULDBLOCK
        ErrorInProgress,             // WSAEINPROGRESS
        ErrorAlreadyInProgress,      // WSAEALREADY
        ErrorNotASocket,             // WSAENOTSOCK
        ErrorDestAddrRequired,       // WSAEDESTADDRREQ
        ErrorMsgTooLong,             // WSAEMSGSIZE
        ErrorInvalidProtocol,        // WSAEPROTOTYPE
        ErrorBadProtocolOption,      // WSAENOPROTOOPT
        ErrorProtocolNotSupported,   // WSEAPROTONOSUPPORT
        ErrorSocketTypeNotSupported, // WSAESOCKTNOSUPPORT
        ErrorOperationNotSupported,  // WSAEOPNOTSUPP
        ErrorProtFamilyNotSupported, // WSAEPFNOSUPPORT
        ErrorAddrFamilyNotSupported, // WSAEAFNOSUPPORT
        ErrorAddrInUse,              // WSAEADDRINUSE
        ErrorAddrNotAvailable,       // WSAEADDRNOTAVAIL
        ErrorNetDown,                // WSAENETDOWN
        ErrorNetUnreachable,         // WSAENETUNREACH
        ErrorNetReset,               // WSAENETRESET
        ErrorConnectionAborted,      // WSAECONNABORTED
        ErrorConnectionReset,        // WSAECONNRESET
        ErrorNoBufferSpace,          // WSAENOBUFS
        ErrorIsConnected,            // WSAEISCONN
        ErrorNotConnected,           // WSAENOTCONNECTED
        ErrorIsShutdown,             // WSAESHUTDOWN
        ErrorIsTimedOut,             // WSAETIMEDOUT
        ErrorConnectionRefused,      // WSAECONNREFUSED
        ErrorHostDown,               // WSAEHOSTDOWN
        ErrorHostUnreachable,        // WSAEHOSTUNREACH
        ErrorTooManyProcesses,       // WSAEPROCLIM
        ErrorSystemNotReady,         // WSAESYSNOTREADY
        ErrorVersionNotSupported,    // WSAEVERNOTSUPPORTED
        ErrorNotInitialized,         // WSAENOTINITIALISED
        ErrorDisconnecting,          // WSAEDISCONN
        ErrorTypeNotFound,           // WSAETYPE_NOT_FOUND
        ErrorHostNotFound,           // WSAEHOST_NOT_FOUND
        ErrorTryAgain,               // WSAETRY_AGAIN
        ErrorNoRecovery,             // WSAENO_RECOVERY
        ErrorNoData,                 // WSAENO_DATA
    };

    /// constructor
    Win360Socket();
    /// destructor
    virtual ~Win360Socket();
    /// static initializer method (called by SysFunc::Setup())
    static void InitNetwork();
    /// is network initialized
    static bool IsNetworkInitialized();

    /// open the socket
    bool Open(Protocol p);
    /// close the socket
    void Close();
    /// return true if the socket is open
    bool IsOpen() const;
    /// get the last error code
    ErrorCode GetErrorCode() const;
    /// get the last error string
    Util::String GetErrorString() const;

    /// set internet address of socket
    void SetAddress(const Net::IpAddress& a);
    /// get internet address of socket
    const Net::IpAddress& GetAddress() const;
    
    /// set the broadcast flag (SO_BROADCAST)
    void SetBroadcast(bool b);
    /// get the broadcast flag
    bool GetBroadcast();
    /// set the keepalive flag (SO_KEEPALIVE)
    void SetKeepAlive(bool b);
    /// get the keepalive flag
    bool GetKeepAlive();
    /// set reuseaddr flag (SO_REUSEADDR)
    void SetReUseAddr(bool b);
    /// get reuseaddr flag
    bool GetReUseAddr();
    /// set nodelay flag (TCP_NODELAY)
    void SetNoDelay(bool b);
    /// get nodelay flag
    bool GetNoDelay();
    /// set receive buffer size
    void SetRecvBufSize(SizeT s);
    /// get receive buffer size
    SizeT GetRecvBufSize();
    /// set send buffer size
    void SetSendBufSize(SizeT s);
    /// get send buffer size
    SizeT GetSendBufSize();
    /// set blocking mode (FIONBIO)
    void SetBlocking(bool b);
    /// get blocking mode
    bool GetBlocking() const;
    /// get the maximum message size that can be sent atomically
    SizeT GetMaxMsgSize();

    /// bind socket to ip address
    bool Bind();
    /// return true if the socket is bound to an address
    bool IsBound() const;
    /// listen for incoming connections (for server sockets)
    bool Listen();
    /// accept incoming connection, return a new socket (for server sockets)
    bool Accept(Ptr<Net::Socket>& outSocket);
    /// connect to the sockets address (for client sockets)
    Result Connect();
    /// test if the socket is currently connected
    bool IsConnected();
    /// send raw data into the socket
    Result Send(const void* buf, SizeT numBytes, SizeT& bytesSent);
    /// return true if recv data is available at the socket
    bool HasRecvData();
    /// receive raw data from the socket
    Result Recv(void* buf, SizeT bufSize, SizeT& bytesReceived);
    /// send raw data to address for connectionless sockets
    Result SendTo(const void* buf, SizeT numBytes, uint addr, ushort port, SizeT& bytesSent);
    /// receive raw data from address for connectionless sockets
    Result RecvFrom(void* buf, SizeT bufSize, uint addr, ushort port, SizeT& bytesReceived);

private:
    friend class Win360IpAddress;

    /// open with an existing socket (called by Accept())
    void OpenWithExistingSocket(SOCKET s);
    /// clear the last error code
    void ClearError();
    /// set error code to WSAGetLastError();
    void SetToLastWSAError();
    /// set WSA error code
    void SetWSAError(int wsaError);
    /// convert a WSA error code to internal error code
    static ErrorCode WSAErrorToErrorCode(int wsaError);
    /// convert error code to human readable string
    static Util::String ErrorAsString(ErrorCode err);
    /// convert WSA error code directly to string
    static Util::String WSAErrorToString(int wsaError);
    /// set a bool socket option
    void SetBoolOption(int optName, bool val);
    /// get a bool socket option
    bool GetBoolOption(int optName);
    /// set an integer socket option
    void SetIntOption(int optName, int val);
    /// get an integer socket option
    int GetIntOption(int optName);

    static bool NetworkInitialized;
    ErrorCode error;
    SOCKET sock;
    Net::IpAddress addr;
    bool isBlocking;
    bool isBound;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
Win360Socket::IsOpen() const
{
    return (0 != this->sock);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Win360Socket::IsBound() const
{
    return this->isBound;
}

//------------------------------------------------------------------------------
/**
    Set internet address of socket.
*/
inline void
Win360Socket::SetAddress(const Net::IpAddress& a)
{
    this->addr = a;
}

//------------------------------------------------------------------------------
/**
    Get internet address of socket.
*/
inline const Net::IpAddress&
Win360Socket::GetAddress() const
{
    return this->addr;
}

//------------------------------------------------------------------------------
/**
*/
inline Win360Socket::ErrorCode 
Win360Socket::GetErrorCode() const
{
    return this->error;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Win360Socket::SetBroadcast(bool b)
{
    this->SetBoolOption(SO_BROADCAST, b);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Win360Socket::GetBroadcast()
{
    return this->GetBoolOption(SO_BROADCAST);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Win360Socket::SetKeepAlive(bool b)
{
    // not supported on Xbox360
    #if __WIN32__   
    this->SetBoolOption(SO_KEEPALIVE, b);
    #endif
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Win360Socket::GetKeepAlive()
{
    #if __WIN32__
    return this->GetBoolOption(SO_KEEPALIVE);
    #else
    // not supported on Xbox360
    return false;
    #endif
}

//------------------------------------------------------------------------------
/**
*/
inline void
Win360Socket::SetReUseAddr(bool b)
{
    this->SetBoolOption(SO_REUSEADDR, b);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Win360Socket::GetReUseAddr()
{
    return this->GetBoolOption(SO_REUSEADDR);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Win360Socket::SetNoDelay(bool b)
{
    this->SetBoolOption(TCP_NODELAY, b);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Win360Socket::GetNoDelay()
{
    return this->GetBoolOption(TCP_NODELAY);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Win360Socket::SetRecvBufSize(SizeT s)
{
    this->SetIntOption(SO_RCVBUF, s);
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
Win360Socket::GetRecvBufSize()
{
    return this->GetIntOption(SO_RCVBUF);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Win360Socket::SetSendBufSize(SizeT s)
{
    this->SetIntOption(SO_SNDBUF, s);
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
Win360Socket::GetSendBufSize()
{
    return this->GetIntOption(SO_SNDBUF);
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
Win360Socket::GetMaxMsgSize()
{
    #if __WIN32__
        return 8192;
    #else
        // see Xbox360 API documentation
        return 1264;
    #endif
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Win360Socket::GetBlocking() const
{
    return this->isBlocking;
}

} // namespace Xbox360
//------------------------------------------------------------------------------
