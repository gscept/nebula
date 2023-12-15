//------------------------------------------------------------------------------
//  @file vkaccelerationstructure.cc
//  @copyright (C) 2023 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "coregraphics/mesh.h"
#include "coregraphics/accelerationstructure.h"
#include "vkaccelerationstructure.h"
#include "vkgraphicsdevice.h"
#include "vkbuffer.h"
#include "vktypes.h"


namespace Vulkan
{
VkBlasAllocator blasAllocator;
VkBlasInstanceAllocator blasInstanceAllocator;
VkTlasAllocator tlasAllocator;

//------------------------------------------------------------------------------
/**
*/
VkDevice
BlasGetVkDevice(const CoreGraphics::BlasId id)
{
    return blasAllocator.Get<Blas_Device>(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
const VkAccelerationStructureKHR
BlasGetVk(const CoreGraphics::BlasId id)
{
    return blasAllocator.Get<Blas_Handle>(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
const VkAccelerationStructureBuildGeometryInfoKHR&
BlasGetVkBuild(const CoreGraphics::BlasId id)
{
    return blasAllocator.Get<Blas_Geometry>(id.id24).buildGeometryInfo;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<VkAccelerationStructureBuildRangeInfoKHR>&
BlasGetVkRanges(const CoreGraphics::BlasId id)
{
    return blasAllocator.Get<Blas_Geometry>(id.id24).rangeInfos;
}

//------------------------------------------------------------------------------
/**
*/
VkDevice
TlasGetVkDevice(const CoreGraphics::TlasId id)
{
    return tlasAllocator.Get<Tlas_Device>(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
VkAccelerationStructureKHR
TlasGetVk(const CoreGraphics::TlasId id)
{
    return tlasAllocator.Get<Tlas_Handle>(id.id24);
}

} // namespace Vulkan

namespace CoreGraphics
{
_IMPL_ACQUIRE_RELEASE(BlasInstanceId, Vulkan::blasInstanceAllocator);

using namespace Vulkan;

//------------------------------------------------------------------------------
/**
*/
BlasId
CreateBlas(const BlasCreateInfo& info)
{
    VkDevice dev = Vulkan::GetCurrentDevice();
    Ids::Id32 id = blasAllocator.Alloc();
    VkAccelerationStructureKHR handle = blasAllocator.ConstGet<Blas_Handle>(id);
    GeometrySetup& setup = blasAllocator.Get<Blas_Geometry>(id);
    blasAllocator.Set<Blas_Device>(id, dev);
    auto type = VkAccelerationStructureBuildTypeKHR::VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;

    constexpr VkBuildAccelerationStructureFlagsKHR Lookup[] = {
        VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR
        , VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR 
        , VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR
        , VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR
        , VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR
    };

    uint flagBits = Util::BitmaskConvert((uint)info.flags, Lookup);

    BufferIdLock _1(CoreGraphics::GetVertexBuffer());
    BufferIdLock _2(CoreGraphics::GetIndexBuffer());
    VkBufferDeviceAddressInfo deviceAddress =
    {
        VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        nullptr,
        VK_NULL_HANDLE
    };
    deviceAddress.buffer = BufferGetVk(CoreGraphics::GetVertexBuffer());
    VkDeviceAddress vboAddr = vkGetBufferDeviceAddress(dev, &deviceAddress);
    deviceAddress.buffer = BufferGetVk(CoreGraphics::GetIndexBuffer());
    VkDeviceAddress iboAddr = vkGetBufferDeviceAddress(dev, &deviceAddress);

    MeshIdLock _0(info.mesh);
    CoreGraphics::VertexLayoutId layout = MeshGetVertexLayout(info.mesh);

    // Get positions format
    const Util::Array<VertexComponent>& components = VertexLayoutGetComponents(layout);
    VkFormat positionsFormat = VkTypes::AsVkVertexType(components[0].GetFormat());
    SizeT stride = VertexLayoutGetStreamSize(layout, 0); // 0 is the position/UV stream

    // Assert the vertex format is supported by raytracing
    VkFormatProperties2 formatProps =
    {
        VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2,
        nullptr
    };
    vkGetPhysicalDeviceFormatProperties2(Vulkan::GetCurrentPhysicalDevice(), positionsFormat, &formatProps);
    n_assert(formatProps.formatProperties.bufferFeatures & VK_FORMAT_FEATURE_2_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR);

    setup.triangleData =
    {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
        .pNext = nullptr,
        .vertexFormat = positionsFormat,
        .vertexData = VkDeviceOrHostAddressConstKHR {.deviceAddress = vboAddr + MeshGetVertexOffset(info.mesh, 0)},
        .vertexStride = (uint64)stride,
        .maxVertex = MeshGetIndexType(info.mesh) == IndexType::Index16 ? 0xFFFE : 0xFFFFFFFE,
        .indexType = MeshGetIndexType(info.mesh) == IndexType::Index16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32,
        .indexData = VkDeviceOrHostAddressConstKHR {.deviceAddress = iboAddr + MeshGetIndexOffset(info.mesh)},
        .transformData = VkDeviceOrHostAddressConstKHR{ .hostAddress = nullptr } // TODO: Support transforms
    };

    VkAccelerationStructureGeometryKHR geometry =
    {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .pNext = nullptr,
        .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
        .geometry = VkAccelerationStructureGeometryDataKHR{ .triangles = setup.triangleData },
        .flags = 0x0 // TODO, add support for avoiding anyhit or single-invocation anyhit optimizations
    };

    // Match the number of geometries to the amount of primitive groups
    const Util::Array<CoreGraphics::PrimitiveGroup>& groups = MeshGetPrimitiveGroups(info.mesh);
    for (IndexT i = 0; i < groups.Size(); i++)
    {
        setup.geometries.Append(geometry);
    }

    setup.buildGeometryInfo =
    {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .pNext = nullptr,
        .type = VkAccelerationStructureTypeKHR::VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        .flags = flagBits,
        .mode = VkBuildAccelerationStructureModeKHR::VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        .srcAccelerationStructure = VK_NULL_HANDLE,
        .dstAccelerationStructure = VK_NULL_HANDLE,
        .geometryCount = (uint)groups.Size(),
        .pGeometries = setup.geometries.Begin(),
        .ppGeometries = nullptr,
        .scratchData = VkDeviceOrHostAddressKHR{ .hostAddress = nullptr }
    };

    Util::Array<uint> maxPrimitiveCounts;

    // Each primitive group is an individual range
    for (IndexT i = 0; i < groups.Size(); i++)
    {
        uint primitiveCount = groups[i].GetNumPrimitives(CoreGraphics::PrimitiveTopology::TriangleList);
        setup.rangeInfos.Append(
        {
            .primitiveCount = (uint)primitiveCount,
            .primitiveOffset = 0, // Primitive offset is defined in the mesh
            .firstVertex = (uint)groups[i].GetBaseIndex(),
            .transformOffset = 0
        });
        maxPrimitiveCounts.Append(primitiveCount);
    }

    // Get build sizes
    setup.buildSizes =
    {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
        nullptr,
        0, 0, 0
    };
    vkGetAccelerationStructureBuildSizesKHR(dev, type, &setup.buildGeometryInfo, maxPrimitiveCounts.Begin(), &setup.buildSizes);

    CoreGraphics::BufferCreateInfo bufferInfo;
    bufferInfo.byteSize = setup.buildSizes.accelerationStructureSize;
    bufferInfo.mode = CoreGraphics::BufferAccessMode::DeviceLocal;
    bufferInfo.usageFlags = CoreGraphics::BufferUsageFlag::ShaderAddress | CoreGraphics::BufferUsageFlag::AccelerationStructure;
    bufferInfo.queueSupport = CoreGraphics::GraphicsQueueSupport;

    // Create main buffer
    CoreGraphics::BufferId blasBuf = CoreGraphics::CreateBuffer(bufferInfo);
    blasAllocator.Set<Blas_Buffer>(id, blasBuf);

    // Create scratch buffer
    bufferInfo.usageFlags = CoreGraphics::BufferUsageFlag::ShaderAddress | CoreGraphics::BufferUsageFlag::ReadWriteBuffer;
    bufferInfo.byteSize = setup.buildSizes.buildScratchSize;
    CoreGraphics::BufferId scratchBuf = CoreGraphics::CreateBuffer(bufferInfo);
    blasAllocator.Set<Blas_Scratch>(id, scratchBuf);

    deviceAddress.buffer = BufferGetVk(scratchBuf);
    VkDeviceAddress scratchAddr = vkGetBufferDeviceAddress(dev, &deviceAddress);
    setup.buildGeometryInfo.scratchData = VkDeviceOrHostAddressKHR{ .deviceAddress = scratchAddr };

    // Now create it
    VkAccelerationStructureCreateInfoKHR createInfo =
    {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        nullptr,
        0x0,
        BufferGetVk(blasBuf),
        0,
        setup.buildSizes.accelerationStructureSize,
        VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        0x0
    };
    VkAccelerationStructureKHR as;
    VkResult res = vkCreateAccelerationStructureKHR(dev, &createInfo, nullptr, &as);
    n_assert(res == VK_SUCCESS);

    blasAllocator.Set<Blas_Handle>(id, as);
    setup.buildGeometryInfo.dstAccelerationStructure = as;

    BlasId ret;
    ret.id24 = id;
    ret.id8 = BlasIdType;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyBlas(const BlasId blas)
{
    blasAllocator.Dealloc(blas.id24);
    CoreGraphics::DelayedDeleteBlas(blas);
    CoreGraphics::DestroyBuffer(blasAllocator.Get<Blas_Buffer>(blas.id24));
    CoreGraphics::DestroyBuffer(blasAllocator.Get<Blas_Scratch>(blas.id24));
}

//------------------------------------------------------------------------------
/**
*/
BlasInstanceId
CreateBlasInstance(const BlasInstanceCreateInfo& info)
{
    VkDevice dev = Vulkan::GetCurrentDevice();
    Ids::Id32 id = blasInstanceAllocator.Alloc();

    VkAccelerationStructureInstanceKHR& setup = blasInstanceAllocator.Get<BlasInstance_Instance>(id);
    VkAccelerationStructureDeviceAddressInfoKHR addrInfo =
    {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
        .pNext = nullptr,
        .accelerationStructure = BlasGetVk(info.blas)
    };
    VkDeviceAddress blasAddr = vkGetAccelerationStructureDeviceAddressKHR(dev, &addrInfo);

    constexpr uint Lookup[] =
    {
        VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
        VK_GEOMETRY_INSTANCE_TRIANGLE_FLIP_FACING_BIT_KHR,
        VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR,
        VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR
    };

    uint flags = Util::BitmaskConvert(info.flags, Lookup);

    setup =
    {
        .transform = VkTransformMatrixKHR(),
        .instanceCustomIndex = info.instanceIndex,
        .mask = info.mask,
        .instanceShaderBindingTableRecordOffset = info.shaderOffset,
        .flags = flags,
        .accelerationStructureReference = blasAddr
    };
    info.transform.store3(&setup.transform.matrix[0][0]);

    // Update buffer with data
    CoreGraphics::BufferIdLock _0(info.buffer);
    CoreGraphics::BufferUpdate(info.buffer, setup, info.offset);

    // Bind buffer and offset to instance
    blasInstanceAllocator.Set<BlasInstance_Buffer>(id, info.buffer);
    blasInstanceAllocator.Set<BlasInstance_BufferMem>(id, (char*)CoreGraphics::BufferMap(info.buffer) + info.offset);
    blasInstanceAllocator.Set<BlasInstance_Transform>(id, info.transform);

    BlasInstanceId ret;
    ret.id24 = id;
    ret.id8 = BlasInstanceIdType;

    BlasInstanceIdRelease(ret);
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyBlasInstance(const BlasInstanceId id)
{
    blasInstanceAllocator.Dealloc(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
void
BlasInstanceSetTransform(const BlasInstanceId id, const Math::mat4& transform)
{
    VkAccelerationStructureInstanceKHR& setup = blasInstanceAllocator.Get<BlasInstance_Instance>(id.id24);
    transform.store3(&setup.transform.matrix[0][0]);

    // Update data in buffer owning the blas instance
    const CoreGraphics::BufferId buf = blasInstanceAllocator.Get<BlasInstance_Buffer>(id.id24);
    char* bufMem = (char*)blasInstanceAllocator.Get<BlasInstance_BufferMem>(id.id24);

    // Use memory pointer directly to circumvent having to sync the buffer 
    memcpy(bufMem, &setup, sizeof(setup));
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
BlasInstanceGetSize()
{
    return sizeof(VkAccelerationStructureInstanceKHR);
}

//------------------------------------------------------------------------------
/**
*/
TlasId
CreateTlas(const TlasCreateInfo& info)
{
    VkDevice dev = Vulkan::GetCurrentDevice();
    Ids::Id32 id = tlasAllocator.Alloc();
    auto type = VkAccelerationStructureBuildTypeKHR::VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;
    tlasAllocator.Set<Tlas_Device>(id, dev);
    SceneSetup& scene = tlasAllocator.Get<Tlas_Scene>(id);

    VkBufferDeviceAddressInfo addrInfo =
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext = nullptr,
        .buffer = BufferGetVk(info.instanceBuffer)
    };
    VkDeviceAddress instanceBufferAddr = vkGetBufferDeviceAddress(dev, &addrInfo);

    VkAccelerationStructureGeometryInstancesDataKHR instanceData =
    {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
        .pNext = nullptr,
        .arrayOfPointers = true,
        .data = VkDeviceOrHostAddressConstKHR{ .deviceAddress = instanceBufferAddr }
    };

    VkAccelerationStructureGeometryKHR geoInfo =
    {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .pNext = nullptr,
        .geometryType = VkGeometryTypeKHR::VK_GEOMETRY_TYPE_INSTANCES_KHR,
        .geometry = { .instances = instanceData }
    };
    scene.geometryInfo =
    {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .pNext = nullptr,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR,
        .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        .srcAccelerationStructure = VK_NULL_HANDLE,
        .dstAccelerationStructure = VK_NULL_HANDLE,
        .geometryCount = 1,
        .pGeometries = &geoInfo,
        .ppGeometries = nullptr,
        .scratchData = VkDeviceOrHostAddressKHR{ .hostAddress = nullptr }
    };

    // Get build sizes
    scene.buildSizes =
    {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
        nullptr,
        0, 0, 0
    };
    vkGetAccelerationStructureBuildSizesKHR(dev, type, &scene.geometryInfo, (uint*)&info.numInstances, &scene.buildSizes);

    CoreGraphics::BufferId tlasBuf, buildScratchBuf, updateScratchBuf;
    VkDeviceAddress buildScratchBufAddr, updateScratchBufAddr;

    {
        CoreGraphics::BufferCreateInfo bufferInfo;
        bufferInfo.byteSize = scene.buildSizes.accelerationStructureSize;
        bufferInfo.mode = CoreGraphics::BufferAccessMode::DeviceLocal;
        bufferInfo.usageFlags = CoreGraphics::BufferUsageFlag::ShaderAddress | CoreGraphics::BufferUsageFlag::AccelerationStructure;
        bufferInfo.queueSupport = CoreGraphics::GraphicsQueueSupport;

        // Create main buffer
        tlasBuf = CoreGraphics::CreateBuffer(bufferInfo);
        tlasAllocator.Set<Tlas_Buffer>(id, tlasBuf);
    }

    {
        // Create build scratch buffer
        CoreGraphics::BufferCreateInfo bufferInfo;
        bufferInfo.byteSize = scene.buildSizes.buildScratchSize;
        bufferInfo.mode = CoreGraphics::BufferAccessMode::DeviceLocal;
        bufferInfo.usageFlags = CoreGraphics::BufferUsageFlag::ShaderAddress | CoreGraphics::BufferUsageFlag::ReadWriteBuffer;
        bufferInfo.queueSupport = CoreGraphics::GraphicsQueueSupport;
        buildScratchBuf = CoreGraphics::CreateBuffer(bufferInfo);
        tlasAllocator.Set<Tlas_BuildScratch>(id, buildScratchBuf);

        VkBufferDeviceAddressInfo addrInfo =
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .pNext = nullptr,
            .buffer = BufferGetVk(buildScratchBuf)
        };
        buildScratchBufAddr = vkGetBufferDeviceAddress(dev, &addrInfo);
        tlasAllocator.Set<Tlas_BuildScratchAddr>(id, buildScratchBufAddr);
    }

    {
        // Create update scratch buffer
        CoreGraphics::BufferCreateInfo bufferInfo;
        bufferInfo.byteSize = scene.buildSizes.updateScratchSize;
        bufferInfo.mode = CoreGraphics::BufferAccessMode::DeviceLocal;
        bufferInfo.usageFlags = CoreGraphics::BufferUsageFlag::ShaderAddress | CoreGraphics::BufferUsageFlag::ReadWriteBuffer;
        bufferInfo.queueSupport = CoreGraphics::GraphicsQueueSupport;
        updateScratchBuf = CoreGraphics::CreateBuffer(bufferInfo);
        tlasAllocator.Set<Tlas_UpdateScratch>(id, updateScratchBuf);

        VkBufferDeviceAddressInfo addrInfo =
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .pNext = nullptr,
            .buffer = BufferGetVk(updateScratchBuf)
        };
        updateScratchBufAddr = vkGetBufferDeviceAddress(dev, &addrInfo);
        tlasAllocator.Set<Tlas_UpdateScratchAddr>(id, updateScratchBufAddr);
    }

    scene.geometryInfo.scratchData = VkDeviceOrHostAddressKHR{ .deviceAddress = buildScratchBufAddr };

    // Now create it
    VkAccelerationStructureCreateInfoKHR createInfo =
    {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        nullptr,
        0x0,
        BufferGetVk(tlasBuf),
        0,
        scene.buildSizes.accelerationStructureSize,
        VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        0x0
    };
    VkAccelerationStructureKHR as;
    VkResult res = vkCreateAccelerationStructureKHR(dev, &createInfo, nullptr, &as);
    n_assert(res == VK_SUCCESS);

    tlasAllocator.Set<Tlas_Handle>(id, as);
    scene.geometryInfo.dstAccelerationStructure = as;

    TlasId ret;
    ret.id24 = id;
    ret.id8 = TlasIdType;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyTlas(const TlasId tlas)
{
    CoreGraphics::DelayedDeleteTlas(tlas);
    CoreGraphics::DestroyBuffer(tlasAllocator.Get<Tlas_Buffer>(tlas.id24));
    CoreGraphics::DestroyBuffer(tlasAllocator.Get<Tlas_BuildScratch>(tlas.id24));
    CoreGraphics::DestroyBuffer(tlasAllocator.Get<Tlas_UpdateScratch>(tlas.id24));
    tlasAllocator.Dealloc(tlas.id24);
}

//------------------------------------------------------------------------------
/**
*/
void
TlasInitBuild(const TlasId tlas)
{
    SceneSetup& scene = tlasAllocator.Get<Tlas_Scene>(tlas.id24);
    scene.geometryInfo.mode = VkBuildAccelerationStructureModeKHR::VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    scene.geometryInfo.scratchData = VkDeviceOrHostAddressKHR{ .deviceAddress = tlasAllocator.Get<Tlas_BuildScratchAddr>(tlas.id24) };
}

//------------------------------------------------------------------------------
/**
*/
void
TlasInitUpdate(const TlasId tlas)
{
    SceneSetup& scene = tlasAllocator.Get<Tlas_Scene>(tlas.id24);
    scene.geometryInfo.mode = VkBuildAccelerationStructureModeKHR::VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    scene.geometryInfo.scratchData = VkDeviceOrHostAddressKHR{ .deviceAddress = tlasAllocator.Get<Tlas_UpdateScratchAddr>(tlas.id24) };
}

} // namespace CoreGraphics

