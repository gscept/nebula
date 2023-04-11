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
#include "lib/standard_shading.fxh"

//#define LOD_DEBUG 1
const int MAX_GRASS_TYPES = 4;
const int MAX_MESH_TYPES = 4;

group(SYSTEM_GROUP) constant VegetationGenerateUniforms [ string Visibility = "CS"; ]
{
    vec4 CameraPosition;
    vec4 CameraForward;
    vec2 WorldSize;
    vec2 InvWorldSize;
    vec2 GenerateQuadSize;
    vec2 GenerateDirection;
    float DensityFactor;
    vec2 HeightMapSize;
    float MinHeight;
    float MaxHeight;
    textureHandle HeightMap;

    float MaxRange;
    float Fov;
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
    bool used;
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
    bool used;
};

group(SYSTEM_GROUP) constant GrassInfoUniforms [ string Visibility = "CS"; ]
{
    GrassInfo GrassInfos[MAX_GRASS_INFOS];
};

const int MAX_VEGETATION_LAYERS = 8;
group(SYSTEM_GROUP) constant VegetationMaterialUniforms[string Visibility = "CS|PS";]
{
    ivec4 VegetationAlbedo[MAX_VEGETATION_LAYERS / 4];
    ivec4 VegetationNormal[MAX_VEGETATION_LAYERS / 4];
    ivec4 VegetationMaterial[MAX_VEGETATION_LAYERS / 4];
    ivec4 VegetationMasks[MAX_VEGETATION_LAYERS / 4];
};

#define sampleAlbedo(index, sampler, uv)                    sample2D(VegetationAlbedo[index / 4][index % 4], sampler, uv)
#define sampleNormal(index, sampler, uv)                    sample2D(VegetationNormal[index / 4][index % 4], sampler, uv)
#define sampleMaterial(index, sampler, uv)                  sample2D(VegetationMaterial[index / 4][index % 4], sampler, uv)
#define sampleMask(type, sampler, uv)					    sample2DLod(VegetationMasks[type / 4][type % 4], sampler, uv, 0)

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
    bool lodIsBillboard;
#if LOD_DEBUG
    uint lod;
#endif
};

group(BATCH_GROUP) rw_buffer       InstanceGrassArguments [ string Visibility = "CS|VS"; ]
{
    InstanceUniforms InstanceGrassUniforms[];
};

group(BATCH_GROUP) rw_buffer       InstanceMeshArguments [ string Visibility = "CS|VS"; ]
{
    InstanceUniforms InstanceMeshUniforms[];
};

