//------------------------------------------------------------------------------
//  posixsocket.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "net/socket/socket.h"

#include <sys/errno.h>
#include <fcntl.h>
#include <netdb.h>

namespace Posix
{
__ImplementClass(Posix::PosixSocket, 'WSCK', Core::RefCounted);
using namespace Util;

bool PosixSocket::NetworkInitialized = false;

const static int INVALID_SOCKET = -1;
const static int SOCKET_ERROR = -1;

//------------------------------------------------------------------------------
/**
*/
PosixSocket::PosixSocket() :
    error(ErrorNone),
    sock(0),
    isBlocking(true),
    isBound(false)
{
    // one time init of Windows Sockets
    if (!NetworkInitialized)
    {
        this->InitNetwork();
    }
}

//------------------------------------------------------------------------------
/**
*/
PosixSocket::~PosixSocket()
{
    if (this->IsOpen())
    {
        this->Close();
    }
}

//------------------------------------------------------------------------------
/**
    This is a one-time init for the Windows Sockets system.
*/
void
PosixSocket::InitNetwork()
{
    n_assert(!NetworkInitialized);
    NetworkInitialized = true;
}   

//------------------------------------------------------------------------------
/**
*/
bool
PosixSocket::Open(Protocol protocol)
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
        this->SetToLastSocketError();
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
PosixSocket::OpenWithExistingSocket(SOCKET s)
{
    n_assert(INVALID_SOCKET != s);
    n_assert(!this->IsOpen());
    this->sock = s;
}

//------------------------------------------------------------------------------
/**
*/
void
PosixSocket::Close()
{
    n_assert(this->IsOpen());
    this->ClearError();

    int res = 0;
    if (this->IsConnected())
    {
        res = shutdown(this->sock, SHUT_RDWR);
        if (SOCKET_ERROR == res)
        {
            // note: the shutdown function may return NotConnected, this
            // is not really an error
            this->SetToLastSocketError();
            if (ErrorNotConnected != this->error)
            {
                n_printf("PosixSocket::Close(): shutdown() failed with '%s'.\n", this->GetErrorString().AsCharPtr());
            }
        }
    }
    res = close(this->sock);
    if (SOCKET_ERROR == res)
    {
        this->SetToLastSocketError();
        n_printf("PosixSocket::Close(): closesocket() failed with '%s'.\n", this->GetErrorString().AsCharPtr());
    }
    this->sock = 0;
}

//------------------------------------------------------------------------------
/**
    Set a boolean option on the socket. This is a private helper
    function.
*/
void
PosixSocket::SetBoolOption(int optName, bool val)
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
        this->SetToLastSocketError();
        n_printf("PosixSocket::SetBoolOption(): setsockopt() failed with '%s'.\n", this->GetErrorString().AsCharPtr());
    }
}

//------------------------------------------------------------------------------
/**
    Get a boolean option on the socket. This is a private helper
    function.
*/
bool
PosixSocket::GetBoolOption(int optName)
{
    n_assert(this->IsOpen());
    this->ClearError();
    int level = SOL_SOCKET;
    if (optName == TCP_NODELAY)
    {
        level = IPPROTO_TCP;
    }
    int optVal = 0;
    socklen_t optValSize = sizeof(optVal);
    int res = getsockopt(this->sock, level, optName, (char*) &optVal, &optValSize);
    if (SOCKET_ERROR == res)
    {
        this->SetToLastSocketError();
        n_printf("PosixSocket::GetBoolOption(): getsockopt() failed with '%s'.\n", this->GetErrorString().AsCharPtr());
    }
    n_assert(sizeof(optVal) == optValSize);
    return (0 != optVal);
}

//------------------------------------------------------------------------------
/**
    Set an int socket option. This is a private helper function.
*/
void
PosixSocket::SetIntOption(int optName, int val)
{
    n_assert(this->IsOpen());
    this->ClearError();
    int res = setsockopt(this->sock, SOL_SOCKET, optName, (const char*) &val, sizeof(val));
    if (SOCKET_ERROR == res)
    {
        this->SetToLastSocketError();
        n_printf("PosixSocket::SetIntOption(): setsockopt() failed with '%s'.\n", this->GetErrorString().AsCharPtr());
    }
}

