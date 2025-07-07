//------------------------------------------------------------------------------
//  material.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "material.h"
#include "shaderconfig.h"
#include "resources/resourceserver.h"
#include "materials/materialtemplates.h"

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
    auto& instanceTablesPerPass = materialAllocator.Get<Material_InstanceTables>(id);
    auto& buffer = materialAllocator.Get<Material_Buffer>(id);
    materialAllocator.Set<Material_Template>(id, entry);

    // Resize all arrays to fit the number of batches
    tablesPerPass.Resize(entry->passes.Size()); // surface tables
    instanceTablesPerPass.Resize(entry->passes.Size()); // instance tables

    // Create material buffer
    CoreGraphics::BufferCreateInfo bufInfo;
    bufInfo.usageFlags = CoreGraphics::BufferUsageFlag::ConstantBuffer;
    bufInfo.byteSize = entry->bufferSize;
    bufInfo.mode = CoreGraphics::BufferAccessMode::HostCached;
    bufInfo.name = entry->bufferName;
    buffer = CoreGraphics::CreateBuffer(bufInfo);

    for (IndexT i = 0; i < entry->textures.Size(); i++)
    {
        auto& value = entry->textures.ValueAtIndex(i);
        if (value->bindlessOffset != -1)
        {
            Resources::ResourceId id = Resources::CreateResource(value->resource, "materials",
                [buffer, value](Resources::ResourceId id)
            {
                CoreGraphics::TextureIdLock _0(id);
                uint handle = CoreGraphics::TextureGetBindlessHandle(id);
                CoreGraphics::BufferUpdate(buffer, &handle, sizeof(handle), value->bindlessOffset);
                CoreGraphics::BufferFlush(buffer);
            });

            CoreGraphics::TextureIdLock _0(id);
            uint handle = CoreGraphics::TextureGetBindlessHandle(id);
            CoreGraphics::BufferUpdate(buffer, &handle, sizeof(handle), value->bindlessOffset);
            break;
        }
    }
    for (IndexT i = 0; i < entry->values.Size(); i++)
    {
        auto& value = entry->values.ValueAtIndex(i);
        CoreGraphics::BufferUpdate(buffer, &value->data, value->GetSize(), value->offset);
    }
    CoreGraphics::BufferFlush(buffer);

#ifdef WITH_NEBULA_EDITOR
    Util::Array<Resources::ResourceId>& textures = materialAllocator.Get<Material_TextureValues>(id);

    textures.Resize(entry->numTextures);
    int numTextures = 0;
    for (IndexT i = 0; i < entry->textures.Size(); i++)
    {
        auto& kvp = entry->textures.KeyValuePairAtIndex(i);
        Resources::ResourceId res = Resources::CreateResource(kvp.Value()->resource, "materials", [i, textures](Resources::ResourceId id)
        {
            textures[i] = id;
        });
        textures[i] = res;
        break;
    }
