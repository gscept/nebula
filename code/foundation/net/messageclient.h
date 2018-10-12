#pragma once
//------------------------------------------------------------------------------
/**
    @class Net::MessageClient

    Wrapper class for the Net::TcpClient that sends data in special message
    container.

    (C) 2009 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "net/tcpclient.h"
#include "net/tcpmessagecodec.h"
#include "io/memorystream.h"
#include "util/queue.h"

//------------------------------------------------------------------------------
namespace Net
{
class MessageClient : public TcpClient
{
    __DeclareClass(MessageClient);
public:
    /// constructor
    MessageClient();
    /// destructor
    virtual ~MessageClient();
    
    /// establish a connection with the server
    virtual Result Connect();
    /// disconnect from the server
    virtual void Disconnect();

    /// send accumulated content of send stream to server
    virtual bool Send();
    /// access to send stream
    virtual const Ptr<IO::Stream>& GetSendStream();
    /// receive data from server into recv stream
    virtual bool Recv();
    /// access to recv stream
    virtual const Ptr<IO::Stream>& GetRecvStream();

private:
    TcpMessageCodec codec;
    Ptr<IO::Stream> sendMessageStream;
    Ptr<IO::Stream> recvMessageStream;
    Util::Queue<Ptr<IO::Stream> > msgQueue;
};

//------------------------------------------------------------------------------
} // namespace Net