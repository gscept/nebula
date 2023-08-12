//------------------------------------------------------------------------------
//  httpclient.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "http/httpclient.h"

namespace Http
{
#ifndef USE_CURL
__ImplementClass(Http::HttpClient, 'HTCL', Http::NebulaHttpClient);
#else
__ImplementClass(Http::HttpClient, 'HTCL', Http::CurlHttpClient);
#endif
}