#pragma once
#ifndef HTTP_HTTPMETHOD_H
#define HTTP_HTTPMETHOD_H
//------------------------------------------------------------------------------
/**
    @class Http::HttpMethod
    
    Http methods enumeration (GET, PUT, etc...).
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "util/string.h"

//------------------------------------------------------------------------------
namespace Http
{
class HttpMethod
{
public:
    /// http methods
    enum Code
    {
        Options,
        Get,
        Head,
        Post,
        Put,
        Delete,
        Trace,
        Connect,
        
        NumHttpMethods,
        InvalidHttpMethod,
    };
    
    /// convert from string
    static Code FromString(const Util::String& str);
    /// convert to string
    static Util::String ToString(Code c);
};

//------------------------------------------------------------------------------
/**
*/
inline HttpMethod::Code
HttpMethod::FromString(const Util::String& str)
{
    if (str == "OPTIONS") return Options;
    else if (str == "GET") return Get;
    else if (str == "HEAD") return Head;
    else if (str == "POST") return Post;
    else if (str == "PUT") return Put;
    else if (str == "DELETE") return Delete;
    else if (str == "TRACE") return Trace;
    else if (str == "CONNECT") return Connect;
    else
    {
        return InvalidHttpMethod;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline Util::String
HttpMethod::ToString(Code c)
{
    switch (c)
    {
        case Options:   return "OPTIONS";
        case Get:       return "GET";
        case Head:      return "HEAD";
        case Post:      return "POST";
        case Put:       return "PUT";
        case Delete:    return "DELETE";
        case Trace:     return "TRACE";
        case Connect:   return "CONNECT";
        default:
            return "InvalidHttpMethod";
    }
}

} // namespace Http
//------------------------------------------------------------------------------
#endif
    