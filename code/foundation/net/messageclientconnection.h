#pragma once
//------------------------------------------------------------------------------
/**
    @class Net::MessageClientConnection

    A wrapper class for the Net::TcpClientConnection.
    
    All data that is send by using Send() will streamed into a container, before
    it will send to the client. The message contains information
    about the full length of the data. This way the receiving site can handle
    messages that are splittet into chunks and may wait until a full message was received.

    The MessageClientConnection will concatenate incoming data chunks to full messages and
    Recv() will only return finished messages.

    (C) 2009 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "net/tcpclientconnection.h"
#include "net/tcpmessagecodec.h"
#include "util/queue.h"

//------------------------------------------------------------------------------
namespace Net
{
class MessageClientConnection : public TcpClientConnection
{
    __DeclareClass(MessageClientConnection);
public:
    // Constructor
    MessageClientConnection();
    // Destructor
    virtual ~MessageClientConnection();
    
    /// connect using provided socket
    virtual bool Connect(const Ptr<Socket>& s);
    /// shutdown the connection
    virtual void Shutdown();

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

private:   
    Ptr<IO::Stream> sendMessageStream;
    Ptr<IO::Stream> recvMessageStream;
    Util::Queue<Ptr<IO::Stream> > msgQueue;
    TcpMessageCodec codec;
};
} // namespace Net