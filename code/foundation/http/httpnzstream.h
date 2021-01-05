#pragma once
//------------------------------------------------------------------------------
/**
    @class Http::HttpNzStream
    
    A version of HttpStream which reads .nz compressed files as created by
    the Nebula archiver tool. .nz is a simple container for ZLIB 
    compressed data.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#include "io/memorystream.h"
#include "http/httpclient.h"

//------------------------------------------------------------------------------
namespace Http
{
class HttpNzStream : public IO::MemoryStream
{
    __DeclareClass(HttpNzStream);
public:
    /// constructor
    HttpNzStream();
    /// destructor
    virtual ~HttpNzStream();
    
    /// open the stream
    virtual bool Open();
};

} // namespace Http
//------------------------------------------------------------------------------
