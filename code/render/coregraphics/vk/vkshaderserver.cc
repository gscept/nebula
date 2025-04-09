//------------------------------------------------------------------------------
// vkshaderserver.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "vkshaderserver.h"
#include "effectfactory.h"
#include "vkgraphicsdevice.h"
#include "vktexture.h"

#include "graphics/bindlessregistry.h"

#include "render/system_shaders/shared.h"

using namespace Resources;
using namespace CoreGraphics;
namespace Vulkan
{

__ImplementClass(Vulkan::VkShaderServer, 'VKSS', Base::ShaderServerBase);
__ImplementInterfaceSingleton(Vulkan::VkShaderServer);
//------------------------------------------------------------------------------
/**
*/
VkShaderServer::VkShaderServer()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
VkShaderServer::~VkShaderServer()
{
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
bool
VkShaderServer::Open()
{
    n_assert(!this->IsOpen());

    // create anyfx factory
    this->factory = new(AnyFX::EffectFactory);
    ShaderServerBase::Open();

    this->pendingViews.SetSignalOnEnqueueEnabled(false);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderServer::Close()
{
    n_assert(this->IsOpen());
    delete this->factory;

    // We need to wait for the GPU to finish here since we are
    // actually unloading shaders and resource table pools after this point
    CoreGraphics::WaitAndClearPendingCommands();

    ShaderServerBase::Close();
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderServer::UpdateResources()
{
    // Just allocate the memory
    uint32_t bufferedFrameIndex = GetBufferedFrameIndex();

    VkDevice dev = GetCurrentDevice();

    // Setup new views for newly streamed LODs
    Util::Array<_PendingView, 32> pendingViewsThisFrame;
    this->pendingViews.DequeueAll(pendingViewsThisFrame);

    // Delete views which have been discarded due to LOD streaming
    for (int i = this->pendingViewDeletes.Size() - 1; i >= 0; i--)
    {
        // if we have cycled through all our frames, safetly delete the view
        if (this->pendingViewDeletes[i].replaceCounter == bufferedFrameIndex)
        {
            vkDestroyImageView(dev, this->pendingViewDeletes[i].view, nullptr);
            this->pendingViewDeletes.EraseIndex(i);
        }
    }

    for (int i = 0; i < pendingViewsThisFrame.Size(); i++)
    {
        const _PendingView& pend = pendingViewsThisFrame[i];

        TextureIdLock _0(pend.tex);
        VkTextureRuntimeInfo& info = textureAllocator.Get<Texture_RuntimeInfo>(pend.tex.id);
        VkImageView oldView = info.view;
        VkResult res = vkCreateImageView(GetCurrentDevice(), &pend.createInfo, nullptr, &info.view);
        n_assert(res == VK_SUCCESS);

        // update texture entries for all tables
        Graphics::ReregisterTexture(pend.tex, info.type, info.bind);

        _PendingViewDelete pendingDelete;
        pendingDelete.view = oldView;
        pendingDelete.replaceCounter = bufferedFrameIndex;
        pendingViewDeletes.Append(pendingDelete);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderServer::AddPendingImageView(CoreGraphics::TextureId tex, VkImageViewCreateInfo viewCreate, uint32_t bind)
{
    _PendingView pend;
    pend.tex = tex;
    pend.createInfo = viewCreate;
    pend.bind = bind;
    this->pendingViews.Enqueue(pend);
}

} // namespace Vulkan
