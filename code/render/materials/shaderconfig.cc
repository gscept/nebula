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
        const CoreGraphics::ShaderId shd = { this->programs[*it.val].shaderId, this->programs[*it.val].shaderType };
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
IndexT 
ShaderConfig::GetConstantIndex(const Util::StringAtom& name)
{
    IndexT idx = this->constantLookup.FindIndex(name);
    if (idx != InvalidIndex)	return this->constantLookup.ValueAtIndex(idx);
    else						return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
IndexT 
ShaderConfig::GetTextureIndex(const Util::StringAtom& name)
{
    IndexT idx = this->textureLookup.FindIndex(name);
    if (idx != InvalidIndex)	return this->textureLookup.ValueAtIndex(idx);
    else						return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
const MaterialVariant
ShaderConfig::GetConstantDefault(IndexT idx)
{
    return this->constants[idx].def;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::TextureId
ShaderConfig::GetTextureDefault(IndexT idx)
{
    return this->textures[idx].defaultValue;
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

} // namespace Materials
