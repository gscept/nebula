//------------------------------------------------------------------------------
//  shadervariable.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/shadervariable.h"

#if __DX11__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::ShaderVariable, 'SHDV', Direct3D11::D3D11ShaderVariable);
}
#elif __OGL4__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::ShaderVariable, 'SHDV', OpenGL4::OGL4ShaderVariable);
}
#elif __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::ShaderVariable, 'SHDV', Vulkan::VkShaderVariable);
}
#elif __DX9__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::ShaderVariable, 'SHDV', Direct3D9::D3D9ShaderVariable);
}
#else
#error "ShaderVariable class not implemented on this platform!"
#endif
