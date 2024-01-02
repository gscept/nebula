//------------------------------------------------------------------------------
//  vkbuffer.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "vkgraphicsdevice.h"
#include "vkcommandbuffer.h"
#include "vkbuffer.h"
#include "util/bit.h"
namespace Vulkan
{
VkBufferAllocator bufferAllocator;

//------------------------------------------------------------------------------
/**
*/
VkBuffer 
BufferGetVk(const CoreGraphics::BufferId id)
{
    return bufferAllocator.ConstGet<Buffer_RuntimeInfo>(id.id24).buf;
}

//------------------------------------------------------------------------------
/**
*/
VkDeviceMemory 
BufferGetVkMemory(const CoreGraphics::BufferId id)
{
    return bufferAllocator.ConstGet<Buffer_LoadInfo>(id.id24).mem.mem;
}

//------------------------------------------------------------------------------
/**
*/
VkDevice 
BufferGetVkDevice(const CoreGraphics::BufferId id)
{
    return bufferAllocator.ConstGet<Buffer_LoadInfo>(id.id24).dev;
}

} // namespace Vulkan
namespace CoreGraphics
{

using namespace Vulkan;
_IMPL_ACQUIRE_RELEASE(BufferId, bufferAllocator);

//------------------------------------------------------------------------------
/**
*/
const BufferId 
CreateBuffer(const BufferCreateInfo& info)
{
    Ids::Id32 id = bufferAllocator.Alloc();
    VkBufferLoadInfo& loadInfo = bufferAllocator.Get<Buffer_LoadInfo>(id);
    VkBufferRuntimeInfo& runtimeInfo = bufferAllocator.Get<Buffer_RuntimeInfo>(id);
    VkBufferMapInfo& mapInfo = bufferAllocator.Get<Buffer_MapInfo>(id);

    loadInfo.dev = Vulkan::GetCurrentDevice();
    runtimeInfo.usageFlags = info.usageFlags;

    VkBufferUsageFlags flags = 0;
    VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    Util::Set<uint32_t> queues;
    if (info.queueSupport != AutomaticQueueSupport)
    {
        if (info.queueSupport & GraphicsQueueSupport)
            queues.Add(CoreGraphics::GetQueueIndex(GraphicsQueueType));
        if (info.queueSupport & ComputeQueueSupport)
            queues.Add(CoreGraphics::GetQueueIndex(ComputeQueueType));
        if (info.queueSupport & TransferQueueSupport)
            queues.Add(CoreGraphics::GetQueueIndex(TransferQueueType));
    }
    else
    {
        if (info.usageFlags & CoreGraphics::TransferBufferSource)
        {
            queues.Add(CoreGraphics::GetQueueIndex(GraphicsQueueType));
            queues.Add(CoreGraphics::GetQueueIndex(TransferQueueType));
        }
        if (info.usageFlags & CoreGraphics::TransferBufferDestination)
        {
            queues.Add(CoreGraphics::GetQueueIndex(GraphicsQueueType));
            queues.Add(CoreGraphics::GetQueueIndex(TransferQueueType));
        }
    }

    constexpr uint UsageLookup[] =
    {
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
        0x0,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR
    };

    flags = Util::BitmaskConvert(info.usageFlags, UsageLookup);

    // force add destination bit if we have data to be uploaded
    if (info.mode == DeviceLocal && info.dataSize != 0)
        flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    if (queues.Size() > 1)
        sharingMode = VK_SHARING_MODE_CONCURRENT;

    // start by creating buffer
    uint size = info.byteSize == 0 ? info.size * info.elementSize : info.byteSize;
    VkBufferCreateInfo bufinfo =
    {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        NULL,
        0,															// use for sparse buffers
        size,
        flags,
        sharingMode,												// can only be accessed from the creator queue,
        (uint)queues.Size(),												// number of queues in family
        queues.Size() > 0 ? queues.KeysAsArray().Begin() : nullptr	// array of queues belonging to family
    };

    VkResult err = vkCreateBuffer(loadInfo.dev, &bufinfo, NULL, &runtimeInfo.buf);
    n_assert(err == VK_SUCCESS);

    CoreGraphics::MemoryPoolType pool = CoreGraphics::MemoryPool_DeviceLocal;
    if (info.mode == DeviceLocal)
        pool = CoreGraphics::MemoryPool_DeviceLocal;
    else if (info.mode == HostLocal)
        pool = CoreGraphics::MemoryPool_HostLocal;
    else if (info.mode == DeviceAndHost)
        pool = CoreGraphics::MemoryPool_DeviceAndHost;
    else if (info.mode == HostCached)
        pool = CoreGraphics::MemoryPool_HostCached;

    uint baseAlignment = 1;
    if (AllBits(info.usageFlags, CoreGraphics::AccelerationStructureScratch))
        baseAlignment = CoreGraphics::GetCurrentAccelerationStructureProperties().minAccelerationStructureScratchOffsetAlignment;
    else if (AllBits(info.usageFlags, CoreGraphics::AccelerationStructureInstances))
        baseAlignment = 16;
    else if (AllBits(info.usageFlags, CoreGraphics::ShaderTable))
        baseAlignment = CoreGraphics::GetCurrentRaytracingProperties().shaderGroupBaseAlignment;

    // now bind memory to buffer
    CoreGraphics::Alloc alloc = AllocateMemory(loadInfo.dev, runtimeInfo.buf, pool, baseAlignment);
    err = vkBindBufferMemory(loadInfo.dev, runtimeInfo.buf, alloc.mem, alloc.offset);
    n_assert(err == VK_SUCCESS);

    loadInfo.mem = alloc;

    if (info.mode == HostLocal || info.mode == HostCached || info.mode == DeviceAndHost)
    {
        // copy contents and flush memory
        char* data = (char*)GetMappedMemory(alloc);
        mapInfo.mappedMemory = data;

        // if we have data, copy the memory to the region
        if (info.data)
        {
            n_assert(info.dataSize <= bufinfo.size);
            memcpy(data, info.data, info.dataSize);

            // if not host-local memory, we need to flush the initial update
            if (info.mode == HostCached || info.mode == DeviceAndHost)
            {
                VkPhysicalDeviceProperties props = Vulkan::GetCurrentProperties();

                VkMappedMemoryRange range;
                range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
                range.pNext = nullptr;
                range.offset = Math::align_down(alloc.offset, props.limits.nonCoherentAtomSize);
                range.size = Math::align(alloc.size, props.limits.nonCoherentAtomSize);
                range.memory = alloc.mem;
                VkResult res = vkFlushMappedMemoryRanges(loadInfo.dev, 1, &range);
                n_assert(res == VK_SUCCESS);
            }
        }
    }
    else if (info.mode == DeviceLocal && info.data != nullptr)
    {
        // if device local and we provide a data pointer, create a temporary staging buffer and perform a copy
        VkBufferCreateInfo tempAllocInfo =
        {
            VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            NULL,
            0,									// use for sparse buffers
            info.dataSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_SHARING_MODE_EXCLUSIVE,			// can only be accessed from the creator queue,
            0,									// number of queues in family
            nullptr								// array of queues belonging to family
        };

        // create temporary buffer
        VkBuffer tempBuffer;
        err = vkCreateBuffer(loadInfo.dev, &tempAllocInfo, nullptr, &tempBuffer);

        // allocate some host-local temporary memory for it
        CoreGraphics::Alloc tempAlloc = AllocateMemory(loadInfo.dev, tempBuffer, CoreGraphics::MemoryPool_HostLocal);
        err = vkBindBufferMemory(loadInfo.dev, tempBuffer, tempAlloc.mem, tempAlloc.offset);
        n_assert(err == VK_SUCCESS);

        // copy data to temporary buffer
        char* buf = (char*)GetMappedMemory(tempAlloc);
        memcpy(buf, info.data, info.dataSize);

        CoreGraphics::CmdBufferId cmd = CoreGraphics::LockGraphicsSetupCommandBuffer();
        VkBufferCopy copy;
        copy.dstOffset = 0;
        copy.srcOffset = 0;
        copy.size = info.dataSize;

        // copy from temp buffer to source buffer in the resource submission context
        vkCmdCopyBuffer(Vulkan::CmdBufferGetVk(cmd), tempBuffer, runtimeInfo.buf, 1, &copy);

        // add delayed delete for this temporary buffer
        Vulkan::DelayedDeleteVkBuffer(loadInfo.dev, tempBuffer);
        CoreGraphics::DelayedFreeMemory(tempAlloc);
        CoreGraphics::UnlockGraphicsSetupCommandBuffer();
    }

    // setup resource
    loadInfo.mode = info.mode;
    loadInfo.size = info.size;
    loadInfo.byteSize = size;
    loadInfo.elementSize = info.elementSize;

    BufferId ret;
    ret.id8 = BufferIdType;
    ret.id24 = id;

#if NEBULA_GRAPHICS_DEBUG
    ObjectSetName(ret, info.name.Value());
#endif

    CoreGraphics::BufferIdRelease(ret);

    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void 
DestroyBuffer(const BufferId id)
{
    bufferAllocator.Acquire(id.id24);
    VkBufferLoadInfo& loadInfo = bufferAllocator.Get<Buffer_LoadInfo>(id.id24);
    const VkBufferMapInfo& mapInfo = bufferAllocator.Get<Buffer_MapInfo>(id.id24);

    CoreGraphics::DelayedDeleteBuffer(id);
    CoreGraphics::DelayedFreeMemory(loadInfo.mem);
    loadInfo.mem = CoreGraphics::Alloc{};
    bufferAllocator.Dealloc(id.id24);
    bufferAllocator.Release(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
const BufferUsageFlags 
BufferGetType(const BufferId id)
{
    return bufferAllocator.ConstGet<Buffer_RuntimeInfo>(id.id24).usageFlags;
}

//------------------------------------------------------------------------------
/**
*/
const SizeT 
BufferGetSize(const BufferId id)
{
    return bufferAllocator.ConstGet<Buffer_LoadInfo>(id.id24).size;
}

//------------------------------------------------------------------------------
/**
*/
const SizeT 
BufferGetElementSize(const BufferId id)
{
    return bufferAllocator.ConstGet<Buffer_LoadInfo>(id.id24).elementSize;
}

//------------------------------------------------------------------------------
/**
*/
const SizeT 
BufferGetByteSize(const BufferId id)
{
    return bufferAllocator.ConstGet<Buffer_LoadInfo>(id.id24).byteSize;
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
BufferGetUploadMaxSize()
{
    return 65536;
}

//------------------------------------------------------------------------------
/**
*/
void* 
BufferMap(const BufferId id)
{
    const VkBufferMapInfo& mapInfo = bufferAllocator.ConstGet<Buffer_MapInfo>(id.id24);
    n_assert2(mapInfo.mappedMemory != nullptr, "Buffer must be created as dynamic or mapped to support mapping");
    return mapInfo.mappedMemory;
}

//------------------------------------------------------------------------------
/**
*/
void 
BufferUnmap(const BufferId id)
{
    // Do nothing on Vulkan
}

//------------------------------------------------------------------------------
/**
*/
void
BufferUpdate(const BufferId id, const void* data, const uint size, const uint offset)
{
    const VkBufferMapInfo& map = bufferAllocator.ConstGet<Buffer_MapInfo>(id.id24);

#if NEBULA_DEBUG
    const VkBufferLoadInfo& setup = bufferAllocator.ConstGet<Buffer_LoadInfo>(id.id24);
    n_assert(size + offset <= (uint)setup.byteSize);
#endif
    byte* buf = (byte*)map.mappedMemory + offset;
    memcpy(buf, data, size);
}

//------------------------------------------------------------------------------
/**
*/
void
BufferUpload(const CoreGraphics::CmdBufferId cmdBuf, const BufferId id, const void* data, const uint size, const uint offset)
{
    n_assert(size <= (uint)BufferGetUploadMaxSize());
    CoreGraphics::CmdUpdateBuffer(cmdBuf, id, offset, size, data);
}

//------------------------------------------------------------------------------
/**
*/
void
BufferFill(const CoreGraphics::CmdBufferId cmdBuf, const BufferId id, char pattern)
{
    const VkBufferLoadInfo& setup = bufferAllocator.ConstGet<Buffer_LoadInfo>(id.id24);
    
    int remainingBytes = setup.byteSize;
    uint numChunks = Math::divandroundup(setup.byteSize, BufferGetUploadMaxSize());
    int chunkOffset = 0;
    for (uint i = 0; i < numChunks; i++)
    {
        int chunkSize = Math::min(remainingBytes, BufferGetUploadMaxSize());
        char* buf = new char[chunkSize];
        memset(buf, pattern, chunkSize);
        vkCmdUpdateBuffer(Vulkan::CmdBufferGetVk(cmdBuf), Vulkan::BufferGetVk(id), chunkOffset, chunkSize, buf);
        chunkOffset += chunkSize;
        remainingBytes -= chunkSize;
        delete[] buf;
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
BufferFlush(const BufferId id, IndexT offset, SizeT size)
{
    const VkBufferLoadInfo& loadInfo = bufferAllocator.ConstGet<Buffer_LoadInfo>(id.id24);
    n_assert(size == NEBULA_WHOLE_BUFFER_SIZE ? true : (uint)offset + size <= loadInfo.byteSize);
    Flush(loadInfo.dev, loadInfo.mem, offset, size);
}

//------------------------------------------------------------------------------
/**
*/
void 
BufferInvalidate(const BufferId id, IndexT offset, SizeT size)
{
    const VkBufferLoadInfo& loadInfo = bufferAllocator.ConstGet<Buffer_LoadInfo>(id.id24);
    n_assert(size == NEBULA_WHOLE_BUFFER_SIZE ? true : (uint)offset + size <= loadInfo.byteSize);
    Invalidate(loadInfo.dev, loadInfo.mem, offset, size);
}

//------------------------------------------------------------------------------
/**
*/
DeviceAddress
BufferGetDeviceAddress(const BufferId id)
{
    VkDevice dev = BufferGetVkDevice(id);
    VkBufferDeviceAddressInfo deviceAddress =
    {
        VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        nullptr,
        BufferGetVk(id)
    };
    return vkGetBufferDeviceAddress(dev, &deviceAddress);
}

} // namespace CoreGraphics
