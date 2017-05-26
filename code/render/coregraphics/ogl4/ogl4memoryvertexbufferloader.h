#pragma once
//------------------------------------------------------------------------------
/**
    @class OpenGL4::OGL4MemoryVertexBufferLoader
    
    Initialize a OGL4VertexBuffer from data in memory on the Win32/Xbox360
    platform. This resource loader only creates static VertexBuffers which are 
    initialized once and are not accessible by the CPU.
    
    (C) 2007 Radon Labs GmbH
	(C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "coregraphics/base/memoryvertexbufferloaderbase.h"

//------------------------------------------------------------------------------
namespace OpenGL4
{
class OGL4MemoryVertexBufferLoader : public Base::MemoryVertexBufferLoaderBase
{
    __DeclareClass(OGL4MemoryVertexBufferLoader);
public:
    /// called by resource when a load is requested
    virtual bool OnLoadRequested();


};

} // namespace OpenGL4
//------------------------------------------------------------------------------
