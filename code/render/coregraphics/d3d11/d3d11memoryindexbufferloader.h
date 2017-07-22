#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D11::D3D11MemoryIndexBufferLoader
    
    Initialize a D3D11IndexBuffer from data in memory for the Win32/Xbox360
    platform. This resource loader only creates static IndexBuffers which are 
    initialized once and are not accessible by the CPU.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013 Individual contributors, see AUTHORS file
*/
//-----------------------------------------------------------------------------
#include "coregraphics/base/memoryindexbufferpoolbase.h"

namespace Direct3D11
{
class D3D11MemoryIndexBufferLoader : public Base::MemoryIndexBufferLoaderBase
{
    __DeclareClass(D3D11MemoryIndexBufferLoader);
public:
    /// called by resource when a load is requested
    virtual bool OnLoadRequested();
};

} // namespace Direct3D11
//------------------------------------------------------------------------------
