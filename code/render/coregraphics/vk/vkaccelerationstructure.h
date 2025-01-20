#pragma once
//------------------------------------------------------------------------------
/**
    Vulkan implementation of GPU acceleration structure

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"
#include "coregraphics/accelerationstructure.h"
namespace Vulkan
{

struct GeometrySetup
{
    VkAccelerationStructureGeometryTrianglesDataKHR triangleData;
    VkAccelerationStructureBuildSizesInfoKHR buildSizes;
    VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo;
    Util::PinnedArray<256, VkAccelerationStructureGeometryKHR> geometries;
    Util::PinnedArray<256, VkAccelerationStructureBuildRangeInfoKHR> rangeInfos;
};

enum
{
    Blas_Device,
    Blas_Handle,
    Blas_Buffer,
    Blas_Scratch,
    Blas_Geometry,
    Blas_View
};

typedef Ids::IdAllocatorSafe<
    0xFFF
    , VkDevice
    , VkAccelerationStructureKHR
    , CoreGraphics::BufferId
    , CoreGraphics::BufferId
    , GeometrySetup
    , VkDeviceAddress
> VkBlasAllocator;
extern VkBlasAllocator blasAllocator;

/// Get device used to create blas
const VkDevice BlasGetVkDevice(const CoreGraphics::BlasId id);
/// Get buffer holding TLAS data
const VkBuffer BlasGetVkBuffer(const CoreGraphics::BlasId id);
/// Get buffer representing the acceleration structure
const VkAccelerationStructureKHR BlasGetVk(const CoreGraphics::BlasId id);
/// Get build info for bottom level acceleration structure
const VkAccelerationStructureBuildGeometryInfoKHR& BlasGetVkBuild(const CoreGraphics::BlasId id);
/// Get range infos for bottom level acceleration structure
const Util::Array<VkAccelerationStructureBuildRangeInfoKHR>& BlasGetVkRanges(const CoreGraphics::BlasId id);

struct InstanceSetup
{
    VkAccelerationStructureGeometryInstancesDataKHR instanceData;
    VkAccelerationStructureBuildGeometryInfoKHR geometryInfo;
};

enum
{
    BlasInstance_Instance,
    BlasInstance_Transform,
};

typedef Ids::IdAllocatorSafe<
    0xFFFF
    , VkAccelerationStructureInstanceKHR
    , Math::mat4
    , uint
> VkBlasInstanceAllocator;
extern VkBlasInstanceAllocator blasInstanceAllocator;


struct SceneSetup
{
    VkAccelerationStructureGeometryKHR geo;
    VkAccelerationStructureBuildGeometryInfoKHR geometryInfo;
    VkAccelerationStructureBuildSizesInfoKHR buildSizes;
    Util::Array<VkAccelerationStructureBuildRangeInfoKHR> rangeInfos;
};

enum
{
    Tlas_Device,
    Tlas_Scene,
    Tlas_Handle,
    Tlas_Buffer,
    Tlas_BuildScratch,
    Tlas_UpdateScratch,
    Tlas_BuildScratchAddr,
    Tlas_UpdateScratchAddr,
};

typedef Ids::IdAllocatorSafe<
    0xFFF
    , VkDevice
    , SceneSetup
    , VkAccelerationStructureKHR
    , CoreGraphics::BufferId
    , CoreGraphics::BufferId
    , CoreGraphics::BufferId
    , VkDeviceAddress
    , VkDeviceAddress
> VkTlasAllocator;
extern VkTlasAllocator tlasAllocator;

/// Get device used to create Tlas
const VkDevice TlasGetVkDevice(const CoreGraphics::TlasId id);
/// Get buffer holding TLAS data
const VkBuffer TlasGetVkBuffer(const CoreGraphics::TlasId id);
/// Get acceleration structure
const VkAccelerationStructureKHR TlasGetVk(const CoreGraphics::TlasId id);
/// Get build info
const VkAccelerationStructureBuildGeometryInfoKHR& TlasGetVkBuild(const CoreGraphics::TlasId id);
/// Get build ranges
const Util::Array<VkAccelerationStructureBuildRangeInfoKHR>& TlasGetVkRanges(const CoreGraphics::TlasId id);

} // namespace Vulkan
