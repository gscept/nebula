#pragma once
//------------------------------------------------------------------------------
/**
    Implements the shader server used by Vulkan.
    
    @copyright
    (C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "util/fixedpool.h"
#include "coregraphics/base/shaderserverbase.h"
#include "coregraphics/config.h"
#include "coregraphics/texture.h"
#include "effectfactory.h"
#include "render/system_shaders/shared.h"

namespace Graphics
{
class FrameScript;
}

namespace Vulkan
{

class VkShaderServer : public Base::ShaderServerBase
{
    __DeclareClass(VkShaderServer);
    __DeclareInterfaceSingleton(VkShaderServer);
public:
    /// constructor
    VkShaderServer();
    /// destructor
    virtual ~VkShaderServer();

    /// open the shader server
    bool Open();
    /// close the shader server
    void Close();
   

    /// add a pending image view update to the update queue, thread safe
    void AddPendingImageView(CoreGraphics::TextureId tex, VkImageViewCreateInfo viewCreate, uint32_t bind);
	
	/// begin frame
    void UpdateResources();

private:

    Threading::CriticalSection bindResourceCriticalSection;
    struct _PendingView
    {
        CoreGraphics::TextureId tex;
        VkImageViewCreateInfo createInfo;
        uint32_t bind;
    };

    struct _PendingViewDelete
    {
        VkImageView view;
        uint32_t replaceCounter;
    };

    Threading::SafeQueue<_PendingView> pendingViews;
    Util::Array<_PendingViewDelete> pendingViewDeletes;

    AnyFX::EffectFactory* factory;
};

} // namespace Vulkan
