#pragma once
//------------------------------------------------------------------------------
/**
    @class Http::HttpClient
    
    (C) 2020 Individual contributors, see AUTHORS file
*/
#ifndef USE_CURL
#include "simple/nebulahttpclient.h"
namespace Http
{
class HttpClient: public Http::NebulaHttpClient
{ 
    __DeclareClass(HttpClient);
};
}
#else
#include "curl/curlhttpclient.h"
namespace Http
{
class HttpClient: public Http::CurlHttpClient
{ 
    __DeclareClass(HttpClient);
};
}

#endif
//------------------------------------------------------------------------------


