//------------------------------------------------------------------------------
//  httpstatus.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "http/httpstatus.h"

namespace Http
{
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
HttpStatus::Code
HttpStatus::FromString(const Util::String& str)
{
    if (str == "100")      return Continue;
    else if (str == "200") return OK;
    else if (str == "400") return BadRequest;
    else if (str == "403") return Forbidden;
    else if (str == "404") return NotFound;
    else if (str == "405") return MethodNotAllowed;
    else if (str == "406") return NotAcceptable;
    else if (str == "500") return InternalServerError;
    else if (str == "503") return ServiceUnavailable;
    else
    {
        return InvalidHttpStatus;
    }
}

//------------------------------------------------------------------------------
/**
*/
String
HttpStatus::ToString(Code c)
{
    switch (c)
    {
        case Continue:                      return "100";
        case OK:                            return "200";
        case BadRequest:                    return "400";
        case Forbidden:                     return "403";
        case NotFound:                      return "404";
        case InternalServerError:           return "500";
        case NotImplemented:                return "501";
        case ServiceUnavailable:            return "503";
        default:
            n_error("Invalid HTTP status code!");
            return "";
    }
}

//------------------------------------------------------------------------------
/**
*/
String
HttpStatus::ToHumanReadableString(Code c)
{
    switch (c)
    {
        case Continue:                      return "Continue";
        case OK:                            return "OK";
        case BadRequest:                    return "Bad Request";
        case Forbidden:                     return "Forbidden";
        case NotFound:                      return "Not Found";
        case InternalServerError:           return "Internal Server Error";
        case NotImplemented:                return "Not Implemented";
        case ServiceUnavailable:            return "Service Unavailable";
        default:
            return "Invalid Status Code";
    }
}

} // namespace Http
