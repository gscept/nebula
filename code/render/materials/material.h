#pragma once
//------------------------------------------------------------------------------
/**
    Materials represent a set of settings and a correlated shader configuration,
    which tells the engine which shader to use and how to apply the constants and textures
    on each respective shader

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idallocator.h"
#include "resources/resourceid.h"
#include "coregraphics/batchgroup.h"
#include "coregraphics/texture.h"
#include "coregraphics/buffer.h"
#include "materialvariant.h"
#include <functional>
namespace Materials
{

class ShaderConfig;
RESOURCE_ID_TYPE(MaterialId);
ID_32_24_8_NAMED_TYPE(MaterialInstanceId, instance, materialId, materialType); // 32 bits instance, 24 bits material, 8 bits type

struct MaterialCreateInfo
{
    ShaderConfig* config;
};
typedef IndexT BatchIndex;


/// Create material
MaterialId CreateMaterial(const MaterialCreateInfo& info);
/// Destroy material
void DestroyMaterial(const MaterialId id);

/// Set constant
void MaterialSetConstant(const MaterialId mat, IndexT name, const MaterialVariant& value);
/// Set texture
void MaterialSetTexture(const MaterialId mat, IndexT name, const CoreGraphics::TextureId tex);

/// Add texture to LOD update
void MaterialAddLODTexture(const MaterialId mat, const Resources::ResourceId tex);
/// Update LOD for material
void MaterialSetLowestLod(const MaterialId mat, float lod);

/// Apply material
void MaterialApply(const MaterialId id, const CoreGraphics::CmdBufferId buf, IndexT index);

/// Get material shader config
ShaderConfig* MaterialGetShaderConfig(const MaterialId mat);
/// Get shader config batch index
Materials::BatchIndex MaterialGetBatchIndex(const MaterialId mat, const CoreGraphics::BatchGroup::Code code);
/// Get sort code
uint64_t MaterialGetSortCode(const MaterialId mat);

struct MaterialConstant
{
    MaterialVariant defaultValue;
    IndexT bufferIndex;
    bool instanceConstant : 1;
    IndexT binding;
    CoreGraphics::BufferId buffer;

    MaterialConstant()
        : buffer(CoreGraphics::InvalidBufferId)
        //, mem(nullptr)
    {}
};

struct MaterialTexture
{
    CoreGraphics::TextureId defaultValue;
    IndexT slot;
};

enum MaterialMembers
{
    Material_ShaderConfig,
    Material_MinLOD,
    Material_LODTextures,
    Material_Table,
    Material_InstanceTables,
    Material_Buffers,
    Material_InstanceBuffers,
    Material_Textures,
    Material_Constants,
};


typedef Ids::IdAllocator<
    ShaderConfig*,
    float,
    Util::Array<Resources::ResourceId>,
    Util::FixedArray<CoreGraphics::ResourceTableId>,                                // surface level resource table, mapped batch -> table
    Util::FixedArray<Util::FixedArray<CoreGraphics::ResourceTableId>>,              // instance level resource table, mapped batch -> table
    Util::FixedArray<Util::Array<Util::Tuple<IndexT, CoreGraphics::BufferId>>>,     // surface level constant buffers, mapped batch -> buffers
    Util::FixedArray<Util::Tuple<IndexT, SizeT>>,                                   // instance level instance buffer, mapped batch -> memory + size
    Util::FixedArray<Util::Array<MaterialTexture>>,                                 // textures
    Util::FixedArray<Util::Array<MaterialConstant>>                                 // constants
> MaterialAllocator;
extern MaterialAllocator materialAllocator;

/// Create material instance
MaterialInstanceId CreateMaterialInstance(const MaterialId material);
/// Destroy material instance
void DestroyMaterialInstance(const MaterialInstanceId materialInstance);

/// Allocate instance constants, call per frame when instance constants are needed
CoreGraphics::ConstantBufferOffset MaterialInstanceAllocate(const MaterialInstanceId mat, const BatchIndex batch);
/// Apply material instance
void MaterialInstanceApply(const MaterialInstanceId id, const CoreGraphics::CmdBufferId buf, IndexT index, IndexT bufferIndex);


/// Get material instance buffer size for batch
SizeT MaterialInstanceBufferSize(const MaterialInstanceId sur, const BatchIndex batch);

enum MaterialInstanceMembers
{
    MaterialInstance_Offsets
};
typedef Ids::IdAllocator<
    uint
> MaterialInstanceAllocator;
extern MaterialInstanceAllocator materialInstanceAllocator;

} // namespace Materials
