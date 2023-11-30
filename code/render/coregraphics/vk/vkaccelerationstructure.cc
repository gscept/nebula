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

namespace CoreGraphics
{

using namespace Vulkan;
VkBLASAllocator vkBlasAllocator;

//------------------------------------------------------------------------------
/**
*/
BottomLevelAccelerationId
CreateBottomLevelAcceleration(const BottomLevelAccelerationCreateInfo& info)
{
    VkDevice dev = Vulkan::GetCurrentDevice();
    Ids::Id32 id = vkBlasAllocator.Alloc();
    VkAccelerationStructureKHR handle = vkBlasAllocator.ConstGet<AS_Handle>(id);
    GeometrySetup& setup = vkBlasAllocator.Get<AS_Geometry>(id);
    auto type = VkAccelerationStructureBuildTypeKHR::VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;

    VkBuildAccelerationStructureFlagsKHR vkBuildFlags = 0x0;
    constexpr VkBuildAccelerationStructureFlagsKHR Lookup[] = {
        VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR
        , VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR 
        , VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR
        , VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR
        , VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR
    };

    int flagIndex = 0;
    int flagBits = (int)info.flags;
    while (flagBits != 0)
    {
        int bit = (1 << flagIndex);
        if ((flagBits & bit) != 0)
        {
            vkBuildFlags |= Lookup[flagIndex];
        }
        flagBits &= ~(1 << flagIndex);
        flagIndex++;
    }

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

    setup.triangleData =
    {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
        nullptr,
        positionsFormat,
        vboAddr,
        (uint64)stride,
        0xFFFFFFFF,
        MeshGetIndexType(info.mesh) == IndexType::Index16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32,
        iboAddr,
        VkDeviceOrHostAddressConstKHR {.hostAddress = nullptr}
    };

    VkAccelerationStructureGeometryKHR geometry =
    {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        nullptr,
        VK_GEOMETRY_TYPE_TRIANGLES_KHR,
        VkAccelerationStructureGeometryDataKHR {.triangles = setup.triangleData },
        0x0 // TODO, add support for avoiding anyhit or single-invocation anyhit optimizations
    };

    // Match the number of geometries to the amount of primitive groups
    const Util::Array<CoreGraphics::PrimitiveGroup>& groups = MeshGetPrimitiveGroups(info.mesh);
    for (IndexT i = 0; i < groups.Size(); i++)
    {
        setup.geometries.Append(geometry);
    }

    setup.buildGeometryInfo =
    {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
        nullptr,
        VkAccelerationStructureTypeKHR::VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        vkBuildFlags,
        VkBuildAccelerationStructureModeKHR::VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        VK_NULL_HANDLE,
        VK_NULL_HANDLE,
        (uint)groups.Size(),
        setup.geometries.Begin(),
        nullptr
    };

    Util::Array<uint> maxPrimitiveCounts;

    // Each primitive group is an individual range
    for (IndexT i = 0; i < groups.Size(); i++)
    {
        uint primitiveCount = groups[i].GetNumPrimitives(CoreGraphics::PrimitiveTopology::TriangleList);
        setup.rangeInfos.Append(
        {
            .primitiveCount = (uint)primitiveCount,
            .primitiveOffset = (uint)MeshGetVertexOffset(info.mesh, 0),
            .firstVertex = (uint)groups[i].GetBaseIndex(),
            .transformOffset = 0
        });
        maxPrimitiveCounts.Append(primitiveCount);
    }

    // Get build sizes
    vkGetAccelerationStructureBuildSizesKHR(dev, type, &setup.buildGeometryInfo, maxPrimitiveCounts.Begin(), &setup.buildSizes);

    BottomLevelAccelerationId ret;
    ret.id24 = id;
    ret.id8 = AccelerationStructureIdType;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyBottomLevelAcceleration(const BottomLevelAccelerationId blac)
{
}

//------------------------------------------------------------------------------
/**
*/
void
BottomLevelAccelerationBuild(const BottomLevelAccelerationId blac)
{
    VkDevice dev = Vulkan::GetCurrentDevice();
    GeometrySetup& setup = vkBlasAllocator.Get<AS_Geometry>(blac.id24);

    // Trigger a build
    const VkAccelerationStructureBuildRangeInfoKHR* ranges = setup.rangeInfos.Begin();
    VkResult res = vkBuildAccelerationStructuresKHR(dev, VK_NULL_HANDLE, 1, &setup.buildGeometryInfo, &ranges);
    n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
TopLevelAccelerationId
CreateTopLevelAcceleration(const TopLevelAccelerationCreateInfo& info)
{
    return TopLevelAccelerationId();
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyTopLevelAcceleration(const TopLevelAccelerationId tlac)
{
}

//------------------------------------------------------------------------------
/**
*/
void
TopLevelAccelerationBuild(const TopLevelAccelerationId tlac)
{
}

} // namespace CoreGraphics


namespace Vulkan
{

} // namespace Vulkan
