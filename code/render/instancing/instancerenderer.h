#pragma once
//------------------------------------------------------------------------------
/**
    @class Instancing::InstanceRenderer
  
    The instance renderer performs actual rendering and updating of shader variables for transforms.

    (C) 2012 Gustav Sterbrant
	(C) 2013-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"

#if __DX11__
#include "instancing/d3d11/d3d11instancerenderer.h"
namespace Instancing
{
class InstanceRenderer : public D3D11InstanceRenderer
{
__DeclareClass(InstanceRenderer);
__DeclareSingleton(InstanceRenderer);
public:
	/// constructor
	InstanceRenderer();
	/// destructor
	virtual ~InstanceRenderer();
};
} // namespace Instancing
#elif __OGL4__
#include "instancing/ogl4/ogl4instancerenderer.h"
namespace Instancing
{
class InstanceRenderer : public OGL4InstanceRenderer
{
	__DeclareClass(InstanceRenderer);
	__DeclareSingleton(InstanceRenderer);
public:
	/// constructor
	InstanceRenderer();
	/// destructor
	virtual ~InstanceRenderer();
};
} // namespace Instancing
#elif __VULKAN__
#include "instancing/vk/vkinstancerenderer.h"
namespace Instancing
{
class InstanceRenderer : public Vulkan::VkInstanceRenderer
{
	__DeclareClass(InstanceRenderer);
	__DeclareSingleton(InstanceRenderer);
public:
	/// constructor
	InstanceRenderer();
	/// destructor
	virtual ~InstanceRenderer();
};
} // namespace Instancing
#else
#error "InstanceRenderer not implemented on this platform!"
#endif

