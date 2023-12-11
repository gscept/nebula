
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

enum class AccelerationStructureBuildFlags
{
    FastBuild = 0x1, // Create and recreate structure swiftly but suffer worse trace performance
    FastTrace = 0x2, // Vice versa of FastBuild, and is exclusive of it
    Dynamic = 0x4,   // Allow updates
    Compact = 0x8,   // Hint to make the structure compact, which can make copys faster
    Small = 0x10,    // Hint to minimize memory spent
};
__ImplementEnumBitOperators(AccelerationStructureBuildFlags);
__ImplementEnumComparisonOperators(AccelerationStructureBuildFlags);

struct BlasCreateInfo
{
    CoreGraphics::MeshId mesh;
    AccelerationStructureBuildFlags flags;
};

ID_24_8_TYPE(BlasId);

/// Create bottom level acceleration structure
BlasId CreateBlas(const BlasCreateInfo& info);
/// Destroy bottom level acceleration structure
void DestroyBlas(const BlasId blac);

enum BlasInstanceFlags
{
    NoFlags = 0x0,
    FaceCullingDisabled = 0x1,
    InvertFace = 0x2,
    ForceOpaque = 0x4,
    NoOpaque = 0x8
};
__ImplementEnumBitOperators(BlasInstanceFlags);

ID_24_8_TYPE(BlasInstanceId);
_DECL_ACQUIRE_RELEASE(BlasInstanceId);
struct BlasInstanceCreateInfo
{
    CoreGraphics::BlasId blas;
    Math::mat4 transform;
    uint instanceIndex;         // Readable in the shader as gl_InstanceCustomIndexKHR
    uint mask;                  // 8 bit visibility mask
    uint shaderOffset;          // Offset into the shader binding table
    BlasInstanceFlags flags;

    CoreGraphics::BufferId buffer; // The buffer to hold the instance being created
    uint offset;                   // The offset into the buffer to where instances are created
};

/// Create an instance to a bottom level acceleration structure
BlasInstanceId CreateBlasInstance(const BlasInstanceCreateInfo& info);
/// Destroy blas instance
void DestroyBlasInstance(const BlasInstanceId id);
/// Set transform of blas
void BlasInstanceSetTransform(const BlasInstanceId id, const Math::mat4& transform);
/// Get instance size (platform dependent)
const SizeT BlasInstanceGetSize();

struct TlasCreateInfo
{
    SizeT numInstances;
    CoreGraphics::BufferId instanceBuffer;
    AccelerationStructureBuildFlags flags;
};

ID_24_8_TYPE(TlasId);

/// Create top level acceleration structure
TlasId CreateTlas(const TlasCreateInfo& info);
/// Destroy top level acceleration structure
void DestroyTlas(const TlasId tlas);

/// Initiate Tlas for build
void TlasInitBuild(const TlasId tlas);
/// Initiate Tlas for update
void TlasInitUpdate(const TlasId tlas);

} // namespace CoreGraphics
