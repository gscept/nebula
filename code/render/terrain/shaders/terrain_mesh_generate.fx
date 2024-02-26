//------------------------------------------------------------------------------
//  @file terrain_mesh_generate.fx  
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include <lib/std.fxh>
#include <lib/shared.fxh>
struct Quad
{
    uvec4 indices;
};

group(BATCH_GROUP) rw_buffer IndexInput
{
    Quad Quads[];
};

struct InputVertex
{
    vec3 position;
    vec2 uv;
};

group(BATCH_GROUP) rw_buffer VertexInput
{
    InputVertex InputVertices[];
};

group(BATCH_GROUP) rw_buffer IndexOutput
{
    uvec3 Indices[];
};

struct OutputVertex
{
    vec3 position;
    uint uv;
};

group(BATCH_GROUP) rw_buffer VertexOutput
{
    OutputVertex OutputVertices[];
};

group(BATCH_GROUP) constant GenerationConstants
{
    mat4 Transform;

    float MinHeight;
    float MaxHeight;
    //uint NumSubdivisions;
    textureHandle HeightMap;
};

struct TerrainPatch
{
    vec2 PosOffset;
    vec2 UvOffset;
};

group(BATCH_GROUP) rw_buffer TerrainPatchData[string Visibility = "CS";]
{
    TerrainPatch Patches[];
};

//------------------------------------------------------------------------------
/**
*/
uint
CompressUV(vec2 uv)
{
    uint x = uint(uv.x * 1000);
    uint y = uint(uv.y * 1000);
    return (x & 0xFFFF) & ((y & 0xFFFF) << 16);
}

//------------------------------------------------------------------------------
/**
void
Subdivide(in Triangle tri, in uint counter)
{
    InputVertex v1 = InputVertices[tri.indices.x];
    InputVertex v2 = InputVertices[tri.indices.y];
    InputVertex v3 = InputVertices[tri.indices.z];

    vec3 edge = (v3.position - v2.position) * 0.5f;
    vec2 uv = (v3.uv - v2.uv) * 0.5f;
    uint newVert = atomicAdd(VertexCounter, 1);
    OutputVertices[newVert].position = edge;
    OutputVertices[newVert].uv = CompressUV(uv);

    Triangle t1, t2;

    t1.indices.x = newVert;
    t1.indices.y = tri.indices.x;
    t1.indices.z = tri.indices.y;

    t2.indices.x = newVert;
    t2.indices.y = tri.indices.z;
    t2.indices.z = tri.indices.x;

    uint offset = atomicAdd(IndexCounter, 2);
    Indices[offset] = t1.indices;
    Indices[offset + 1] = t2.indices;
}
*/


//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 64
shader
void
csMeshGenerate()
{
    TerrainPatch terrainPatch = Patches[gl_WorkGroupID.y];

    Quad q = Quads[gl_GlobalInvocationID.x];
    InputVertex v1 = InputVertices[q.indices.x];
    InputVertex v2 = InputVertices[q.indices.y];
    InputVertex v3 = InputVertices[q.indices.z];
    InputVertex v4 = InputVertices[q.indices.w];

    vec2 UV1 = v1.uv + terrainPatch.UvOffset;
    vec2 UV2 = v2.uv + terrainPatch.UvOffset;
    vec2 UV3 = v3.uv + terrainPatch.UvOffset;
    vec2 UV4 = v4.uv + terrainPatch.UvOffset;
    vec3 P1 = (Transform * vec4(v1.position + vec3(terrainPatch.PosOffset.x, 0, terrainPatch.PosOffset.y), 1)).xyz;
    vec3 P2 = (Transform * vec4(v2.position + vec3(terrainPatch.PosOffset.x, 0, terrainPatch.PosOffset.y), 1)).xyz;
    vec3 P3 = (Transform * vec4(v3.position + vec3(terrainPatch.PosOffset.x, 0, terrainPatch.PosOffset.y), 1)).xyz;
    vec3 P4 = (Transform * vec4(v4.position + vec3(terrainPatch.PosOffset.x, 0, terrainPatch.PosOffset.y), 1)).xyz;

    vec2 pixelSize = textureSize(basic2D(HeightMap), 0);
    pixelSize = vec2(1.0f) / pixelSize;

    vec3 offset = vec3(-pixelSize.x, pixelSize.x, 0.0f);
    //Normal = CalculateNormalFromHeight(UV, offset);

    float h1 = sample2DLod(HeightMap, Basic2DSampler, UV1, 0).r;
    float h2 = sample2DLod(HeightMap, Basic2DSampler, UV2, 0).r;
    float h3 = sample2DLod(HeightMap, Basic2DSampler, UV3, 0).r;
    P1.y = MinHeight + h1 * (MaxHeight - MinHeight);
    P2.y = MinHeight + h2 * (MaxHeight - MinHeight);
    P3.y = MinHeight + h3 * (MaxHeight - MinHeight);

    // First output base vertices
    OutputVertices[q.indices.x].position = P1;
    OutputVertices[q.indices.x].uv = CompressUV(v1.uv);
    OutputVertices[q.indices.y].position = P2;
    OutputVertices[q.indices.y].uv = CompressUV(v2.uv);
    OutputVertices[q.indices.z].position = P3;
    OutputVertices[q.indices.z].uv = CompressUV(v3.uv);
    OutputVertices[q.indices.w].position = P4;
    OutputVertices[q.indices.w].uv = CompressUV(v4.uv);

    Indices[gl_GlobalInvocationID.x * 2] = uvec3(q.indices.x, q.indices.y, q.indices.z);
    Indices[gl_GlobalInvocationID.x * 2 + 1] = uvec3(q.indices.x, q.indices.z, q.indices.w);

    // Then subdivide, which will output indices
    //Subdivide(tri, 1);
}

//------------------------------------------------------------------------------
/**
*/
program Main[string Mask = "Main";]
{
    ComputeShader = csMeshGenerate();
};
