//------------------------------------------------------------------------------
//  stdtcpclientconnection.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "net/tcp/stdtcpclientconnection.h"
#include "io/memorystream.h"

namespace Net
{
__ImplementClass(Net::StdTcpClientConnection, 'STCC', Core::RefCounted);

using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
StdTcpClientConnection::StdTcpClientConnection()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
StdTcpClientConnection::~StdTcpClientConnection()
{
    this->Shutdown();
}

//------------------------------------------------------------------------------
/**
*/
bool
StdTcpClientConnection::Connect(const Ptr<Socket>& sock)
{
    n_assert(!this->socket.isvalid());
    n_assert(sock.isvalid());
    n_assert(sock->IsOpen());
    
    // check if socket is really actually connected
    if (sock->IsConnected())
    {
        this->socket = sock;
        this->socket->SetBlocking(true);
        this->socket->SetNoDelay(true);
        this->sendStream = MemoryStream::Create();
        this->recvStream = MemoryStream::Create();
        n_printf("Client from addr=%s connected!\n", 
            this->socket->GetAddress().GetHostAddr().AsCharPtr());
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
StdTcpClientConnection::IsConnected() const
{
    if (this->socket.isvalid())
    {
        return this->socket->IsConnected();
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
StdTcpClientConnection::Shutdown()
{
    if (this->socket.isvalid())
    {
        this->socket->Close();
        this->socket = nullptr;
    }
    this->sendStream = nullptr;
    this->recvStream = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
const IpAddress&
StdTcpClientConnection::GetClientAddress() const
{
    n_assert(this->socket.isvalid());
    return this->socket->GetAddress();
}

//------------------------------------------------------------------------------
/**
*/
Socket::Result
StdTcpClientConnection::Send(const Ptr<Stream>& stream)
{
    n_assert(stream.isvalid());
    if (stream->GetSize() == 0)
    {
        // nothing to send
        return Socket::Success;
    }
    
    Socket::Result res = Socket::Success;
    stream->SetAccessMode(Stream::ReadAccess);
    if (stream->Open())
    {
        // we may not exceed the maximum message size...
        // so we may have to split the send data into
        // multiple packets
        SizeT maxMsgSize = this->socket->GetMaxMsgSize();
        SizeT sendSize = stream->GetSize();
        uchar* ptr = (uchar*) stream->Map();
        SizeT overallBytesSent = 0;
        while ((Socket::Success == res) && (overallBytesSent < sendSize))
        {
            SizeT bytesToSend = sendSize - overallBytesSent;
            if (bytesToSend > maxMsgSize)
            {
                bytesToSend = maxMsgSize;
            }
            SizeT bytesSent = 0;
            res = this->socket->Send(ptr, bytesToSend, bytesSent);
            ptr += bytesSent;
            overallBytesSent += bytesSent;
        }
        stream->Unmap();
        stream->Close();
    }
    return res;
}

//------------------------------------------------------------------------------
/**
*/
Socket::Result
StdTcpClientConnection::Send()
{
    n_assert(this->sendStream.isvalid());
    Socket::Result res = this->Send(this->sendStream);
    this->sendStream->SetSize(0);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
Socket::Result
StdTcpClientConnection::Recv()
{
    n_assert(this->recvStream.isvalid());
    this->recvStream->SetAccessMode(Stream::WriteAccess);
    this->recvStream->SetSize(0);
    Socket::Result res = Socket::Success;
    if (this->recvStream->Open())
    {
        // NOTE: the following loop will make sure that Recv()
        // never blocks
        uchar buf[1024];
        while ((Socket::Success == res) && (this->socket->HasRecvData()))
        {
            SizeT bytesReceived = 0;
            res = this->socket->Recv(&buf, sizeof(buf), bytesReceived);
            if ((bytesReceived > 0) && (Socket::Success == res))
            {
                this->recvStream->Write(buf, bytesReceived);
            }
        }
        this->recvStream->Close();
    }
    this->recvStream->SetAccessMode(Stream::ReadAccess);
    if (Socket::Success == res)
    {
        if (this->recvStream->GetSize() > 0)
        {
            return Socket::Success;
        }
        else
        {
            return Socket::WouldBlock;
        }
    }
    else
    {
        return res;
    }
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<Stream>&
StdTcpClientConnection::GetSendStream()
{
    return this->sendStream;
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<Stream>&
StdTcpClientConnection::GetRecvStream()
{
    return this->recvStream;
}

} // namespace Net
