//------------------------------------------------------------------------------
//  stdtcpclient.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "net/tcp/stdtcpclient.h"
#include "io/memorystream.h"

namespace Net
{
__ImplementClass(Net::StdTcpClient, 'STCL', Core::RefCounted);

using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
StdTcpClient::StdTcpClient() :
    blocking(true),
    inConnectionState(false)
{
    // create send and receive streams
    this->sendStream = MemoryStream::Create();
    this->recvStream = MemoryStream::Create();
}

//------------------------------------------------------------------------------
/**
*/
StdTcpClient::~StdTcpClient()
{
    if (this->IsConnected())
    {
        this->Disconnect();
    }
    this->sendStream = nullptr;
    this->recvStream = nullptr;
}

//------------------------------------------------------------------------------
/**
    Establish a connection with the server. If the client is set to
    non-blocking at the time this method is called, it will return immediately
    with the result Connecting. To check if the connection is standing, just
    call Connect() again in intervals which will eventually return Success.
    On a blocking client, connect returns after a connection has been established,
    or with a time out when no connection could be established.
*/
StdTcpClient::Result
StdTcpClient::Connect()
{
    n_assert(!this->socket.isvalid());
    n_assert(this->sendStream.isvalid());
    n_assert(this->recvStream.isvalid());

    // create a new socket and try to connect to server
    this->socket = Socket::Create();
    if (this->socket->Open(Socket::TCP))
    {
        this->socket->SetAddress(this->serverAddr);
        this->socket->SetBlocking(this->blocking);
        this->socket->SetReUseAddr(true);
        this->socket->SetNoDelay(true);

        /*
        n_printf("StdTcpClient: connecting to host '%s(%s)' port '%d'...\n", 
            this->serverAddr.GetHostName().AsCharPtr(),
            this->serverAddr.GetHostAddr().AsCharPtr(),
            this->serverAddr.GetPort());
        */

        Socket::Result res = this->socket->Connect();
        if (Socket::Error == res)
        {
            n_printf("StdTcpClient: failed to connect to host '%s(%s)' port '%d'!.\n",
                this->serverAddr.GetHostName().AsCharPtr(),
                this->serverAddr.GetHostAddr().AsCharPtr(),
                this->serverAddr.GetPort());
            this->socket = nullptr;
            return Error;
        }
        else if (Socket::Success == res)
        {
            // n_printf("StdTcpClient: connection established\n");
            this->inConnectionState = true;
            return Success;
        }
        else
        {
            // n_printf("StdTcpClient: connecting...\n");
            return Connecting;
        }
    }
    n_printf("StdTcpClient: failed to open socket!\n");
    this->socket = nullptr;
    return Error;
}

//------------------------------------------------------------------------------
/**
    Return true if the socket is currently connected. This will actually
    probe the connection using a select().
*/
bool
StdTcpClient::IsConnected()
{
    if (this->socket.isvalid())
    {
        if (this->socket->IsConnected())
        {
            return true;
        }
        else
        {
            this->Disconnect();
            return false;
        }
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------
/**
    This disconnects the current connection.
*/
void
StdTcpClient::Disconnect()
{
    if (this->inConnectionState)
    {
        n_assert(this->socket.isvalid());
        this->socket->Close();
        this->socket = nullptr;
    }
    this->inConnectionState = false;
}

//------------------------------------------------------------------------------
/**
*/
bool
StdTcpClient::Send()
{
    n_assert(this->sendStream.isvalid());
    if (this->sendStream->GetSize() == 0)
    {
        // nothing to send
        return true;
    }

    this->sendStream->SetAccessMode(Stream::ReadAccess);
    if (this->sendStream->Open())
    {
        // put the socket into blocking mode, so that if the
        // outgoing data doesn't fit into the send buffer the
        // socket will block until the next block of data can be written
        bool wasBlocking = this->socket->GetBlocking();
        if (!wasBlocking)
        {
            this->socket->SetBlocking(true);
        }

        // we may not exceed the maximum message size...
        // so we may have to split the send data into
        // multiple packets
        SizeT maxMsgSize = this->socket->GetMaxMsgSize();
        Stream::Size sendSize = this->sendStream->GetSize();
        n_assert(sendSize < INT_MAX);
        uchar* ptr = (uchar*) this->sendStream->Map();
        SizeT overallBytesSent = 0;
        Socket::Result socketResult = Socket::Success;
        while ((Socket::Success == socketResult) && (overallBytesSent < sendSize))
        {
            SizeT bytesToSend = (SizeT)sendSize - overallBytesSent;
            if (bytesToSend > maxMsgSize)
            {
                bytesToSend = maxMsgSize;
            }
            SizeT bytesSent = 0;
            socketResult = this->socket->Send(ptr, bytesToSend, bytesSent);
            if (Socket::Success == socketResult)
            {
                ptr += bytesSent;
                overallBytesSent += bytesSent;
            }
            else
            {
                // send failed, try to re-connect, and re-send
                bool clientWasBlocking = this->blocking;
                this->SetBlocking(true);
                this->Disconnect();
                Result connectResult = this->Connect();
                if (Success == connectResult)
                {
                    this->socket->SetBlocking(true);
                    n_printf("StdTcpClient re-connected!\n");
                    socketResult = Socket::Success;
                }
                else if (Connecting == connectResult)
                {
                    n_printf("StdTcpClient re-connect failed with 'connecting' (can't happen)\n");
                }
                else
                {
                    n_printf("StdTcpClient re-connect failed with 'error'\n");
                }
                this->SetBlocking(clientWasBlocking);
            }
        }
        this->sendStream->Unmap();
        if (!wasBlocking)
        {
            this->socket->SetBlocking(false);
        }
        this->sendStream->Close();
        this->sendStream->SetSize(0);
        if ((Socket::Success == socketResult) && (overallBytesSent == sendSize))
        {
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
StdTcpClient::Recv()
{
    n_assert(this->recvStream.isvalid());
    this->recvStream->SetAccessMode(Stream::WriteAccess);
    this->recvStream->SetSize(0);
    if (this->recvStream->Open())
    {
        uchar buf[1024];
        Socket::Result res = Socket::Success;
        do
        {
            // NOTE: if this is a blocking client, the first call
            // to Recv() will block until data is available
            SizeT bytesReceived = 0;
            res = this->socket->Recv(&buf, sizeof(buf), bytesReceived);
            if ((bytesReceived > 0) && (Socket::Success == res))
            {
                this->recvStream->Write(buf, bytesReceived);
            }
        }
        while ((Socket::Success == res) && this->socket->HasRecvData());
        this->recvStream->Close();
    }
    this->recvStream->SetAccessMode(Stream::ReadAccess);
    return (this->recvStream->GetSize() > 0);
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<Stream>&
StdTcpClient::GetSendStream()
{
    return this->sendStream;
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<Stream>&
StdTcpClient::GetRecvStream()
{
    return this->recvStream;
}

} // namespace Net