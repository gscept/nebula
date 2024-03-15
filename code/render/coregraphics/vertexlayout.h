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
#include "resources/resourceid.h"
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
};

/// create new vertex layout
const VertexLayoutId CreateVertexLayout(const VertexLayoutCreateInfo& info);
/// destroy vertex layout
void DestroyVertexLayout(const VertexLayoutId id);

/// Get byte size
const SizeT VertexLayoutGetSize(const VertexLayoutId id);
/// Get byte size per stream
const SizeT VertexLayoutGetStreamSize(const VertexLayoutId id, IndexT stream);
/// get components
const Util::Array<VertexComponent>& VertexLayoutGetComponents(const VertexLayoutId id);


enum class VertexLayoutType
{
    Invalid,
    Normal,     // Normal vertices for static geometry
    Colors,     // Geometry with per-vertex colors
    SecondUV,   // Secondary UV set geometry 
    Skin,       // Skinned geometry with weight and joint indices
    Particle,   // Special vertex layout for particles
    NumTypes
};

#pragma pack(push, 1)
struct BaseVertex
{
    float position[3];
    short uv[2];
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
    float skinWeights[4];
    Math::byte4u skinIndices;
};
#pragma pack(pop)

} // CoreGraphics
