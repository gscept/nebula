#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D11::D3D11MemoryVertexBufferLoader
    
    Initialize a D3D11VertexBuffer from data in memory on the Win32/Xbox360
    platform. This resource loader only creates static VertexBuffers which are 
    initialized once and are not accessible by the CPU.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/base/memoryvertexbufferpoolbase.h"

namespace Direct3D11
{
class D3D11MemoryVertexBufferLoader : public Base::MemoryVertexBufferLoaderBase
{
    __DeclareClass(D3D11MemoryVertexBufferLoader);
public:
    /// called by resource when a load is requested
    virtual bool OnLoadRequested();


};

} // namespace Direct3D11
//------------------------------------------------------------------------------
