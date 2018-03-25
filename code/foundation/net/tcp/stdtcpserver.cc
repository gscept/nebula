//------------------------------------------------------------------------------
//  stdtcperver.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "net/tcp/stdtcpserver.h"
#include "io/console.h"

namespace Net
{
__ImplementClass(Net::StdTcpServer, 'STSV', Core::RefCounted);
__ImplementClass(Net::StdTcpServer::ListenerThread, 'tclt', Threading::Thread);

using namespace Util;
using namespace Threading;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
StdTcpServer::StdTcpServer() :
    isOpen(false)
{
    this->connectionClassRtti = &TcpClientConnection::RTTI;
}

//------------------------------------------------------------------------------
/**
*/
StdTcpServer::~StdTcpServer()
{
    n_assert(!this->IsOpen());
}

//------------------------------------------------------------------------------
/**
*/
bool
StdTcpServer::Open()
{
    n_assert(!this->isOpen);
    n_assert(!this->listenerThread.isvalid());
    n_assert(this->clientConnections.IsEmpty());

    // create the listener thread
    this->listenerThread = ListenerThread::Create();
    this->listenerThread->SetName("StdTcpServer::ListenerThread");
    this->listenerThread->SetTcpServer(this);
    this->listenerThread->SetAddress(this->ipAddress);
    this->listenerThread->SetCoreId(System::Cpu::MiscThreadCore);
    this->listenerThread->SetClientConnectionClass(*this->connectionClassRtti);
    this->listenerThread->Start();
    
    this->isOpen = true;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
StdTcpServer::Close()
{
    n_assert(this->isOpen);
    n_assert(this->listenerThread.isvalid());
    
    // stop the listener thread
    this->listenerThread->Stop();
    this->listenerThread = nullptr;

    // disconnect client connections
    this->connectionCritSect.Enter();
    IndexT clientIndex;
    for (clientIndex = 0; clientIndex < this->clientConnections.Size(); clientIndex++)
    {
        const Ptr<TcpClientConnection>& curConnection = this->clientConnections[clientIndex];
        curConnection->Shutdown();
    }
    this->clientConnections.Clear();
    this->connectionCritSect.Leave();

    this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
*/
void
StdTcpServer::AddClientConnection(const Ptr<TcpClientConnection>& conn)
{
    n_assert(conn.isvalid());
    this->connectionCritSect.Enter();
    this->clientConnections.Append(conn);
    this->connectionCritSect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
Array<Ptr<TcpClientConnection> >
StdTcpServer::Recv()
{
    Array<Ptr<TcpClientConnection> > clientsWithData;

    // iterate over all clients, and check for new data,
    // if the client connection has been closed, remove
    // the client from the list
    this->connectionCritSect.Enter();
    IndexT clientIndex;
    for (clientIndex = 0; clientIndex < this->clientConnections.Size();)
    {
        const Ptr<TcpClientConnection>& cur = this->clientConnections[clientIndex];
        bool dropClient = false;
        if (cur->IsConnected())
        {
            Socket::Result res = cur->Recv();
            if (res == Socket::Success)
            {
                clientsWithData.Append(cur);
            }
            else if ((res == Socket::Error) || (res == Socket::Closed))
            {
                // some error occured, drop the connection
                dropClient = true;
            }
        }
        else
        {
            dropClient = true;
        }
        if (dropClient)
        {
            // connection has been closed, remove the client
            cur->Shutdown();
            this->clientConnections.EraseIndex(clientIndex);
        }
        else
        {
            clientIndex++;
        }
    }
    this->connectionCritSect.Leave();
    return clientsWithData;
}

//------------------------------------------------------------------------------
/**
*/
bool
StdTcpServer::Broadcast(const Ptr<Stream>& msg)
{
    bool result = true;
    this->connectionCritSect.Enter();
    IndexT i;
    for (i = 0; i < this->clientConnections.Size(); i++)
    {
        if (Socket::Success != this->clientConnections[i]->Send(msg))
        {
            result = false;
        }
    }
    this->connectionCritSect.Leave();
    return result;
}

//------------------------------------------------------------------------------
/**
*/
void
StdTcpServer::ListenerThread::SetTcpServer(StdTcpServer* serv)
{
    n_assert(0 != serv);
    this->tcpServer = serv;
}

//------------------------------------------------------------------------------
/**
*/
void
StdTcpServer::ListenerThread::SetAddress(const IpAddress& a)
{
    this->ipAddress = a;
}

//------------------------------------------------------------------------------
/**
*/
void
StdTcpServer::ListenerThread::SetClientConnectionClass(const Core::Rtti& type)
{
    this->connectionClassRtti = &type;
}

//------------------------------------------------------------------------------
/**
    This is the actual listening method which runs in a separat thread.
    It creates a server socket and listens for incoming connections. When
    a client connects, a new TcpClientConnection will be created and
    added to the array of active connections.
*/
void
StdTcpServer::ListenerThread::DoWork()
{
    // we need a minimal Nebula3 runtime to work
    n_printf("ListenerThread started!\n");

    // create a server socket
    Ptr<Socket> serverSocket = Socket::Create();
    if (serverSocket->Open(Socket::TCP))
    {
        serverSocket->SetAddress(this->ipAddress);
        serverSocket->SetReUseAddr(true);
        if (serverSocket->Bind())
        {
            n_printf("ListenerThread listening...\n");
            while (!this->ThreadStopRequested() && serverSocket->Listen())
            {
                // check if this was the wakeup function which woke us up...
                if (!this->ThreadStopRequested())
                {
                    Ptr<Socket> newSocket;
                    if (serverSocket->Accept(newSocket))
                    {
                        // create a new connection object and add to connection array
                        TcpClientConnection* newConnection = (TcpClientConnection*) this->connectionClassRtti->Create();
                        if (newConnection->Connect(newSocket))
                        {
                            this->tcpServer->AddClientConnection(newConnection);
                        }
                    }
                }
            }
        }
        else
        {
            n_printf("StdTcpServer::ListenerThread: Socket::Bind() failed!");
        }
        serverSocket->Close();
    }
    n_printf("ListenerThread shutting down!\n");

    // NOTE: we don't close the thread locale console manually, because
    // the other objects may have something to say while they're destroyed
}

//------------------------------------------------------------------------------
/**
    Emit a wakeup signal to the listener thread, this will just connect
    to the socket which wakes up the Socket::Listen() method.
*/
void
StdTcpServer::ListenerThread::EmitWakeupSignal()
{
    Ptr<Socket> socket = Socket::Create();
    if (socket->Open(Socket::TCP))
    {
        socket->SetAddress(IpAddress("127.0.0.1", this->ipAddress.GetPort()));
        socket->Connect();
        socket->Close();
    }
}

} // namespace Net