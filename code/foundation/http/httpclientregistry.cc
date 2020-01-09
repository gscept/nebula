//------------------------------------------------------------------------------
//  httpclientregistry.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"

// HttpClientRegistry not implemented on the Wii
#if __NEBULA_HTTP_FILESYSTEM__
#include "http/httpclientregistry.h"

namespace Http
{
__ImplementClass(Http::HttpClientRegistry, 'HTCR', Core::RefCounted);
__ImplementSingleton(Http::HttpClientRegistry);

using namespace IO;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
HttpClientRegistry::HttpClientRegistry() :
    isValid(false)
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
HttpClientRegistry::~HttpClientRegistry()
{
    if (this->IsValid())
    {
        this->Discard();
    }
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
HttpClientRegistry::Setup()
{
    n_assert(!this->isValid);
    this->isValid = true;
}

//------------------------------------------------------------------------------
/**
*/
void
HttpClientRegistry::Discard()
{
    n_assert(this->IsValid());

    // force disconnect on all open connections
    IndexT i;
    for (i = 0; i < this->connections.Size(); i++)
    {
        this->connections.ValueAtIndex(i).httpClient->Disconnect();
    }
    this->connections.Clear();

    this->isValid = false;
}

//------------------------------------------------------------------------------
/**
    This disconnects all connections with a use count of 0.
*/
void
HttpClientRegistry::DisconnectIdle()
{
    // start at the back of the array since we're removing elements
    IndexT i;
    for (i = this->connections.Size() - 1; i >= 0; i--)
    {
        const Connection& con = this->connections.ValueAtIndex(i);
        if (0 == con.useCount)
        {
            if (con.httpClient->IsConnected())
            {
                con.httpClient->Disconnect();
            }
            this->connections.EraseAtIndex(i);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
Ptr<HttpClient>
HttpClientRegistry::ObtainConnection(const URI& uri)
{
    StringAtom host = uri.Host();
    if (this->connections.Contains(host))
    {
        // connection already exists, increment connection count
        Connection& con = this->connections[host];
        con.useCount++;

        // reconnect if the connection has been dropped for some reason
        if (!con.httpClient->IsConnected())
        {
            con.httpClient->Connect(uri);
        }
        return con.httpClient;
    }
    else
    {
        // connection doesn't exist yet, create a new Http client and
        // add to connections
        Connection newCon;
        newCon.useCount = 1;
        newCon.httpClient = HttpClient::Create();
        newCon.httpClient->Connect(uri);
        this->connections.Add(host, newCon);
        return newCon.httpClient;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
HttpClientRegistry::ReleaseConnection(const URI& uri)
{
    StringAtom host = uri.Host();
    IndexT index = this->connections.FindIndex(host);
    if (InvalidIndex != index)
    {
        Connection& con = this->connections.ValueAtIndex(index);
        n_assert(con.useCount > 0);
        con.useCount--;
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
HttpClientRegistry::IsConnected(const URI& uri) const
{
    StringAtom host = uri.Host();
    if (this->connections.Contains(host))
    {
        return this->connections[host].httpClient->IsConnected();
    }
    else
    {
        return false;
    }
}

} // namespace Http
#endif //__WII__