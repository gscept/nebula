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

struct MaterialTemplateValue
{
    enum Type
    {
        Bool,
        Scalar,
        Vec2,
        Vec3,
        Vec4,
        Resource,
        BindlessResource
    } type;

    union
    {
        bool b;
        float f;
        Math::vec2 f2;
        Math::vec3 f3;
        Math::vec4 f4;
        const char* resource;
    } data;

    // Get size of data
    SizeT GetSize() const
    {
        switch (this->type)
        {
            case Bool:
                return 1;
            case Scalar:
                return 4;
            case Vec2:
                return 8;
            case Vec3:      // Vec3 expands to Vec4 because neither SSE nor GPU's can actually store only 3 floats
            case Vec4:
                return 16;
            case BindlessResource:
                return 4;
            default: 
                return 1;
        }
    }

};

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
    Materials::MaterialTemplateValue* def;
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
    Materials::MaterialTemplateValue* def;

    // Returns true if valid
    const bool Valid() const
    {
        return offset != InvalidIndex && slot != InvalidIndex && group != InvalidIndex;
    }
};

enum class MaterialProperties
{
    BRDF,
    BSDF,
    GLTF,
    Unlit,
    Unlit2,
    Unlit3,
    Unlit4,
    Skybox,
    Legacy,

    Num
};

} // namespace Materials

namespace MaterialTemplates
{

struct Entry
{
    struct Pass
    {
        CoreGraphics::ShaderId shader;
        CoreGraphics::ShaderProgramId program;
        uint index;
    };
    
    // Return HashCode for hash table support
    const uint HashCode() const { return this->uniqueId; }
    const char* name;
    uint uniqueId;
    Materials::MaterialProperties properties;
    CoreGraphics::VertexLayoutType vertexLayout;
    Util::Dictionary<const char*, Materials::MaterialTemplateValue*> values;
    Util::Dictionary<CoreGraphics::BatchGroup::Code, Pass*> passes;
    Util::Array<Util::Array<Materials::ShaderConfigBatchTexture*>> texturesPerBatch;
    Util::Array<Util::Array<Materials::ShaderConfigBatchConstant*>> constantsPerBatch;
    Util::Array<Util::Dictionary<uint, uint>> textureBatchLookup;
    Util::Array<Util::Dictionary<uint, uint>> constantBatchLookup;
};

} // namespace MaterialTemplates
