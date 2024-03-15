//------------------------------------------------------------------------------
//  @file terrain_mesh_generate.fx  
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include <lib/std.fxh>
#include <lib/shared.fxh>

group(BATCH_GROUP) rw_buffer IndexInput
{
    uint Indices[];
};

struct InputVertex
{
    vec3 position;
    int uv;
};

group(BATCH_GROUP) rw_buffer VertexInput
{
    InputVertex InputVertices[];
};

struct OutputVertex
{
    vec3 position;
    int uv;
};

group(BATCH_GROUP) rw_buffer VertexOutput
{
    OutputVertex OutputVertices[];
};

group(BATCH_GROUP) constant GenerationConstants
{
    mat4 Transform;
    vec2 WorldSize;
    float MinHeight;
    float MaxHeight;

    uint VerticesPerPatch;
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
int
CompressUV(vec2 uv)
{
    int x = int(uv.x * 1000);
    int y = int(uv.y * 1000);
    return (x & 0xFFFF) | ((y & 0xFFFF) << 16);
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

    uvec3 indices = uvec3(
        Indices[gl_GlobalInvocationID.x * 3]
        , Indices[gl_GlobalInvocationID.x * 3 + 1]
        , Indices[gl_GlobalInvocationID.x * 3 + 2]
    );
    InputVertex v1 = InputVertices[indices.x];
    InputVertex v2 = InputVertices[indices.y];
    InputVertex v3 = InputVertices[indices.z];

    vec3 P1 = (Transform * vec4(v1.position, 1)).xyz;
    vec3 P2 = (Transform * vec4(v2.position, 1)).xyz;
    vec3 P3 = (Transform * vec4(v3.position, 1)).xyz;
    vec2 UV1 = (terrainPatch.PosOffset + P1.xz) / WorldSize + 0.5f;
    vec2 UV2 = (terrainPatch.PosOffset + P2.xz) / WorldSize + 0.5f;
    vec2 UV3 = (terrainPatch.PosOffset + P3.xz) / WorldSize + 0.5f;

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
    OutputVertex o1, o2, o3;
    o1.position = P1;
    o1.uv = CompressUV(UV1);
    o2.position = P2;
    o2.uv = CompressUV(UV2);
    o3.position = P3;
    o3.uv = CompressUV(UV3);
    OutputVertices[gl_WorkGroupID.y * VerticesPerPatch + indices.x] = o1;
    OutputVertices[gl_WorkGroupID.y * VerticesPerPatch + indices.y] = o2;
    OutputVertices[gl_WorkGroupID.y * VerticesPerPatch + indices.z] = o3;

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
