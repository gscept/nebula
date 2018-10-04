#pragma once
//------------------------------------------------------------------------------
/**
    @class OpenGL4::OGL4MemoryIndexBufferLoader
    
    Initialize a OGL4IndexBuffer from data in memory for the Win32/Xbox360
    platform. This resource loader only creates static IndexBuffers which are 
    initialized once and are not accessible by the CPU.
    
    (C) 2007 Radon Labs GmbH
	(C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "coregraphics/base/memoryindexbufferpoolbase.h"

//------------------------------------------------------------------------------
namespace OpenGL4
{
class OGL4MemoryIndexBufferLoader : public Base::MemoryIndexBufferLoaderBase
{
    __DeclareClass(OGL4MemoryIndexBufferLoader);
public:
    /// called by resource when a load is requested
    virtual bool OnLoadRequested();
};

} // namespace OpenGL4
//------------------------------------------------------------------------------
