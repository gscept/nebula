//------------------------------------------------------------------------------
//  shadervariableinstance.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/shadervariableinstance.h"

#if (__DX11__ || __DX9__)
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::ShaderVariableInstance, 'SDVI', Base::ShaderVariableInstanceBase);
}
#elif  __OGL4__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::ShaderVariableInstance, 'SDVI', OpenGL4::OGL4ShaderVariableInstance);
}
#elif __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::ShaderVariableInstance, 'SDVI', Vulkan::VkShaderVariableInstance);
}
#else
#error "ShaderVariableInstance class not implemented on this platform!"
#endif
