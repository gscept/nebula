//------------------------------------------------------------------------------
//  raytracing.fxh
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include <lib/std.fxh>
#include <lib/shared.fxh>
#include <material_interfaces.fx>
#include <lib/pbr.fxh>
#include <lib/lighting_functions.fxh>

group(BATCH_GROUP) accelerationStructure TLAS;
group(BATCH_GROUP) write rgba16f image2D RaytracingOutput;

#define MESH_BINDING group(BATCH_GROUP) binding(50)
#define OBJECT_BINDING group(BATCH_GROUP) binding(49)

struct HitResult
{
    vec3 albedo;
    float alpha;
    vec4 material;
    vec3 normal;
    float depth;
    uint miss;
};

// Declare type for vertex positions and uv
ptr alignment(16) struct VertexPosUv
{
    vec3 position;
    int uv;
};

ptr alignment(8) struct VertexAttributeNormals
{
    ivec2 normal_tangent;
};

ptr alignment(4) struct VertexAttributeSecondaryUv
{
    ivec2 normal_tangent;
    uint uv;
};

ptr alignment(4) struct VertexAttributeColor
{
    ivec2 normal_tangent;
    uint color;
};

ptr alignment(4) struct VertexAttributeSkin
{
    ivec2 normal_tangent;
    vec4 weights;
    uint indices;
};

ptr alignment(4) struct VertexAttributeDummy
{
    uint dummy;
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
    VertexAttributeDummy AttrPtr;
    Indexes16 IndexPtr;
    uint Use16BitIndex;
    uint MaterialOffset;
    uint VertexLayout;
};

OBJECT_BINDING rw_buffer ObjectBuffer
{
    Object Objects[];
};

//------------------------------------------------------------------------------
/**
*/
float 
BaryCentricFloat(float f0, float f1, float f2, vec3 coords)
{
    return f0 * coords.x + f1 * coords.y + f2 * coords.z;
}

//------------------------------------------------------------------------------
/**
*/
vec2
BaryCentricVec2(vec2 f0, vec2 f1, vec2 f2, vec3 coords)
{
    return f0 * coords.x + f1 * coords.y + f2 * coords.z;
}

//------------------------------------------------------------------------------
/**
*/
vec3
BaryCentricVec3(vec3 f0, vec3 f1, vec3 f2, vec3 coords)
{
    return f0 * coords.x + f1 * coords.y + f2 * coords.z;
}

//------------------------------------------------------------------------------
/**
*/
vec4
BaryCentricVec4(vec4 f0, vec4 f1, vec4 f2, vec3 coords)
{
    return f0 * coords.x + f1 * coords.y + f2 * coords.z;
}

//------------------------------------------------------------------------------
/**
    Unpack signed short UVs to float
*/
vec2
UnpackUV32(int packedUv)
{
    int x = packedUv & 0xFFFF;
    int y = (packedUv >> 16) & 0xFFFF;
    return vec2(x, y) * (1.0f / 1000.0f);
}

//------------------------------------------------------------------------------
/**
*/
vec3
UnpackNormal32(int packedNormal)
{
    int x = packedNormal & 0xFF;
    int y = (packedNormal >> 8) & 0xFF;
    int z = (packedNormal >> 16) & 0xFF;
    vec3 unpacked = vec3(x, y, z) / 128.0f;
    return unpacked;
}

//------------------------------------------------------------------------------
/**
*/
float
UnpackSign(int packedNormal)
{
    int sig = (packedNormal >> 24) & 0xFF;
    return sig == 127 ? 1.0f : 0.0f;
}

//------------------------------------------------------------------------------
/**
*/
void
SampleTerrain(in Object obj, uint prim, in vec3 baryCoords, out uvec3 indices, out vec2 uv, out mat3 tbn)
{
    // Sample the index buffer
    if (obj.Use16BitIndex == 1)
        indices = uvec3(obj.IndexPtr[prim * 3].index, obj.IndexPtr[prim * 3 + 1].index, obj.IndexPtr[prim * 3 + 2].index);
    else
    {
        Indexes32 i32Ptr = Indexes32(obj.IndexPtr);
        indices = uvec3(i32Ptr[prim * 3].index, i32Ptr[prim * 3 + 1].index, i32Ptr[prim * 3 + 2].index);
    }

    vec2 uv0 = UnpackUV32((obj.PositionsPtr[indices.x]).uv);
    vec2 uv1 = UnpackUV32((obj.PositionsPtr[indices.y]).uv);
    vec2 uv2 = UnpackUV32((obj.PositionsPtr[indices.z]).uv);
    uv = BaryCentricVec2(uv0, uv1, uv2, baryCoords);

    tbn = PlaneTBN(vec3(0, 1, 0));
}

