//------------------------------------------------------------------------------
// messageclient.cc
// (C) 2009 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "net/messageclient.h"

namespace Net
{
__ImplementClass(Net::MessageClient,'MSGC',Net::TcpClient);

using namespace IO;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
MessageClient::MessageClient()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
MessageClient::~MessageClient()
{
    if (this->IsConnected())
    {
        this->Disconnect();
    }
}

//------------------------------------------------------------------------------
/**
    Establish a connection with the server. See TcpClient for details.
*/
TcpClient::Result
MessageClient::Connect()
{
    n_assert(!this->sendMessageStream.isvalid());
    n_assert(!this->recvMessageStream.isvalid());
    TcpClient::Result res = TcpClient::Connect();
    if (TcpClient::Success == res)
    {
        this->sendMessageStream = MemoryStream::Create();
    }
    return res;
}

//------------------------------------------------------------------------------
/**
*/
void
MessageClient::Disconnect()
{
    TcpClient::Disconnect();
    this->sendMessageStream = nullptr;
    this->recvMessageStream = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
bool
MessageClient::Send()
{
    n_assert(this->sendMessageStream.isvalid());
    if (this->sendMessageStream->GetSize() == 0)
    {
        // nothing to send
        return Socket::Success;
    }
    Socket::Result res = Socket::Error;
    this->sendMessageStream->SetAccessMode(Stream::ReadAccess);
    this->codec.EncodeToMessage(this->sendMessageStream,TcpClient::GetSendStream());
    this->sendMessageStream->SetSize(0);
    return TcpClient::Send();
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<IO::Stream>&
MessageClient::GetSendStream()
{
    return this->sendMessageStream;
}

//------------------------------------------------------------------------------
/**
*/
bool
MessageClient::Recv()
{
    // first check if new data is available from the socket
    if (TcpClient::Recv())
    {
        // decode incoming data into separate message, and queue the
        // message for later use
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
    }

    // set the oldest received message from the message queue
    // as receive stream (we either return 1 complete message,
    // or none at all per call to Recv()
    if (!this->msgQueue.IsEmpty())
    {
        this->recvMessageStream = this->msgQueue.Dequeue();
        return true;
    }
    else
    {
        // no messages to return
        return false;
    }
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<IO::Stream>&
MessageClient::GetRecvStream()
{
    return this->recvMessageStream;
}

} // namespace Net