group(SYSTEM_GROUP) rw_buffer       DrawCount [ string Visibility = "CS"; ]
{
    uvec4 NumMeshDraws[MAX_MESH_INFOS / 4];
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
*/
vec3
CalculateNormalFromHeight(vec2 pixel, ivec3 offset, vec2 scale)
{
    float hl = sample2DLod(HeightMap, TextureSampler, (pixel + offset.xz) * scale, 0).r;
    float hr = sample2DLod(HeightMap, TextureSampler, (pixel + offset.yz) * scale, 0).r;
    float ht = sample2DLod(HeightMap, TextureSampler, (pixel + offset.zx) * scale, 0).r;
    float hb = sample2DLod(HeightMap, TextureSampler, (pixel + offset.zy) * scale, 0).r;
    vec3 normal = vec3(0, 0, 0);
    normal.x = (hl - hr);
    normal.y = 2.0f;
    normal.z = (ht - hb);
    normal *= vec3((MaxHeight - MinHeight), 1, (MaxHeight - MinHeight));
    normal += vec3(MinHeight, 0, MinHeight);
    normal = normalize(normal);
    return normal;
}

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 1
shader
void
csClearDraws()
{
    NumGrassDraws = 0;
    for (uint i = 0; i < MAX_MESH_INFOS; i++)
        NumMeshDraws[i] = uvec4(0,0,0,0);

    DrawIndexedCommand command;
    command.indexCount = 6 * NumGrassBlades;
    command.instanceCount = 0;
    command.startIndex = 0;
    command.offsetVertex = 0;
    command.startInstance = 0;
    GrassDraw = command;
}

//------------------------------------------------------------------------------
/**
*/
vec2
SnapToGrid(vec2 worldPos, vec2 gridSize)
{
    vec2 fractionalPart = fmod(worldPos, gridSize);
    return worldPos - fractionalPart;
}

//------------------------------------------------------------------------------
/**
    This shader outputs the draws to be consumed later
*/
[local_size_x] = 64
shader
void
csGenerateDraws()
{
    vec2 cameraPos = CameraPosition.xz;
    vec2 cameraQuad = GenerateQuadSize;

    float scaleFactor = DensityFactor;
    // calculate this fragments position around the camera
    vec2 worldPos = cameraPos - cameraQuad * 0.5f * scaleFactor + gl_GlobalInvocationID.xy * scaleFactor;
    //vec2 worldPos = cameraPos - sin(gl_GlobalInvocationID.x)

    // clamp world position to world grid so we don't move the patches with the camera
    //worldPos = floor(worldPos);
    worldPos = SnapToGrid(worldPos, vec2(scaleFactor));

    // add some random offset to each patch to avoid a perfect grid pattern
    float random = hash12(worldPos);
    vec2 randomOffset = vec2(sin(random * 3.14), cos(random * 3.14));
    worldPos += randomOffset;

    // calculate texture sampling position for masks and such
    vec2 samplePos = (WorldSize * 0.5f + worldPos) * InvWorldSize;
    float dist = dot(cameraPos - worldPos, cameraPos - worldPos);

    // cut-off circle
    if (dist < MaxRange)
    {
        // load height map
        float height = sample2DLod(HeightMap, TextureSampler, samplePos, 0).r;
        height = MinHeight + height * (MaxHeight - MinHeight);

        vec3 pos = vec3(worldPos.x, height, worldPos.y);
        vec3 cameraToPos = CameraPosition.xyz - pos;
        float angle = dot(normalize(cameraToPos), CameraForward.xyz);

        if (angle < Fov)
            return;

        vec2 pixel = samplePos * HeightMapSize;
        vec2 invHeightMapSize = 1.0f / HeightMapSize;

        ivec3 offset = ivec3(-1, 1, 0.0f);
        vec3 normal = CalculateNormalFromHeight(pixel, offset, invHeightMapSize);

        // go through grass types and generate uniforms
        uint textureIndex = 0;
        for (int i = 0; i < NumGrassTypes; i++)
        {
            GrassInfo grassInfo = GrassInfos[i];

            if (!grassInfo.used)
                continue;

            float mask = sampleMask(i, PointSampler, samplePos).r;

            // calculate rules
            bool heightCutoff = height < grassInfo.heightThreshold;
            bool angleCutoff = dot(normal, vec3(0, 1, 0)) > grassInfo.slopeThreshold;

            if ((random > grassInfo.distribution)
                && heightCutoff 
                && angleCutoff)
            {
                uint entry = atomicAdd(NumGrassDraws, 1);

                InstanceUniforms instanceUniform;
                instanceUniform.position.xz = worldPos;
                instanceUniform.position.y = height;
                instanceUniform.random = random;
                instanceUniform.sincos.x = randomOffset.x;
                instanceUniform.sincos.y = randomOffset.y;
                instanceUniform.textureIndex = textureIndex;
#if LOD_DEBUG
                instanceUniform.lod = 0;
#endif
                InstanceGrassUniforms[entry] = instanceUniform;

                atomicAdd(GrassDraw.instanceCount, 1);
            }

            textureIndex++;
        }

        // go through mesh types and generate uniforms and draws
        for (int i = 0; i < NumMeshTypes; i++)
        {
            MeshInfo meshInfo = MeshInfos[i];

            if (!meshInfo.used)
                continue;

            // calculate rules
            bool heightCutoff = height < meshInfo.heightThreshold;
            bool angleCutoff = dot(normal, vec3(0, 1, 0)) > meshInfo.slopeThreshold;

            float mask = sampleMask(i, PointSampler, samplePos).r;
            int lod = meshInfo.numLods - 1;
            for (int j = 0; j < meshInfo.numLods; j++)
            {
                if (meshInfo.lodDistances[j] > dot(cameraToPos, cameraToPos))
                {
                    lod = j;
                    break;
                }
            }

            if (random > meshInfo.distribution && heightCutoff && angleCutoff)
            {
                uint entry = atomicAdd(NumMeshDraws[i / 4][i % 4], 1);

                InstanceUniforms instanceUniform;
                instanceUniform.position.xz = worldPos;
                instanceUniform.position.y = height;
                instanceUniform.random = random;
                instanceUniform.sincos.x = randomOffset.x;
                instanceUniform.sincos.y = randomOffset.y;
                instanceUniform.textureIndex = textureIndex;
                instanceUniform.lodIsBillboard = false;// lod == (meshInfo.numLods - 1);
#if LOD_DEBUG
                instanceUniform.lod = lod;
#endif
                InstanceMeshUniforms[entry] = instanceUniform;

                DrawIndexedCommand command;
                command.indexCount = meshInfo.lodIndexCount[lod];
                command.instanceCount = 1;
                command.startIndex = meshInfo.lodIndexOffsets[lod];
                command.offsetVertex = meshInfo.lodVertexOffsets[lod];
                command.startInstance = entry;  // pretend to use instancing
                MeshDraws[entry + i * MAX_MESH_INFOS] = command;
            }

            textureIndex++;
        }
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
    , out vec3 WorldSpacePos
    , out vec4 ViewSpacePos
    )
{
    InstanceUniforms instanceUniforms = InstanceGrassUniforms[gl_InstanceID];
    float windWeight = position.y * instanceUniforms.random;
    vec3 displacement = vec3(sin(windWeight * Time_Random_Luminance_X.x), 0, cos(windWeight * Time_Random_Luminance_X.x));

    UV = uv;
    Normal = normal;
    Tangent = normalize(vec3(position.x, 0.0f, position.z));
    InstanceTexture = instanceUniforms.textureIndex;
    WorldSpacePos = position + instanceUniforms.position + displacement * 0.2f;
    ViewSpacePos = View * vec4(WorldSpacePos, 1);

    gl_Position = Projection * ViewSpacePos;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psDrawGrassZ(
    in vec3 normal
    , in vec3 tangent
    , in vec2 uv
    , in flat uint instanceTexture
    , in vec3 worldSpacePos
    , in vec4 viewSpacePos)
{
    vec4 albedo = sampleAlbedo(instanceTexture, TextureSampler, uv);

    if (albedo.a < 0.5f) // alpha cut-off
        discard;
}

//------------------------------------------------------------------------------
/**
*/
[earlydepth]
shader
void
psDrawGrass(
    in vec3 normal
    , in vec3 tangent
    , in vec2 uv
    , in flat uint instanceTexture
    , in vec3 worldSpacePos
    , in vec4 viewSpacePos
    , [color0] out vec4 Color)
{
    vec3 albedo = sampleAlbedo(instanceTexture, TextureSampler, uv).rgb;
    
    vec3 geometryNormal = normal;

    // flip normal if other side
    if (!gl_FrontFacing)
        geometryNormal *= -1.0f;

    vec3 binormal = cross(geometryNormal, tangent);
    mat3 tbn = mat3(tangent, binormal, geometryNormal);
    vec4 normalSample = sampleNormal(instanceTexture, TextureSampler, uv);

    geometryNormal.xy = (normalSample.ag * 2.0f) - 1.0f;
    geometryNormal.z = saturate(sqrt(1.0f - dot(geometryNormal.xy, geometryNormal.xy)));
    geometryNormal = tbn * geometryNormal;

    vec4 material = sampleMaterial(instanceTexture, TextureSampler, uv);
    vec3 light = CalculateLight(worldSpacePos, gl_FragCoord.xyz, viewSpacePos.xyz, albedo, material, geometryNormal);

    //Color = vec4(debugCoord.xy, 0, 1);
    Color = vec4(light.rgb, 1);
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsDrawMeshVColor(
    [slot = 0] in vec3 position
    , [slot = 1] in vec3 normal
    , [slot = 2] in vec2 uv
    , [slot = 3] in vec3 tangent
    , [slot = 5] in vec4 color
    , out vec3 Normal
    , out vec3 Tangent
    , out vec2 UV
    , out flat uint InstanceTexture
    , out vec3 WorldSpacePos
    , out vec4 ViewSpacePos)
{
    InstanceUniforms instanceUniforms = InstanceMeshUniforms[gl_InstanceID];
    vec3 displacement = vec3(sin(color.r * Time_Random_Luminance_X.x * instanceUniforms.random), 0, cos(color.r * Time_Random_Luminance_X.x * instanceUniforms.random));
    mat3 rotation = mat3(
        instanceUniforms.sincos.y, 0, instanceUniforms.sincos.x,
        0, 1, 0,
        -instanceUniforms.sincos.x, 0, instanceUniforms.sincos.y);
    vec3 rotatedPos = rotation * (position + displacement * 0.1f);
    

    UV = uv;
    UV.y = 1.0f - UV.y;
    Normal = rotation * normal;
    Tangent = rotation * tangent;
    InstanceTexture = instanceUniforms.textureIndex;
    WorldSpacePos = instanceUniforms.position + rotatedPos * 0.05f;
    if (!instanceUniforms.lodIsBillboard)
        ViewSpacePos = View * vec4(WorldSpacePos, 1);
    else
        ViewSpacePos = vec4(WorldSpacePos, 1);

    gl_Position = Projection * ViewSpacePos;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsDrawMesh(
    [slot = 0] in vec3 position
    , [slot = 2] in vec2 uv
    , [slot = 1] in vec3 normal
    , [slot = 3] in vec3 tangent
    , out vec3 Normal
    , out vec3 Tangent
    , out vec2 UV
    , out flat uint InstanceTexture
    , out vec3 WorldSpacePos
    , out vec4 ViewSpacePos
#ifdef LOD_DEBUG
    , out flat uint Lod
#endif
    )
{
    InstanceUniforms instanceUniforms = InstanceMeshUniforms[gl_InstanceID];
    vec3 displacement = vec3(sin(position.y * Time_Random_Luminance_X.x * instanceUniforms.random), 0, cos(position.y * Time_Random_Luminance_X.x * instanceUniforms.random));
    mat3 rotation = mat3(
        instanceUniforms.sincos.y, 0, instanceUniforms.sincos.x,
        0, 1, 0,
        -instanceUniforms.sincos.x, 0, instanceUniforms.sincos.y);
    vec3 rotatedPos = rotation * (position + displacement * 0.1f);


    UV = uv;
    UV.y = 1.0f - UV.y;
    Normal = rotation * normal;
    Tangent = rotation * tangent;
    InstanceTexture = instanceUniforms.textureIndex;
    WorldSpacePos = instanceUniforms.position + rotatedPos * 0.025f;
    if (!instanceUniforms.lodIsBillboard)
        ViewSpacePos = View * vec4(WorldSpacePos, 1);
    else
        ViewSpacePos = vec4(WorldSpacePos + EyePos.xyz, 1);

#ifdef LOD_DEBUG
    Lod = instanceUniforms.lod;
#endif
    gl_Position = Projection * ViewSpacePos;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psDrawMeshZ(
    in vec3 normal
    , in vec3 tangent
    , in vec2 uv
    , in flat uint instanceTexture
    , in vec3 worldSpacePos
    , in vec4 viewSpacePos)
{
    vec4 albedo = sampleAlbedo(instanceTexture, TextureSampler, uv);

    if (albedo.a < 0.5f) // alpha cut-off
        discard;
}

//------------------------------------------------------------------------------
/**
*/
[earlydepth]
shader
void
psDrawMesh(
    in vec3 normal
    , in vec3 tangent
    , in vec2 uv
    , in flat uint instanceTexture
    , in vec3 worldSpacePos
    , in vec4 viewSpacePos
#ifdef LOD_DEBUG
    , in flat uint lod
#endif
    , [color0] out vec4 Color)
{
    vec3 albedo = sampleAlbedo(instanceTexture, TextureSampler, uv).rgb;
    
    vec3 geometryNormal = normal;

#ifdef LOD_DEBUG
    vec4 colors[] =
    {
        vec4(1,0,0,0)
        , vec4(0, 1, 0, 0)
        , vec4(0, 0, 1, 0)
        , vec4(1, 1, 0, 0)
        , vec4(0, 1, 1, 0)
        , vec4(1, 0, 1, 0)
    };
    Color = colors[lod % 6];
    return;
#endif

    // flip normal if other side
    if (!gl_FrontFacing)
        geometryNormal *= -1.0f;

    vec3 binormal = cross(geometryNormal, tangent);
    mat3 tbn = mat3(tangent, binormal, geometryNormal);
    vec4 normalSample = sampleNormal(instanceTexture, TextureSampler, uv);

    geometryNormal.xy = (normalSample.ag * 2.0f) - 1.0f;
    geometryNormal.z = saturate(sqrt(1.0f - dot(geometryNormal.xy, geometryNormal.xy)));
    geometryNormal = tbn * geometryNormal;

    vec4 material = sampleMaterial(instanceTexture, TextureSampler, uv);

    //vec3 light = vec3(0, 0, 0); 
    vec3 light = CalculateLight(worldSpacePos, gl_FragCoord.xyz, viewSpacePos.xyz, albedo, material, geometryNormal);
    /*
    vec3 viewVec = normalize(EyePos.xyz - worldSpacePos.xyz);  
    vec3 F0 = CalculateF0(albedo.rgb, material[MAT_METALLIC], vec3(0.04));
    vec3 viewNormal = (View * vec4(geometryNormal.xyz, 0)).xyz;

    uint3 index3D = CalculateClusterIndex(gl_FragCoord.xy / BlockSize, viewSpacePos.z, InvZScale, InvZBias);
    uint idx = Pack3DTo1D(index3D, NumCells.x, NumCells.y);

    vec3 light = vec3(0, 0, 0);
    light += CalculateGlobalLight(albedo.rgb, material, F0, viewVec, geometryNormal.xyz, viewSpacePos, vec4(worldSpacePos, 1));
    light += LocalLights(idx, albedo.rgb, material, F0, viewSpacePos, viewNormal, gl_FragCoord.z);
    //light += IBL(albedo, F0, normal, viewVec, material);
    light += albedo.rgb * material[MAT_EMISSIVE];
    */

    //Color = vec4(debugCoord.xy, 0, 1);
    Color = vec4(light.rgb, 1);
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
    DepthWrite = false;
    DepthFunc = Equal;
};

render_state GrassZState
{
    CullMode = None;
    DepthFunc = Less;
};

render_state MeshState
{
    CullMode = None;
    DepthWrite = false;
    DepthFunc = Equal;
};

render_state MeshZState
{
    CullMode = None;
    DepthFunc = Less;
};


SimpleTechnique(VegetationMeshDrawVColorZ, "VegetationMeshDrawVColorZ", vsDrawMeshVColor(), psDrawMeshZ(), MeshZState);
SimpleTechnique(VegetationMeshDrawVColor, "VegetationMeshDrawVColor", vsDrawMeshVColor(), psDrawMesh(), MeshState);
SimpleTechnique(VegetationMeshDrawZ, "VegetationMeshDrawZ", vsDrawMesh(), psDrawMeshZ(), MeshZState);
SimpleTechnique(VegetationMeshDraw, "VegetationMeshDraw", vsDrawMesh(), psDrawMesh(), MeshState);

SimpleTechnique(VegetationGrassDrawZ, "VegetationGrassDrawZ", vsDrawGrass(), psDrawGrassZ(), GrassZState);
SimpleTechnique(VegetationGrassDraw, "VegetationGrassDraw", vsDrawGrass(), psDrawGrass(), GrassState);
program VegetationClear[ string Mask = "VegetationClear"; ]
{
    ComputeShader = csClearDraws();
};
program VegetationGenerateDraws [ string Mask = "VegetationGenerateDraws"; ]
{
    ComputeShader = csGenerateDraws();
};
