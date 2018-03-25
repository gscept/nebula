//------------------------------------------------------------------------------
//  messageclientconnection.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#if !__WII__
#include "net/messageclientconnection.h"
#include "io/textwriter.h"
#include "io/memorystream.h"
//------------------------------------------------------------------------------

using namespace IO;
using namespace Util;

namespace Net
{
__ImplementClass(Net::MessageClientConnection,'MGCC',Net::TcpClientConnection);

//------------------------------------------------------------------------------
/**
    Constructor	
*/
MessageClientConnection::MessageClientConnection()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Destructor	
*/
MessageClientConnection::~MessageClientConnection()
{
    n_assert(!this->IsConnected());
}

//------------------------------------------------------------------------------
/**
    Connect using provided socket. See TcpClientConnection for details.
*/
bool
MessageClientConnection::Connect(const Ptr<Socket>& s)
{
    n_assert(!this->sendMessageStream.isvalid());
    n_assert(!this->recvMessageStream.isvalid());
    n_assert(this->msgQueue.IsEmpty());
    bool res = TcpClientConnection::Connect(s);
    if (res)
    {
        this->sendMessageStream = MemoryStream::Create();
    }
    return res;
}

//------------------------------------------------------------------------------
/**
    This disconnects the current connection.
*/
void
MessageClientConnection::Shutdown()
{
    n_assert(this->msgQueue.IsEmpty());
    TcpClientConnection::Shutdown();
    this->sendMessageStream = nullptr;
    this->recvMessageStream = nullptr;
}

//------------------------------------------------------------------------------
/**
    Writes accumulated send stream into a message container and sends
    this container.
*/
Socket::Result
MessageClientConnection::Send()
{
    return this->Send(this->sendMessageStream);
}

//------------------------------------------------------------------------------
/**
	Writes given stream into a message container and sends this container.
*/
Socket::Result
MessageClientConnection::Send(const Ptr<IO::Stream> &stream)
{
    n_assert(stream.isvalid());
    if (stream->GetSize() == 0)
    {
        // nothing to send
        return Socket::Success;
    }

    Socket::Result res = Socket::Error;
    stream->SetAccessMode(Stream::ReadAccess);
    this->codec.EncodeToMessage(stream, this->sendStream);

	n_printf("MessageClientConnection: sending %d bytes\n", this->sendStream->GetSize());

    return TcpClientConnection::Send(sendStream);
}

//------------------------------------------------------------------------------
/**
	Returns the stream to which data for sending is written.
*/
const Ptr<IO::Stream>&
MessageClientConnection::GetSendStream()
{
    return this->sendMessageStream;
}

//------------------------------------------------------------------------------
/**
	Receive data from the clients, but returns only Success, if a
    complete message was available. This message will either return one
    complete message, or none at all.
*/
Socket::Result
MessageClientConnection::Recv()
{
    // first check if new data is available from the socket...
    Socket::Result returnValue = TcpClientConnection::Recv();
    if (Socket::Success == returnValue)
    {
        // decode incoming data into separate messages, and queue 
        // the messages for later use
        this->codec.DecodeStream(this->recvStream);
        if (this->codec.HasMessages())
        {
            Array<Ptr<Stream> > msgArray = this->codec.DequeueMessages();
            IndexT i;
            for (i = 0; i < msgArray.Size(); i++)
            {
                this->msgQueue.Enqueue(msgArray[i]);
            }
        }
        else
        {
            // Connection blocks if received data, but no complete message
            returnValue = Socket::WouldBlock;
        }
    }

    // now write the oldest message from the message queue
    // to the recv stream, so that exactly one 
    if (!this->msgQueue.IsEmpty())
    {
        this->recvMessageStream = this->msgQueue.Dequeue();
        
        // as long as messages are available, and no error occured,
        // set the result to Success
        if (Socket::Error != returnValue)
        {
            returnValue = Socket::Success;
        }
    }
    else
    {
        // no messages available
        this->recvMessageStream = nullptr;
        if (Socket::Error != returnValue)
        {
            returnValue = Socket::WouldBlock;
        }
    }
    return returnValue;
}

//------------------------------------------------------------------------------
/**
    Returns the stream with the received data	
*/
const Ptr<IO::Stream>&
MessageClientConnection::GetRecvStream()
{
    return this->recvMessageStream;
}

} // namespace Net

#endif // #if not __WII__