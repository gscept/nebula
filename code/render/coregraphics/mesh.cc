//------------------------------------------------------------------------------
//  mesh.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/mesh.h"
#if (__DX11__ || __DX9__ || __OGL4__ || __VULKAN__)
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::Mesh, 'MESH', Base::MeshBase);
}
#else
#error "Mesh class not implemented on this platform!"
#endif
