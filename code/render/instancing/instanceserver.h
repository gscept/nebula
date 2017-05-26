#pragma once
//------------------------------------------------------------------------------
/**
    @class Instancing::InstanceServer
  
    The instance server collects all instances of a model and render each node instanced,
	so as to decrease draw calls when rendering huge amounts of identical objects

    (C) 2012 Gustav Sterbrant
*/
//------------------------------------------------------------------------------

#if __DX11__
#include "instancing/d3d11/d3d11instanceserver.h"
namespace Instancing
{
class InstanceServer : public D3D11InstanceServer
{
__DeclareClass(InstanceServer);
__DeclareSingleton(InstanceServer);
public:
	/// constructor
	InstanceServer();
	/// destructor
	virtual ~InstanceServer();
};
} // namespace Instancing
#elif __OGL4__
#include "instancing/ogl4/ogl4instanceserver.h"
namespace Instancing
{
class InstanceServer : public OGL4InstanceServer
{
	__DeclareClass(InstanceServer);
	__DeclareSingleton(InstanceServer);
public:
	/// constructor
	InstanceServer();
	/// destructor
	virtual ~InstanceServer();
};
} // namespace Instancing
#elif __VULKAN__
#include "instancing/vk/vkinstanceserver.h"
namespace Instancing
{
class InstanceServer : public Vulkan::VkInstanceServer
{
	__DeclareClass(InstanceServer);
	__DeclareSingleton(InstanceServer);
public:
	/// constructor
	InstanceServer();
	/// destructor
	virtual ~InstanceServer();
};
} // namespace Instancing
#else
#error "InstanceServer not implemented on this platform!"
#endif

