//------------------------------------------------------------------------------
//  win32socket.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "net/socket/socket.h"

namespace Win32
{
__ImplementClass(Win32::Win32Socket, 'XSCK', Core::RefCounted);
using namespace Util;

bool Win32Socket::NetworkInitialized = false;

//------------------------------------------------------------------------------
/**
*/
Win32Socket::Win32Socket() :
    error(ErrorNone),
    sock(0),
    isBlocking(true),
    isBound(false)
{
    n_assert(NetworkInitialized);
}

//------------------------------------------------------------------------------
/**
*/
Win32Socket::~Win32Socket()
{
    if (this->IsOpen())
    {
        this->Close();
    }
}

//------------------------------------------------------------------------------
/**
    This is a one-time init for the Windows Sockets system. The method
    is called from SysFunc::Setup() once at startup before any threads
    are launched.
*/
void
Win32Socket::InitNetwork()
{
    n_assert(!NetworkInitialized);

    // first setup the Xbox networking stuff
    #if __XBOX360__
    Xbox360::Xbox360Network::SetupNetwork();
    #endif

    // now proceed as usual
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (0 != result)
    {
        n_error("WSAStartup() failed with '%s'!", WSAErrorToString(result).AsCharPtr());
    }
    NetworkInitialized = true;
}   

//------------------------------------------------------------------------------
/**
*/
bool
Win32Socket::Open(Protocol protocol)
{
    n_assert(!this->IsOpen());

    this->ClearError();

    int sockType;
    int protType;
    switch (protocol)
    {
        case TCP:    
            sockType = SOCK_STREAM; 
            protType = IPPROTO_TCP;
            break;
        case UDP:  
            sockType = SOCK_DGRAM; 
            protType = IPPROTO_UDP;
            break;
        default:
            // can't happen.
            n_error("Invalid socket type!");
            sockType = SOCK_STREAM; 
            protType = IPPROTO_TCP;
            break;
    }
    this->sock = socket(AF_INET, sockType, protType);
    if (INVALID_SOCKET == this->sock)
    {
        this->sock = 0;
        this->SetToLastWSAError();
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    Open the socket object with an existing initialized system socket.
    This is a private method and only called by Accept() on the
    new socket created by the accept() function.
*/
void
Win32Socket::OpenWithExistingSocket(SOCKET s)
{
    n_assert(INVALID_SOCKET != s);
    n_assert(!this->IsOpen());
    this->sock = s;
}

//------------------------------------------------------------------------------
/**
*/
void
Win32Socket::Close()
{
    n_assert(this->IsOpen());
    this->ClearError();

    int res = 0;
    if (this->IsConnected())
    {
        res = shutdown(this->sock, SD_BOTH);
        if (SOCKET_ERROR == res)
        {
            // note: the shutdown function may return NotConnected, this
            // is not really an error
            this->SetToLastWSAError();
            if (ErrorNotConnected != this->error)
            {
                n_printf("Win32Socket::Close(): shutdown() failed with '%s'.\n", this->GetErrorString().AsCharPtr());
            }
        }
    }
    res = closesocket(this->sock);
    if (SOCKET_ERROR == res)
    {
        this->SetToLastWSAError();
        n_printf("Win32Socket::Close(): closesocket() failed with '%s'.\n", this->GetErrorString().AsCharPtr());
    }
    this->sock = 0;
}

//------------------------------------------------------------------------------
/**
    Set a boolean option on the socket. This is a private helper
    function.
*/
void
Win32Socket::SetBoolOption(int optName, bool val)
{
    n_assert(this->IsOpen());
    this->ClearError();
    int level = SOL_SOCKET;
    if (optName == TCP_NODELAY)
    {
        level = IPPROTO_TCP;
    }
    int optVal = val ? 1 : 0;
    int res = setsockopt(this->sock, level, optName, (const char*) &optVal, sizeof(optVal));
    if (SOCKET_ERROR == res)
    {
        this->SetToLastWSAError();
        n_printf("Win32Socket::SetBoolOption(): setsockopt() failed with '%s'.\n", this->GetErrorString().AsCharPtr());
    }
}

//------------------------------------------------------------------------------
/**
    Get a boolean option on the socket. This is a private helper
    function.
*/
bool
Win32Socket::GetBoolOption(int optName)
{
    n_assert(this->IsOpen());
    this->ClearError();
    int level = SOL_SOCKET;
    if (optName == TCP_NODELAY)
    {
        level = IPPROTO_TCP;
    }
    int optVal = 0;
    int optValSize = sizeof(optVal);
    int res = getsockopt(this->sock, level, optName, (char*) &optVal, &optValSize);
    if (SOCKET_ERROR == res)
    {
        this->SetToLastWSAError();
        n_printf("Win32Socket::GetBoolOption(): getsockopt() failed with '%s'.\n", this->GetErrorString().AsCharPtr());
    }
    n_assert(sizeof(optVal) == optValSize);
    return (0 != optVal);
}

//------------------------------------------------------------------------------
/**
    Set an int socket option. This is a private helper function.
*/
void
Win32Socket::SetIntOption(int optName, int val)
{
    n_assert(this->IsOpen());
    this->ClearError();
    int res = setsockopt(this->sock, SOL_SOCKET, optName, (const char*) &val, sizeof(val));
    if (SOCKET_ERROR == res)
    {
        this->SetToLastWSAError();
        n_printf("Win32Socket::SetIntOption(): setsockopt() failed with '%s'.\n", this->GetErrorString().AsCharPtr());
    }
}

//------------------------------------------------------------------------------
/**
    Get an int socket option. This is a private helper function.
*/
int
Win32Socket::GetIntOption(int optName)
{
    n_assert(this->IsOpen());
    this->ClearError();
    int optVal = 0;
    int optValSize = sizeof(optVal);
    int res = getsockopt(this->sock, SOL_SOCKET, optName, (char*) &optVal, &optValSize);
    if (SOCKET_ERROR == res)
    {
        this->SetToLastWSAError();
        n_printf("Win32Socket::GetIntOption(): getsockopt() failed with '%s'.\n", this->GetErrorString().AsCharPtr());
    }
    n_assert(sizeof(optVal) == optValSize);
    return optVal;
}

//------------------------------------------------------------------------------
/**
    Set the socket to blocking mode.
*/
void
Win32Socket::SetBlocking(bool b)
{
    n_assert(this->IsOpen());
    this->ClearError();
    u_long arg = b ? 0 : 1;
    int res = ioctlsocket(this->sock, FIONBIO, &arg);
    if (SOCKET_ERROR == res)
    {
        this->SetToLastWSAError();
        n_printf("Win32Socket::SetBlocking(): ioctlsocket() failed with '%s'.\n", this->GetErrorString().AsCharPtr());
    }
    this->isBlocking = b;
}

//------------------------------------------------------------------------------
/**
    Bind the socket to its ip address set with SetAddress() and
    SetPort(). After binding the socket to an address, call
    the Listen() method to wait for incoming connections. This method
    only makes sense for server sockets.
*/
bool
Win32Socket::Bind()
{
    n_assert(this->IsOpen());
    n_assert(!this->IsBound());
    this->ClearError();
    const sockaddr_in& sockAddr = this->addr.GetSockAddr();
    int res = bind(this->sock, (const sockaddr*) &sockAddr, sizeof(sockAddr));
    if (SOCKET_ERROR == res)
    {
        this->SetToLastWSAError();
        n_printf("Win32Socket::Bind(): bind() failed with '%s'!\n", this->GetErrorString().AsCharPtr());
        return false;
    }
    this->isBound = true;
    return true;
}

//------------------------------------------------------------------------------
/**
    Wait for incoming connections to a server socket. Call this 
    method on server side after binding the socket to its address.
*/
bool
Win32Socket::Listen()
{
    n_assert(this->IsOpen());
    n_assert(this->IsBound());
    this->ClearError();
    int res = listen(this->sock, SOMAXCONN);
    if (SOCKET_ERROR == res)
    {
        this->SetToLastWSAError();
        n_printf("Win32Socket::Listen(): listen() failed with '%s'!\n", this->GetErrorString().AsCharPtr());
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    Accept an incoming connection to a server socket. This will spawn
    a new socket for the connection which will be returned in the provided
    pointer reference. The address of the returned socket will be set to
    the address of the "connecting entity".
*/
bool
Win32Socket::Accept(Ptr<Net::Socket>& outSocket)
{
    n_assert(this->IsOpen());
    n_assert(this->IsBound());

	// check if socket will block
	fd_set readFd;
	FD_ZERO(&readFd);
	FD_SET(this->sock,&readFd);
	timeval tv;
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	if(select(0,&readFd,0,0,&tv) != 1)
	{
		return false;
	}

    this->ClearError();
    outSocket = nullptr;
    sockaddr_in sockAddr;
    int sockAddrSize = sizeof(sockAddr);
    SOCKET newSocket = accept(this->sock, (sockaddr*) &sockAddr, &sockAddrSize);
    if (INVALID_SOCKET == newSocket)
    {
        this->SetToLastWSAError();
        n_printf("Win32Socket::Accept(): accept() failed with '%s'!\n", this->GetErrorString().AsCharPtr());
        return false;
    }
    outSocket = Net::Socket::Create();
    outSocket->SetAddress(Win32IpAddress(sockAddr));
    outSocket->OpenWithExistingSocket(newSocket);
    outSocket->SetBlocking(this->isBlocking);
    
    return true;
}

//------------------------------------------------------------------------------
/**
    Connect to a server socket. This method is called by a client socket
    to connect to a server socket identified by the socket object's address.
    A non-blocking socket will return immediately with WouldBlock, since the
    connection cannot be established immediately. In this case, just continue
    to call Connect() until the method returns Success, or alternative, check
    the IsConnected() method, which will also return true once the connection
    has been establish.
*/
Win32Socket::Result
Win32Socket::Connect()
{
    n_assert(this->IsOpen());
    n_assert(!this->IsBound());

    this->ClearError();
    const sockaddr_in& sockAddr = this->addr.GetSockAddr();
    int res = connect(this->sock, (const sockaddr*) &sockAddr, sizeof(sockAddr));
    if (SOCKET_ERROR == res)
    {
        // special handling for non-blocking sockets
        int wsaError = WSAGetLastError();
        if (!this->GetBlocking())
        {
            if (WSAEWOULDBLOCK == wsaError)
            {
                return WouldBlock;
            }
            else if (WSAEALREADY == wsaError)
            {
                // connection is underway but not finished yet
                return WouldBlock;
            }
            else if (WSAEISCONN == wsaError)
            {
                // the connection is established
                return Success;
            }
            // fallthrough: a normal error
        }
        this->SetWSAError(wsaError);
        n_printf("Win32Socket::Connect(): connect() failed with '%s'!\n", this->GetErrorString().AsCharPtr());
        return Error;
    }
    return Success;
}

//------------------------------------------------------------------------------
/**
    This tests if the socket is actually connected by doing a select()
    on the socket to probe for writability. So the IsConnected() method
    basically checks whether data can be sent through the socket.
*/
bool
Win32Socket::IsConnected()
{
    n_assert(this->IsOpen());
    fd_set writeSet = { 1, { this->sock } };
    TIMEVAL timeVal = { 0, 0 };
    int res = select(0, 0, &writeSet, 0, &timeVal);
    if (SOCKET_ERROR == res)
    {
        this->SetToLastWSAError();
        n_printf("Win32Socket::IsConnected(): select() failed with '%s'!\n", this->GetErrorString().AsCharPtr());
        return false;
    }
    else if (0 == res)
    {
        return false;
    }
    else
    {
        return true;
    }
}

//------------------------------------------------------------------------------
/**
    Send raw data into the socket. Note that depending on the buffer size
    of the underlying socket implementation and other sockets, the method
    may not be able to send all provided data. In this case, the returned 
    content of bytesSent will be less then numBytes, even though the
    return value will be Success. It is up to the caller to handle the
    extra data which hasn't been sent with the current call.
*/
Win32Socket::Result
Win32Socket::Send(const void* buf, SizeT numBytes, SizeT& bytesSent)
{
    n_assert(this->IsOpen());
    n_assert(0 != buf);
    this->ClearError();
    bytesSent = 0;
    int res = send(this->sock, (const char*) buf, numBytes, 0);
    if (SOCKET_ERROR == res)
    {
        if (WSAEWOULDBLOCK == res)
        {
            return WouldBlock;
        }
        else
        {
            this->SetToLastWSAError();
            n_printf("Win32Socket::Send(): send() failed with '%s'\n", this->GetErrorString().AsCharPtr());
            return Error;
        }
    }
    else
    {
        bytesSent = res;
        return Success;
    }
}

//------------------------------------------------------------------------------
/**
    This method checks if the socket has received data available. Use
    this method in a loop with Recv() to get all data waiting at the
    socket. This method will never block.
*/
bool
Win32Socket::HasRecvData()
{
    n_assert(this->IsOpen());
    fd_set readSet = { 1, { this->sock } };
    TIMEVAL timeVal = { 0, 0 };
    int res = select(0, &readSet, 0, 0, &timeVal);
    if (SOCKET_ERROR == res)
    {
        this->SetToLastWSAError();
        n_printf("Win32Socket::HasRecvData(): select() failed with '%s'!\n", this->GetErrorString().AsCharPtr());
        return false;
    }
    else if (0 == res)
    {
        return false;
    }
    else
    {
        return true;
    }
}

//------------------------------------------------------------------------------
/**
    Receive raw data from a socket and write the received data into the
    provided buffer. On a blocking socket this method will block until
    data arrives at the socket. A non-blocking socket would immediately return in
    this case with a WouldBlock result. When valid data has been received
    the method will return with a Success result and the bytesReceived argument
    will contain the number of received bytes. It is not guaranteed that a single
    receive will return all data waiting on the socket. To make sure that the
    socket is really empty, call Recv() in a loop until HasRecvData()
    returns false.
    When the socket has been gracefully closed by the other side, the method will 
    return with a Closed return value. Everything else will return with an Error 
    return code. Call GetErrorCode() or GetErrorString() to find out more in this case.
*/
Win32Socket::Result
Win32Socket::Recv(void* buf, SizeT bufSize, SizeT& bytesReceived)
{
    n_assert(this->IsOpen());
    n_assert(0 != buf);
    this->ClearError();
    bytesReceived = 0;
    int res = recv(this->sock, (char*) buf, bufSize, 0);
    if (0 == res)
    {
        // connection has been gracefully closed
        return Closed;
    }
    else if (SOCKET_ERROR == res)
    {
        // catch special error conditions
        int wsaError = WSAGetLastError();
        if (WSAEMSGSIZE == wsaError)
        {
            // more data is pending
            bytesReceived = bufSize;    // FIXME: is this correct?
            return Success;
        }
        if (!this->isBlocking)
        {
            // socket is non-blocking and no data is available
            if (WSAEWOULDBLOCK == wsaError)
            {
                return WouldBlock;
            }
        }

        // fallthrough: a real error
        this->SetToLastWSAError();
        n_printf("Win32Socket::Recv(): recv() failed with '%s'\n", this->GetErrorString().AsCharPtr());
        return Error;
    }
    else
    {
        bytesReceived = res;
        return Success;
    }
}

//------------------------------------------------------------------------------
/**
    FIXME: this is the send method for connectionless sockets using the 
    UDP protocol.
*/
Win32Socket::Result
Win32Socket::SendTo(const void* /*buf*/, SizeT /*numBytes*/, uint /*addr*/, ushort /*port*/, SizeT& /*bytesSent*/)
{
    n_error("Win32Socket::SendTo(): IMPLEMENT ME!");
    return Error;
}

//------------------------------------------------------------------------------
/**
    FIXME: this is the recv method for connectionless socket using the UDP
    protocol.
*/
Win32Socket::Result
Win32Socket::RecvFrom(void* /*buf*/, SizeT /*bufSize*/, uint /*addr*/, ushort /*port*/, SizeT& /*bytesReceived*/)
{
    n_error("Win32Socket::RecvFrom(): IMPLEMENT ME!");
    return Error;
}

//------------------------------------------------------------------------------
/**
    Sets the internal error code to NoError.
*/
void
Win32Socket::ClearError()
{
    this->error = ErrorNone;
}

//------------------------------------------------------------------------------
/**
    Sets the internal error code to WSAGetLastError().
*/
void
Win32Socket::SetToLastWSAError()
{
    this->error = WSAErrorToErrorCode(WSAGetLastError());
}

//------------------------------------------------------------------------------
/**
    Sets the provided WSA error as error code.
*/
void
Win32Socket::SetWSAError(int wsaError)
{
    this->error = WSAErrorToErrorCode(wsaError);
}

//------------------------------------------------------------------------------
/**
    This method converts an Windows Socket error code into a
    portable error code used by Win32Socket.
*/
Win32Socket::ErrorCode
Win32Socket::WSAErrorToErrorCode(int wsaErrorCode)
{
    switch (wsaErrorCode)
    {
        case WSAEINTR:              return ErrorInterrupted; break;            
        case WSAEACCES:             return ErrorPermissionDenied; break;       
        case WSAEFAULT:             return ErrorBadAddress; break;             
        case WSAEINVAL:             return ErrorInvalidArgument; break;        
        case WSAEMFILE:             return ErrorTooManyOpenFiles; break;       
        case WSAEWOULDBLOCK:        return ErrorWouldBlock; break;             
        case WSAEINPROGRESS:        return ErrorInProgress; break;             
        case WSAEALREADY:           return ErrorAlreadyInProgress; break;      
        case WSAENOTSOCK:           return ErrorNotASocket; break;             
        case WSAEDESTADDRREQ:       return ErrorDestAddrRequired; break;       
        case WSAEMSGSIZE:           return ErrorMsgTooLong; break;             
        case WSAEPROTOTYPE:         return ErrorInvalidProtocol; break;        
        case WSAENOPROTOOPT:        return ErrorBadProtocolOption; break;      
        case WSAEPROTONOSUPPORT:    return ErrorProtocolNotSupported; break;   
        case WSAESOCKTNOSUPPORT:    return ErrorSocketTypeNotSupported; break; 
        case WSAEOPNOTSUPP:         return ErrorOperationNotSupported; break;  
        case WSAEPFNOSUPPORT:       return ErrorProtFamilyNotSupported; break; 
        case WSAEAFNOSUPPORT:       return ErrorAddrFamilyNotSupported; break; 
        case WSAEADDRINUSE:         return ErrorAddrInUse; break;              
        case WSAEADDRNOTAVAIL:      return ErrorAddrNotAvailable; break;       
        case WSAENETDOWN:           return ErrorNetDown; break;                
        case WSAENETUNREACH:        return ErrorNetUnreachable; break;         
        case WSAENETRESET:          return ErrorNetReset; break;               
        case WSAECONNABORTED:       return ErrorConnectionAborted; break;      
        case WSAECONNRESET:         return ErrorConnectionReset; break;        
        case WSAENOBUFS:            return ErrorNoBufferSpace; break;          
        case WSAEISCONN:            return ErrorIsConnected; break;            
        case WSAENOTCONN:           return ErrorNotConnected; break;           
        case WSAESHUTDOWN:          return ErrorIsShutdown; break;             
        case WSAETIMEDOUT:          return ErrorIsTimedOut; break;             
        case WSAECONNREFUSED:       return ErrorConnectionRefused; break;      
        case WSAEHOSTDOWN:          return ErrorHostDown; break;               
        case WSAEHOSTUNREACH:       return ErrorHostUnreachable; break;        
        case WSAEPROCLIM:           return ErrorTooManyProcesses; break;       
        case WSASYSNOTREADY:        return ErrorSystemNotReady; break;         
        case WSAVERNOTSUPPORTED:    return ErrorVersionNotSupported; break;    
        case WSANOTINITIALISED:     return ErrorNotInitialized; break;         
        case WSAEDISCON:            return ErrorDisconnecting; break;          
        case WSATYPE_NOT_FOUND:     return ErrorTypeNotFound; break;           
        case WSAHOST_NOT_FOUND:     return ErrorHostNotFound; break;           
        case WSATRY_AGAIN:          return ErrorTryAgain; break;               
        case WSANO_RECOVERY:        return ErrorNoRecovery; break;             
        case WSANO_DATA:            return ErrorNoData; break;                 
        default:                    
            return ErrorUnknown; 
            break;
    }
}

//------------------------------------------------------------------------------
/**
    Convert an error code to a human readable error message.
*/
String
Win32Socket::ErrorAsString(ErrorCode err)
{
    switch (err)
    {
        case ErrorNone:                      return "No error.";
        case ErrorUnknown:                   return "Unknown error (not mapped by Win32Socket class).";
        case ErrorInterrupted:               return "Interrupted function call.";
        case ErrorPermissionDenied:          return "Permission denied.";
        case ErrorBadAddress:                return "Bad address.";
        case ErrorInvalidArgument:           return "Invalid argument.";
        case ErrorTooManyOpenFiles:          return "Too many open files (sockets).";
        case ErrorWouldBlock:                return "Operation would block.";
        case ErrorInProgress:                return "Operation now in progress.";
        case ErrorAlreadyInProgress:         return "Operation already in progress.";
        case ErrorNotASocket:                return "Socket operation on non-socket.";
        case ErrorDestAddrRequired:          return "Destination address required";
        case ErrorMsgTooLong:                return "Message too long.";
        case ErrorInvalidProtocol:           return "Protocol wrong type for socket.";
        case ErrorBadProtocolOption:         return "Bad protocal option.";
        case ErrorProtocolNotSupported:      return "Protocol not supported.";
        case ErrorSocketTypeNotSupported:    return "Socket type not supported.";
        case ErrorOperationNotSupported:     return "Operation not supported.";
        case ErrorProtFamilyNotSupported:    return "Protocol family not supported.";
        case ErrorAddrFamilyNotSupported:    return "Address family not supported by protocol family.";
        case ErrorAddrInUse:                 return "Address already in use.";
        case ErrorAddrNotAvailable:          return "Cannot assign requested address.";
        case ErrorNetDown:                   return "Network is down.";
        case ErrorNetUnreachable:            return "Network is unreachable.";
        case ErrorNetReset:                  return "Network dropped connection on reset.";
        case ErrorConnectionAborted:         return "Software caused connection abort.";
        case ErrorConnectionReset:           return "Connection reset by peer.";
        case ErrorNoBufferSpace:             return "No buffer space available.";
        case ErrorIsConnected:               return "Socket is already connected.";
        case ErrorNotConnected:              return "Socket is not connected.";
        case ErrorIsShutdown:                return "Cannot send after socket shutdown.";
        case ErrorIsTimedOut:                return "Connection timed out.";
        case ErrorConnectionRefused:         return "Connection refused.";
        case ErrorHostDown:                  return "Host is down.";
        case ErrorHostUnreachable:           return "No route to host.";
        case ErrorTooManyProcesses:          return "Too many processes.";
        case ErrorSystemNotReady:            return "Network subsystem is unavailable.";
        case ErrorVersionNotSupported:       return "Winsock.dll version out of range.";
        case ErrorNotInitialized:            return "Successful WSAStartup not yet performed.";
        case ErrorDisconnecting:             return "Graceful shutdown in progress.";
        case ErrorTypeNotFound:              return "Class type not found.";
        case ErrorHostNotFound:              return "Host not found.";
        case ErrorTryAgain:                  return "Nonauthoritative host not found.";
        case ErrorNoRecovery:                return "This is a nonrecoverable error.";
        case ErrorNoData:                    return "Valid name, no data record of requested type.";
        default:
            n_error("Win32Socket::ErrorAsString(): unhandled error code!");
            return "";
    }
}

//------------------------------------------------------------------------------
/**
*/
String
Win32Socket::WSAErrorToString(int wsaError)
{
    return ErrorAsString(WSAErrorToErrorCode(wsaError));
}

//------------------------------------------------------------------------------
/**
*/
String
Win32Socket::GetErrorString() const
{
    return ErrorAsString(this->error);
}

//------------------------------------------------------------------------------
/**
*/
bool 
Win32Socket::IsNetworkInitialized()
{
    return NetworkInitialized;
}
} // namespace Win32
