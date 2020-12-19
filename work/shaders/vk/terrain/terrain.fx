//------------------------------------------------------------------------------
//  terrain.fx
//  (C) 2020 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"
#include "lib/techniques.fxh"
#include "lib/pbr.fxh"

const int MAX_MATERIAL_TEXTURES = 16;
const int MAX_BIOMES = 16;

group(SYSTEM_GROUP) constant TerrainSystemUniforms [ string Visibility = "VS|HS|DS|PS|CS"; ]
{
    float MinLODDistance;
    float MaxLODDistance;
    float MinTessellation;
    float MaxTessellation;

    uint NumBiomes;
    uint Debug;
    float VirtualLodDistance;

    textureHandle TerrainPosBuffer;
    textureHandle IndirectionBuffer;
    textureHandle AlbedoPhysicalCacheBuffer;
    textureHandle NormalPhysicalCacheBuffer;
    textureHandle MaterialPhysicalCacheBuffer;
    textureHandle AlbedoLowresBuffer; 
    textureHandle NormalLowresBuffer;
    textureHandle MaterialLowresBuffer;


    vec4 BiomeRules[MAX_BIOMES];                    // rules are x: slope, y: height, z: UV scaling factor, w: mip 
};

// this is used to keep track of how many lights we have active
group(BATCH_GROUP) constant TerrainRuntimeUniforms [ string Visibility = "VS|HS|DS|PS|CS"; ]
{
    mat4 Transform;

    float MinHeight;
    float MaxHeight;
    float WorldSizeX;
    float WorldSizeZ;

    uint NumTilesX;
    uint NumTilesY;
    uint TileWidth;
    uint TileHeight;

    uvec2 VirtualTerrainSubTextureSize;
    uvec2 VirtualTerrainNumSubTextures;
    float PhysicalInvPaddedTextureSize;
    uint PhysicalTileSize;
    uint PhysicalTilePaddedSize;
    uint PhysicalTilePadding;

    uvec4 VirtualTerrainTextureSize;
    uvec2 VirtualTerrainPageSize;
    uvec2 VirtualTerrainNumPages;
    uint VirtualTerrainNumMips;

    textureHandle HeightMap;
    textureHandle DecisionMap;

    uint VirtualPageBufferNumPages;
    uvec4 VirtualPageBufferMipOffsets[4];
    uvec4 VirtualPageBufferMipSizes[4];
};

group(DYNAMIC_OFFSET_GROUP) constant TerrainTileUpdateUniforms [ string Visbility = "PS|CS"; ]
{
    vec2 SparseTileWorldOffset;
    float MetersPerTile;
};

group(SYSTEM_GROUP) texture2D       MaterialMaskArray[MAX_BIOMES];
group(SYSTEM_GROUP) texture2DArray  MaterialAlbedoArray[MAX_BIOMES];
group(SYSTEM_GROUP) texture2DArray  MaterialNormalArray[MAX_BIOMES];
group(SYSTEM_GROUP) texture2DArray  MaterialPBRArray[MAX_BIOMES];

struct TerrainSubTexture
{
    vec2 worldCoordinate;
    uvec2 indirectionOffset;
    uint maxMip;
    uint tiles;
};

group(SYSTEM_GROUP) rw_buffer       TerrainSubTexturesBuffer [ string Visibility = "PS|CS"; ]
{
    TerrainSubTexture SubTextures[];
};

const int MAX_PAGE_UPDATES = 1024;

struct PageUpdateList
{
    uint NumEntries;
    uvec4 Entry[MAX_PAGE_UPDATES];
};

group(SYSTEM_GROUP) rw_buffer       PageUpdateListBuffer [ string Visibility = "PS|CS"; ]
{
    PageUpdateList PageList;
};

group(SYSTEM_GROUP) rw_buffer       PageStatusBuffer [ string Visibility = "PS"; ]
{
    uint PageStatuses[];
};

#define sampleBiomeAlbedo(biome, sampler, uv, layer)        texture(sampler2DArray(MaterialAlbedoArray[biome], sampler), vec3(uv, layer))
#define sampleBiomeNormal(biome, sampler, uv, layer)        texture(sampler2DArray(MaterialNormalArray[biome], sampler), vec3(uv, layer))
#define sampleBiomeMaterial(biome, sampler, uv, layer)      texture(sampler2DArray(MaterialPBRArray[biome], sampler), vec3(uv, layer))
#define sampleBiomeMask(biome, sampler, uv)                 texture(sampler2D(MaterialMaskArray[biome], sampler), uv)
#define sampleBiomeMaskLod(biome, sampler, uv, lod)         textureLod(sampler2D(MaterialMaskArray[biome], sampler), uv, lod)

#define fetchBiomeMask(biome, sampler, uv, lod)             texelFetch(sampler2D(MaterialMaskArray[biome], sampler), uv, lod)


group(DYNAMIC_OFFSET_GROUP) constant PatchUniforms [ string Visibility = "VS|PS"; ]
{
    vec2 OffsetPatchPos;
    vec2 OffsetPatchUV;
    vec2 PatchUvScale;
};

group(SYSTEM_GROUP) sampler_state TextureSampler
{
    Filter = Linear;
};

group(SYSTEM_GROUP) sampler_state PointSampler
{
    Filter = Point;
};

group(SYSTEM_GROUP) sampler_state AnisoSampler
{
    Filter = Anisotropic;
    MaxAnisotropic = 4;
};

