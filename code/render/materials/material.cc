//------------------------------------------------------------------------------
//  material.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "material.h"
#include "shaderconfig.h"
#include "resources/resourceserver.h"
#include "materials/materialtemplates.h"

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
CreateMaterial(const MaterialTemplates::Entry* entry)
{
    Ids::Id32 id = materialAllocator.Alloc();

    //materialAllocator.Set<Material_ShaderConfig>(id, info.config);
    materialAllocator.Set<Material_MinLOD>(id, 1.0f);

    auto& tablesPerPass = materialAllocator.Get<Material_Table>(id);
    auto& buffersPerPass = materialAllocator.Get<Material_Buffers>(id);
    auto& instanceTablesPerPass = materialAllocator.Get<Material_InstanceTables>(id);
    materialAllocator.Set<Material_Template>(id, entry);

    // Resize all arrays to fit the number of batches
    tablesPerPass.Resize(entry->passes.Size()); // surface tables
    buffersPerPass.Resize(entry->passes.Size()); // surface buffers
    instanceTablesPerPass.Resize(entry->passes.Size()); // instance tables

    // Go through passes
    for (auto pass : entry->passes)
    {
        const CoreGraphics::ShaderId shader = pass.Value()->shader;
        const CoreGraphics::ShaderProgramId prog = pass.Value()->program;
        const IndexT passIndex = pass.Value()->index;

        // create resource tables
        CoreGraphics::ResourceTableId surfaceTable = CoreGraphics::ShaderCreateResourceTable(shader, NEBULA_BATCH_GROUP, 256);
        if (surfaceTable != CoreGraphics::InvalidResourceTableId)
            CoreGraphics::ObjectSetName(surfaceTable, Util::String::Sprintf("Material '%s' pass table", entry->name).AsCharPtr());
        tablesPerPass[passIndex] = surfaceTable;

        auto& instanceTables = instanceTablesPerPass[passIndex];
        if (ShaderHasResourceTable(shader, NEBULA_INSTANCE_GROUP))
        {
            instanceTables.Resize(CoreGraphics::GetNumBufferedFrames());
            for (auto& table : instanceTables)
            {
                table = CoreGraphics::ShaderCreateResourceTable(shader, NEBULA_INSTANCE_GROUP, 256);
                if (table != CoreGraphics::InvalidResourceTableId)
                    CoreGraphics::ObjectSetName(table, Util::String::Sprintf("Material '%s' instance table", entry->name).AsCharPtr());
            }
        }

        uint it = 0;
        uint64 batchGroupMask = CoreGraphics::ShaderGetConstantBufferBindingMask(shader, NEBULA_BATCH_GROUP);
        uint64 currentMask = batchGroupMask;
        uint numBuffers = Util::PopCnt(batchGroupMask);
        buffersPerPass[passIndex].Resize(numBuffers);

        while (currentMask != 0)
        {
            if (AllBits(currentMask, 1 << it))
            {
                IndexT slot = it;
				uint bufferIndex = CoreGraphics::ShaderCalculateConstantBufferIndex(batchGroupMask, it);
                if (bufferIndex != 0xFFFFFFFF)
                {
                    CoreGraphics::BufferId buf = CoreGraphics::ShaderCreateConstantBuffer(shader, NEBULA_BATCH_GROUP, bufferIndex);

                    // Calculate the index by counting the active bits at the point of the iterator
                    buffersPerPass[passIndex][bufferIndex] = Util::MakePair(slot, buf);

                    if (buf != CoreGraphics::InvalidBufferId)
                    {
                        CoreGraphics::ResourceTableSetConstantBuffer(surfaceTable, { buf, slot, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
                    }
                }
            }
            currentMask &= ~(1 << it);
            it++;
        }

        uint64 instanceGroupMask = CoreGraphics::ShaderGetConstantBufferBindingMask(shader, NEBULA_INSTANCE_GROUP);
        it = 0;
        while (instanceGroupMask != 0)
        {
            if (AllBits(instanceGroupMask, 1 << it))
            {
                IndexT slot = it;
                SizeT bufSize = CoreGraphics::ShaderGetConstantBufferSize(shader, NEBULA_INSTANCE_GROUP, CoreGraphics::ShaderCalculateConstantBufferIndex(batchGroupMask, it));
                IndexT bufferIndex = 0;
                for (const auto& table : instanceTables)
                    CoreGraphics::ResourceTableSetConstantBuffer(table, { CoreGraphics::GetConstantBuffer(bufferIndex++), slot, 0, bufSize, 0, false, true });
            }
            instanceGroupMask &= ~(1 << it);
            it++;
        }

        // Bind textures to surface table
        const Util::Array<ShaderConfigBatchTexture*>& textures = entry->texturesPerBatch[passIndex];
        for (auto& texture : textures)
        {
            if (texture->slot != InvalidIndex)
            {
                CoreGraphics::TextureId tex = Resources::CreateResource(texture->def->data.resource, "materials", nullptr, nullptr, true, false);
                CoreGraphics::ResourceTableSetTexture(surfaceTable, { tex, texture->slot });
            }            
        }

        // Update constant buffers with default values
        const Util::Array<ShaderConfigBatchConstant*>& constants = entry->constantsPerBatch[passIndex];
        for (auto& constant : constants)
        {
            if (constant->group == NEBULA_BATCH_GROUP)
            {
                // Get the buffer index by using the constant slot
                uint bufferIndex = CoreGraphics::ShaderCalculateConstantBufferIndex(batchGroupMask, constant->slot);

                CoreGraphics::BufferId buffer = buffersPerPass[passIndex][bufferIndex].second;
                if (constant->def->type != MaterialTemplateValue::Type::BindlessResource)
                    CoreGraphics::BufferUpdate(buffer, &constant->def->data, constant->def->GetSize(), constant->offset);
                else
                {
                    CoreGraphics::TextureId tex = Resources::CreateResource(constant->def->data.resource, "materials", nullptr, nullptr, true, false);
                    CoreGraphics::TextureIdLock _0(tex);
                    uint handle = CoreGraphics::TextureGetBindlessHandle(tex);
                    CoreGraphics::BufferUpdate(buffer, &handle, constant->def->GetSize(), constant->offset);
                }
            }
        }

        // Finish off by comitting all table changes
        if (surfaceTable != CoreGraphics::InvalidResourceTableId)
            CoreGraphics::ResourceTableCommitChanges(surfaceTable);

        for (const auto& table : instanceTables)
            CoreGraphics::ResourceTableCommitChanges(table);
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
MaterialSetConstant(const MaterialId mat, const ShaderConfigBatchConstant* bind, const MaterialVariant& value)
{
    const MaterialTemplates::Entry* temp = materialAllocator.Get<Material_Template>(mat.resourceId);
    const auto& buffersPerPass = materialAllocator.Get<Material_Buffers>(mat.resourceId);

    for (const auto& pass : temp->passes)
    {
        uint64 batchGroupMask = CoreGraphics::ShaderGetConstantBufferBindingMask(pass.Value()->shader, NEBULA_BATCH_GROUP);
        uint bufferIndex = CoreGraphics::ShaderCalculateConstantBufferIndex(batchGroupMask, bind->slot);
        if (bufferIndex != 0xFFFFFFFF)
        {
            CoreGraphics::BufferId buf = buffersPerPass[pass.Value()->index][bufferIndex].second;
            CoreGraphics::BufferUpdate(buf, value.Get(), value.size, bind->offset);
        }        
    }
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialSetTexture(const MaterialId mat, const ShaderConfigBatchTexture* bind, const CoreGraphics::TextureId tex)
{
    const MaterialTemplates::Entry* temp = materialAllocator.Get<Material_Template>(mat.resourceId);
    for (const auto& pass : temp->passes)
    {
        CoreGraphics::ResourceTableId table = materialAllocator.Get<Material_Table>(mat.resourceId)[pass.Value()->index];
        CoreGraphics::ResourceTableSetTexture(table, { tex, bind->slot, 0, CoreGraphics::InvalidSamplerId, false });
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
    return Ids::Index(materialAllocator.Get<Material_BufferOffset>(id.resourceId));
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
const MaterialTemplates::Entry*
MaterialGetTemplate(const MaterialId mat)
{
    return materialAllocator.Get<Material_Template>(mat.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
const Materials::BatchIndex
MaterialGetBatchIndex(const MaterialId mat, const CoreGraphics::BatchGroup::Code code)
{
    return materialAllocator.Get<Material_Template>(mat.resourceId)->passes[code]->index;
}

//------------------------------------------------------------------------------
/**
*/
uint64_t
MaterialGetSortCode(const MaterialId mat)
{
    return materialAllocator.Get<Material_Template>(mat.resourceId)->uniqueId;
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
