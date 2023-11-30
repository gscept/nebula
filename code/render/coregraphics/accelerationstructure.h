#pragma once
//------------------------------------------------------------------------------
/**
    Acceleration structures are used to enable ray tracing on the GPU by dividing 
    the scene into a BVH. 

    The structure is split into two objects, bottom level meant for geometry data,
    and top level meant for scene representation. 

    Bottom level acceleration structures are setup with pointers to geometry data
    and should update if Build is called after vertices are transformed, assuming that the
    transformation of the vertices pointed to by the mesh are actually written to the buffer.

    Top level acceleration structures are setup with references to bottom level structures, 
    and can be updated every frame, or as needed. 

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
__ImplementEnumComparisonOperators(AccelerationBuildFlags);

struct BottomLevelAccelerationCreateInfo
{
    CoreGraphics::MeshId mesh;
    AccelerationBuildFlags flags;
};

ID_24_8_TYPE(BottomLevelAccelerationId);

/// Create bottom level acceleration structure
BottomLevelAccelerationId CreateBottomLevelAcceleration(const BottomLevelAccelerationCreateInfo& info);
/// Destroy bottom level acceleration structure
void DestroyBottomLevelAcceleration(const BottomLevelAccelerationId blac);

/// Build bottom level acceleration structure on CPU
void BottomLevelAccelerationBuild(const BottomLevelAccelerationId blac);


struct TopLevelAccelerationCreateInfo
{
    Util::Array<BottomLevelAccelerationId> geometries;
    AccelerationBuildFlags flags;
};

ID_24_8_TYPE(TopLevelAccelerationId);

/// Create top level acceleration structure
TopLevelAccelerationId CreateTopLevelAcceleration(const TopLevelAccelerationCreateInfo& info);
/// Destroy top level acceleration structure
void DestroyTopLevelAcceleration(const TopLevelAccelerationId tlac);

/// Build top level acceleration structure on the CPU
void TopLevelAccelerationBuild(const TopLevelAccelerationId tlac);

} // namespace CoreGraphics