#endif

    // Go through passes
    for (auto pass : entry->passes)
    {
        const CoreGraphics::ShaderId shader = pass.Value()->shader;
        const CoreGraphics::ShaderProgramId prog = pass.Value()->program;
        const IndexT passIndex = pass.Value()->index;

        // create resource tables
        CoreGraphics::ResourceTableId surfaceTable = CoreGraphics::ShaderCreateResourceTable(shader, NEBULA_BATCH_GROUP, 256);
        if (surfaceTable != CoreGraphics::InvalidResourceTableId)
            CoreGraphics::ObjectSetName(surfaceTable, Util::String::Sprintf("Material '%s' pass '%s' table", entry->name, pass.Value()->name).AsCharPtr());
        tablesPerPass[passIndex] = surfaceTable;

        CoreGraphics::ResourceTableSetConstantBuffer(surfaceTable, { materialAllocator.Get<Material_Buffer>(id), MaterialInterfaces::MaterialBufferSlot, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });

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

        uint64_t instanceGroupMask = CoreGraphics::ShaderGetConstantBufferBindingMask(shader, NEBULA_INSTANCE_GROUP);
        uint it = 0;
        while (instanceGroupMask != 0)
        {
            if (AllBits(instanceGroupMask, 1 << it))
            {
                IndexT slot = it;
                uint64_t bufSize = CoreGraphics::ShaderGetConstantBufferSize(shader, NEBULA_INSTANCE_GROUP, CoreGraphics::ShaderCalculateConstantBufferIndex(instanceGroupMask, it));
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
                CoreGraphics::TextureId tex = Resources::CreateResource(texture->def->resource, "materials", nullptr, nullptr, true, false);
                CoreGraphics::ResourceTableSetTexture(surfaceTable, { tex, texture->slot });
            }
        }

        // Finish off by comitting all table changes
        if (surfaceTable != CoreGraphics::InvalidResourceTableId)
            CoreGraphics::ResourceTableCommitChanges(surfaceTable);

        for (const auto& table : instanceTables)
            CoreGraphics::ResourceTableCommitChanges(table);
    }

    MaterialId ret = id;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyMaterial(const MaterialId id)
{
    materialAllocator.Dealloc(id.id);
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialSetTexture(const MaterialId mat, const ShaderConfigBatchTexture* bind, const Resources::ResourceId tex)
{
    const MaterialTemplates::Entry* temp = materialAllocator.Get<Material_Template>(mat.id);

#ifdef WITH_NEBULA_EDITOR
    Util::Array<Resources::ResourceId>& textures = materialAllocator.Get<Material_TextureValues>(mat.id);
    textures[bind->def->textureIndex] = tex;
#endif

    for (const auto& pass : temp->passes)
    {
        CoreGraphics::ResourceTableId table = materialAllocator.Get<Material_Table>(mat.id)[pass.Value()->index];
        CoreGraphics::ResourceTableSetTexture(table, { tex, bind->slot, 0, CoreGraphics::InvalidSamplerId, false });
    }
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialSetTexture(const MaterialId mat, uint name, const Resources::ResourceId tex)
{
    const MaterialTemplates::Entry* temp = materialAllocator.Get<Material_Template>(mat.id);

#ifdef WITH_NEBULA_EDITOR
    Util::Array<Resources::ResourceId>& textures = materialAllocator.Get<Material_TextureValues>(mat.id);
    textures[temp->texturesByHash[name]->textureIndex] = tex;
#endif

    for (IndexT i = 0; i < temp->passes.Size(); i++)
    {
        const auto& pass = temp->passes.KeyValuePairAtIndex(i);
        uint textureIndex = temp->textureBatchLookup[pass.Value()->index].FindIndex(name);
        const auto texture = temp->texturesPerBatch[pass.Value()->index][temp->textureBatchLookup[pass.Value()->index].ValueAtIndex(textureIndex)];
        if (texture->slot != InvalidIndex)
        {
            CoreGraphics::ResourceTableId table = materialAllocator.Get<Material_Table>(mat.id)[pass.Value()->index];
            CoreGraphics::ResourceTableSetTexture(table, { tex, texture->slot, 0, CoreGraphics::InvalidSamplerId, false });
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialSetTextureBindless(const MaterialId mat, uint name, const uint handle, const uint offset, const Resources::ResourceId tex)
{

#ifdef WITH_NEBULA_EDITOR
    const MaterialTemplates::Entry* temp = materialAllocator.Get<Material_Template>(mat.id);
    Util::Array<Resources::ResourceId>& textures = materialAllocator.Get<Material_TextureValues>(mat.id);
    textures[temp->texturesByHash[name]->textureIndex] = tex;
#endif

    const auto& buf = materialAllocator.Get<Material_Buffer>(mat.id);
    CoreGraphics::BufferUpdate(buf, &handle, sizeof(handle), offset);
    CoreGraphics::BufferFlush(buf);
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialSetConstants(const MaterialId mat, const void* data, const uint size)
{
    const auto& buf = materialAllocator.Get<Material_Buffer>(mat.id);
    CoreGraphics::BufferUpdate(buf, data, size);
    CoreGraphics::BufferFlush(buf);

    IndexT materialBufferBinding = Materials::MaterialGetBufferBinding(mat);
    const MaterialBindlessBufferBinding& bindlessBinding = Materials::MaterialGetBindlessForEditor(mat);
    if (bindlessBinding.buffer != nullptr)
    {
        memcpy(bindlessBinding.buffer, data, size);
        *bindlessBinding.dirtyFlag = true;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialSetConstant(const MaterialId mat, const void* data, const uint size, const uint offset)
{
    const auto& buf = materialAllocator.Get<Material_Buffer>(mat.id);
    CoreGraphics::BufferUpdate(buf, data, size, offset);
    CoreGraphics::BufferFlush(buf);
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialSetBufferBinding(const MaterialId id, IndexT index)
{
    materialAllocator.Set<Material_BufferOffset>(id.id, index);
}

//------------------------------------------------------------------------------
/**
*/
IndexT
MaterialGetBufferBinding(const MaterialId id)
{
    return Ids::Index(materialAllocator.Get<Material_BufferOffset>(id.id));
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialAddLODTexture(const MaterialId mat, const Resources::ResourceId tex)
{
    Threading::CriticalScope scope(&materialTextureLoadSection);
    materialAllocator.Get<Material_LODTextures>(mat.id).Append(tex);

    // When a new texture is added, make sure to update it's LOD as well
    Resources::SetMinLod(tex, materialAllocator.Get<Material_MinLOD>(mat.id), false);
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialSetLowestLod(const MaterialId mat, float lod)
{
    Threading::CriticalScope scope(&materialTextureLoadSection);
    Util::Array<Resources::ResourceId>& textures = materialAllocator.Get<Material_LODTextures>(mat.id);
    float& minLod = materialAllocator.Get<Material_MinLOD>(mat.id);
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
    return materialAllocator.Get<Material_Template>(mat.id);
}

//------------------------------------------------------------------------------
/**
*/
const Materials::BatchIndex
MaterialGetBatchIndex(const MaterialId mat, const MaterialTemplates::BatchGroup batch)
{
    return materialAllocator.Get<Material_Template>(mat.id)->passes[batch]->index;
}

//------------------------------------------------------------------------------
/**
*/
uint64_t
MaterialGetSortCode(const MaterialId mat)
{
    return materialAllocator.Get<Material_Template>(mat.id)->uniqueId;
}

#ifdef WITH_NEBULA_EDITOR
//------------------------------------------------------------------------------
/**
*/
void
MaterialBindlessForEditor(const MaterialId mat, char* buf, bool* dirtyFlag)
{
    MaterialBindlessBufferBinding binding{buf, dirtyFlag};
    materialAllocator.Set<Material_BufferPointer>(mat.id, binding);
}

//------------------------------------------------------------------------------
/**
*/
const MaterialBindlessBufferBinding&
MaterialGetBindlessForEditor(const MaterialId mat)
{
    return materialAllocator.Get<Material_BufferPointer>(mat.id);
}

//------------------------------------------------------------------------------
/**
*/
ubyte*
MaterialGetConstants(const MaterialId mat)
{
    return (ubyte*)CoreGraphics::BufferMap(materialAllocator.Get<Material_Buffer>(mat.id));
}

//------------------------------------------------------------------------------
/**
*/
const Resources::ResourceId
MaterialGetTexture(const MaterialId mat, const IndexT i)
{
    return materialAllocator.Get<Material_TextureValues>(mat.id)[i];
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialInvalidate(const MaterialId mat)
{
    auto& buf = materialAllocator.Get<Material_Buffer>(mat.id);
    CoreGraphics::BufferFlush(buf);
}
#endif

//------------------------------------------------------------------------------
/**
*/
void
MaterialApply(const MaterialId id, const CoreGraphics::CmdBufferId buf, IndexT index)
{
    CoreGraphics::CmdSetResourceTable(buf, materialAllocator.Get<Material_Table>(id.id)[index], NEBULA_BATCH_GROUP, CoreGraphics::GraphicsPipeline, nullptr);
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
    ret.material = material.id;
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
        n_assert((offset & 0xFFFFFFFF00000000) == 0x0);
        CoreGraphics::CmdSetResourceTable(buf, table, NEBULA_INSTANCE_GROUP, CoreGraphics::GraphicsPipeline, { (uint)offset });
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
