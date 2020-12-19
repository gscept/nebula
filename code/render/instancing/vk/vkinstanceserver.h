#pragma once
//------------------------------------------------------------------------------
/**
    Implements the entry point to the instance rendering subsystem, for Vulkan.
    
    (C) 2016-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "instancing/base/instanceserverbase.h"
namespace Vulkan
{
class VkInstanceServer : public Base::InstanceServerBase
{
    __DeclareSingleton(VkInstanceServer);
    __DeclareClass(VkInstanceServer);
public:
    /// constructor
    VkInstanceServer();
    /// destructor
    virtual ~VkInstanceServer();

    /// opens server
    bool Open();
    /// close server
    void Close();

    /// render
    void Render(IndexT frameIndex);
private:

    CoreGraphics::ShaderFeature::Mask instancingFeatureBits;
};
} // namespace Vulkan