#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::MeshBuilderVertex
    
    Contains per-vertex data in a mesh builder object.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "math/vec4.h"
#include "math/mat4.h"
#include "math/half.h"
#include "coregraphics/vertexlayout.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{

class MeshBuilderVertex
{
public:

    enum Components
    {
        Position = 0x0,
        Uvs = 0x1,
        Normals = 0x2,
        Tangents = 0x4,
        Color = 0x8,
        SecondUv = 0x10,
        SkinWeights = 0x20,
        SkinIndices = 0x40
    };
    typedef uint ComponentMask;

    /// vertex flags
    enum Flag
    {
        Redundant = (1<<0),
    };
    /// a mask of Flags
    typedef uint FlagMask;

    /// constructor
    MeshBuilderVertex();
    /// Set layout
    void SetComponents(const ComponentMask componentMask);
    /// Get vertex type based on components
    static const CoreGraphics::VertexLayoutType GetVertexLayoutType(const ComponentMask componentMask);
    /// Get size of vertex
    static const SizeT GetSize(const ComponentMask componentMask);

    /// Set position
    void SetPosition(const Math::vec4& pos);
    /// Set uv
    void SetUv(const Math::vec2& uv);
    /// Set normal
    void SetNormal(const Math::vec3& n);
    /// Set tangent
    void SetTangent(const Math::vec3& t);
    /// Set tangent sign
    void SetSign(const float f);
    /// Set color
    void SetColor(const Math::vec4& c);
    /// Set secondary uv
    void SetSecondaryUv(const Math::vec2& uv);
    /// Set skin weights
    void SetSkinWeights(const Math::vec4& w);
    /// Set skin indices
    void SetSkinIndices(const Math::uint4& indices);

    /// equality operator
    bool operator==(const MeshBuilderVertex& rhs) const;
    /// inequality operator
    bool operator!=(const MeshBuilderVertex& rhs) const;
    /// less-then operator
    bool operator<(const MeshBuilderVertex& rhs) const;
    /// greather-then operator
    bool operator>(const MeshBuilderVertex& rhs) const;

    /// set vertex flag
    void SetFlag(Flag f);
    /// unset vertex flag
    void UnsetFlag(Flag f);
    /// check vertex flag
    bool CheckFlag(Flag f) const;

    /// compare vertex against other, return -1, 0 or +1
    int Compare(const MeshBuilderVertex& rhs) const;
    /// transform the vertex
    void Transform(const Math::mat4& m);

private:
    friend class MeshBuilder;
    friend class MeshBuilderSaver;
    friend class SkinPartitioner;
    friend class SkinFragment;
    friend class NFbxScene;
    friend class Scene;
    ComponentMask componentMask;
    FlagMask flagMask;

    /// we only allow one component of each type. That means, ex. the normal and normalb4n maps to the same index in the comps array.
    struct VertexBase
    {
        Math::vec4 position;
        Math::vec2 uv;
    } base;

    struct VertexAttributes
    {
        struct Normal
        {
            Math::vec3 normal, tangent;
            float sign;
        } normal;

        struct SecondUV
        {
            Math::vec2 uv2;
        } secondUv;

        struct Color
        {
            Math::vec4 color;
        } color;

        struct Skin
        {
            Math::vec4 weights;
            Math::uint4 indices;
            Math::uint4 remapIndices;
        } skin;
    } attributes;
};

//------------------------------------------------------------------------------
/**
*/
inline void 
MeshBuilderVertex::SetComponents(const ComponentMask components)
{
    this->componentMask = components;
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::VertexLayoutType
MeshBuilderVertex::GetVertexLayoutType(const ComponentMask componentMask)
{
    if (OnlyBits(componentMask,
                MeshBuilderVertex::Components::Position
                | MeshBuilderVertex::Components::Uvs
                | MeshBuilderVertex::Components::Normals 
                | MeshBuilderVertex::Components::Tangents))
        return CoreGraphics::VertexLayoutType::Normal;
    else if (OnlyBits(componentMask,
                MeshBuilderVertex::Components::Position
                | MeshBuilderVertex::Components::Uvs
                | MeshBuilderVertex::Components::Normals 
                | MeshBuilderVertex::Components::Tangents 
                | MeshBuilderVertex::Components::Color))
        return CoreGraphics::VertexLayoutType::Colors;
    else if (OnlyBits(componentMask,
                MeshBuilderVertex::Components::Position
                | MeshBuilderVertex::Components::Uvs
                | MeshBuilderVertex::Components::Normals
                | MeshBuilderVertex::Components::Tangents
                | MeshBuilderVertex::Components::SkinIndices
                | MeshBuilderVertex::Components::SkinWeights))
        return CoreGraphics::VertexLayoutType::Skin;
    else if (OnlyBits(componentMask,
                MeshBuilderVertex::Components::Position
                | MeshBuilderVertex::Components::Uvs
                | MeshBuilderVertex::Components::Normals
                | MeshBuilderVertex::Components::Tangents
                | MeshBuilderVertex::Components::SecondUv))
        return CoreGraphics::VertexLayoutType::SecondUV;
    else 
        return CoreGraphics::VertexLayoutType::Invalid;
}

//------------------------------------------------------------------------------
/**
*/
inline const SizeT 
MeshBuilderVertex::GetSize(const ComponentMask componentMask)
{
    // The type enum is the size of the vertex
    SizeT ret = 0;
    CoreGraphics::VertexLayoutType type = GetVertexLayoutType(componentMask);
    switch (type)
    {
        case CoreGraphics::VertexLayoutType::Normal:
            ret += sizeof(CoreGraphics::NormalVertex);
            break;
        case CoreGraphics::VertexLayoutType::Colors:
            ret += sizeof(CoreGraphics::ColorVertex);
            break;
        case CoreGraphics::VertexLayoutType::Skin:
            ret += sizeof(CoreGraphics::SkinVertex);
            break;
        case CoreGraphics::VertexLayoutType::SecondUV:
            ret += sizeof(CoreGraphics::SecondUVVertex);
            break;
    }
    
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
MeshBuilderVertex::SetPosition(const Math::vec4& pos)
{
    this->base.position = pos;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
MeshBuilderVertex::SetUv(const Math::vec2& uv)
{
    this->base.uv = uv;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
MeshBuilderVertex::SetNormal(const Math::vec3& n)
{
    this->attributes.normal.normal = n;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
MeshBuilderVertex::SetTangent(const Math::vec3& t)
{
    this->attributes.normal.tangent = t;
}

//------------------------------------------------------------------------------
/**
*/
inline void
MeshBuilderVertex::SetSign(const float f)
{
    this->attributes.normal.sign = f;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
MeshBuilderVertex::SetColor(const Math::vec4& c)
{
    this->attributes.color.color = c;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
MeshBuilderVertex::SetSecondaryUv(const Math::vec2& uv)
{
    this->attributes.secondUv.uv2 = uv;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
MeshBuilderVertex::SetSkinWeights(const Math::vec4& w)
{
    this->attributes.skin.weights = w;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
MeshBuilderVertex::SetSkinIndices(const Math::uint4& indices)
{
    this->attributes.skin.indices = indices;
}

//------------------------------------------------------------------------------
/**
*/
inline void
MeshBuilderVertex::SetFlag(Flag f)
{
    this->flagMask |= f;
}

//------------------------------------------------------------------------------
/**
*/
inline void
MeshBuilderVertex::UnsetFlag(Flag f)
{
    this->flagMask &= ~f;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
MeshBuilderVertex::CheckFlag(Flag f) const
{
    return (0 != (this->flagMask & f));
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------
    