#pragma once
//------------------------------------------------------------------------------
/**
    Vulkan implementation of GPU acceleration structure

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"
namespace Vulkan
{


struct GeometrySetup
{
    VkAccelerationStructureGeometryTrianglesDataKHR triangleData;
    VkAccelerationStructureBuildSizesInfoKHR buildSizes;
    VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo;
    Util::Array<VkAccelerationStructureGeometryKHR> geometries;
    Util::Array<VkAccelerationStructureBuildRangeInfoKHR> rangeInfos;
};

enum
{
    AS_Handle,
    AS_Geometry,
    AS_View
};


typedef Ids::IdAllocatorSafe<
    0xFFF
    , VkAccelerationStructureKHR
    , GeometrySetup
    , VkDeviceAddress
> VkBLASAllocator;

extern VkBLASAllocator vkBlasAllocator;

} // namespace Vulkan
