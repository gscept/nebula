//------------------------------------------------------------------------------
//  raytracing.fxh
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#define MESH_BINDING group(BATCH_GROUP) binding(50)

struct HitResult
{
    vec3 radiance;
    vec3 normal;
    float depth;
};

// Declare type for vertex positions and uv
ptr struct VertexPosUv
{
    vec3 position;
    uint uv;
};

ptr struct VertexAttributeNormals
{
    uvec2 normal_tangent;
};

ptr struct VertexAttributeSecondaryUv
{
    uint uv;
};

ptr struct VertexAttributeColor
{
    uint color;
};

ptr struct VertexAttributeSkin
{
    vec4 weights;
    uint indices;
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