//------------------------------------------------------------------------------
/**
    Tessellation terrain vertex shader
*/
shader
void
vsTerrain(
    [slot=0] in vec3 position,
    [slot=1] in vec2 uv,
    out vec4 Position,
    out vec2 UV,
    out vec2 LocalUV,
    out vec3 Normal,
    out float Tessellation) 
{
    vec3 offsetPos = position + vec3(OffsetPatchPos.x, 0, OffsetPatchPos.y);
    vec4 modelSpace = Transform * vec4(offsetPos, 1);
    Position = modelSpace;
    UV = uv + OffsetPatchUV;
    LocalUV = uv;

    float vertexDistance = distance( Position.xyz, EyePos.xyz);
    float factor = 1.0f - saturate((MinLODDistance - vertexDistance) / (MinLODDistance - MaxLODDistance));
    float decision = 1.0f - sample2D(DecisionMap, TextureSampler, UV).r;
    Tessellation = MinTessellation + factor * (MaxTessellation - MinTessellation) * decision;

    vec2 sampleUV = (Position.xz / vec2(WorldSizeX, WorldSizeZ)) - 0.5f;

    vec2 pixelSize = textureSize(basic2D(HeightMap), 0);
    pixelSize = vec2(1.0f) / pixelSize;

    vec3 offset = vec3(pixelSize.x, pixelSize.y, 0.0f) * 0.125f;
    float hl = MinHeight + sample2DLod(HeightMap, TextureSampler, UV - offset.xz, 1).r * (MaxHeight - MinHeight);
    float hr = MinHeight + sample2DLod(HeightMap, TextureSampler, UV + offset.xz, 1).r * (MaxHeight - MinHeight);
    float ht = MinHeight + sample2DLod(HeightMap, TextureSampler, UV - offset.zy, 1).r * (MaxHeight - MinHeight);
    float hb = MinHeight + sample2DLod(HeightMap, TextureSampler, UV + offset.zy, 1).r * (MaxHeight - MinHeight);
    Normal.x = hl - hr;
    Normal.y = 2.0f;
    Normal.z = ht - hb;
    Normal = normalize(Normal.xyz);

    gl_Position = modelSpace;
}

//------------------------------------------------------------------------------
/**
*/
float
TessellationFactorScreenSpace(vec4 p0, vec4 p1)
{
    // Calculate edge mid point
    vec4 midPoint = 0.5 * (p0 + p1);
    // Sphere radius as distance between the control points
    float radius = distance(p0, p1) / 2.0;

    // View space
    vec4 v0 = Transform * View * midPoint;

    // Project into clip space
    vec4 clip0 = (Projection * (v0 - vec4(radius, vec3(0.0))));
    vec4 clip1 = (Projection * (v0 + vec4(radius, vec3(0.0))));

    // Get normalized device coordinates
    clip0 /= clip0.w;
    clip1 /= clip1.w;

    // Convert to viewport coordinates
    clip0.xy *= vec2(1280, 1024);
    clip1.xy *= vec2(1280, 1024);

    // Return the tessellation factor based on the screen size 
    // given by the distance of the two edge control points in screen space
    // and a reference (min.) tessellation size for the edge set by the application
    return clamp(distance(clip0, clip1) / 20.0f * 2.5f, MinTessellation, MaxTessellation);
}

//------------------------------------------------------------------------------
/**
    Tessellation terrain hull shader
*/
[inputvertices] = 4
[outputvertices] = 4
shader
void
hsTerrain(
    in vec4 position[],
    in vec2 uv[],
    in vec2 localUv[],
    in vec3 normal[],
    in float tessellation[],
    out vec2 UV[],
    out vec2 LocalUV[],
    out vec4 Position[],
    out vec3 Normal[]) 
{
    UV[gl_InvocationID]         = uv[gl_InvocationID];
    LocalUV[gl_InvocationID]    = localUv[gl_InvocationID];
    Position[gl_InvocationID]   = position[gl_InvocationID];
    Normal[gl_InvocationID]     = normal[gl_InvocationID];

    // provoking vertex gets to decide tessellation factors
    if (gl_InvocationID == 0)
    {
        vec4 EdgeTessFactors;
        //EdgeTessFactors.x = TessellationFactorScreenSpace(Position[2], Position[0]);
        //EdgeTessFactors.y = TessellationFactorScreenSpace(Position[0], Position[1]);
        //EdgeTessFactors.z = TessellationFactorScreenSpace(Position[1], Position[3]);
        //EdgeTessFactors.w = TessellationFactorScreenSpace(Position[3], Position[2]);
        EdgeTessFactors.x = 0.5f * (tessellation[2] + tessellation[0]);
        EdgeTessFactors.y = 0.5f * (tessellation[0] + tessellation[1]);
        EdgeTessFactors.z = 0.5f * (tessellation[1] + tessellation[3]);
        EdgeTessFactors.w = 0.5f * (tessellation[3] + tessellation[2]);


        gl_TessLevelOuter[0] = EdgeTessFactors.x;
        gl_TessLevelOuter[1] = EdgeTessFactors.y;
        gl_TessLevelOuter[2] = EdgeTessFactors.z;
        gl_TessLevelOuter[3] = EdgeTessFactors.w;
        gl_TessLevelInner[0] = mix(gl_TessLevelOuter[0], gl_TessLevelOuter[3], 0.5f);
        gl_TessLevelInner[1] = mix(gl_TessLevelOuter[2], gl_TessLevelOuter[1], 0.5f);
    }
}

