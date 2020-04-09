//------------------------------------------------------------------------------
//  shaderserver.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/shaderserver.h"

namespace CoreGraphics
{
#if __VULKAN__
__ImplementClass(CoreGraphics::ShaderServer, 'SHSV', Vulkan::VkShaderServer);
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
