#pragma once
#ifndef HTTP_HTTPSTATUS_H
#define HTTP_HTTPSTATUS_H
//------------------------------------------------------------------------------
/**
    @class Http::HttpStatus
  
    HTTP status code enumeration (e.g. 404 Not Found).
    
    @copyright
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "util/string.h"

//------------------------------------------------------------------------------
namespace Http
{
class HttpStatus
{
public:
    /// status codes
    enum Code
    {
        Continue                        = 100,
        OK                              = 200,
        BadRequest                      = 400,
        Forbidden                       = 403,
        NotFound                        = 404,
        MethodNotAllowed                = 405,
        NotAcceptable                   = 406,
        InternalServerError             = 500,
        NotImplemented                  = 501,
        ServiceUnavailable              = 503,

        InvalidHttpStatus = 0xffff
    };

    /// convert from string
    static Code FromString(const Util::String& str);
    /// convert to string
    static Util::String ToString(Code c);
    /// convert from long
    static Code FromLong(long l);
    /// convert code to human readable string
    static Util::String ToHumanReadableString(Code c);
};

} // namespace Http
//------------------------------------------------------------------------------
#endif
