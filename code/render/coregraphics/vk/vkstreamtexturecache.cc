//------------------------------------------------------------------------------
// vkstreamtexturecache.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkstreamtexturecache.h"
#include "coregraphics/memorytexturecache.h"
#include "coregraphics/texture.h"
#include "io/ioserver.h"
#include "coregraphics/vk/vktypes.h"
#include "IL/il.h"
#include "coregraphics/load/glimltypes.h"

#include "vkloader.h"
#include "vkgraphicsdevice.h"
#include "math/scalar.h"
#include "vkshaderserver.h"
#include "profiling/profiling.h"
#include "threading/interlocked.h"

#include "graphics/bindlessregistry.h"
namespace Vulkan
{

__ImplementClass(Vulkan::VkStreamTextureCache, 'VKTL', Resources::ResourceStreamCache);

using namespace CoreGraphics;
using namespace Resources;
using namespace IO;
//------------------------------------------------------------------------------
/**
*/
VkStreamTextureCache::VkStreamTextureCache()
{
    this->async = true;
    this->placeholderResourceName = "tex:system/white.dds";
    this->failResourceName = "tex:system/error.dds";

    this->streamerThreadName = "Texture Pool Streamer Thread";
}

//------------------------------------------------------------------------------
/**
*/
VkStreamTextureCache::~VkStreamTextureCache()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline Resources::ResourceUnknownId
VkStreamTextureCache::AllocObject()
{
    return textureCache->AllocObject();
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkStreamTextureCache::DeallocObject(const Resources::ResourceUnknownId id)
{
    textureCache->DeallocObject(id);
}

//------------------------------------------------------------------------------
/**
*/
ResourceCache::LoadStatus
VkStreamTextureCache::LoadFromStream(const Resources::ResourceId res, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate)
{
    N_SCOPE_ACCUM(CreateAndLoad, TextureStream);
    n_assert(stream.isvalid());
    n_assert(stream->CanBeMapped());
    n_assert(this->GetState(res) == Resources::Resource::Pending);

    // Map memory, we will keep the memory mapping so we can stream in LODs later
    void* srcData = stream->MemoryMap();
    uint srcDataSize = stream->GetSize();

    const int NumBasicLods = immediate ? 1000 : 5;

    // load using gliml
    gliml::context ctx;
    if (ctx.load_dds(srcData, srcDataSize))
    {
        // during the load-phase, we can safetly get the structs
        __LockName(textureCache->Allocator(), lock, Util::ArrayAllocatorAccess::Write);
        VkTextureRuntimeInfo& runtimeInfo = textureCache->Get<Texture_RuntimeInfo>(res.resourceId);
        VkTextureLoadInfo& loadInfo = textureCache->Get<Texture_LoadInfo>(res.resourceId);
        VkTextureStreamInfo& streamInfo = textureCache->Get<Texture_StreamInfo>(res.resourceId);

        streamInfo.mappedBuffer = srcData;
        streamInfo.bufferSize = srcDataSize;
        streamInfo.stream = stream;
        streamInfo.ctx = ctx;

        loadInfo.dev = Vulkan::GetCurrentDevice();

        VkPhysicalDevice physicalDev = Vulkan::GetCurrentPhysicalDevice();
        VkDevice dev = Vulkan::GetCurrentDevice();

        int numMips = ctx.num_mipmaps(0);
        int mips = Math::max(0, numMips - NumBasicLods);
        int depth = ctx.image_depth(0, 0);
        int width = ctx.image_width(0, 0);
        int height = ctx.image_height(0, 0);
        bool isCube = ctx.num_faces() > 1;

        streamInfo.lowestLod = Math::max(0, mips);

        CoreGraphics::PixelFormat::Code nebulaFormat = CoreGraphics::Gliml::ToPixelFormat(ctx);
        VkFormat vkformat = VkTypes::AsVkFormat(nebulaFormat);
        VkTypes::VkBlockDimensions block = VkTypes::AsVkBlockSize(vkformat);

        runtimeInfo.type = isCube ? CoreGraphics::TextureCube : ctx.is_3d() ? CoreGraphics::Texture3D : CoreGraphics::Texture2D;

        // use linear if we really have to
        VkFormatProperties formatProps;
        vkGetPhysicalDeviceFormatProperties(physicalDev, vkformat, &formatProps);
        bool forceLinear = false;
        if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT))
        {
            forceLinear = true;
        }

        // create image
        VkExtent3D extents;
        extents.width = width;
        extents.height = height;
        extents.depth = depth;

        VkImageCreateInfo info =
        {
            VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            NULL,
            isCube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : VkImageCreateFlags(0),
            VkTypes::AsVkImageType(runtimeInfo.type),
            vkformat,
            extents,
            (uint32_t)numMips,
            (uint32_t)ctx.num_faces(),
            VK_SAMPLE_COUNT_1_BIT,
            forceLinear ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_SHARING_MODE_EXCLUSIVE,
            0,
            nullptr,
            VK_IMAGE_LAYOUT_UNDEFINED
        };
        VkResult stat = vkCreateImage(dev, &info, NULL, &loadInfo.img);
        n_assert(stat == VK_SUCCESS);

        CoreGraphics::Alloc alloc = AllocateMemory(loadInfo.dev, loadInfo.img, CoreGraphics::MemoryPool_DeviceLocal);
        stat = vkBindImageMemory(loadInfo.dev, loadInfo.img, alloc.mem, alloc.offset);
        n_assert(stat == VK_SUCCESS);
        loadInfo.mem = alloc;

        // create image view
        VkImageSubresourceRange subres;
        subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subres.baseArrayLayer = 0;
        subres.baseMipLevel = Math::max(mips, 0);
        subres.layerCount = info.arrayLayers;
        subres.levelCount = Math::min(numMips, NumBasicLods);

        VkComponentMapping mapping;
        mapping.r = VkSwizzle[(uint)loadInfo.swizzle.red];
        mapping.g = VkSwizzle[(uint)loadInfo.swizzle.green];
        mapping.b = VkSwizzle[(uint)loadInfo.swizzle.blue];
        mapping.a = VkSwizzle[(uint)loadInfo.swizzle.alpha];

        VkImageViewCreateInfo viewCreate =
        {
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            nullptr,
            0,
            loadInfo.img,
            VkTypes::AsVkImageViewType(runtimeInfo.type),
            vkformat,
            mapping,
            subres
        };
        stat = vkCreateImageView(dev, &viewCreate, NULL, &runtimeInfo.view);
        n_assert(stat == VK_SUCCESS);

        // use resource submission
        CoreGraphics::CmdBufferId cmdBuf = CoreGraphics::LockTransferSetupCommandBuffer();

        // Transition to transfer
        CoreGraphics::CmdBarrier(cmdBuf,
            CoreGraphics::PipelineStage::ImageInitial,
            CoreGraphics::PipelineStage::TransferWrite,
            CoreGraphics::BarrierDomain::Global,
            {
                TextureBarrierInfo
                {
                    res,
                    CoreGraphics::ImageSubresourceInfo(CoreGraphics::ImageAspect::ColorBits, subres.baseMipLevel, subres.levelCount, subres.baseArrayLayer, subres.layerCount)
                }
            },
            nullptr);

        // now load texture by walking through all images and mips
        for (int i = 0; i < ctx.num_faces(); i++)
        {
            for (int j = subres.baseMipLevel; j < ctx.num_mipmaps(i); j++)
            {
                // Perform a texture update
                CoreGraphics::TextureUpdate(
                    cmdBuf
                    , QueueType::TransferQueueType
                    , res
                    , ctx.image_width(i, j)
                    , ctx.image_height(i, j)
                    , j
                    , subres.baseArrayLayer + i
                    , ctx.image_size(i, j)
                    , (byte*)ctx.image_data(i, j));
            }
        }

        // Transition back to read, and exchange queue ownership with the graphics queue
        CoreGraphics::CmdBeginMarker(cmdBuf, NEBULA_MARKER_TRANSFER, stream->GetURI().LocalPath().AsCharPtr());
        CoreGraphics::CmdBarrier(cmdBuf,
            CoreGraphics::PipelineStage::TransferWrite,
            CoreGraphics::PipelineStage::TransferWrite,
            CoreGraphics::BarrierDomain::Global,
            {
                TextureBarrierInfo
                {
                    res,
                    CoreGraphics::ImageSubresourceInfo(CoreGraphics::ImageAspect::ColorBits, subres.baseMipLevel, subres.levelCount, subres.baseArrayLayer, subres.layerCount)
                }
            },
            nullptr,
            Vulkan::GetQueueFamily(QueueType::TransferQueueType),
            Vulkan::GetQueueFamily(QueueType::GraphicsQueueType));
        CoreGraphics::CmdEndMarker(cmdBuf);
        CoreGraphics::UnlockTransferSetupCommandBuffer();

        // Do the same barrier on the handover buffer, and exchange queue ownership with the graphics queue
        CoreGraphics::CmdBufferId handoverCmdBuf = CoreGraphics::LockGraphicsSetupCommandBuffer();

        CoreGraphics::CmdBeginMarker(handoverCmdBuf, NEBULA_MARKER_TRANSFER, stream->GetURI().LocalPath().AsCharPtr());

        // First duplicate the transfer queue barrier
        CoreGraphics::CmdBarrier(handoverCmdBuf,
            CoreGraphics::PipelineStage::TransferWrite,
            CoreGraphics::PipelineStage::TransferWrite,
            CoreGraphics::BarrierDomain::Global,
            {
                TextureBarrierInfo
                {
                    res,
                    CoreGraphics::ImageSubresourceInfo(CoreGraphics::ImageAspect::ColorBits, subres.baseMipLevel, subres.levelCount, subres.baseArrayLayer, subres.layerCount)
                }
            },
            nullptr,
            Vulkan::GetQueueFamily(QueueType::TransferQueueType),
            Vulkan::GetQueueFamily(QueueType::GraphicsQueueType));

        // Then perform the actual image layout change
        CoreGraphics::CmdBarrier(handoverCmdBuf,
            CoreGraphics::PipelineStage::TransferWrite,
            CoreGraphics::PipelineStage::AllShadersRead,
            CoreGraphics::BarrierDomain::Global,
            {
                TextureBarrierInfo
                {
                    res,
                    CoreGraphics::ImageSubresourceInfo(CoreGraphics::ImageAspect::ColorBits, subres.baseMipLevel, subres.levelCount, subres.baseArrayLayer, subres.layerCount)
                }
            });
        CoreGraphics::CmdEndMarker(handoverCmdBuf);
        CoreGraphics::UnlockGraphicsSetupCommandBuffer();

        loadInfo.dims.width = width;
        loadInfo.dims.height = height;
        loadInfo.dims.depth = depth;
        loadInfo.layers = info.arrayLayers;
        loadInfo.mips = Math::max(numMips, 1);
        loadInfo.format = nebulaFormat;// VkTypes::AsNebulaPixelFormat(vkformat);
        loadInfo.dev = dev;
        loadInfo.swapExtension = Ids::InvalidId32;
        loadInfo.stencilExtension = Ids::InvalidId32;
        loadInfo.sparseExtension = Ids::InvalidId32;
        runtimeInfo.bind = Graphics::RegisterTexture(TextureId(res), runtimeInfo.type);

#if NEBULA_GRAPHICS_DEBUG
        ObjectSetName((TextureId)res, stream->GetURI().LocalPath().AsCharPtr());
#endif
        return ResourceCache::Success;
    }
    stream->MemoryUnmap();
    return ResourceCache::Failed;
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkStreamTextureCache::Unload(const Resources::ResourceId id)
{
    __LockName(textureCache->Allocator(), lock, Util::ArrayAllocatorAccess::Write);
    VkTextureStreamInfo& streamInfo = textureCache->Get<Texture_StreamInfo>(id.resourceId);

    streamInfo.stream->MemoryUnmap();
    textureCache->Unload(id);
}

//------------------------------------------------------------------------------
/**
*/
void 
VkStreamTextureCache::StreamMaxLOD(const Resources::ResourceId& id, const float lod, bool immediate)
{
    N_SCOPE_ACCUM(StreamMaxLOD, TextureStream);

    __LockName(textureCache->Allocator(), lock, Util::ArrayAllocatorAccess::Write);
    VkTextureStreamInfo& streamInfo = textureCache->Get<Texture_StreamInfo>(id.resourceId);
    const VkTextureLoadInfo& loadInfo = textureCache->Get<Texture_LoadInfo>(id.resourceId);
    VkTextureRuntimeInfo& runtimeInfo = textureCache->Get<Texture_RuntimeInfo>(id.resourceId);

    // if the lod is undefined, just add 1 mip
    IndexT adjustedLod = Math::max(0.0f, Math::ceil(loadInfo.mips * lod));

    // abort if the lod is already higher
    if (streamInfo.lowestLod <= (uint32_t)adjustedLod)
        return;

    // bump lod
    adjustedLod = Math::min(adjustedLod, (IndexT)loadInfo.mips);
    IndexT maxLod = loadInfo.mips - streamInfo.lowestLod;

    VkDevice dev = Vulkan::GetCurrentDevice();

    const gliml::context& ctx = streamInfo.ctx;

    VkImageSubresourceRange subres;
    subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subres.baseArrayLayer = 0;
    subres.baseMipLevel = adjustedLod;
    subres.layerCount = loadInfo.layers;
    subres.levelCount = 1;

    // create image
    VkExtent3D extents;
    extents.width = ctx.image_width(0, 0);
    extents.height = ctx.image_height(0, 0);
    extents.depth = ctx.image_depth(0, 0);

    // create image view
    VkImageSubresourceRange viewSubres;
    viewSubres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewSubres.baseArrayLayer = 0;
    viewSubres.baseMipLevel = adjustedLod;
    viewSubres.layerCount = loadInfo.layers;
    viewSubres.levelCount = loadInfo.mips - viewSubres.baseMipLevel;

    VkComponentMapping mapping;
    mapping.r = VkSwizzle[(uint)loadInfo.swizzle.red];
    mapping.g = VkSwizzle[(uint)loadInfo.swizzle.green];
    mapping.b = VkSwizzle[(uint)loadInfo.swizzle.blue];
    mapping.a = VkSwizzle[(uint)loadInfo.swizzle.alpha];

    VkImageViewCreateInfo viewCreate =
    {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr,
        0,
        loadInfo.img,
        VkTypes::AsVkImageViewType(runtimeInfo.type),
        VkTypes::AsVkFormat(loadInfo.format),
        mapping,
        viewSubres
    };

    // use resource submission
    CoreGraphics::CmdBufferId cmdBuf = CoreGraphics::LockTransferSetupCommandBuffer();

    // transition to transfer
    CoreGraphics::CmdBarrier(cmdBuf,
        CoreGraphics::PipelineStage::ImageInitial,
        CoreGraphics::PipelineStage::TransferWrite,
        CoreGraphics::BarrierDomain::Global,
        {
            TextureBarrierInfo
            {
                id,
                CoreGraphics::ImageSubresourceInfo(CoreGraphics::ImageAspect::ColorBits, viewSubres.baseMipLevel, viewSubres.levelCount, viewSubres.baseArrayLayer, viewSubres.layerCount)
            }
        });

    // now load texture by walking through all images and mips
    for (int i = 0; i < ctx.num_faces(); i++)
    {
        for (int j = adjustedLod; j < (IndexT)streamInfo.lowestLod; j++)
        {
            // Perform a texture update
            CoreGraphics::TextureUpdate(
                cmdBuf
                , QueueType::TransferQueueType
                , id
                , ctx.image_width(i, j)
                , ctx.image_height(i, j)
                , j
                , subres.baseArrayLayer + i
                , ctx.image_size(i, j)
                , (byte*)ctx.image_data(i, j));
        }
    }

    // Transition image to read
    CoreGraphics::CmdBarrier(cmdBuf,
        CoreGraphics::PipelineStage::TransferWrite,
        CoreGraphics::PipelineStage::TransferRead,
        CoreGraphics::BarrierDomain::Global,
        {
            TextureBarrierInfo
            {
                id,
                CoreGraphics::ImageSubresourceInfo(CoreGraphics::ImageAspect::ColorBits, viewSubres.baseMipLevel, viewSubres.levelCount, viewSubres.baseArrayLayer, viewSubres.layerCount)
            }
        });
    CoreGraphics::UnlockTransferSetupCommandBuffer();

    // perform final transition on graphics queue
    CoreGraphics::CmdBufferId handoverCmdBuf = CoreGraphics::LockGraphicsSetupCommandBuffer();

    CoreGraphics::CmdBarrier(handoverCmdBuf,
        CoreGraphics::PipelineStage::TransferRead,
        CoreGraphics::PipelineStage::GraphicsShadersRead,
        CoreGraphics::BarrierDomain::Global,
        {
            TextureBarrierInfo
            {
                id,
                CoreGraphics::ImageSubresourceInfo(CoreGraphics::ImageAspect::ColorBits, viewSubres.baseMipLevel, viewSubres.levelCount, viewSubres.baseArrayLayer, viewSubres.layerCount)
            }
        });
    CoreGraphics::UnlockGraphicsSetupCommandBuffer();

    // update lod info and add image view for recreation
    streamInfo.lowestLod = adjustedLod;
    if (immediate)
    {
        VkResult res = vkCreateImageView(GetCurrentDevice(), &viewCreate, nullptr, &runtimeInfo.view);
        n_assert(res == VK_SUCCESS);
    }
    else
    {
        VkShaderServer::Instance()->AddPendingImageView(TextureId(id), viewCreate, runtimeInfo.bind);
    }
}

} // namespace Vulkan
