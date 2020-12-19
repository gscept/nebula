#pragma once
//------------------------------------------------------------------------------
/**
    @class Instancing::InstanceServer
  
    The instance server collects all instances of a model and render each node instanced,
    so as to decrease draw calls when rendering huge amounts of identical objects

    (C) 2012 Gustav Sterbrant
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

#if __VULKAN__
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