//------------------------------------------------------------------------------
/**
    Tessellation terrain shader
*/
[inputvertices] = 4
[winding] = ccw
[topology] = quad
[partition] = even
shader
void
dsTerrain(
    in vec2 uv[],
    in vec2 localUv[],
    in vec4 position[],
    in vec3 normal[],
    out vec2 UV,
    out vec2 LocalUV,
    out vec3 ViewPos,
    out vec3 Normal,
    out vec3 WorldPos) 
{
    WorldPos = mix(
        mix(position[0].xyz, position[1].xyz, gl_TessCoord.x),
        mix(position[2].xyz, position[3].xyz, gl_TessCoord.x),
        gl_TessCoord.y);
        
    Normal = mix(
        mix(normal[0], normal[1], gl_TessCoord.x),
        mix(normal[2], normal[3], gl_TessCoord.x),
        gl_TessCoord.y);

    LocalUV = mix(
        mix(localUv[0], localUv[1], gl_TessCoord.x),
        mix(localUv[2], localUv[3], gl_TessCoord.x),
        gl_TessCoord.y);


    UV = (WorldPos.xz / vec2(WorldSizeX, WorldSizeZ)) - 0.5f;

    float heightValue = sample2DLod(HeightMap, TextureSampler, UV, 0).r;
    WorldPos.y = MinHeight + heightValue * (MaxHeight - MinHeight);
    //WorldPos.y = Height;

    // when we have height adjusted, calculate the view position
    ViewPos = EyePos.xyz - WorldPos.xyz;

    // calculate normals
    /*
    vec2 pixelSize = textureSize(basic2D(HeightMap), 0);
    pixelSize = vec2(1.0f) / pixelSize;

    vec3 offset = vec3(pixelSize.x, pixelSize.y, 0.0f) * 0.05f;
    float hl = MinHeight + sample2DLod(HeightMap, TextureSampler, UV - offset.xz, 1).r * (MaxHeight - MinHeight);
    float hr = MinHeight + sample2DLod(HeightMap, TextureSampler, UV + offset.xz, 1).r * (MaxHeight - MinHeight);
    float ht = MinHeight + sample2DLod(HeightMap, TextureSampler, UV - offset.zy, 1).r * (MaxHeight - MinHeight);
    float hb = MinHeight + sample2DLod(HeightMap, TextureSampler, UV + offset.zy, 1).r * (MaxHeight - MinHeight);
    Normal.x = hl - hr;
    Normal.y = 2.0f;
    Normal.z = ht - hb;
    Normal = normalize(Normal.xyz);
    */
    gl_Position = ViewProjection * vec4(WorldPos, 1);
}