//------------------------------------------------------------------------------
/**
    Get an int socket option. This is a private helper function.
*/
int
PosixSocket::GetIntOption(int optName)
{
    n_assert(this->IsOpen());
    this->ClearError();
    int optVal = 0;
    socklen_t optValSize = sizeof(optVal);
    int res = getsockopt(this->sock, SOL_SOCKET, optName, (char*) &optVal, &optValSize);
    if (SOCKET_ERROR == res)
    {
        this->SetToLastSocketError();
        n_printf("PosixSocket::GetIntOption(): getsockopt() failed with '%s'.\n", this->GetErrorString().AsCharPtr());
    }
    n_assert(sizeof(optVal) == optValSize);
    return optVal;
}

//------------------------------------------------------------------------------
/**
    Set the socket to blocking mode.
*/
void
PosixSocket::SetBlocking(bool b)
{
    n_assert(this->IsOpen());
    this->ClearError();
    int flags = fcntl(this->sock, F_GETFL, 0);
    if (SOCKET_ERROR == flags)
    {
        this->SetToLastSocketError();
        n_printf("PosixSocket::SetBlocking(): fcntl(F_GETFL) failed with '%s'.\n", this->GetErrorString().AsCharPtr());
    }
    if (!b)
    {
        flags |= O_NONBLOCK;
    }
    else
    {
        flags &= ~O_NONBLOCK;
    }
    flags = fcntl(this->sock, F_SETFL, flags);
    if (SOCKET_ERROR == flags)
    {
        this->SetToLastSocketError();
        n_printf("PosixSocket::SetBlocking(): fcntl(F_SETFL) failed with '%s'.\n", this->GetErrorString().AsCharPtr());
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
PosixSocket::Bind()
{
    n_assert(this->IsOpen());
    n_assert(!this->IsBound());
    this->ClearError();
    const sockaddr& sockAddr = this->addr.GetSockAddr();
    int res = bind(this->sock, &sockAddr, sizeof(sockAddr));
    if (SOCKET_ERROR == res)
    {
        this->SetToLastSocketError();
        n_printf("PosixSocket::Bind(): bind() failed with '%s'!\n", this->GetErrorString().AsCharPtr());
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
PosixSocket::Listen()
{
    n_assert(this->IsOpen());
    n_assert(this->IsBound());
    this->ClearError();
    int res = listen(this->sock, SOMAXCONN);
    if (SOCKET_ERROR == res)
    {
        this->SetToLastSocketError();
        n_printf("PosixSocket::Listen(): listen() failed with '%s'!\n", this->GetErrorString().AsCharPtr());
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
PosixSocket::Accept(Ptr<Net::Socket>& outSocket)
{
    n_assert(this->IsOpen());
    n_assert(this->IsBound());

    this->ClearError();
    outSocket = nullptr;
    sockaddr_in sockAddr;
    socklen_t sockAddrSize = sizeof(sockAddr);
    SOCKET newSocket = accept(this->sock, (sockaddr*) &sockAddr, &sockAddrSize);
    if (INVALID_SOCKET == newSocket)
    {
        this->SetToLastSocketError();
        n_printf("PosixSocket::Accept(): accept() failed with '%s'!\n", this->GetErrorString().AsCharPtr());
        return false;
    }
    Net::IpAddress ipAddr;
    ipAddr.SetSockAddr((sockaddr &)sockAddr);
    outSocket = Net::Socket::Create();
    outSocket->SetAddress(ipAddr);
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
PosixSocket::Result
PosixSocket::Connect()
{
    n_assert(this->IsOpen());
    n_assert(!this->IsBound());

    this->ClearError();
    const sockaddr& sockAddr = this->addr.GetSockAddr();
    int res = connect(this->sock, &sockAddr, sizeof(sockAddr));
    if (SOCKET_ERROR == res)
    {
        // special handling for non-blocking sockets
        int lastError = errno;
        if (!this->GetBlocking())
        {
            if (EWOULDBLOCK == lastError)
            {
                return WouldBlock;
            }
            else if (EALREADY == lastError)
            {
                // connection is underway but not finished yet
                return WouldBlock;
            }
            else if (EISCONN == lastError)
            {
                // the connection is established
                return Success;
            }            
            // fallthrough: a normal error
        }      
        this->SetSocketError(lastError);
        n_printf("PosixSocket::Connect(): connect() failed with '%s'!\n", this->GetErrorString().AsCharPtr());
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
PosixSocket::IsConnected()
{
    n_assert(this->IsOpen());
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(this->sock, &readSet);
    
    timeval timeVal = { 0, 0 };
    int res = select(this->sock + 1, 0, &readSet, 0, &timeVal); 
    if (SOCKET_ERROR == res)
    {
        this->SetToLastSocketError();
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
PosixSocket::Result
PosixSocket::Send(const void* buf, SizeT numBytes, SizeT& bytesSent)
{
    n_assert(this->IsOpen());
    n_assert(0 != buf);
    this->ClearError();
    bytesSent = 0;
    int res = send(this->sock, (const char*) buf, numBytes, 0);
    if (SOCKET_ERROR == res)
    {
        if (EWOULDBLOCK == res)
        {
            return WouldBlock;
        }
        else
        {
            this->SetToLastSocketError();
            return Error;
        }
    }
    bytesSent = res;
    return Success;
}

//------------------------------------------------------------------------------
/**
    This method checks if the socket has received data available. Use
    this method in a loop with Recv() to get all data waiting at the
    socket. This method will never block.
*/
bool
PosixSocket::HasRecvData()
{
    n_assert(this->IsOpen());
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(this->sock, &readSet);
    
    timeval timeVal = { 0, 0 };
    int res = select(this->sock + 1, &readSet, 0, 0, &timeVal);
    if (SOCKET_ERROR == res)
    {
        this->SetToLastSocketError();
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
PosixSocket::Result
PosixSocket::Recv(void* buf, SizeT bufSize, SizeT& bytesReceived)
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
        int lastError = errno;
        if (EMSGSIZE == lastError)
        {
            // more data is pending
            bytesReceived = bufSize;    // FIXME: is this correct?
            return Success;
        }
        if (!this->isBlocking)
        {
            // socket is non-blocking and no data is available
            if (EWOULDBLOCK == lastError)
            {
                return WouldBlock;
            }
        }

        // fallthrough: a real error
        this->SetToLastSocketError();
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
PosixSocket::Result
PosixSocket::SendTo(const void* /*buf*/, SizeT /*numBytes*/, uint /*addr*/, ushort /*port*/, SizeT& /*bytesSent*/)
{
    n_error("PosixSocket::SendTo(): IMPLEMENT ME!");
    return Error;
}

//------------------------------------------------------------------------------
/**
    FIXME: this is the recv method for connectionless socket using the UDP
    protocol.
*/
PosixSocket::Result
PosixSocket::RecvFrom(void* /*buf*/, SizeT /*bufSize*/, uint /*addr*/, ushort /*port*/, SizeT& /*bytesReceived*/)
{
    n_error("PosixSocket::RecvFrom(): IMPLEMENT ME!");
    return Error;
}

//------------------------------------------------------------------------------
/**
    Sets the internal error code to NoError.
*/
void
PosixSocket::ClearError()
{
    this->error = ErrorNone;
}

//------------------------------------------------------------------------------
/**
    Sets the internal error code to errno.
*/
void
PosixSocket::SetToLastSocketError()
{
    this->error = SocketErrorToErrorCode(errno);
}

//------------------------------------------------------------------------------
/**
    Sets the provided socket error as error code.
*/
void
PosixSocket::SetSocketError(int wsaError)
{
    this->error = SocketErrorToErrorCode(wsaError);
}

//------------------------------------------------------------------------------
/**
    This method converts a socket error code into a
    portable error code used by PosixSocket.
*/
PosixSocket::ErrorCode
PosixSocket::SocketErrorToErrorCode(int wsaErrorCode)
{
    switch (wsaErrorCode)
    {
        case EINTR:              return ErrorInterrupted; break;           
	case EPIPE:		 return ErrorBrokenPipe; break;
        case EACCES:             return ErrorPermissionDenied; break;       
        case EFAULT:             return ErrorBadAddress; break;             
        case EINVAL:             return ErrorInvalidArgument; break;        
        case EMFILE:             return ErrorTooManyOpenFiles; break;       
        case EWOULDBLOCK:        return ErrorWouldBlock; break;             
        case EINPROGRESS:        return ErrorInProgress; break;             
        case EALREADY:           return ErrorAlreadyInProgress; break;      
        case ENOTSOCK:           return ErrorNotASocket; break;             
        case EDESTADDRREQ:       return ErrorDestAddrRequired; break;       
        case EMSGSIZE:           return ErrorMsgTooLong; break;             
        case EPROTOTYPE:         return ErrorInvalidProtocol; break;        
        case ENOPROTOOPT:        return ErrorBadProtocolOption; break;      
        case EPROTONOSUPPORT:    return ErrorProtocolNotSupported; break;   
        case ESOCKTNOSUPPORT:    return ErrorSocketTypeNotSupported; break; 
        case EOPNOTSUPP:         return ErrorOperationNotSupported; break;  
        case EPFNOSUPPORT:       return ErrorProtFamilyNotSupported; break; 
        case EAFNOSUPPORT:       return ErrorAddrFamilyNotSupported; break; 
        case EADDRINUSE:         return ErrorAddrInUse; break;              
        case EADDRNOTAVAIL:      return ErrorAddrNotAvailable; break;
        case ENETDOWN:           return ErrorNetDown; break;
        case ENETUNREACH:        return ErrorNetUnreachable; break;         
        case ENETRESET:          return ErrorNetReset; break;               
        case ECONNABORTED:       return ErrorConnectionAborted; break;      
        case ECONNRESET:         return ErrorConnectionReset; break;        
        case ENOBUFS:            return ErrorNoBufferSpace; break;          
        case EISCONN:            return ErrorIsConnected; break;            
        case ENOTCONN:           return ErrorNotConnected; break;           
        case ESHUTDOWN:          return ErrorIsShutdown; break;             
        case ETIMEDOUT:          return ErrorIsTimedOut; break;             
        case ECONNREFUSED:       return ErrorConnectionRefused; break;      
        case EHOSTDOWN:          return ErrorHostDown; break;               
        case EHOSTUNREACH:       return ErrorHostUnreachable; break;            
        default:
            return ErrorUnknown;
            break;
    }
}

//------------------------------------------------------------------------------
/**
    This method converts a host error code into a
    portable error code used by PosixSocket.
*/
PosixSocket::ErrorCode
PosixSocket::HostErrorToErrorCode(int wsaErrorCode)
{
    switch(wsaErrorCode)
    {
        case HOST_NOT_FOUND:     return ErrorHostNotFound; break;
        case TRY_AGAIN:          return ErrorTryAgain; break;
        case NO_RECOVERY:        return ErrorNoRecovery; break;
        case NO_DATA:            return ErrorNoData; break;
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
PosixSocket::ErrorAsString(ErrorCode err)
{
    switch (err)
    {
        case ErrorNone:                      return "No error.";
        case ErrorUnknown:                   return "Unknown error (not mapped by PosixSocket class).";
        case ErrorInterrupted:               return "Interrupted function call.";
	case ErrorBrokenPipe:		     return "Broken pipe.";
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
            n_error("PosixSocket::ErrorAsString(): unhandled error code!");
            return "";
    }
}

//------------------------------------------------------------------------------
/**
*/
String
PosixSocket::SocketErrorToString(int wsaError)
{
    return ErrorAsString(SocketErrorToErrorCode(wsaError));
}

//------------------------------------------------------------------------------
/**
*/
String
PosixSocket::HostErrorToString(int wsaError)
{
    return ErrorAsString(HostErrorToErrorCode(wsaError));
}

//------------------------------------------------------------------------------
/**
*/
String
PosixSocket::GetErrorString() const
{
    return ErrorAsString(this->error);
}

} // namespace Posix
