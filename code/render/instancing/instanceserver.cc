//------------------------------------------------------------------------------
//  instanceserver.cc
//  (C) 2012 Gustav Sterbrant
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "instancing/instanceserver.h"
namespace Instancing
{
#if __VULKAN__
__ImplementClass(Instancing::InstanceServer, 'INSR', Vulkan::VkInstanceServer);
#else
#error "InstanceServer class not implemented on this platform!"
#endif

__ImplementSingleton(Instancing::InstanceServer);

//------------------------------------------------------------------------------
/**
*/
InstanceServer::InstanceServer()
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
InstanceServer::~InstanceServer()
{
	__DestructSingleton;
}

} // namespace Instancing
