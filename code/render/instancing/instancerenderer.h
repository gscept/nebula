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

#if __VULKAN__
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

