//------------------------------------------------------------------------------
//  shadowserver.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "lighting/shadowserver.h"

namespace Lighting
{
#if __VULKAN__
__ImplementClass(Lighting::ShadowServer, 'SDSV', Lighting::VkShadowServer);
#elif __OGL4__
__ImplementClass(Lighting::ShadowServer, 'SDSV', Lighting::OGL4ShadowServer);
#elif __DX11__
__ImplementClass(Lighting::ShadowServer, 'SDSV', Lighting::D3D11ShadowServer);
#elif __DX9__
__ImplementClass(Lighting::ShadowServer, 'SDSV', Lighting::D3D9ShadowServer);
#elif __WII__
__ImplementClass(Lighting::ShadowServer, 'SDSV', Wii::WiiShadowServer);
#elif __PS3__
__ImplementClass(Lighting::ShadowServer, 'SDSV', Lighting::ShadowServerBase);
#else
#error "ShadowServer class not implemented on this platform!"
#endif
__ImplementSingleton(Lighting::ShadowServer);

//------------------------------------------------------------------------------
/**
*/
ShadowServer::ShadowServer()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
ShadowServer::~ShadowServer()
{
    __DestructSingleton;
}

} // namespace Lighting
