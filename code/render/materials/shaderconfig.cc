//------------------------------------------------------------------------------
//  materialtype.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "shaderconfig.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/shader.h"
#include "coregraphics/config.h"
#include "coregraphics/resourcetable.h"
namespace Materials
{

IndexT ShaderConfig::ShaderConfigUniqueIdCounter = 0;
//------------------------------------------------------------------------------
/**
*/
ShaderConfig::ShaderConfig() 
    : currentBatch(CoreGraphics::BatchGroup::InvalidBatchGroup)
    , currentBatchIndex(InvalidIndex)
    , vertexType(-1)
    , isVirtual(false)
    , uniqueId(ShaderConfigUniqueIdCounter++)
{
}

//------------------------------------------------------------------------------
/**
*/
ShaderConfig::~ShaderConfig()
{
}

//------------------------------------------------------------------------------
/**
*/
void 
ShaderConfig::Setup()
{
    // setup binding in each program (should be identical)
    auto it = this->batchToIndexMap.Begin();
    while (it != this->batchToIndexMap.End())
    {
        const CoreGraphics::ShaderId shd = this->programs[*it.val].As<CoreGraphics::ShaderId>();
        IndexT i;
        for (i = 0; i < this->textures.Size(); i++)
        {
            const ShaderConfigTexture& tex = this->textures[i];
            IndexT slot = CoreGraphics::ShaderGetResourceSlot(shd, tex.name.AsCharPtr());
            if (slot != InvalidIndex)
            {
                ShaderConfigBatchTexture batchTex;
                batchTex.slot = slot;
                this->texturesByBatch[*it.val].Append(batchTex);
            }
            else
            {
                this->texturesByBatch[*it.val].Append({ InvalidIndex });
            }
        }

        for (i = 0; i < this->constants.Size(); i++)
        {
            const ShaderConfigConstant& constant = this->constants[i];
            IndexT slot = CoreGraphics::ShaderGetConstantSlot(shd, constant.name);
            
            // only bind if there is a binding
            if (slot != -1)
            {
                ShaderConfigBatchConstant batchConstant;
                batchConstant.slot = slot;
                batchConstant.offset = CoreGraphics::ShaderGetConstantBinding(shd, constant.name.AsCharPtr());
                batchConstant.group = CoreGraphics::ShaderGetConstantGroup(shd, constant.name);
                this->constantsByBatch[*it.val].Append(batchConstant);
            }
            else
            {
                this->constantsByBatch[*it.val].Append({ InvalidIndex, InvalidIndex, InvalidIndex });
            }
        }

        it++;
    }
}

//------------------------------------------------------------------------------
/**
*/
MaterialId
ShaderConfig::CreateMaterial()
{
    Ids::Id32 sur = this->materialAllocator.Alloc();

    // resize all arrays
    this->materialAllocator.Get<MaterialTable>(sur).Resize(this->batchToIndexMap.Size()); // surface tables
    this->materialAllocator.Get<InstanceTable>(sur).Resize(this->batchToIndexMap.Size()); // instance tables
    this->materialAllocator.Get<MaterialBuffers>(sur).Resize(this->batchToIndexMap.Size()); // surface buffers
    this->materialAllocator.Get<InstanceBuffers>(sur).Resize(this->batchToIndexMap.Size()); // instance buffers
    this->materialAllocator.Get<Textures>(sur).Resize(this->batchToIndexMap.Size()); // textures
    this->materialAllocator.Get<Constants>(sur).Resize(this->batchToIndexMap.Size()); // constants

    // go through all batches
    auto batchIt = this->batchToIndexMap.Begin();
    while (batchIt != this->batchToIndexMap.End())
    {
        const CoreGraphics::BatchGroup::Code code = *batchIt.key;
        const CoreGraphics::ShaderProgramId prog = this->programs[*batchIt.val];

        // create temporary shader id (this is safe)
        const CoreGraphics::ShaderId shd = prog.As<const CoreGraphics::ShaderId>();

        // create resource tables
        CoreGraphics::ResourceTableId surfaceTable = CoreGraphics::ShaderCreateResourceTable(shd, NEBULA_BATCH_GROUP, 256);
        if (surfaceTable != CoreGraphics::InvalidResourceTableId)
            CoreGraphics::ObjectSetName(surfaceTable, Util::String::Sprintf("Material '%s' batch table", this->name.AsCharPtr()).AsCharPtr());
        this->materialAllocator.Get<MaterialTable>(sur)[*batchIt.val] = surfaceTable;

        CoreGraphics::ResourceTableId instanceTable = CoreGraphics::ShaderCreateResourceTable(shd, NEBULA_INSTANCE_GROUP, 256);
        if (instanceTable != CoreGraphics::InvalidResourceTableId)
            CoreGraphics::ObjectSetName(instanceTable, Util::String::Sprintf("Material '%s' instance table", this->name.AsCharPtr()).AsCharPtr());
        this->materialAllocator.Get<InstanceTable>(sur)[*batchIt.val] = instanceTable;
        
        // get constant buffer count
        SizeT numBuffers = CoreGraphics::ShaderGetConstantBufferCount(shd);

        // get arrays to pre-allocated buffers
        Util::Array<Util::Tuple<IndexT, CoreGraphics::BufferId>>& surfaceBuffers = this->materialAllocator.Get<MaterialBuffers>(sur)[*batchIt.val];
        Util::Tuple<IndexT, SizeT>& instanceBuffer = this->materialAllocator.Get<InstanceBuffers>(sur)[*batchIt.val];
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
                    CoreGraphics::ResourceTableSetConstantBuffer(surfaceTable, { buf, slot, 0, false, false, -1, 0 });

                    // add to surface
                    surfaceBuffers.Append(Util::MakeTuple(slot, buf));
                }
            }			
            else if (group == NEBULA_INSTANCE_GROUP && instanceTable != CoreGraphics::InvalidResourceTableId)
            {
                n_assert2(Util::Get<0>(instanceBuffer) == InvalidIndex, "Only one per-instance constant buffer can be present");
                CoreGraphics::BufferId buf = CoreGraphics::GetGraphicsConstantBuffer();
                if (buf != CoreGraphics::InvalidBufferId)
                {
                    SizeT bufSize = CoreGraphics::ShaderGetConstantBufferSize(shd, j);
                    CoreGraphics::ResourceTableSetConstantBuffer(instanceTable, { buf, slot, 0, false, true, bufSize, 0 });

                    // add to surface
                    instanceBuffer = Util::MakeTuple(slot, bufSize);
                }
            }
        }

        // Setup textures
        const Util::Array<ShaderConfigBatchTexture>& textures = this->texturesByBatch[*batchIt.val];
        for (j = 0; j < textures.Size(); j++)
        {
            const ShaderConfigTexture& baseTex = this->textures[j];
            const ShaderConfigBatchTexture& tex = textures[j];
            MaterialTexture surTex;
            surTex.slot = tex.slot;
            surTex.defaultValue = baseTex.defaultValue;
            if (tex.slot != InvalidIndex)
                CoreGraphics::ResourceTableSetTexture(surfaceTable, { baseTex.defaultValue, tex.slot, 0, CoreGraphics::InvalidSamplerId, false });

            this->materialAllocator.Get<Textures>(sur)[*batchIt.val].Append(surTex);
        }

        // update tables
        if (surfaceTable != CoreGraphics::InvalidResourceTableId)
            CoreGraphics::ResourceTableCommitChanges(surfaceTable);

        if (instanceTable != CoreGraphics::InvalidResourceTableId)
            CoreGraphics::ResourceTableCommitChanges(instanceTable);

        const Util::Array<ShaderConfigBatchConstant>& constants = this->constantsByBatch[*batchIt.val];
        for (j = 0; j < constants.Size(); j++)
        {
            const ShaderConfigConstant& baseConstant = this->constants[j];
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
            }
            else if (constant.group == NEBULA_INSTANCE_GROUP)
            {
                surConst.defaultValue = nullptr;
                surConst.instanceConstant = true;
                surConst.bufferIndex = 0;
                surConst.buffer = CoreGraphics::GetGraphicsConstantBuffer();
            }

            this->materialAllocator.Get<Constants>(sur)[*batchIt.val].Append(surConst);
        }

        batchIt++;
    }

    return sur;
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderConfig::DestroyMaterial(MaterialId sur)
{
    this->materialAllocator.Dealloc(sur.id);
}

