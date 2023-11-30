#pragma once
//------------------------------------------------------------------------------
/**
    Acceleration structures are used to enable ray tracing on the GPU by dividing 
    the scene into a BVH. 

    @copyright
    (C) 2023 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "mesh.h"
namespace CoreGraphics
{

enum class AccelerationBuildFlags
{
    FastBuild = 0x1, // Create and recreate structure swiftly but suffer worse trace performance
    FastTrace = 0x2, // Vice versa of FastBuild, and is exclusive of it
    Dynamic = 0x4,   // Allow updates
    Compact = 0x8,   // Hint to make the structure compact, which can make copys faster
    Small = 0x10,    // Hint to minimize memory spent
};
__ImplementEnumBitOperators(AccelerationBuildFlags);

struct BottomLevelAccelerationCreateInfo
{
    CoreGraphics::MeshId mesh;
};


ID_24_8_TYPE(BottomLevelAccelerationId);

/// Create bottom level acceleration structure
BottomLevelAccelerationId CreateBottomLevelAcceleration(const BottomLevelAccelerationCreateInfo& info);

/// Build bottom level acceleration structure on CPU
void BottomLevelAccelerationBuild(const BottomLevelAccelerationId blac, const AccelerationBuildFlags flags);


struct TopLevelAccelerationCreateInfo
{
    Util::Array<BottomLevelAccelerationId> geometries;
};

ID_24_8_TYPE(TopLevelAccelerationId);

/// Create top level acceleration structure
TopLevelAccelerationId CreateTopLevelAcceleration(const TopLevelAccelerationCreateInfo& info);

/// Build top level acceleration structure on the CPU
void TopLevelAccelerationBuild(const TopLevelAccelerationId tlac, const AccelerationBuildFlags flags);

} // namespace CoreGraphics
