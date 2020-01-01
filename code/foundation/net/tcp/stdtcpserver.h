#pragma once
//------------------------------------------------------------------------------
/**
    @class Net::StdTcpServer

    A TcpServer opens a socket and listens for connecting TcpClients. This
    listen process happens in its own listener-thread and thus doesn't block
    the application. Each connected client is represented through a
    TcpClientConnection object which can be used by the application
    to communicate with a specific client.

    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "threading/thread.h"
#include "util/array.h"
#include "net/tcpclientconnection.h"
#include "net/socket/socket.h"
#include "threading/criticalsection.h"

//------------------------------------------------------------------------------
namespace Net
{
class StdTcpServer : public Core::RefCounted
{
    __DeclareClass(StdTcpServer);
public:
    /// constructor
    StdTcpServer();
    /// destructor
    virtual ~StdTcpServer();
    /// set address, hostname can be "any", "self" or "inetself"
    void SetAddress(const IpAddress& addr);
    /// get address
    const IpAddress& GetAddress() const;
    /// set client connection class
    void SetClientConnectionClass(const Core::Rtti& type);
    /// get client connection class
    const Core::Rtti& GetClientConnectionClass();
    /// open the server
    bool Open();
    /// close the server
    void Close();
    /// return true if server is open
    bool IsOpen() const;
    /// poll clients connections for received data, call this frequently!
    Util::Array<Ptr<TcpClientConnection> > Recv();
    /// broadcast a message to all clients
    bool Broadcast(const Ptr<IO::Stream>& msg);

private:
    /// a private listener thread class
    class ListenerThread : public Threading::Thread
    {
        __DeclareClass(ListenerThread);
    public:
        /// set pointer to parent tcp server
        void SetTcpServer(StdTcpServer* tcpServer);
        /// set ip address
        void SetAddress(const IpAddress& a);
        /// set client connection class
        void SetClientConnectionClass(const Core::Rtti& type);
    private:
        /// implements the actual listener method
        virtual void DoWork();
        /// send a wakeup signal
        virtual void EmitWakeupSignal();

        Ptr<StdTcpServer> tcpServer;
        IpAddress ipAddress;
        Ptr<Socket> socket;
        const Core::Rtti* connectionClassRtti;
    };
    friend class ListenerThread;
    /// add a client connection (called by the listener thread)
    void AddClientConnection(const Ptr<TcpClientConnection>& connection);

    IpAddress ipAddress;
    Ptr<ListenerThread> listenerThread;
    bool isOpen;
    Util::Array<Ptr<TcpClientConnection> > clientConnections;
    Threading::CriticalSection connectionCritSect;
    const Core::Rtti* connectionClassRtti;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
StdTcpServer::IsOpen() const
{
    return this->isOpen;
}

//------------------------------------------------------------------------------
/**
*/
inline void
StdTcpServer::SetAddress(const IpAddress& addr)
{
    this->ipAddress = addr;
}

//------------------------------------------------------------------------------
/**
*/
inline const IpAddress&
StdTcpServer::GetAddress() const
{
    return this->ipAddress;
}

//------------------------------------------------------------------------------
/**
*/
inline void
StdTcpServer::SetClientConnectionClass(const Core::Rtti& type)
{
    this->connectionClassRtti = &type;
}

//------------------------------------------------------------------------------
/**	
*/
inline const Core::Rtti&
StdTcpServer::GetClientConnectionClass()
{
    return *this->connectionClassRtti;
}
} // namespace Net
//------------------------------------------------------------------------------
