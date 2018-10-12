//------------------------------------------------------------------------------
//  httprequesthandler.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "http/httprequesthandler.h"

namespace Http
{
__ImplementClass(Http::HttpRequestHandler, 'HRHD', Core::RefCounted);

using namespace IO;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
HttpRequestHandler::HttpRequestHandler()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
HttpRequestHandler::~HttpRequestHandler()
{
    this->pendingRequests.SetSignalOnEnqueueEnabled(false);
    // empty
}

//------------------------------------------------------------------------------
/**
    Put a http request into the request handlers message queue. This method
    is meant to be called from another thread.
*/
void
HttpRequestHandler::PutRequest(const Ptr<HttpRequest>& httpRequest)
{
    this->pendingRequests.Enqueue(httpRequest);
}

//------------------------------------------------------------------------------
/**
    Handle all pending http requests in the pending queue. This method
    must be called frequently from the thread which created this
    request handler.
*/
void
HttpRequestHandler::HandlePendingRequests()
{
    this->pendingRequests.DequeueAll(this->curWorkRequests);
    IndexT i;
    for (i = 0; i < this->curWorkRequests.Size(); i++)
    {
        this->HandleRequest(this->curWorkRequests[i]);
        this->curWorkRequests[i]->SetHandled(true);
    }
}

//------------------------------------------------------------------------------
/**
    Overwrite this method in your subclass. This method will be called by the
    HttpServer if AcceptsRequest() returned true. The request handler should
    properly process the request by filling the responseContentStream with
    data (for instance a HTML page), set the MediaType on the 
    responseContentStream (for instance "text/html") and return with a
    HttpStatus code (usually HttpStatus::OK).
*/
void
HttpRequestHandler::HandleRequest(const Ptr<HttpRequest>& request)
{
    request->SetStatus(HttpStatus::NotFound);
}

} // namespace Http
