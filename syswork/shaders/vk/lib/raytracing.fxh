//------------------------------------------------------------------------------
//  raytracing.fxh
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include <lib/std.fxh>
#include <lib/shared.fxh>
#include <lib/materials.fxh>

group(BATCH_GROUP) accelerationStructure TLAS;
group(BATCH_GROUP) write rgba16f image2D RaytracingOutput;

#define MESH_BINDING group(BATCH_GROUP) binding(50)
#define OBJECT_BINDING group(BATCH_GROUP) binding(49)

struct HitResult
{
    vec3 radiance;
    vec3 normal;
    float depth;
};

// Declare type for vertex positions and uv
ptr alignment(16) struct VertexPosUv
{
    vec3 position;
    uint uv;
};

ptr alignment(8) struct VertexAttributeNormals
{
    uvec2 normal_tangent;
};

ptr alignment(4) struct VertexAttributeSecondaryUv
{
    uint uv;
};

ptr alignment(4) struct VertexAttributeColor
{
    uint color;
};

ptr alignment(32) struct VertexAttributeSkin
{
    vec4 weights;
    uint indices;
};

ptr alignment(4) struct Indexes32
{
    uint index;
};

ptr alignment(2) struct Indexes16
{
    uint16_t index;
};

MESH_BINDING rw_buffer Geometry
{
    Indexes32 Index32Ptr;
    Indexes16 Index16Ptr;
    VertexPosUv PositionsPtr;
    VertexAttributeNormals NormalsPtr;
    VertexAttributeSecondaryUv SecondaryUVPtr;
    VertexAttributeColor ColorsPtr;
    VertexAttributeSkin SkinPtr;
};

struct Object
{
    VertexPosUv PositionsPtr;
    VertexAttributeNormals AttrPtr;
    Indexes16 IndexPtr;
    uint Use16BitIndex;
    uint MaterialOffset;
};

OBJECT_BINDING rw_buffer ObjectBuffer
{
    Object Objects[];
};

//------------------------------------------------------------------------------
/**
*/
float 
BaryCentricFloat(float f0, float f1, float f2, vec2 coords)
{
    return mix(mix(f0, f1, coords.x), f2, coords.y);
}

//------------------------------------------------------------------------------
/**
*/
vec2
BaryCentricVec2(vec2 f0, vec2 f1, vec2 f2, vec2 coords)
{
    return mix(mix(f0, f1, coords.x), f2, coords.y);
}

//------------------------------------------------------------------------------
/**
*/
vec3
BaryCentricVec3(vec3 f0, vec3 f1, vec3 f2, vec2 coords)
{
    return mix(mix(f0, f1, coords.x), f2, coords.y);
}

//------------------------------------------------------------------------------
/**
*/
vec4
BaryCentricVec4(vec4 f0, vec4 f1, vec4 f2, vec2 coords)
{
    return mix(mix(f0, f1, coords.x), f2, coords.y);
}

//------------------------------------------------------------------------------
/**
    Unpack signed short UVs to float
*/
vec2
UnpackUV32(uint packedUv)
{
    uint x = packedUv & 0xFFFF;
    uint y = (packedUv >> 16) & 0xFFFF;
    return vec2(x, y) * (1.0f / 1000.0f);
}

//------------------------------------------------------------------------------
/**
*/
vec3
UnpackNormal32(uint packedNormal)
{
    uint x = packedNormal & 0xFF;
    uint y = (packedNormal >> 8) & 0xFF;
    uint z = (packedNormal >> 12) & 0xFF;
    return vec3(x, y, z) / 255.0f;
}

#define OFFSET_PTR(ptr, offset, type) type(VoidPtr(ptr) + offset)