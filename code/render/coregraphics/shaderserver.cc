//------------------------------------------------------------------------------
//  shaderserver.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/shaderserver.h"

namespace CoreGraphics
{
#if __DX11__
__ImplementClass(CoreGraphics::ShaderServer, 'SHSV', Direct3D11::D3D11ShaderServer);
__ImplementSingleton(CoreGraphics::ShaderServer);
#elif __OGL4__
__ImplementClass(CoreGraphics::ShaderServer, 'SHSV', OpenGL4::OGL4ShaderServer);
__ImplementSingleton(CoreGraphics::ShaderServer);
#elif __VULKAN__
__ImplementClass(CoreGraphics::ShaderServer, 'SHSV', Vulkan::VkShaderServer);
__ImplementSingleton(CoreGraphics::ShaderServer);
#elif __DX9__
__ImplementClass(CoreGraphics::ShaderServer, 'SHSV', Direct3D9::D3D9ShaderServer);
__ImplementSingleton(CoreGraphics::ShaderServer);
#else
#error "ShaderServer class not implemented on this platform!"
#endif

//------------------------------------------------------------------------------
/**
*/
ShaderServer::ShaderServer()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
ShaderServer::~ShaderServer()
{
    __DestructSingleton;
}

} // namespace CoreGraphics
