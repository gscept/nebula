#pragma once
//------------------------------------------------------------------------------
/**
    A material type declares the draw steps and associated shaders

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/hashtable.h"
#include "coregraphics/batchgroup.h"
#include "coregraphics/shader.h"
#include "memory/arenaallocator.h"
#include "util/variant.h"
#include "util/fixedarray.h"
#include "util/arraystack.h"
#include "material.h"
#include "materialvariant.h"
#include "coregraphics/texture.h"
#include "coregraphics/buffer.h"
#include "coregraphics/graphicsdevice.h"

namespace Materials
{


struct ShaderConfigTexture
{
    Util::String name;
    CoreGraphics::TextureId defaultValue;
    CoreGraphics::TextureType type;
    bool system : 1;
};

struct ShaderConfigBatchTexture
{
    IndexT slot;
};

struct ShaderConfigConstant
{
    Util::String name;
    MaterialVariant def, min, max;
    bool system : 1;
};

struct ShaderConfigBatchConstant
{
    IndexT offset, slot, group;
};

class ShaderConfig
{
public:

    /// constructor
    ShaderConfig();
    /// destructor
    ~ShaderConfig();

    /// setup after loading
    void Setup();

    /// get constant index
    IndexT GetConstantIndex(const Util::StringAtom& name);
    /// get texture index
    IndexT GetTextureIndex(const Util::StringAtom& name);

    /// get default value for constant
    const MaterialVariant GetConstantDefault(IndexT idx);
    /// get default value for texture
    const CoreGraphics::TextureId GetTextureDefault(IndexT idx);

    /// Get batch index from surface config
    BatchIndex GetBatchIndex(const CoreGraphics::BatchGroup::Code batch);

    /// get name
    const Util::String& GetName();
    /// get hash code 
    const uint32_t HashCode() const;

    /// Bind shader for a batch and return the batch index
    IndexT BindShader(const CoreGraphics::CmdBufferId buf, CoreGraphics::BatchGroup::Code batch);

private:
    friend class ShaderConfigServer;
    friend class MaterialCache;
    friend MaterialId CreateMaterial(const MaterialCreateInfo& info);
    friend void MaterialSetConstant(const MaterialId mat, IndexT name, const MaterialVariant& value);
    friend void MaterialSetTexture(const MaterialId mat, IndexT name, const CoreGraphics::TextureId tex);

    Util::HashTable<CoreGraphics::BatchGroup::Code, BatchIndex> batchToIndexMap;

    Util::Dictionary<Util::StringAtom, IndexT> textureLookup;
    Util::Dictionary<Util::StringAtom, IndexT> constantLookup;
    Util::Array<CoreGraphics::ShaderProgramId> programs;
    Util::Array<ShaderConfigTexture> textures;
    Util::Array<ShaderConfigConstant> constants;
    Util::Array<Util::Array<ShaderConfigBatchTexture>> texturesByBatch;
    Util::Array<Util::Array<ShaderConfigBatchConstant>> constantsByBatch;
    bool isVirtual;
    Util::String name;
    Util::String description;
    Util::String group;
    uint vertexType;

    IndexT uniqueId;
    static IndexT ShaderConfigUniqueIdCounter;

    CoreGraphics::BatchGroup::Code currentBatch;
    IndexT currentBatchIndex;
};

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
ShaderConfig::GetName()
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline const uint32_t
ShaderConfig::HashCode() const
{
    return (uint32_t)this->uniqueId;
}

} // namespace Materials
