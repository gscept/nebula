//------------------------------------------------------------------------------
//  vegetation.fx
//  (C) 2020 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/shared.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"
#include "lib/indirectdraw.fxh"
#include "lib/math.fxh"
#include "lib/pbr.fxh"

const int MAX_GRASS_TYPES = 4;
const int MAX_MESH_TYPES = 4;
group(SYSTEM_GROUP) constant VegetationGenerateUniforms [ string Visibility = "CS"; ]
{
    vec4 CameraPosition;
    vec2 WorldSize;
    vec2 InvWorldSize;
    vec2 GenerateQuadSize;
    vec2 HeightMapSize;
    float MinHeight;
    float MaxHeight;
	textureHandle HeightMap;

    int NumGrassBlades;
    int NumGrassTypes;
    int NumMeshTypes;
};

const int MAX_MESH_INFOS = 8;
struct MeshInfo
{
    int numLods;
    int vertexCount;
    int indexCount;
    float distribution;
    float slopeThreshold;
    float heightThreshold;
    ivec4 lodIndexOffsets;          // hold max 4 lods
    ivec4 lodVertexOffsets;
    ivec4 lodIndexCount;
    ivec4 lodVertexCount;
    vec4 lodDistances;
    uint textureIndex;
};

group(SYSTEM_GROUP) constant MeshInfoUniforms [ string Visibility = "CS"; ]
{
    MeshInfo MeshInfos[MAX_MESH_INFOS];
};

const int MAX_GRASS_INFOS = 8;
struct GrassInfo
{
    float distribution;
    float slopeThreshold;
    float heightThreshold;
    uint textureIndex;
};

group(SYSTEM_GROUP) constant GrassInfoUniforms [ string Visibility = "CS"; ]
{
    GrassInfo GrassInfos[MAX_GRASS_INFOS];
};

group(SYSTEM_GROUP) texture2D		GrassMaskArray[MAX_GRASS_TYPES];
group(SYSTEM_GROUP) texture2D		MeshMaskArray[MAX_MESH_TYPES];

group(SYSTEM_GROUP) texture2D		MaskTextureArray[MAX_MESH_TYPES];
group(SYSTEM_GROUP) texture2D       AlbedoTextureArray[MAX_GRASS_TYPES + MAX_MESH_TYPES];
group(SYSTEM_GROUP) texture2D       NormalTextureArray[MAX_GRASS_TYPES + MAX_MESH_TYPES];
group(SYSTEM_GROUP) texture2D       MaterialTextureArray[MAX_GRASS_TYPES + MAX_MESH_TYPES];

#define sampleAlbedo(index, sampler, uv)                    texture(sampler2D(AlbedoTextureArray[index], sampler), uv)
#define sampleNormal(index, sampler, uv)                    texture(sampler2D(NormalTextureArray[index], sampler), uv)
#define sampleMaterial(index, sampler, uv)                  texture(sampler2D(MaterialTextureArray[index], sampler), uv)

#define sampleGrassMask(type, sampler, uv)					textureLod(sampler2D(GrassMaskArray[type], sampler), uv, 0)
#define sampleMeshMask(type, sampler, uv)					textureLod(sampler2D(MeshMaskArray[type], sampler), uv, 0)

group(SYSTEM_GROUP) rw_buffer		IndirectGrassDrawBuffer [ string Visibility = "CS"; ]
{
	DrawIndexedCommand GrassDraw;
};

group(SYSTEM_GROUP) rw_buffer		IndirectMeshDrawBuffer [ string Visibility = "CS"; ]
{
    DrawIndexedCommand MeshDraws[];
};

struct InstanceUniforms
{
    vec3 position;
    uint textureIndex;
    vec2 sincos;
    float random;   // use for wind sway
};

group(SYSTEM_GROUP) rw_buffer       InstanceGrassUniformBuffer [ string Visibility = "CS|VS"; ]
{
    InstanceUniforms InstanceGrassUniforms[];
};

