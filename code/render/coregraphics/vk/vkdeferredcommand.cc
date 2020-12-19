//------------------------------------------------------------------------------
// vkdeferreddelegate.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkdeferredcommand.h"
#include "vkgraphicsdevice.h"
#include "vkutilities.h"

namespace Vulkan
{

//------------------------------------------------------------------------------
/**
*/
VkDeferredCommand::VkDeferredCommand() : 
    dev(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
VkDeferredCommand::~VkDeferredCommand()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
VkDeferredCommand::RunDelegate()
{
    n_assert(this->dev != 0);

    // this means the fence is signaled
    switch (this->del.type)
    {
    case FreeCmdBuffers:
        vkFreeCommandBuffers(this->dev, this->del.cmdbufferfree.pool, this->del.cmdbufferfree.numBuffers, this->del.cmdbufferfree.buffers);
        break;
    case FreeMemory:
        Memory::Free(Memory::ScratchHeap, this->del.memory.data);
        break;
    case FreeBuffer:
        vkFreeMemory(this->dev, this->del.buffer.mem, nullptr);
        vkDestroyBuffer(this->dev, this->del.buffer.buf, nullptr);
        break;
    case FreeImage:
        vkFreeMemory(this->dev, this->del.image.mem, nullptr);
        vkDestroyImage(this->dev, this->del.image.img, nullptr);
        break;
    case BindDescriptorSets:
        if (this->del.descSetBind.type == VK_PIPELINE_BIND_POINT_GRAPHICS)
        {
            Vulkan::BindDescriptorsGraphics(this->del.descSetBind.sets, this->del.descSetBind.baseSet, this->del.descSetBind.numSets, this->del.descSetBind.offsets, this->del.descSetBind.numOffsets);
        }
        else
        {
            Vulkan::BindDescriptorsCompute(this->del.descSetBind.sets, this->del.descSetBind.baseSet, this->del.descSetBind.numSets, this->del.descSetBind.offsets, this->del.descSetBind.numOffsets);
        }
        break;
    
    case UpdateBuffer:
        VkUtilities::BufferUpdate(this->del.bufferUpd.buf, this->del.bufferUpd.offset, this->del.bufferUpd.size, this->del.bufferUpd.data);
        break;
    case UpdateImage:
        VkUtilities::ImageUpdate(this->del.imageUpd.img, this->del.imageUpd.info, this->del.imageUpd.mip, this->del.imageUpd.face, this->del.imageUpd.size, this->del.imageUpd.data);
        break;
    case DestroyPipeline:
        vkDestroyPipeline(this->dev, this->del.pipelineDestroy.pipeline, nullptr);
        break;
    case ClearColorImage:
        VkUtilities::ImageColorClear(this->del.imgColorClear.img, this->del.queue, this->del.imgColorClear.layout, this->del.imgColorClear.clearValue, this->del.imgColorClear.region);
        break;
    case ClearDepthStencilImage:
        VkUtilities::ImageDepthStencilClear(this->del.imgDepthStencilClear.img, this->del.queue, this->del.imgDepthStencilClear.layout, this->del.imgDepthStencilClear.clearValue, this->del.imgDepthStencilClear.region);
        break;
    case ImageLayoutTransition:
        VkUtilities::ImageLayoutTransition(this->del.queue, this->del.imgBarrier.left, this->del.imgBarrier.right, this->del.imgBarrier.barrier);
        break;
    case ImageOwnershipChange:
        VkUtilities::ImageOwnershipChange(this->del.queue, this->del.imgOwnerChange.left, this->del.imgOwnerChange.right, this->del.imgOwnerChange.barrier);
        break;
    }
}


} // namespace Vulkan