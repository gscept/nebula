#pragma once
#ifndef DEBUG_MESHPAGEHANDLER_H
#define DEBUG_MESHPAGEHANDLER_H
//------------------------------------------------------------------------------
/**
    @class Debug::MeshPageHandler
    
    Provide an HTML debug page with information about shared mesh resources.

    Usage:    
    http://host/mesh                    - provide a list of all meshes with their properties
    http://host/mesh?meshinfo=[resId]   - display information about specific mesh    
    
    @copyright
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "http/httprequesthandler.h"
#include "resources/resourceid.h"

//------------------------------------------------------------------------------
namespace Debug
{
class MeshPageHandler : public Http::HttpRequestHandler
{
    __DeclareClass(MeshPageHandler);
public:
    /// constructor
    MeshPageHandler();
    /// handle a http request, the handler is expected to fill the content stream with response data
    virtual void HandleRequest(const Ptr<Http::HttpRequest>& request);

private:
    /// display information about specific mesh
    Http::HttpStatus::Code HandleMeshInfoRequest(const Util::String& resId, const Ptr<IO::Stream>& responseContentStream);
    /// dump mesh vertices
    Http::HttpStatus::Code HandleVertexDumpRequest(const Util::String& resId, IndexT minIndex, IndexT maxIndex, const Ptr<IO::Stream>& responseContentStream);
};

} // namespace Debug
//------------------------------------------------------------------------------
#endif

    