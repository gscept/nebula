//------------------------------------------------------------------------------
//  httprequest.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "http/httprequest.h"

namespace Http
{
__ImplementClass(Http::HttpRequest, 'HTRQ', Messaging::Message);
__ImplementMsgId(HttpRequest);

//------------------------------------------------------------------------------
/**
*/
HttpRequest::HttpRequest() :
    method(HttpMethod::InvalidHttpMethod),
    status(HttpStatus::InvalidHttpStatus)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
HttpRequest::~HttpRequest()
{
    // empty
}

} // namespace Http