//------------------------------------------------------------------------------
/**
    Pixel shader for multilayered painting
*/
shader
void
psTerrainShadow([color0] out vec2 Shadow) 
{   
    float depth = gl_FragCoord.z / gl_FragCoord.w;
    float moment1 = depth;
    float moment2 = depth * depth;

    // Adjusting moments (this is sort of bias per pixel) using derivative
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    moment2 += 0.25f*(dx*dx+dy*dy);

    Shadow = vec2(moment1, moment2);
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsScreenSpace(
    [slot = 0] in vec3 position,
    [slot = 2] in vec2 uv,
    out vec2 ScreenUV)
{
    gl_Position = vec4(position, 1);
    ScreenUV = uv;
}

//------------------------------------------------------------------------------
/**
*/
float
SlopeBlending(float angle, float worldNormalY)
{
    return 1.0f - saturate(worldNormalY - angle) * (1.0f / (1.0f - angle));
}

//------------------------------------------------------------------------------
/**
*/
float
HeightBlend(float worldY, float height, float falloff)
{
    return saturate((worldY - (height - falloff * 0.5f)) / falloff);
}

//------------------------------------------------------------------------------
/**
*/
void
SampleSlopeRule(
    in uint i, 
    in uint baseArrayIndex, 
    in float angle, 
    in float mask, 
    in vec2 uv, 
    out vec3 outAlbedo, 
    out vec3 outMaterial, 
    out vec3 outNormal)
{
    /*
    Array slots:
        0 - flat surface
        1 - slope surface
        2 - height surface
        3 - height slope surface
    */
    outAlbedo = sampleBiomeAlbedo(i, AnisoSampler, uv, baseArrayIndex).rgb * mask * (1.0f - angle);
    outAlbedo += sampleBiomeAlbedo(i, AnisoSampler, uv, baseArrayIndex + 1).rgb * mask * angle;
    outMaterial = sampleBiomeMaterial(i, AnisoSampler, uv, baseArrayIndex).rgb * mask * (1.0f - angle);
    outMaterial += sampleBiomeMaterial(i, AnisoSampler, uv, baseArrayIndex + 1).rgb * mask * angle;
    outNormal = sampleBiomeNormal(i, AnisoSampler, uv, baseArrayIndex).rgb * (1.0f - angle);
    outNormal += sampleBiomeNormal(i, AnisoSampler, uv, baseArrayIndex + 1).rgb * angle;
}

//------------------------------------------------------------------------------
/**
    Pack data entry 
*/
uvec4
PackPageDataEntry(uint status, uint subTextureIndex, uint mip, uint pageCoordX, uint pageCoordY, uint subTextureTileX, uint subTextureTileY)
{
    uvec4 ret;
    ret.x = (status & 0x3) | (subTextureIndex << 2);
    ret.y = mip;
    ret.z = (pageCoordX & 0xFFFF) | (pageCoordY << 16);
    ret.w = (subTextureTileX & 0xFFFF) | (subTextureTileY << 16);
    return ret;
}

//------------------------------------------------------------------------------
/**
    Pack data entry
*/
uvec4
PageDataSetStatus(uvec4 data, uint status)
{
    uvec4 ret = data;
    ret.x &= ~0x3;
    ret.x |= (status & 0x3);
    return ret;
}

//------------------------------------------------------------------------------
/**
    Pack data entry
*/
uint
PageDataGetStatus(uvec4 data)
{
    return data.x & 0x3;
}

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 1
shader
void
csTerrainPageClearUpdateBuffer()
{
    PageList.NumEntries = 0u;
}

//------------------------------------------------------------------------------
/**
    Calculate the values needed to insert and extract tile data
*/
void
CalculateTileCoords(in uint mip, in uint maxTiles, in vec2 relativePos, in uvec2 subTextureIndirectionOffset, out uvec2 pageCoord, out uvec2 subTextureTile, out vec2 tilePosFract)
{
    // calculate the amount of meters a single tile is, this is adjusted by the mip and the number of tiles at max lod
    vec2 metersPerTile = VirtualTerrainSubTextureSize / float(maxTiles >> mip);

    // calculate subtexture tile index by dividing the relative position by the amount of meters there are per tile
    vec2 tilePos = relativePos / metersPerTile;
    tilePosFract = fract(tilePos);
    subTextureTile = uvec2(tilePos);

    // the actual page within that tile is the indirection offset of the whole
    // subtexture, plus the sub texture tile index
    pageCoord = (subTextureIndirectionOffset >> mip) + subTextureTile;
}

//------------------------------------------------------------------------------
/**
    Pixel shader for outputting our Terrain GBuffer, and update the mip requirement buffer
*/
shader
void
psTerrainPrepass(
    in vec2 uv,
    in vec2 localUv,
    in vec3 viewPos,
    in vec3 normal,
    in vec3 worldPos,
    [color0] out vec4 Pos)
{
    Pos.xy = worldPos.xz;
    Pos.z = 0.0f;
    Pos.w = 0.0f;

    // convert world space to positive integer interval [0..WorldSize]
    vec2 worldSize = vec2(WorldSizeX, WorldSizeZ);
    vec2 unsignedPos = worldPos.xz + worldSize * 0.5f;
    ivec2 subTextureCoord = ivec2(unsignedPos / VirtualTerrainSubTextureSize);

    if (any(lessThan(subTextureCoord, ivec2(0, 0))) || any(greaterThanEqual(subTextureCoord, VirtualTerrainNumSubTextures)))
        return;

    // calculate subtexture index
    uint subTextureIndex = subTextureCoord.x + subTextureCoord.y * VirtualTerrainNumSubTextures.x;
    TerrainSubTexture subTexture = SubTextures[subTextureIndex];

    // if this subtexture is bound on the CPU side, use it
    if (subTexture.tiles != 0)
    {
        // calculate LOD
        const float lodScale = 4 * subTexture.tiles;
        vec2 dy = dFdyFine(worldPos.xz * lodScale);
        vec2 dx = dFdxFine(worldPos.xz * lodScale);
        float d = max(1.0f, max(dot(dx, dx), dot(dy, dy)));
        d = clamp(sqrt(d), 1.0f, pow(2, subTexture.maxMip));
        float lod = log2(d);

        // calculate pixel position relative to the world coordinate for the subtexture
        vec2 relativePos = worldPos.xz - subTexture.worldCoordinate;

        // the mip levels would be those rounded up, and down from the lod value we receive
        uint upperMip = uint(ceil(lod));
        uint lowerMip = uint(floor(lod));

        // calculate tile coords
        uvec2 subTextureTile;
        uvec2 pageCoord;
        vec2 dummy;
        CalculateTileCoords(lowerMip, subTexture.tiles, relativePos, subTexture.indirectionOffset, pageCoord, subTextureTile, dummy);

        // since we have a buffer, we must find the appropriate offset and size into the buffer for this mip
        uint mipOffset = VirtualPageBufferMipOffsets[lowerMip / 4][lowerMip % 4];
        uint mipSize = VirtualPageBufferMipSizes[lowerMip / 4][lowerMip % 4];

        uint index = mipOffset + pageCoord.x + pageCoord.y * mipSize;
        uint status = atomicExchange(PageStatuses[index], 1u);
        if (status == 0x0)
        {
            uvec4 entry = PackPageDataEntry(1u, subTextureIndex, lowerMip, pageCoord.x, pageCoord.y, subTextureTile.x, subTextureTile.y);

            uint entryIndex = atomicAdd(PageList.NumEntries, 1u);
            PageList.Entry[entryIndex] = entry;
        }

        /*
        Pos.x = pageCoord.x;
        Pos.y = pageCoord.y;
        Pos.z = subTextureIndex;
        Pos.w = subTexture.maxMip;
        */

        // if the mips are not identical, we need to repeat this process for the upper mip
        if (upperMip != lowerMip)
        {
            // otherwise, we have to account for both by calculating new tile coords for the upper mip
            uvec2 subTextureTile;
            uvec2 pageCoord;
            CalculateTileCoords(upperMip, subTexture.tiles, relativePos, subTexture.indirectionOffset, pageCoord, subTextureTile, dummy);

            mipOffset = VirtualPageBufferMipOffsets[upperMip / 4][upperMip % 4];
            mipSize = VirtualPageBufferMipSizes[upperMip / 4][upperMip % 4];

            index = mipOffset + pageCoord.x + pageCoord.y * mipSize;
            uint status = atomicExchange(PageStatuses[index], 1u);
            if (status == 0x0)
            {
                uvec4 entry = PackPageDataEntry(1u, subTextureIndex, upperMip, pageCoord.x, pageCoord.y, subTextureTile.x, subTextureTile.y);

                uint entryIndex = atomicAdd(PageList.NumEntries, 1u);
                PageList.Entry[entryIndex] = entry;
            }
        }

        // if the position has w == 1, it means we found a page
        Pos.z = lod;
        Pos.w = 1.0f;
    }
}

//------------------------------------------------------------------------------
/**
*/
float
PackIndirection(uint mip, uint physicalOffsetX, uint physicalOffsetY)
{
    uint res = (mip & 0xF) | ((physicalOffsetX & 0x3FFF) << 4) | ((physicalOffsetY & 0x3FFF) << 18);
    return uintBitsToFloat(res);
}

//------------------------------------------------------------------------------
/**
*/
vec3
UnpackIndirection(uint indirection)
{
    vec3 ret;

    /* IndirectionEntry is formatted as such:
        uint mip : 4;
        uint physicalOffsetX : 14;
        uint physicalOffsetY : 14;
    */

    ret.z = indirection & 0xF;
    ret.x = (indirection >> 4) & 0x3FFF;
    ret.y = (indirection >> 18) & 0x3FFF;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psTerrainTileUpdate(
    in vec2 uv,
    [color0] out vec4 Albedo,
    [color1] out vec3 Normal,
    [color2] out vec3 Material)
{
    // calculate 
    vec2 worldSize = vec2(WorldSizeX, WorldSizeZ);
    vec2 invWorldSize = 1.0f / worldSize;
    vec2 worldPos2D = vec2(SparseTileWorldOffset + uv * MetersPerTile) + worldSize * 0.5f;
    vec2 inputUv = worldPos2D;

    float heightValue = sample2DLod(HeightMap, TextureSampler, inputUv * invWorldSize, 0).r;
    float height = MinHeight + heightValue * (MaxHeight - MinHeight);

    vec3 worldPos = vec3(worldPos2D.x, height, worldPos2D.y);

    // calculate normals by grabbing pixels around our UV
    ivec3 offset = ivec3(2, 2, 0.0f);
    float hl = sample2DLod(HeightMap, TextureSampler, (inputUv - offset.xz) * invWorldSize, 0).r;
    float hr = sample2DLod(HeightMap, TextureSampler, (inputUv + offset.xz) * invWorldSize, 0).r;
    float ht = sample2DLod(HeightMap, TextureSampler, (inputUv - offset.zy) * invWorldSize, 0).r;
    float hb = sample2DLod(HeightMap, TextureSampler, (inputUv + offset.zy) * invWorldSize, 0).r;
    vec3 normal = vec3(0, 0, 0);
    normal.x = MinHeight + (hl - hr) * (MaxHeight - MinHeight);
    normal.y = 4.0f;
    normal.z = MinHeight + (ht - hb) * (MaxHeight - MinHeight);
    normal = normalize(normal.xyz);

    // setup the TBN
    vec3 tangent = cross(normal.xyz, vec3(0, 0, 1));
    tangent = normalize(cross(normal.xyz, tangent));
    vec3 binormal = normalize(cross(normal.xyz, tangent));
    mat3 tbn = mat3(tangent, binormal, normal.xyz);

    // calculate weights for triplanar mapping
    vec3 triplanarWeights = abs(normal.xyz);
    triplanarWeights = normalize(max(triplanarWeights * triplanarWeights, 0.001f));
    float norm = (triplanarWeights.x + triplanarWeights.y + triplanarWeights.z);
    triplanarWeights /= vec3(norm, norm, norm);

    vec3 totalAlbedo = vec3(0, 0, 0);
    vec3 totalMaterial = vec3(0, 0, 0);
    vec3 totalNormal = vec3(0, 0, 0);

    for (uint i = 0; i < NumBiomes; i++)
    {
        // get biome data
        float mask = sampleBiomeMaskLod(i, TextureSampler, inputUv * invWorldSize, 0).r;
        vec4 rules = BiomeRules[i];

        // calculate rules
        float angle = saturate((1.0f - dot(normal, vec3(0, 1, 0))) / 0.5f);
        float heightCutoff = saturate(max(0, height - rules.y) / 25.0f);

        const vec2 tilingFactor = vec2(64);

        if (mask > 0.0f)
        {
            vec3 blendNormal = vec3(0, 0, 0);
            if (heightCutoff == 0.0f)
            {
                vec3 albedo = vec3(0, 0, 0);
                vec3 normal = vec3(0, 0, 0);
                vec3 material = vec3(0, 0, 0);

                SampleSlopeRule(i, 0, angle, mask, worldPos.yz / tilingFactor, albedo, material, normal);
                totalAlbedo += albedo * triplanarWeights.x;
                totalMaterial += material * triplanarWeights.x;
                blendNormal += normal * triplanarWeights.x;

                SampleSlopeRule(i, 0, angle, mask, worldPos.xz / tilingFactor, albedo, material, normal);
                totalAlbedo += albedo * triplanarWeights.y;
                totalMaterial += material * triplanarWeights.y;
                blendNormal += normal * triplanarWeights.y;

                SampleSlopeRule(i, 0, angle, mask, worldPos.xy / tilingFactor, albedo, material, normal);
                totalAlbedo += albedo * triplanarWeights.z;
                totalMaterial += material * triplanarWeights.z;
                blendNormal += normal * triplanarWeights.z;

                blendNormal.xy = blendNormal.xy * 2.0f - 1.0f;
                blendNormal.z = saturate(sqrt(1.0f - dot(blendNormal.xy, blendNormal.xy)));
                totalNormal += (tbn * blendNormal) * mask;
            }
            else
            {
                vec3 albedo = vec3(0, 0, 0);
                vec3 normal = vec3(0, 0, 0);
                vec3 material = vec3(0, 0, 0);
                SampleSlopeRule(i, 2, angle, mask, worldPos.yz / tilingFactor, albedo, material, normal);
                totalAlbedo += albedo * triplanarWeights.x * heightCutoff;
                totalMaterial += material * triplanarWeights.x * heightCutoff;
                blendNormal += normal * triplanarWeights.x * heightCutoff;
                SampleSlopeRule(i, 2, angle, mask, worldPos.xz / tilingFactor, albedo, material, normal);
                totalAlbedo += albedo * triplanarWeights.y * heightCutoff;
                totalMaterial += material * triplanarWeights.y * heightCutoff;
                blendNormal += normal * triplanarWeights.y * heightCutoff;
                SampleSlopeRule(i, 2, angle, mask, worldPos.xy / tilingFactor, albedo, material, normal);
                totalAlbedo += albedo * triplanarWeights.z * heightCutoff;
                totalMaterial += material * triplanarWeights.z * heightCutoff;
                blendNormal += normal * triplanarWeights.z * heightCutoff;
                SampleSlopeRule(i, 0, angle, mask, worldPos.yz / tilingFactor, albedo, material, normal);
                totalAlbedo += albedo * triplanarWeights.x * (1.0f - heightCutoff);
                totalMaterial += material * triplanarWeights.x * (1.0f - heightCutoff);
                blendNormal += normal * triplanarWeights.x * (1.0f - heightCutoff);
                SampleSlopeRule(i, 0, angle, mask, worldPos.xz / tilingFactor, albedo, material, normal);
                totalAlbedo += albedo * triplanarWeights.y * (1.0f - heightCutoff);
                totalMaterial += material * triplanarWeights.y * (1.0f - heightCutoff);
                blendNormal += normal * triplanarWeights.y * (1.0f - heightCutoff);
                SampleSlopeRule(i, 0, angle, mask, worldPos.xy / tilingFactor, albedo, material, normal);
                totalAlbedo += albedo * triplanarWeights.z * (1.0f - heightCutoff);
                totalMaterial += material * triplanarWeights.z * (1.0f - heightCutoff);
                blendNormal += normal * triplanarWeights.z * (1.0f - heightCutoff);

                blendNormal.xy = blendNormal.xy * 2.0f - 1.0f;
                blendNormal.z = saturate(sqrt(1.0f - dot(blendNormal.xy, blendNormal.xy)));
                totalNormal += (tbn * blendNormal) * mask;
            }
        }
    }

    // write output to virtual textures
    Albedo = vec4(totalAlbedo, 1.0f);
    Normal = totalNormal;
    Material = totalMaterial;
}

//------------------------------------------------------------------------------
/**
    Calculate pixel GBuffer data
*/
shader
void
psScreenSpaceVirtual(
    in vec2 ScreenUV,
    [color0] out vec4 Albedo,
    [color1] out vec3 Normal,
    [color2] out vec4 Material)
{
    // sample position, lod and texture sampling mode from screenspace buffer
    vec4 pos = sample2DLod(TerrainPosBuffer, TextureSampler, ScreenUV, 0);
    if (pos.w == 2.0f)
        discard;

    // calculate the subtexture coordinate
    vec2 worldSize = vec2(WorldSizeX, WorldSizeZ);
    vec2 worldPos = pos.xy;
    vec2 worldUv = (worldPos + worldSize * 0.5f) / worldSize;
    vec2 unsignedPos = worldPos + worldSize * 0.5f;
    ivec2 subTextureCoord = ivec2(unsignedPos / VirtualTerrainSubTextureSize);

    if (any(lessThan(subTextureCoord, ivec2(0, 0))) || any(greaterThanEqual(subTextureCoord, VirtualTerrainNumSubTextures)))
    {
        Albedo = sample2D(AlbedoLowresBuffer, AnisoSampler, worldUv);
        Albedo.a = 1.0f;
        Normal = sample2D(NormalLowresBuffer, AnisoSampler, worldUv).xyz;
        Material = sample2D(MaterialLowresBuffer, AnisoSampler, worldUv);
        return;
    }

    // get subtexture
    uint subTextureIndex = subTextureCoord.x + subTextureCoord.y * VirtualTerrainNumSubTextures.x;
    TerrainSubTexture subTexture = SubTextures[subTextureIndex];

    if (subTexture.tiles != 0)
    {
        if (pos.w == 1.0f)
        {
            int lowerMip = int(floor(pos.z));
            int upperMip = int(ceil(pos.z));

            vec2 relativePos = worldPos - subTexture.worldCoordinate;

            // calculate lower mip page coord, page tile coord, and the fractional of the page tile
            uvec2 pageCoordLower;
            uvec2 dummy;
            vec2 subTextureTileFractLower;
            CalculateTileCoords(lowerMip, subTexture.tiles, relativePos, subTexture.indirectionOffset, pageCoordLower, dummy, subTextureTileFractLower);

            // physicalUv represents the pixel offset for this pixel into that page, add padding to account for anisotropy
            vec2 physicalUvLower = subTextureTileFractLower * (PhysicalTileSize) + PhysicalTilePadding;

            // if we need to sample two lods, do bilinear interpolation ourselves
            if (upperMip != lowerMip)
            {
                uvec2 pageCoordUpper;
                uvec2 dummy;
                vec2 subTextureTileFractUpper;
                CalculateTileCoords(upperMip, subTexture.tiles, relativePos, subTexture.indirectionOffset, pageCoordUpper, dummy, subTextureTileFractUpper);
                vec2 physicalUvUpper = subTextureTileFractUpper * (PhysicalTileSize) + PhysicalTilePadding;

                // get the indirection coord and normalize it to the physical space
                vec3 indirectionUpper = UnpackIndirection(floatBitsToUint(fetch2D(IndirectionBuffer, PointSampler, ivec2(pageCoordUpper), upperMip).x));
                vec3 indirectionLower = UnpackIndirection(floatBitsToUint(fetch2D(IndirectionBuffer, PointSampler, ivec2(pageCoordLower), lowerMip).x));

                vec4 albedo0;
                vec4 normal0;
                vec4 material0;
                vec4 albedo1;
                vec4 normal1;
                vec4 material1;

                // if valid mip, sample from physical cache
                if (indirectionUpper.z != 0xF)
                {
                    // convert from texture space to normalized space
                    indirectionUpper.xy = (indirectionUpper.xy + physicalUvUpper) * vec2(PhysicalInvPaddedTextureSize);
                    albedo0 = sample2DLod(AlbedoPhysicalCacheBuffer, AnisoSampler, indirectionUpper.xy, 0);
                    normal0 = sample2DLod(NormalPhysicalCacheBuffer, AnisoSampler, indirectionUpper.xy, 0);
                    material0 = sample2DLod(MaterialPhysicalCacheBuffer, AnisoSampler, indirectionUpper.xy, 0);
                }
                else
                {
                    // otherwise, pick fallback texture
                    albedo0 = sample2D(AlbedoLowresBuffer, AnisoSampler, worldUv);
                    normal0 = sample2D(NormalLowresBuffer, AnisoSampler, worldUv);
                    material0 = sample2D(MaterialLowresBuffer, AnisoSampler, worldUv);
                }

                // same here
                if (indirectionLower.z != 0xF)
                {
                    // convert from texture space to normalized space
                    indirectionLower.xy = (indirectionLower.xy + physicalUvLower) * vec2(PhysicalInvPaddedTextureSize);
                    albedo1 = sample2DLod(AlbedoPhysicalCacheBuffer, AnisoSampler, indirectionLower.xy, 0);
                    normal1 = sample2DLod(NormalPhysicalCacheBuffer, AnisoSampler, indirectionLower.xy, 0);
                    material1 = sample2DLod(MaterialPhysicalCacheBuffer, AnisoSampler, indirectionLower.xy, 0);
                }
                else
                {
                    albedo1 = sample2D(AlbedoLowresBuffer, AnisoSampler, worldUv);
                    normal1 = sample2D(NormalLowresBuffer, AnisoSampler, worldUv);
                    material1 = sample2D(MaterialLowresBuffer, AnisoSampler, worldUv);
                }

                float weight = fract(pos.z);
                Albedo = lerp(albedo1, albedo0, weight);
                Normal = lerp(normal1, normal0, weight).xyz;
                Material = lerp(material1, material0, weight);
            }
            else
            {
                // do the cheap path and just do a single lookup
                vec3 indirection = UnpackIndirection(floatBitsToUint(fetch2D(IndirectionBuffer, PointSampler, ivec2(pageCoordLower), lowerMip).x));
                vec3 orig = indirection;

                // use physical cache if indirection is valid
                if (indirection.z != 0xF)
                {
                    indirection.xy = (indirection.xy + physicalUvLower) * vec2(PhysicalInvPaddedTextureSize);
                    Albedo = sample2DLod(AlbedoPhysicalCacheBuffer, AnisoSampler, indirection.xy, 0);
                    Normal = sample2DLod(NormalPhysicalCacheBuffer, AnisoSampler, indirection.xy, 0).xyz;
                    Material = sample2DLod(MaterialPhysicalCacheBuffer, AnisoSampler, indirection.xy, 0);
                }
                else
                {
                    // otherwise, pick fallback texture
                    Albedo = sample2D(AlbedoLowresBuffer, AnisoSampler, worldUv);
                    Albedo.a = 1.0f;
                    Normal = sample2D(NormalLowresBuffer, AnisoSampler, worldUv).xyz;
                    Material = sample2D(MaterialLowresBuffer, AnisoSampler, worldUv);
                }
            }
        }
        else
        {
            Albedo = sample2D(AlbedoLowresBuffer, AnisoSampler, worldUv);
            Albedo.a = 1.0f;
            Normal = sample2D(NormalLowresBuffer, AnisoSampler, worldUv).xyz;
            Material = sample2D(MaterialLowresBuffer, AnisoSampler, worldUv);
        }

    }
    else
    {
        Albedo = sample2D(AlbedoLowresBuffer, AnisoSampler, worldUv);
        Albedo.a = 1.0f;
        Normal = sample2D(NormalLowresBuffer, AnisoSampler, worldUv).xyz;
        Material = sample2D(MaterialLowresBuffer, AnisoSampler, worldUv);
    }
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psGenerateLowresFallback(
    [color0] out vec4 Albedo,
    [color1] out vec3 Normal,
    [color2] out vec3 Material)
{
    // calculate 
    vec2 textureSize = RenderTargetDimensions[0].zw;
    vec2 pixel = vec2(gl_FragCoord.xy);
    vec2 uv = pixel * textureSize;
    vec2 pixelToWorldScale = vec2(WorldSizeX, WorldSizeZ) * textureSize;

    float heightValue = sample2DLod(HeightMap, TextureSampler, uv, 0).r;
    float height = MinHeight + heightValue * (MaxHeight - MinHeight);

    vec3 worldPos = vec3(pixel.x * pixelToWorldScale.x, height, pixel.y * pixelToWorldScale.y);

    // calculate normals by grabbing pixels around our UV
    ivec3 offset = ivec3(2, 2, 0.0f);
    float hl = sample2DLod(HeightMap, TextureSampler, (pixel - offset.xz) * textureSize, 0).r;
    float hr = sample2DLod(HeightMap, TextureSampler, (pixel + offset.xz) * textureSize, 0).r;
    float ht = sample2DLod(HeightMap, TextureSampler, (pixel - offset.zy) * textureSize, 0).r;
    float hb = sample2DLod(HeightMap, TextureSampler, (pixel + offset.zy) * textureSize, 0).r;
    vec3 normal = vec3(0, 0, 0);
    normal.x = MinHeight + (hl - hr) * (MaxHeight - MinHeight);
    normal.y = 4.0f;
    normal.z = MinHeight + (ht - hb) * (MaxHeight - MinHeight);
    normal = normalize(normal.xyz);

    // setup the TBN
    vec3 tangent = cross(normal.xyz, vec3(0, 0, 1));
    tangent = normalize(cross(normal.xyz, tangent));
    vec3 binormal = normalize(cross(normal.xyz, tangent));
    mat3 tbn = mat3(tangent, binormal, normal.xyz);

    // calculate weights for triplanar mapping
    vec3 triplanarWeights = abs(normal.xyz);
    triplanarWeights = normalize(max(triplanarWeights * triplanarWeights, 0.001f));
    float norm = (triplanarWeights.x + triplanarWeights.y + triplanarWeights.z);
    triplanarWeights /= vec3(norm, norm, norm);

    vec3 totalAlbedo = vec3(0, 0, 0);
    vec3 totalMaterial = vec3(0, 0, 0);
    vec3 totalNormal = vec3(0, 0, 0);

    for (uint i = 0; i < NumBiomes; i++)
    {
        // get biome data
        float mask = sampleBiomeMaskLod(i, TextureSampler, uv, 0).r;
        vec4 rules = BiomeRules[i];

        // calculate rules
        float angle = saturate((1.0f - dot(normal, vec3(0, 1, 0))) / 0.5f);
        float heightCutoff = saturate(max(0, height - rules.y) / 25.0f);

        const vec2 tilingFactor = vec2(64.0f);

        if (mask > 0.0f)
        {
            vec3 blendNormal = vec3(0, 0, 0);
            if (heightCutoff == 0.0f)
            {
                vec3 albedo = vec3(0, 0, 0);
                vec3 normal = vec3(0, 0, 0);
                vec3 material = vec3(0, 0, 0);

                SampleSlopeRule(i, 0, angle, mask, worldPos.yz / tilingFactor, albedo, material, normal);
                totalAlbedo += albedo * triplanarWeights.x;
                totalMaterial += material * triplanarWeights.x;
                blendNormal += normal * triplanarWeights.x;

                SampleSlopeRule(i, 0, angle, mask, worldPos.xz / tilingFactor, albedo, material, normal);
                totalAlbedo += albedo * triplanarWeights.y;
                totalMaterial += material * triplanarWeights.y;
                blendNormal += normal * triplanarWeights.y;

                SampleSlopeRule(i, 0, angle, mask, worldPos.xy / tilingFactor, albedo, material, normal);
                totalAlbedo += albedo * triplanarWeights.z;
                totalMaterial += material * triplanarWeights.z;
                blendNormal += normal * triplanarWeights.z;

                blendNormal.xy = blendNormal.xy * 2.0f - 1.0f;
                blendNormal.z = saturate(sqrt(1.0f - dot(blendNormal.xy, blendNormal.xy)));
                totalNormal += (tbn * blendNormal) * mask;
            }
            else
            {
                vec3 albedo = vec3(0, 0, 0);
                vec3 normal = vec3(0, 0, 0);
                vec3 material = vec3(0, 0, 0);
                SampleSlopeRule(i, 2, angle, mask, worldPos.yz / tilingFactor, albedo, material, normal);
                totalAlbedo += albedo * triplanarWeights.x * heightCutoff;
                totalMaterial += material * triplanarWeights.x * heightCutoff;
                blendNormal += normal * triplanarWeights.x * heightCutoff;
                SampleSlopeRule(i, 2, angle, mask, worldPos.xz / tilingFactor, albedo, material, normal);
                totalAlbedo += albedo * triplanarWeights.y * heightCutoff;
                totalMaterial += material * triplanarWeights.y * heightCutoff;
                blendNormal += normal * triplanarWeights.y * heightCutoff;
                SampleSlopeRule(i, 2, angle, mask, worldPos.xy / tilingFactor, albedo, material, normal);
                totalAlbedo += albedo * triplanarWeights.z * heightCutoff;
                totalMaterial += material * triplanarWeights.z * heightCutoff;
                blendNormal += normal * triplanarWeights.z * heightCutoff;
                SampleSlopeRule(i, 0, angle, mask, worldPos.yz / tilingFactor, albedo, material, normal);
                totalAlbedo += albedo * triplanarWeights.x * (1.0f - heightCutoff);
                totalMaterial += material * triplanarWeights.x * (1.0f - heightCutoff);
                blendNormal += normal * triplanarWeights.x * (1.0f - heightCutoff);
                SampleSlopeRule(i, 0, angle, mask, worldPos.xz / tilingFactor, albedo, material, normal);
                totalAlbedo += albedo * triplanarWeights.y * (1.0f - heightCutoff);
                totalMaterial += material * triplanarWeights.y * (1.0f - heightCutoff);
                blendNormal += normal * triplanarWeights.y * (1.0f - heightCutoff);
                SampleSlopeRule(i, 0, angle, mask, worldPos.xy / tilingFactor, albedo, material, normal);
                totalAlbedo += albedo * triplanarWeights.z * (1.0f - heightCutoff);
                totalMaterial += material * triplanarWeights.z * (1.0f - heightCutoff);
                blendNormal += normal * triplanarWeights.z * (1.0f - heightCutoff);

                blendNormal.xy = blendNormal.xy * 2.0f - 1.0f;
                blendNormal.z = saturate(sqrt(1.0f - dot(blendNormal.xy, blendNormal.xy)));
                totalNormal += (tbn * blendNormal) * mask;
            }
        }
    }

    Albedo = vec4(totalAlbedo, 1.0f);
    Normal = totalNormal;
    Material = totalMaterial;
}

//------------------------------------------------------------------------------
/**
*/
render_state TerrainState
{
    CullMode = Back;
    //FillMode = Line;
};

render_state FinalState
{
};

render_state TerrainShadowState
{
    CullMode = Back;
    DepthClamp = false;
    DepthEnabled = false;
    DepthWrite = false;
    BlendEnabled[0] = true;
    SrcBlend[0] = One;
    DstBlend[0] = One;
    BlendOp[0] = Min;
};

TessellationTechnique(TerrainPrepass, "TerrainPrepass", vsTerrain(), psTerrainPrepass(), hsTerrain(), dsTerrain(), TerrainState);
SimpleTechnique(TerrainVirtualScreenSpace, "TerrainVirtualScreenSpace", vsScreenSpace(), psScreenSpaceVirtual(), FinalState);
SimpleTechnique(TerrainTileUpdate, "TerrainTileUpdate", vsScreenSpace(), psTerrainTileUpdate(), FinalState);
SimpleTechnique(TerrainLowresFallback, "TerrainLowresFallback", vsScreenSpace(), psGenerateLowresFallback(), FinalState);

program TerrainPageClearUpdateBuffer [ string Mask = "TerrainPageClearUpdateBuffer"; ]
{
    ComputeShader = csTerrainPageClearUpdateBuffer();
};