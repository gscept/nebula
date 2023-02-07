#pragma once
//------------------------------------------------------------------------------
/**
    Vertex layout declares the interface between application and vertex shader

    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "coregraphics/vertexcomponent.h"
#include "coregraphics/shader.h"
#include "math/half.h"
namespace CoreGraphics
{

RESOURCE_ID_TYPE(VertexLayoutId);

/// max number of vertex streams
static const IndexT MaxNumVertexStreams = 16;

struct VertexLayoutCreateInfo
{
    Util::Array<VertexComponent> comps;
};

struct VertexLayoutInfo
{
    Util::StringAtom signature;
    SizeT vertexByteSize;
    Util::Array<VertexComponent> comps;
    ShaderProgramId shader;
};

/// create new vertex layout
const VertexLayoutId CreateVertexLayout(const VertexLayoutCreateInfo& info);
/// destroy vertex layout
void DestroyVertexLayout(const VertexLayoutId id);

/// get byte size
const SizeT VertexLayoutGetSize(const VertexLayoutId id);
/// get components
const Util::Array<VertexComponent>& VertexLayoutGetComponents(const VertexLayoutId id);


enum class VertexLayoutType
{
    Invalid,
    Normal,     // Normal vertices for static geometry
    Colors,     // Geometry with per-vertex colors
    SecondUV,   // Secondary UV set geometry 
    Skin,       // Skinned geometry with weight and joint indices
    NumTypes
};

#pragma pack(push, 1)
struct BaseVertex
{
    float position[3];
    ushort uv[2];
};

struct NormalVertex
{
    Math::byte4 normal;
    Math::byte4 tangent;
};

struct SecondUVVertex : NormalVertex
{
    ushort uv2[2];
};

struct ColorVertex : NormalVertex
{
    /*
        normal
        tangent
        color
    */
    Math::byte4u color;
};

struct SkinVertex : NormalVertex
{
    /*
        normal
        tangent
        indices
        weights
    */
    Math::byte4u skinWeights;
    Math::byte4u skinIndices;
};
#pragma pack(pop)

} // CoreGraphics
