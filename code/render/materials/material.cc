//------------------------------------------------------------------------------
//  material.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "material.h"
#include "shaderconfig.h"
#include "resources/resourceserver.h"

#include "system_shaders/material_interface.h"
namespace Materials
{

MaterialAllocator materialAllocator;
MaterialInstanceAllocator materialInstanceAllocator;
Threading::CriticalSection materialTextureLoadSection;

//------------------------------------------------------------------------------
/**
*/
MaterialId
CreateMaterial(const MaterialCreateInfo& info)
{
    Ids::Id32 id = materialAllocator.Alloc();

    materialAllocator.Set<Material_ShaderConfig>(id, info.config);
    materialAllocator.Set<Material_MinLOD>(id, 1.0f);

    // Resize all arrays to fit the number of batches
    materialAllocator.Get<Material_Table>(id).Resize(info.config->batchToIndexMap.Size()); // surface tables
    materialAllocator.Get<Material_InstanceTables>(id).Resize(info.config->batchToIndexMap.Size()); // instance tables
    materialAllocator.Get<Material_Buffers>(id).Resize(info.config->batchToIndexMap.Size()); // surface buffers
    materialAllocator.Get<Material_InstanceBuffers>(id).Resize(info.config->batchToIndexMap.Size()); // instance buffers
    materialAllocator.Get<Material_Textures>(id).Resize(info.config->batchToIndexMap.Size()); // textures
    materialAllocator.Get<Material_Constants>(id).Resize(info.config->batchToIndexMap.Size()); // constants

    // go through all batches
    auto batchIt = info.config->batchToIndexMap.Begin();
    while (batchIt != info.config->batchToIndexMap.End())
    {
        const CoreGraphics::ShaderProgramId prog = info.config->programs[*batchIt.val];

        // create temporary shader id (this is safe)
        const CoreGraphics::ShaderId shd = { prog.shaderId, prog.shaderType };

        // create resource tables
        CoreGraphics::ResourceTableId surfaceTable = CoreGraphics::ShaderCreateResourceTable(shd, NEBULA_BATCH_GROUP, 256);
        if (surfaceTable != CoreGraphics::InvalidResourceTableId)
            CoreGraphics::ObjectSetName(surfaceTable, Util::String::Sprintf("Material '%s' batch table", info.config->name.AsCharPtr()).AsCharPtr());
        materialAllocator.Get<Material_Table>(id)[*batchIt.val] = surfaceTable;

        auto& instanceTables = materialAllocator.Get<Material_InstanceTables>(id)[*batchIt.val];
        if (ShaderHasResourceTable(shd, NEBULA_INSTANCE_GROUP))
        {
            instanceTables.Resize(CoreGraphics::GetNumBufferedFrames());
            for (auto& table : instanceTables)
            {
                table = CoreGraphics::ShaderCreateResourceTable(shd, NEBULA_INSTANCE_GROUP, 256);
                if (table != CoreGraphics::InvalidResourceTableId)
                    CoreGraphics::ObjectSetName(table, Util::String::Sprintf("Material '%s' instance table", info.config->name.AsCharPtr()).AsCharPtr());
            }
        }

        // get constant buffer count
        SizeT numBuffers = CoreGraphics::ShaderGetConstantBufferCount(shd);

        // get arrays to pre-allocated buffers
        Util::Array<Util::Tuple<IndexT, CoreGraphics::BufferId>>& surfaceBuffers = materialAllocator.Get<Material_Buffers>(id)[*batchIt.val];
        Util::Tuple<IndexT, SizeT>& instanceBuffer = materialAllocator.Get<Material_InstanceBuffers>(id)[*batchIt.val];
        instanceBuffer = Util::MakeTuple(InvalidIndex, 0);

        // create instance of constant buffers
        IndexT j;
        for (j = 0; j < numBuffers; j++)
        {
            IndexT slot = CoreGraphics::ShaderGetConstantBufferResourceSlot(shd, j);
            IndexT group = CoreGraphics::ShaderGetConstantBufferResourceGroup(shd, j);
            if (group == NEBULA_BATCH_GROUP && surfaceTable != CoreGraphics::InvalidResourceTableId)
            {
                CoreGraphics::BufferId buf = CoreGraphics::ShaderCreateConstantBuffer(shd, j);
                if (buf != CoreGraphics::InvalidBufferId)
                {
                    CoreGraphics::ResourceTableSetConstantBuffer(surfaceTable, { buf, slot, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });

                    // add to surface
                    surfaceBuffers.Append(Util::MakeTuple(slot, buf));
                }
            }
            else if (group == NEBULA_INSTANCE_GROUP && !instanceTables.IsEmpty())
            {
                n_assert2(Util::Get<0>(instanceBuffer) == InvalidIndex, "Only one per-instance constant buffer can be present");
                SizeT bufSize = CoreGraphics::ShaderGetConstantBufferSize(shd, j);
                IndexT bufferIndex = 0;
                for (const auto& table : instanceTables)
                    CoreGraphics::ResourceTableSetConstantBuffer(table, { CoreGraphics::GetConstantBuffer(bufferIndex++), slot, 0, bufSize, 0, false, true });

                // add to surface
                instanceBuffer = Util::MakeTuple(slot, bufSize);
            }
        }

        // Setup textures
        const Util::Array<ShaderConfigBatchTexture>& textures = info.config->texturesByBatch[*batchIt.val];
        for (j = 0; j < textures.Size(); j++)
        {
            const ShaderConfigTexture& baseTex = info.config->textures[j];
            const ShaderConfigBatchTexture& tex = textures[j];
            MaterialTexture surTex;
            surTex.slot = tex.slot;
            surTex.defaultValue = baseTex.defaultValue;
            if (tex.slot != InvalidIndex)
                CoreGraphics::ResourceTableSetTexture(surfaceTable, { baseTex.defaultValue, tex.slot, 0, CoreGraphics::InvalidSamplerId, false });

            materialAllocator.Get<Material_Textures>(id)[*batchIt.val].Append(surTex);
        }

        // update tables
        if (surfaceTable != CoreGraphics::InvalidResourceTableId)
            CoreGraphics::ResourceTableCommitChanges(surfaceTable);

        for (const auto& table : instanceTables)
            CoreGraphics::ResourceTableCommitChanges(table);

        const Util::Array<ShaderConfigBatchConstant>& constants = info.config->constantsByBatch[*batchIt.val];
        for (j = 0; j < constants.Size(); j++)
        {
            const ShaderConfigConstant& baseConstant = info.config->constants[j];
            const ShaderConfigBatchConstant& constant = constants[j];
            MaterialConstant surConst;
            surConst.defaultValue = baseConstant.def;
            surConst.binding = constant.offset;
            surConst.bufferIndex = InvalidIndex;
            surConst.instanceConstant = false;
            if (constant.group == NEBULA_BATCH_GROUP)
            {
                surConst.instanceConstant = false;

                // go through surface-level buffers to find slot which matches
                IndexT k;
                for (k = 0; k < surfaceBuffers.Size(); k++)
                {
                    if (Util::Get<0>(surfaceBuffers[k]) == constant.slot)
                    {
                        surConst.bufferIndex = k;
                        surConst.buffer = Util::Get<1>(surfaceBuffers[k]);
                        break;
                    }
                }

                // Make sure to also update the buffer with the default value
                if (baseConstant.def.GetType() == MaterialVariant::Type::TextureHandle)
                    CoreGraphics::BufferUpdate(surConst.buffer, baseConstant.def.Get<MaterialVariant::TextureHandleTuple>().handle, surConst.binding);
                else
                    CoreGraphics::BufferUpdate(surConst.buffer, baseConstant.def.Get(), MaterialVariant::TypeToSize(baseConstant.def.type), surConst.binding);
            }
            else if (constant.group == NEBULA_INSTANCE_GROUP)
            {
                surConst.defaultValue = nullptr;
                surConst.instanceConstant = true;
                surConst.bufferIndex = 0;
                surConst.buffer = CoreGraphics::InvalidBufferId;
            }

            materialAllocator.Get<Material_Constants>(id)[*batchIt.val].Append(surConst);
        }

        batchIt++;
    }

    MaterialId ret;
    ret.resourceId = id;
    ret.resourceType = CoreGraphics::MaterialIdType;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyMaterial(const MaterialId id)
{
    materialAllocator.Dealloc(id.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialSetConstant(const MaterialId mat, IndexT name, const MaterialVariant& value)
{
    ShaderConfig* type = materialAllocator.Get<Material_ShaderConfig>(mat.resourceId);
    auto it = type->batchToIndexMap.Begin();
    while (it != type->batchToIndexMap.End())
    {
        const MaterialConstant& constant = materialAllocator.Get<Material_Constants>(mat.resourceId)[*it.val][name];
        if (constant.buffer != CoreGraphics::InvalidBufferId &&
            constant.binding != UINT_MAX &&
            constant.instanceConstant == false)
        {
            if (value.GetType() == MaterialVariant::Type::TextureHandle)
                CoreGraphics::BufferUpdate(constant.buffer, value.Get<MaterialVariant::TextureHandleTuple>().handle, constant.binding);
            else
                CoreGraphics::BufferUpdate(constant.buffer, value.Get(), MaterialVariant::TypeToSize(value.type), constant.binding);
        }
        it++;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialSetTexture(const MaterialId mat, IndexT name, const CoreGraphics::TextureId tex)
{
    ShaderConfig* type = materialAllocator.Get<Material_ShaderConfig>(mat.resourceId);
    auto it = type->batchToIndexMap.Begin();
    while (it != type->batchToIndexMap.End())
    {
        const MaterialTexture& surTex = materialAllocator.Get<Material_Textures>(mat.resourceId)[*it.val][name];
        if (surTex.slot != InvalidIndex)
            CoreGraphics::ResourceTableSetTexture(materialAllocator.Get<Material_Table>(mat.resourceId)[*it.val], { tex, surTex.slot, 0, CoreGraphics::InvalidSamplerId, false });
        it++;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialSetBufferBinding(const MaterialId id, IndexT index)
{
    materialAllocator.Set<Material_BufferOffset>(id.resourceId, index);
}

//------------------------------------------------------------------------------
/**
*/
IndexT
MaterialGetBufferBinding(const MaterialId id)
{
    return materialAllocator.Get<Material_BufferOffset>(id.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialAddLODTexture(const MaterialId mat, const Resources::ResourceId tex)
{
    Threading::CriticalScope scope(&materialTextureLoadSection);
    materialAllocator.Get<Material_LODTextures>(mat.resourceId).Append(tex);

    // When a new texture is added, make sure to update it's LOD as well
    Resources::SetMinLod(tex, materialAllocator.Get<Material_MinLOD>(mat.resourceId), false);
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialSetLowestLod(const MaterialId mat, float lod)
{
    Threading::CriticalScope scope(&materialTextureLoadSection);
    Util::Array<Resources::ResourceId>& textures = materialAllocator.Get<Material_LODTextures>(mat.resourceId);
    float& minLod = materialAllocator.Get<Material_MinLOD>(mat.resourceId);
    if (minLod <= lod)
        return;
    minLod = lod;

    for (IndexT i = 0; i < textures.Size(); i++)
    {
        Resources::SetMinLod(textures[i], lod, false);
    }
}

//------------------------------------------------------------------------------
/**
*/
ShaderConfig*
MaterialGetShaderConfig(const MaterialId mat)
{
    return materialAllocator.Get<Material_ShaderConfig>(mat.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
Materials::BatchIndex
MaterialGetBatchIndex(const MaterialId mat, const CoreGraphics::BatchGroup::Code code)
{
    return materialAllocator.Get<Material_ShaderConfig>(mat.resourceId)->GetBatchIndex(code);
}

//------------------------------------------------------------------------------
/**
*/
uint64_t
MaterialGetSortCode(const MaterialId mat)
{
    return materialAllocator.Get<Material_ShaderConfig>(mat.resourceId)->HashCode();
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialApply(const MaterialId id, const CoreGraphics::CmdBufferId buf, IndexT index)
{
    CoreGraphics::CmdSetResourceTable(buf, materialAllocator.Get<Material_Table>(id.resourceId)[index], NEBULA_BATCH_GROUP, CoreGraphics::GraphicsPipeline, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
MaterialInstanceId
CreateMaterialInstance(const MaterialId material)
{
    Ids::Id32 inst = materialInstanceAllocator.Alloc();

    // create id
    MaterialInstanceId ret;
    ret.instance = inst;
    ret.materialId = material.resourceId;
    ret.materialType = material.resourceType;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyMaterialInstance(const MaterialInstanceId materialInstance)
{
    materialInstanceAllocator.Dealloc(materialInstance.instance);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ConstantBufferOffset
MaterialInstanceAllocate(const MaterialInstanceId mat, const BatchIndex batch)
{
    const Util::Tuple<IndexT, SizeT>& buffer = materialAllocator.Get<Material_InstanceBuffers>(mat.materialId)[batch];

    SizeT bufferSize = Util::Get<1>(buffer);
    CoreGraphics::ConstantBufferOffset offset = CoreGraphics::AllocateConstantBufferMemory(bufferSize);
    materialInstanceAllocator.Get<MaterialInstance_Offsets>(mat.instance) = offset;
    return offset;
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialInstanceApply(const MaterialInstanceId id, const CoreGraphics::CmdBufferId buf, IndexT index, IndexT bufferIndex)
{
    const auto& tables = materialAllocator.Get<Material_InstanceTables>(id.materialId)[index];
    if (!tables.IsEmpty())
    {
        const CoreGraphics::ResourceTableId table = tables[bufferIndex];
        n_assert(table != CoreGraphics::InvalidResourceTableId);

        // Set instance table
        CoreGraphics::ConstantBufferOffset offset = materialInstanceAllocator.Get<MaterialInstance_Offsets>(id.instance);
        CoreGraphics::CmdSetResourceTable(buf, table, NEBULA_INSTANCE_GROUP, CoreGraphics::GraphicsPipeline, { offset });
    }
}

//------------------------------------------------------------------------------
/**
*/
SizeT
MaterialInstanceBufferSize(const MaterialInstanceId mat, const BatchIndex batch)
{
    const Util::Tuple<IndexT, SizeT>& buffer = materialAllocator.Get<Material_InstanceBuffers>(mat.materialId)[batch];
    return Util::Get<1>(buffer);
}

} // namespace Material
