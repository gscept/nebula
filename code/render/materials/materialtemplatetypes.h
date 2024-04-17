#pragma once
//------------------------------------------------------------------------------
/**
    Types for material templates

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/shader.h"
namespace MaterialTemplates
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
        Color,
    } type;
    union
    {
        bool b;
        float f;
        Math::float2 f2;
        Math::float3 f3;
        Math::float4 f4;
    } data;
    uint offset;
#ifdef WITH_NEBULA_EDITOR
    const char* desc;
#endif
    SizeT GetSize() const
    {
        switch (this->type)
        {
            case Bool: return 1;
            case Scalar: return 4;
            case Vec2: return 8;
            case Vec3:
            case Color:
            case Vec4: return 16;
            default: return 1;
        }
    }
};

struct MaterialTemplateTexture
{
    uint bindlessOffset;
    const char* resource;
#ifdef WITH_NEBULA_EDITOR
    const char* desc;
    uint hashedName;
    uint textureIndex;
#endif
};

struct Entry
{
    struct Pass
    {
        CoreGraphics::ShaderId shader;
        CoreGraphics::ShaderProgramId program;
        uint index;
        const char* name;
        IndexT bufferIndex;
    };
    const uint HashCode() const { return this->uniqueId; }
    const char* name;
    uint uniqueId;
    uint properties;
    const char* bufferName;
    uint bufferSize;
    CoreGraphics::VertexLayoutType vertexLayout;
    uint numTextures;
    Util::Dictionary<const char*, const MaterialTemplateValue*> values;
    Util::Dictionary<const char*, const MaterialTemplateTexture*> textures;
#ifdef WITH_NEBULA_EDITOR
    Util::Dictionary<uint, const MaterialTemplateValue*> valuesByHash;
    Util::Dictionary<uint, const MaterialTemplateTexture*> texturesByHash;
#endif WITH_NEBULA_EDITOR
    Util::Dictionary<CoreGraphics::BatchGroup::Code, Pass*> passes;
    Util::Array<Util::Array<Materials::ShaderConfigBatchTexture*>> texturesPerBatch;
    Util::Array<Util::Dictionary<uint, uint>> textureBatchLookup;
};

} // namespace MaterialTemplates