//------------------------------------------------------------------------------
/**
*/
MaterialInstanceId 
ShaderConfig::CreateMaterialInstance(const MaterialId id)
{
    Ids::Id32 inst = this->materialInstanceAllocator.Alloc();

    // create id
    MaterialInstanceId ret;
    ret.instance = inst;
    ret.material = id.id;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void 
ShaderConfig::DestroyMaterialInstance(const MaterialInstanceId id)
{
    this->materialInstanceAllocator.Dealloc(id.instance);
}

//------------------------------------------------------------------------------
/**
*/
IndexT 
ShaderConfig::GetMaterialConstantIndex(const Util::StringAtom& name)
{
    IndexT idx = this->constantLookup.FindIndex(name);
    if (idx != InvalidIndex)	return this->constantLookup.ValueAtIndex(idx);
    else						return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
IndexT 
ShaderConfig::GetMaterialTextureIndex(const Util::StringAtom& name)
{
    IndexT idx = this->textureLookup.FindIndex(name);
    if (idx != InvalidIndex)	return this->textureLookup.ValueAtIndex(idx);
    else						return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
const ShaderConfigVariant
ShaderConfig::GetMaterialConstantDefault(const MaterialId sur, IndexT idx)
{
    auto constant = this->constants[idx];
    return constant.def;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::TextureId
ShaderConfig::GetMaterialTextureDefault(const MaterialId sur, IndexT idx)
{
    return (*this->materialAllocator.Get<Textures>(sur.id).Begin())[idx].defaultValue;
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderConfig::SetMaterialConstant(const MaterialId sur, IndexT name, const ShaderConfigVariant& value)
{
    auto it = this->batchToIndexMap.Begin();
    while (it != this->batchToIndexMap.End())
    {
        const MaterialConstant& constant = this->materialAllocator.Get<Constants>(sur.id)[*it.val][name];
        if (constant.buffer != CoreGraphics::InvalidBufferId &&
            constant.binding != UINT_MAX &&
            constant.instanceConstant == false)
        {
            if (value.GetType() == ShaderConfigVariant::Type::TextureHandle)
                CoreGraphics::BufferUpdate(constant.buffer, value.Get<ShaderConfigVariant::TextureHandleTuple>().handle, constant.binding);
            else
                CoreGraphics::BufferUpdate(constant.buffer, value.Get(), ShaderConfigVariant::TypeToSize(value.type), constant.binding);
        }
        it++;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderConfig::SetMaterialTexture(const MaterialId sur, IndexT name, const CoreGraphics::TextureId tex)
{
    auto it = this->batchToIndexMap.Begin();
    while (it != this->batchToIndexMap.End())
    {
        const MaterialTexture& surTex = this->materialAllocator.Get<Textures>(sur.id)[*it.val][name];
        if (surTex.slot != InvalidIndex)
            CoreGraphics::ResourceTableSetTexture(this->materialAllocator.Get<MaterialTable>(sur.id)[*it.val], { tex, surTex.slot, 0, CoreGraphics::InvalidSamplerId, false });
        it++;
    }
}

//------------------------------------------------------------------------------
/**
*/
BatchIndex
ShaderConfig::GetBatchIndex(const CoreGraphics::BatchGroup::Code batch)
{
    IndexT i = this->batchToIndexMap.FindIndex(batch);
    n_assert(i != InvalidIndex);
    IndexT batchIndex = this->batchToIndexMap.ValueAtIndex(batch, i);
    return batchIndex;
}

//------------------------------------------------------------------------------
/**
*/
SizeT
ShaderConfig::GetInstanceBufferSize(const MaterialInstanceId sur, const BatchIndex batch)
{
    const Util::Tuple<IndexT, SizeT>& buffer = this->materialAllocator.Get<InstanceBuffers>(sur.material)[batch];
    return Util::Get<1>(buffer);
}

//------------------------------------------------------------------------------
/**
    Before setting any constants, we need to first allocate constants in our constants ring buffer.
    This is safe to do once per frame, it doesn't actually allocate any memory.
*/
CoreGraphics::ConstantBufferOffset
ShaderConfig::AllocateInstanceConstants(const MaterialInstanceId sur, const BatchIndex batch)
{
    const Util::Tuple<IndexT, SizeT>& buffer = this->materialAllocator.Get<InstanceBuffers>(sur.material)[batch];

    SizeT bufferSize = Util::Get<1>(buffer);
    CoreGraphics::ConstantBufferOffset offset = CoreGraphics::AllocateGraphicsConstantBufferMemory(bufferSize);
    this->materialInstanceAllocator.Get<MaterialInstanceOffsets>(sur.instance) = offset;
    return offset;
}

//------------------------------------------------------------------------------
/**
*/
IndexT
ShaderConfig::BindShader(const CoreGraphics::CmdBufferId buf, CoreGraphics::BatchGroup::Code batch)
{
    IndexT idx = this->batchToIndexMap[batch];
    if (idx != InvalidIndex)
    {
        CoreGraphics::CmdSetShaderProgram(buf, this->programs[idx]);
        return idx;
    }
    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderConfig::ApplyMaterial(const CoreGraphics::CmdBufferId buf, IndexT index, const MaterialId id)
{
    CoreGraphics::CmdSetResourceTable(buf, this->materialAllocator.Get<MaterialTable>(id.id)[index], NEBULA_BATCH_GROUP, CoreGraphics::GraphicsPipeline, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderConfig::ApplyMaterialInstance(const CoreGraphics::CmdBufferId buf, IndexT index, const MaterialInstanceId id)
{
    const CoreGraphics::ResourceTableId table = this->materialAllocator.Get<InstanceTable>(id.material)[index];
    if (table != CoreGraphics::InvalidResourceTableId)
    {
        // Set instance table
        CoreGraphics::ConstantBufferOffset offset = this->materialInstanceAllocator.Get<MaterialInstanceOffsets>(id.instance);
        CoreGraphics::CmdSetResourceTable(buf, table, NEBULA_INSTANCE_GROUP, CoreGraphics::GraphicsPipeline, { offset });
    }
}

} // namespace Materials
