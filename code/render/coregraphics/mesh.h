#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::Mesh
  
    A mesh contains a vertex buffer, an optional index buffer
    and a number of PrimitiveGroup objects. Meshes can be loaded directly 
    from a mesh resource file.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/    
#if (__DX11__ || __DX9__ || __OGL4__ || __VULKAN__)
#include "coregraphics/base/meshbase.h"
namespace CoreGraphics
{
class Mesh : public Base::MeshBase
{
    __DeclareClass(Mesh);
};
}
#else
#error "Mesh not implemented on this platform!"
#endif
//------------------------------------------------------------------------------