group(SYSTEM_GROUP) rw_buffer       InstanceMeshUniformBuffer [ string Visibility = "CS|VS"; ]
{
    InstanceUniforms InstanceMeshUniforms[];
};

group(SYSTEM_GROUP) rw_buffer       DrawCount [ string Visibility = "CS"; ]
{
    uint NumMeshDraws;
    uint NumGrassDraws;
};

group(SYSTEM_GROUP) sampler_state TextureSampler
{
    Filter = Linear;
};

group(SYSTEM_GROUP) sampler_state PointSampler
{
    Filter = Point;
};

//------------------------------------------------------------------------------
/**
    This shader outputs the draws to be consumed later
*/
[local_size_x] = 64
shader
void
csGenerateDraws()
{
    if (gl_GlobalInvocationID.x == 0 && gl_GlobalInvocationID.y == 0)
    {
        NumGrassDraws = 0;
        NumMeshDraws = 0;
    }
    barrier();
    memoryBarrier();

    vec2 cameraPos = CameraPosition.xz;
    vec2 cameraQuad = GenerateQuadSize;

    // calculate this fragments position around the camera
    vec2 worldPos = cameraPos - cameraQuad * 0.5f + gl_GlobalInvocationID.xy;

    // clamp world position to world grid so we don't move the patches with the camera
    worldPos = floor(worldPos);

    // add some random offset to each patch to avoid a perfect grid pattern
    float random = hash12(worldPos);
    vec2 randomOffset = vec2(sin(random), cos(random));
    worldPos += randomOffset;

    // calculate texture sampling position for masks and such
    vec2 samplePos = (worldPos + WorldSize * 0.5f) * InvWorldSize;
    float dist = distance(cameraPos, worldPos);

    // cut-off circle
    if (dist < 50.0f)
    {
        // load height map
        float height = sample2DLod(HeightMap, TextureSampler, samplePos, 0).r;
        height = MinHeight + height * (MaxHeight - MinHeight);

        vec2 pixel = samplePos * HeightMapSize;
        vec2 invHeightMapSize = 1.0f / HeightMapSize;

        ivec3 offset = ivec3(1, 1, 0.0f);
        float hl = sample2DLod(HeightMap, TextureSampler, (pixel - offset.xz) * invHeightMapSize, 0).r;
        float hr = sample2DLod(HeightMap, TextureSampler, (pixel + offset.xz) * invHeightMapSize, 0).r;
        float ht = sample2DLod(HeightMap, TextureSampler, (pixel - offset.zy) * invHeightMapSize, 0).r;
        float hb = sample2DLod(HeightMap, TextureSampler, (pixel + offset.zy) * invHeightMapSize, 0).r;
        vec3 normal = vec3(0, 0, 0);
        normal.x = MinHeight + (hl - hr) * (MaxHeight - MinHeight);
        normal.y = 2.0f;
        normal.z = MinHeight + (ht - hb) * (MaxHeight - MinHeight);
        normal = normalize(normal.xyz);

        // go through grass types and generate uniforms
        uint textureIndex = 0;
        for (int i = 0; i < NumGrassTypes; i++)
        {
            GrassInfo grassInfo = GrassInfos[i];

            float mask = sampleGrassMask(i, PointSampler, samplePos).r;

            // calculate rules
            bool heightCutoff = height < grassInfo.heightThreshold;
            bool angleCutoff = dot(normal, vec3(0, 1, 0)) > grassInfo.slopeThreshold;


            if (random > grassInfo.distribution && heightCutoff && angleCutoff)
            {
                uint entry = atomicAdd(NumGrassDraws, 1);

                InstanceUniforms instanceUniform;
                instanceUniform.position.xz = worldPos;
                instanceUniform.position.y = height;
                instanceUniform.random = random;
                instanceUniform.sincos.x = randomOffset.x;
                instanceUniform.sincos.y = randomOffset.y;
                instanceUniform.textureIndex = textureIndex;
                InstanceGrassUniforms[entry] = instanceUniform;
            }

            textureIndex++;
        }

        // go through mesh types and generate uniforms and draws
        for (int i = 0; i < NumMeshTypes; i++)
        {
            MeshInfo meshInfo = MeshInfos[i];

            // calculate rules
            bool heightCutoff = height < meshInfo.heightThreshold;
            bool angleCutoff = dot(normal, vec3(0, 1, 0)) > meshInfo.slopeThreshold;

            float mask = sampleMeshMask(i, PointSampler, samplePos).r;
            int lod = 0;
            for (int j = 0; j < meshInfo.numLods; j++)
            {
                if (meshInfo.lodDistances[j] < dist)
                {
                    lod = j;
                    break;
                }
            }

            if (random > meshInfo.distribution && heightCutoff && angleCutoff)
            {
                uint entry = atomicAdd(NumMeshDraws, 1);

                InstanceUniforms instanceUniform;
                instanceUniform.position.xz = worldPos;
                instanceUniform.position.y = height;
                instanceUniform.random = random;
                instanceUniform.sincos.x = randomOffset.x;
                instanceUniform.sincos.y = randomOffset.y;
                instanceUniform.textureIndex = textureIndex;
                InstanceMeshUniforms[entry] = instanceUniform;

                DrawIndexedCommand command;
                command.indexCount = meshInfo.lodIndexCount[lod];
                command.instanceCount = 1;
                command.startIndex = meshInfo.lodIndexOffsets[lod];
                command.offsetVertex = meshInfo.lodVertexOffsets[lod];
                command.startInstance = entry;  // pretend to use instancing
                MeshDraws[entry] = command;
            }

            textureIndex++;
        }
    }

    // wait for all other threads to arrive at the same point
    barrier();
    memoryBarrier();

    // update indirect draws for grass (they use instancing, so only need one draw call)
    if (gl_GlobalInvocationID.x == 0
        && gl_GlobalInvocationID.y == 0
        && NumGrassDraws > 0)
    {
        DrawIndexedCommand command;
        command.indexCount = 6 * NumGrassBlades;
        command.instanceCount = NumGrassDraws;
        command.startIndex = 0;
        command.offsetVertex = 0;
        command.startInstance = 0;
        GrassDraw = command;
    }
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsDrawGrass(
    [slot=0] in vec3 position
    , [slot=1] in vec3 normal
    , [slot=2] in vec2 uv
    , out vec3 Normal
    , out vec3 Tangent
    , out vec2 UV
    , out flat uint InstanceTexture
    , out flat float InstanceRandom
    )
{
    InstanceUniforms instanceUniforms = InstanceGrassUniforms[gl_InstanceID];
    float windWeight = position.y * instanceUniforms.random;
    vec3 displacement = vec3(sin(windWeight * Time_Random_Luminance_X.x), 0, cos(windWeight * Time_Random_Luminance_X.x));
    gl_Position = ViewProjection * vec4(position + instanceUniforms.position + displacement * 0.2f, 1);

    UV = uv;
    Normal = normal;
    Tangent = normalize(vec3(position.x, 0.0f, position.z));
    InstanceTexture = instanceUniforms.textureIndex;
    InstanceRandom = instanceUniforms.random;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psDrawGrass(
    in vec3 normal
    , in vec3 tangent
    , in vec2 uv
    , in flat uint instanceTexture
    , in flat float instanceRandom
    , [color0] out vec4 Albedo
    , [color1] out vec3 Normal
    , [color2] out vec4 Material)
{
    Albedo = sampleAlbedo(instanceTexture, TextureSampler, uv);
    if (Albedo.a < 0.5f) // alpha cut-off
        discard;

    vec3 geometryNormal = normal;

    // flip normal if other side
    if (!gl_FrontFacing)
        geometryNormal *= -1.0f;

    vec3 binormal = cross(geometryNormal, tangent);
    mat3 tbn = mat3(tangent, binormal, geometryNormal);
    vec4 normalSample = sampleNormal(instanceTexture, TextureSampler, uv);

    Normal.xy = (normalSample.ag * 2.0f) - 1.0f;
    Normal.z = saturate(sqrt(1.0f - dot(Normal.xy, Normal.xy)));
    Normal = tbn * Normal;

    Material = sampleMaterial(instanceTexture, TextureSampler, uv);
}


//------------------------------------------------------------------------------
/**
*/
shader
void
vsDrawMesh(
    [slot = 0] in vec3 position
    , [slot = 1] in vec3 normal
    , [slot = 2] in vec2 uv
    , [slot = 3] in vec3 tangent
    , [slot = 4] in vec3 binormal
    , [slot = 5] in vec4 color
    , out vec3 Normal
    , out vec3 Tangent
    , out vec3 Binormal
    , out vec2 UV
    , out flat uint InstanceTexture
    , out flat float InstanceRandom)
{
    InstanceUniforms instanceUniforms = InstanceMeshUniforms[gl_InstanceID];
    vec3 displacement = vec3(sin(color.r * Time_Random_Luminance_X.x * instanceUniforms.random), 0, cos(color.r * Time_Random_Luminance_X.x * instanceUniforms.random));
    mat3 rotation = mat3(
        instanceUniforms.sincos.y, 0, instanceUniforms.sincos.x,
        0, 1, 0,
        -instanceUniforms.sincos.x, 0, instanceUniforms.sincos.y);
    vec3 rotatedPos = rotation * (position + displacement * 0.1f);
    gl_Position = ViewProjection * vec4(instanceUniforms.position + rotatedPos, 1);

    UV = uv;
    UV.y = 1.0f - UV.y;
    Normal = rotation * normal;
    Tangent = rotation * tangent;
    Binormal = rotation * binormal;
    InstanceTexture = instanceUniforms.textureIndex;
    InstanceRandom = instanceUniforms.random;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psDrawMesh(
    in vec3 normal
    , in vec3 tangent
    , in vec3 binormal
    , in vec2 uv
    , in flat uint instanceTexture
    , in flat float instanceRandom
    , [color0] out vec4 Albedo
    , [color1] out vec3 Normal
    , [color2] out vec4 Material)
{
    Albedo = sampleAlbedo(instanceTexture, TextureSampler, uv);
    
    if (Albedo.a < 0.5f) // alpha cut-off
        discard;

    vec3 geometryNormal = normal;

    // flip normal if other side
    if (!gl_FrontFacing)
        geometryNormal *= -1.0f;

    mat3 tbn = mat3(tangent, binormal, geometryNormal);
    vec4 normalSample = sampleNormal(instanceTexture, TextureSampler, uv);

    Normal.xy = (normalSample.ag * 2.0f) - 1.0f;
    Normal.z = saturate(sqrt(1.0f - dot(Normal.xy, Normal.xy)));
    Normal = tbn * Normal;

    Material = sampleMaterial(instanceTexture, TextureSampler, uv);
}

//------------------------------------------------------------------------------
/**
*/
render_state GrassState
{
    /*
    BlendEnabled[0] = true;
    SrcBlend[0] = SrcAlpha;
    DstBlend[0] = OneMinusSrcAlpha;


    */
    CullMode = None;
    DepthEnabled = true;
    DepthWrite = true;
    DepthFunc = Less;
};

render_state MeshState
{

};

SimpleTechnique(VegetationGrassDraw, "VegetationGrassDraw", vsDrawGrass(), psDrawGrass(), GrassState);
SimpleTechnique(VegetationMeshDraw, "VegetationMeshDraw", vsDrawMesh(), psDrawMesh(), MeshState);
program VegetationGenerateDraws [ string Mask = "VegetationGenerateDraws"; ]
{
	ComputeShader = csGenerateDraws();
};
