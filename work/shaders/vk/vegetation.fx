//------------------------------------------------------------------------------
//  vegetation.fx
//  (C) 2020 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/shared.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"
#include "lib/indirectdraw.fxh"

const int MAX_GRASS_TYPES = 4;
const int MAX_MESH_TYPES = 4;
group(SYSTEM_GROUP) constant VegetationGenerateUniforms [ string Visibility = "CS"; ]
{
    vec4 CameraPosition;
    vec2 WorldSize;
    vec2 InvWorldSize;
    vec2 GenerateQuadSize;
    float MinHeight;
    float MaxHeight;
	textureHandle HeightMap;

    int NumGrassBlades;
    int NumGrassTypes;
    // contains - x: distribution, y: -, z: slope threshold, w: height threshold
    vec4 GrassParameters[MAX_GRASS_TYPES];

    int NumMeshTypes;
    // contains - x: distribution, y: vertex/index count, z: slope threshold, w: height threshold
    vec4 MeshParameters0[MAX_MESH_TYPES];
    // contains - x: offset vertex, y: first index, z: -, w: -
    vec4 MeshParameters1[MAX_MESH_TYPES];
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

struct InstanceVertex
{
    vec3 position;
    uint textureIndex;
    float random;   // use for wind sway
};

group(SYSTEM_GROUP) rw_buffer       InstanceGrassUniformBuffer [ string Visibility = "CS"; ]
{
    InstanceVertex InstanceGrassUniforms[];
};

group(SYSTEM_GROUP) rw_buffer       InstanceMeshUniformBuffer [ string Visibility = "CS"; ]
{
    InstanceVertex InstanceMeshUniforms[];
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

    worldPos = floor(worldPos);
    vec2 samplePos = (worldPos + WorldSize * 0.5f) * InvWorldSize;

    // cut-off circle
    if (distance(cameraPos, worldPos) < 50.0f)
    {
        // load height map
        float height = sample2DLod(HeightMap, TextureSampler, samplePos, 0).r;
        height = MinHeight + height * (MaxHeight - MinHeight);

        // go through grass types and generate uniforms, 
        for (int i = 0; i < NumGrassTypes; i++)
        {
            vec4 params = GrassParameters[i];
            float mask = sampleGrassMask(i, PointSampler, samplePos).r;

            float rnd = hash12(samplePos);
            if (rnd > params.x)
            {
                uint entry = atomicAdd(NumGrassDraws, 1);

                InstanceVertex instanceUniform;
                instanceUniform.position.xz = worldPos;
                instanceUniform.position.y = height;
                instanceUniform.random = rnd;
                instanceUniform.textureIndex = i;
                InstanceGrassUniforms[entry] = instanceUniform;
            }
        }

        // go through mesh types and generate uniforms and draws
        for (int i = 0; i < NumMeshTypes; i++)
        {
            vec4 params0 = MeshParameters0[i];
            vec4 params1 = MeshParameters1[i];
            float mask = sampleMeshMask(i, PointSampler, samplePos).r;

            float rnd = hash12(samplePos);
            if (rnd > params0.x)
            {
                uint entry = atomicAdd(NumMeshDraws, 1);

                InstanceVertex instanceUniform;
                instanceUniform.position.xz = worldPos;
                instanceUniform.position.y = height;
                instanceUniform.random = rnd;
                instanceUniform.textureIndex = i;
                InstanceMeshUniforms[entry] = instanceUniform;

                DrawIndexedCommand command;
                command.indexCount = uint(params0.y);
                command.instanceCount = 1;
                command.startIndex = uint(params1.y);
                command.offsetVertex = uint(params1.x);
                command.startInstance = 0;
                MeshDraws[entry] = command;
            }
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
    , [slot=3] in vec3 instanceOffset
    , [slot=4] in uint instanceTexture
    , [slot=5] in float instanceRandom
    , out vec3 Normal
    , out vec3 Tangent
    , out vec2 UV
    , out flat uint InstanceTexture
    , out flat float InstanceRandom)
{
    gl_Position = ViewProjection * vec4(instanceOffset + position, 1);

    UV = uv;
    Tangent = normalize(vec3(position.x, 0.0f, position.z));
    InstanceTexture = instanceTexture;
    InstanceRandom = instanceRandom;
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
    [slot = 0] in vec3 position,
    , [slot = 1] in vec3 normal,
    , [slot = 2] in vec2 uv,
    , [slot = 3] in vec3 tangent,
    , [slot = 4] in vec3 binormal,
    , [slot = 5] in vec3 instanceOffset
    , [slot = 6] in uint instanceTexture
    , [slot = 7] in float instanceRandom
    , out vec3 Normal
    , out vec3 Tangent
    , out vec2 UV
    , out flat uint InstanceTexture
    , out flat float InstanceRandom)
{
    gl_Position = ViewProjection * vec4(instanceOffset + position, 1);

    UV = uv;
    Tangent = normalize(vec3(position.x, 0.0f, position.z));
    InstanceTexture = instanceTexture;
    InstanceRandom = instanceRandom;
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

SimpleTechnique(VegetationGrassDraw, "VegetationGrassDraw", vsDrawGrass(), psDrawGrass(), GrassState);
program VegetationGenerateDraws [ string Mask = "VegetationGenerateDraws"; ]
{
	ComputeShader = csGenerateDraws();
};
