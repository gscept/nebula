#pragma once
//------------------------------------------------------------------------------
/**
    @class Net::StdTcpClientConnection
  
    A TcpClientConnection represents a connected TcpClient on the
    server side. TcpClientConnection objects are created and maintained
    by a TcpServer object over the lifetime of a client connection.
    TcpClientConnection objects are used to communicate directly with
    the specific client represented by the connection object. 

    TcpClientConnection objects are generally non-blocking. To receive
    data from the client, call the Recv() method until it returns true,
    this indicates that received data is available in the RecvStream. To
    read data from the RecvStream attach a StreamReader which matches the
    data format your expecting from the client (e.g. BinaryReader, TextReader,
    XmlReader, etc...). To send data back to the client just do the reverse:
    write data to the SendStream, and at any time call the Send() method which
    will send all data accumulated in the SendStream to the client.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/    
#include "core/refcounted.h"
#include "net/socket/ipaddress.h"
#include "io/stream.h"
#include "net/socket/socket.h"

//------------------------------------------------------------------------------
namespace Net
{
class StdTcpClientConnection : public Core::RefCounted
{
    __DeclareClass(StdTcpClientConnection);
public:
    /// constructor
    StdTcpClientConnection();
    /// destructor
    virtual ~StdTcpClientConnection();
    /// connect using provided socket
    virtual bool Connect(const Ptr<Socket>& s);
    /// get the connection status
    bool IsConnected() const;
    /// shutdown the connection
    virtual void Shutdown();
    /// get the client's ip address
    const IpAddress& GetClientAddress() const;
    /// send accumulated content of send stream to server
    virtual Socket::Result Send();
    /// directly send a stream to the server, often prevents a memory copy
    virtual Socket::Result Send(const Ptr<IO::Stream>& stream);
    /// access to send stream
    virtual const Ptr<IO::Stream>& GetSendStream();
    /// receive data from server into recv stream
    virtual Socket::Result Recv();
    /// access to recv stream
    virtual const Ptr<IO::Stream>& GetRecvStream();

protected:
    Ptr<Socket> socket;
    Ptr<IO::Stream> sendStream;
    Ptr<IO::Stream> recvStream;
};

} // namespace Net
//------------------------------------------------------------------------------
