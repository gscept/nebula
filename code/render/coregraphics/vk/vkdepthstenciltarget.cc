//------------------------------------------------------------------------------
// vkdepthstenciltarget.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkdepthstenciltarget.h"
#include "coregraphics/displaydevice.h"
#include "vkrenderdevice.h"
#include "vkutilities.h"
#include "vkscheduler.h"

using namespace CoreGraphics;
namespace Vulkan
{

__ImplementClass(Vulkan::VkDepthStencilTarget, 'VKDS', Base::DepthStencilTargetBase);
//------------------------------------------------------------------------------
/**
*/
VkDepthStencilTarget::VkDepthStencilTarget()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
VkDepthStencilTarget::~VkDepthStencilTarget()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
VkDepthStencilTarget::Setup()
{
    // setup base class
    DepthStencilTargetBase::Setup();

    // if we have a relative size on the dept-stencil target, calculate actual size
    if (this->useRelativeSize)
    {
        Ptr<CoreGraphics::Window> wnd = DisplayDevice::Instance()->GetCurrentWindow();
        this->SetWidth(SizeT(wnd->GetDisplayMode().GetWidth() * this->relWidth));
        this->SetHeight(SizeT(wnd->GetDisplayMode().GetHeight() * this->relHeight));
    }

    VkExtent3D extents;
    extents.width = this->width;
    extents.height = this->height;
    extents.depth = 1;

    this->viewport.x = 0;
    this->viewport.y = 0;
    this->viewport.width = (float)this->width;
    this->viewport.height = (float)this->height;
    this->viewport.minDepth = 0;
    this->viewport.maxDepth = 1;

    this->scissor.offset.x = 0;
    this->scissor.offset.y = 0;
    this->scissor.extent.width = this->width;
    this->scissor.extent.height = this->height;

    VkImageCreateInfo imgInfo =
    {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        NULL,
        0,
        VK_IMAGE_TYPE_2D,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        extents,
        1,
        1,
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        NULL,
        VK_IMAGE_LAYOUT_UNDEFINED
    };

    // create image
    VkResult res = vkCreateImage(VkRenderDevice::dev, &imgInfo, NULL, &this->image);
    n_assert(res == VK_SUCCESS);

    // allocate buffer backing and bind to image
    uint32_t size;
    VkUtilities::AllocateImageMemory(this->image, this->mem, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, size);
    vkBindImageMemory(VkRenderDevice::dev, this->image, this->mem, 0);

    VkImageSubresourceRange subres;
    subres.baseArrayLayer = 0;
    subres.baseMipLevel = 0;
    subres.layerCount = 1;
    subres.levelCount = 1;
    subres.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    VkComponentMapping mapping;
    mapping.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    mapping.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    mapping.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    mapping.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    VkImageViewCreateInfo viewInfo =
    {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        NULL,
        0,
        this->image,
        VK_IMAGE_VIEW_TYPE_2D,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        mapping,
        subres
    };

    res = vkCreateImageView(VkRenderDevice::dev, &viewInfo, NULL, &this->view);
    n_assert(res == VK_SUCCESS);

    // change image layout
    VkClearDepthStencilValue clear = {1, 0};
    VkScheduler* scheduler = VkScheduler::Instance();
    scheduler->PushImageLayoutTransition(VkDeferredCommand::Graphics, VkUtilities::ImageMemoryBarrier(this->image, subres, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL));
    scheduler->PushImageDepthStencilClear(this->image, VkDeferredCommand::Graphics, VK_IMAGE_LAYOUT_GENERAL, clear, subres);
    scheduler->PushImageLayoutTransition(VkDeferredCommand::Graphics, VkUtilities::ImageMemoryBarrier(this->image, subres, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL));
}

//------------------------------------------------------------------------------
/**
*/
void
VkDepthStencilTarget::Discard()
{
    DepthStencilTargetBase::Discard();
    vkDestroyImageView(VkRenderDevice::dev, this->view, NULL);
    vkDestroyImage(VkRenderDevice::dev, this->image, NULL);
    vkFreeMemory(VkRenderDevice::dev, this->mem, NULL);
}

//------------------------------------------------------------------------------
/**
*/
void
VkDepthStencilTarget::OnDisplayResized(SizeT width, SizeT height)
{

}

//------------------------------------------------------------------------------
/**
*/
void
VkDepthStencilTarget::BeginPass()
{
    DepthStencilTargetBase::BeginPass();

    // transfer image
    VkImageSubresourceRange subres;
    subres.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    subres.baseArrayLayer = 0;
    subres.baseMipLevel = 0;
    subres.layerCount = 1;
    subres.levelCount = 1;
    VkUtilities::ImageLayoutTransition(VkDeferredCommand::Graphics, VkUtilities::ImageMemoryBarrier(this->image, subres, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL));
}

//------------------------------------------------------------------------------
/**
*/
void
VkDepthStencilTarget::EndPass()
{
    // transfer image
    VkImageSubresourceRange subres;
    subres.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    subres.baseArrayLayer = 0;
    subres.baseMipLevel = 0;
    subres.layerCount = 1;
    subres.levelCount = 1;
    VkUtilities::ImageLayoutTransition(VkDeferredCommand::Graphics, VkUtilities::ImageMemoryBarrier(this->image, subres, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL));
    
    DepthStencilTargetBase::EndPass();
}

} // namespace Vulkan