//------------------------------------------------------------------------------
/**
*/
void
SampleGeometry(in Object obj, uint prim, in vec3 baryCoords, out uvec3 indices, out vec2 uv, out mat3 tbn)
{
    // Sample the index buffer
    if (obj.Use16BitIndex == 1)
        indices = uvec3(obj.IndexPtr[prim * 3].index, obj.IndexPtr[prim * 3 + 1].index, obj.IndexPtr[prim * 3 + 2].index);
    else
    {
        Indexes32 i32Ptr = Indexes32(obj.IndexPtr);
        indices = uvec3(i32Ptr[prim * 3].index, i32Ptr[prim * 3 + 1].index, i32Ptr[prim * 3 + 2].index);
    }

    vec2 uv0 = UnpackUV32((obj.PositionsPtr[indices.x]).uv);
    vec2 uv1 = UnpackUV32((obj.PositionsPtr[indices.y]).uv);
    vec2 uv2 = UnpackUV32((obj.PositionsPtr[indices.z]).uv);
    uv = BaryCentricVec2(uv0, uv1, uv2, baryCoords);
    
    vec3 n1, n2, n3;
    vec3 t1, t2, t3;
    float sign;

    
    switch (obj.VertexLayout)
    {
        case 1: // Normal
        {
            VertexAttributeNormals attrs = VertexAttributeNormals(obj.AttrPtr);
            n1 = UnpackNormal32(attrs[indices.x].normal_tangent.x);
            n2 = UnpackNormal32(attrs[indices.y].normal_tangent.x);
            n3 = UnpackNormal32(attrs[indices.z].normal_tangent.x);
            t1 = UnpackNormal32(attrs[indices.x].normal_tangent.y);
            t2 = UnpackNormal32(attrs[indices.y].normal_tangent.y);
            t3 = UnpackNormal32(attrs[indices.z].normal_tangent.y);
            sign = UnpackSign(attrs[indices.x].normal_tangent.y);
            break;
        }
        case 4: // Skin
        {
            VertexAttributeSkin attrs = VertexAttributeSkin(obj.AttrPtr);
            n1 = UnpackNormal32(attrs[indices.x].normal_tangent.x);
            n2 = UnpackNormal32(attrs[indices.y].normal_tangent.x);
            n3 = UnpackNormal32(attrs[indices.z].normal_tangent.x);
            t1 = UnpackNormal32(attrs[indices.x].normal_tangent.y);
            t2 = UnpackNormal32(attrs[indices.y].normal_tangent.y);
            t3 = UnpackNormal32(attrs[indices.z].normal_tangent.y);
            sign = UnpackSign(attrs[indices.x].normal_tangent.y);
            break;
        }
    }
    
    vec3 norm = BaryCentricVec3(n1, n2, n3, baryCoords);
    vec3 tang = BaryCentricVec3(t1, t2, t3, baryCoords);

    tbn = TangentSpace(tang, norm, sign);
}
#define OFFSET_PTR(ptr, offset, type) type(VoidPtr(ptr) + offset)


//------------------------------------------------------------------------------
/**
    Calculate 3D index from screen position and depth
*/
uint3
CalculateClusterIndexRT(vec2 screenPos, float depth, float scale, float bias)
{
    uint i = uint(screenPos.x);
    uint j = uint(screenPos.y);
    uint k = uint(log2(-depth) * scale + bias);

    return uint3(i, j, k);
}

#define USE_SCALARIZATION_LOOP 0
//------------------------------------------------------------------------------
/**
*/
vec3
CalculateLightRT(vec3 worldSpacePos, float depth, vec3 albedo, vec4 material, vec3 normal)
{
    vec3 clusterCenter = EyePos.xyz - worldSpacePos;
    uvec3 index3D = uvec3(clusterCenter) / BlockSize.xxx;
    uint idx = Pack3DTo1D(index3D, NumCells.x, NumCells.y);

    vec3 light = vec3(0, 0, 0);
    vec3 viewVec = normalize(EyePos.xyz - worldSpacePos.xyz);
    vec3 F0 = CalculateF0(albedo.rgb, material[MAT_METALLIC], vec3(0.04));
    light += CalculateGlobalLight(albedo, material, F0, viewVec, normal, worldSpacePos);

#if USE_SCALARIZATION_LOOP
    // Get the mask for the invocation
    uint4 laneMask = gl_SubgroupEqMask;

    // Get the execution mask for the wavefront
    uint4 execMask = subgroupBallot(true);
    uint firstWaveIndex = subgroupBroadcastFirst(idx);

    // Check if all waves use the same index and do this super cheaply
    if (subgroupBallot(firstWaveIndex == idx) == execMask)
    {
        light += LocalLights(firstWaveIndex, albedo, material, F0, worldSpacePos, normal, clipXYZ.z);
    }
    else
    {
        // Scalarization loop
        while ((laneMask & execMask) != uint4(0))
        {
            uint scalarIdx = subgroupBroadcastFirst(idx);
            uint4 currentMask = subgroupBallot(scalarIdx == idx);
            execMask &= ~currentMask;

            // If this wave uses the cell we loaded into SGPR, use it
            // this will effectively scalarize the light lists
            if (scalarIdx == idx)
            {
                light += LocalLights(scalarIdx, albedo, material, F0, worldSpacePos, normal, clipXYZ.z);
            }
        }
    }
#else
    light += LocalLights(idx, albedo, material, F0, worldSpacePos, normal, depth);
#endif

    //light += IBL(albedo, F0, normal, viewVec, material);
    light += albedo.rgb * material[MAT_EMISSIVE];
    return light;
}