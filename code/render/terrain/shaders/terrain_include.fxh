//------------------------------------------------------------------------------
//  @file terrain_include.fxh
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"
#include "lib/techniques.fxh"
#include "lib/pbr.fxh"
#include "lib/clustering.fxh"
#include "lib/lighting_functions.fxh"
const int MAX_MATERIAL_TEXTURES = 16;
const int MAX_BIOMES = 16;
const int MAX_TILES_PER_FRAME = 256;

//------------------------------------------------------------------------------
/**
            GENERIC PARAMS
*/
//------------------------------------------------------------------------------
group(SYSTEM_GROUP) constant TerrainSystemUniforms[string Visibility = "VS|HS|DS|PS|CS";]
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
group(SYSTEM_GROUP) readwrite rg16f image2D TerrainShadowMap;


group(SYSTEM_GROUP) constant MaterialLayers[string Visibility = "PS|CS"; ]
{
    ivec4 MaterialAlbedos[MAX_BIOMES];
    ivec4 MaterialNormals[MAX_BIOMES];
    ivec4 MaterialPBRs[MAX_BIOMES];
    ivec4 MaterialMasks[MAX_BIOMES];
    ivec4 MaterialWeights[MAX_BIOMES];
};

#define sampleBiomeAlbedo(biome, sampler, uv, layer)        sample2D(MaterialAlbedos[biome][layer], sampler, uv)
#define sampleBiomeNormal(biome, sampler, uv, layer)        sample2D(MaterialNormals[biome][layer], sampler, uv)
#define sampleBiomeMaterial(biome, sampler, uv, layer)      sample2D(MaterialPBRs[biome][layer], sampler, uv)
#define sampleBiomeMask(biome, sampler, uv)                 sample2D(MaterialMasks[biome / 4][biome % 4], sampler, uv)
#define sampleBiomeMaskLod(biome, sampler, uv, lod)         sample2DLod(MaterialMasks[biome / 4][biome % 4], sampler, uv, lod)

#define fetchBiomeMask(biome, sampler, uv, lod)             fetch2D(MaterialMasks[biome / 4][biome % 4], sampler, uv, lod)

// These are per-terrain uniforms
group(BATCH_GROUP) constant TerrainRuntimeUniforms[string Visibility = "VS|HS|DS|PS|CS";]
{
    mat4 Transform;

    float MinHeight;
    float MaxHeight;
    float WorldSizeX;
    float WorldSizeZ;

    vec2 DataBufferSize;

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

    uvec2 LowresResolution;
    uint LowresNumMips;
    float LowresFadeStart;
    float LowresFadeDistance;

    uvec2 IndirectionResolution;
    uint IndirectionNumMips;

    textureHandle HeightMap;
    textureHandle DecisionMap;

    uint VirtualPageBufferNumPages;
    uvec4 VirtualPageBufferMipOffsets[4];
    uvec4 VirtualPageBufferMipSizes[4];
};


struct TerrainSubTexture
{
    vec2 worldCoordinate;
    // Contains 
    //          indirection offset x as first 12 bits 
    //          indirection offset y as next 12 bits
    //          maxMip as the next 4 bits
    //          leaving 4 unused
    uint packed0;
};

group(SYSTEM_GROUP) rw_buffer TerrainSubTexturesBuffer[string Visibility = "PS|CS";]
{
    TerrainSubTexture SubTextures[];
};

const uint MAX_PAGE_UPDATES = 4096;

#define fetchIndirection(coords, mip, bias) UnpackIndirection(floatBitsToUint(fetch2D(IndirectionBuffer, PointSampler, coords, int(mip + bias)).x));

struct PageUpdateList
{
    uvec4 Entry[MAX_PAGE_UPDATES];
    uint PageStatuses[MAX_PAGE_UPDATES];
    int NumEntries;
};

group(SYSTEM_GROUP) rw_buffer       PageUpdateListBuffer[string Visibility = "PS|CS";]
{
    PageUpdateList PageList;
};

group(SYSTEM_GROUP) rw_buffer       PageStatusBuffer[string Visibility = "PS|CS";]
{
    uint PageStatuses[];
};

struct TerrainPatch
{
    vec2 PosOffset;
    vec2 UvOffset;
};

group(SYSTEM_GROUP) rw_buffer TerrainPatchData[string Visibility = "VS|PS";]
{
    TerrainPatch Patches[];
};

group(SYSTEM_GROUP) sampler_state TextureSampler
{
    Filter = Linear;
};

group(SYSTEM_GROUP) sampler_state AnisoSampler
{
    Filter = Anisotropic;
    MaxAnisotropic = 4;
};

//------------------------------------------------------------------------------
/**
    TILE WRITE PARAMS
*/
//------------------------------------------------------------------------------
group(BATCH_GROUP) write rgba32f image2D AlbedoCacheOutputBC;
group(BATCH_GROUP) write rgba32f image2D NormalCacheOutputBC;
group(BATCH_GROUP) write rgba32f image2D MaterialCacheOutputBC;

group(BATCH_GROUP) write rgba8 image2D AlbedoLowresOutput;
group(BATCH_GROUP) write rgba8 image2D NormalLowresOutput;
group(BATCH_GROUP) write rgba8 image2D MaterialLowresOutput;

struct TileWrite
{
    uvec2 WriteOffset_MetersPerTile;
    vec2 WorldOffset;
};
group(BATCH_GROUP) constant TilesToWrite
{
    TileWrite TileWrites[MAX_TILES_PER_FRAME];
};


//------------------------------------------------------------------------------
/**
*/
vec3
CalculateNormalFromHeight(vec2 uv, vec3 offset)
{
    float hl = sample2DLod(HeightMap, TextureSampler, uv + offset.xz, 0).r;
    float hr = sample2DLod(HeightMap, TextureSampler, uv + offset.yz, 0).r;
    float ht = sample2DLod(HeightMap, TextureSampler, uv + offset.zx, 0).r;
    float hb = sample2DLod(HeightMap, TextureSampler, uv + offset.zy, 0).r;
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
    out vec4 outMaterial,
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
    outMaterial = sampleBiomeMaterial(i, AnisoSampler, uv, baseArrayIndex) * mask * (1.0f - angle);
    outMaterial += sampleBiomeMaterial(i, AnisoSampler, uv, baseArrayIndex + 1) * mask * angle;
    outNormal = sampleBiomeNormal(i, AnisoSampler, uv, baseArrayIndex).rgb * (1.0f - angle);
    outNormal += sampleBiomeNormal(i, AnisoSampler, uv, baseArrayIndex + 1).rgb * angle;
}

//------------------------------------------------------------------------------
/**
    Pack data entry
*/
uvec4
PackPageDataEntry(uint status, uint subTextureIndex, uint mip, uint maxMip, uint subTextureTileX, uint subTextureTileY)
{
    uvec4 ret;
    ret.x = (status & 0x3) | (subTextureIndex << 2);
    ret.y = (mip & 0xF) | ((maxMip & 0xF) << 4) | ((subTextureTileX & 0xFF) << 8) | ((subTextureTileY & 0xFF) << 16);

    // Delete these
    ret.z = subTextureTileX;
    ret.w = subTextureTileY;
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
*/
void
UnpackSubTexture(TerrainSubTexture subTex, out uvec2 worldOffset, out uvec2 indirectionOffset, out uint mip, out uint mipBias, out uint tiles)
{
    indirectionOffset.x = subTex.packed0 & 0xFFF;
    indirectionOffset.y = (subTex.packed0 >> 12) & 0xFFF;
    mip = (subTex.packed0 >> 24) & 0xF;
    mipBias = (subTex.packed0 >> 28) & 0xF;
    tiles = 1 << mip;
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
uvec3
UnpackIndirection(uint indirection)
{
    uvec3 ret;

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
void
SampleTerrain(
    uint biome
    , mat3 tbn
    , float angle
    , float heightCutoff
    , float mask
    , vec2 tilingFactor
    , vec3 worldPos
    , vec3 triplanarWeights
    , inout vec3 outAlbedo
    , inout vec4 outMaterial
    , inout vec3 outNormal
)
{
    if (mask > 0.0f)
    {
        vec3 blendNormal = vec3(0, 0, 0);
        if (heightCutoff == 0.0f)
        {
            vec3 albedo = vec3(0, 0, 0);
            vec3 normal = vec3(0, 0, 0);
            vec4 material = vec4(0, 0, 0, 0);

            SampleSlopeRule(biome, 0, angle, mask, worldPos.yz / tilingFactor, albedo, material, normal);
            outAlbedo += albedo * triplanarWeights.x;
            outMaterial += material * triplanarWeights.x;
            blendNormal += normal * triplanarWeights.x;

            SampleSlopeRule(biome, 0, angle, mask, worldPos.xz / tilingFactor, albedo, material, normal);
            outAlbedo += albedo * triplanarWeights.y;
            outMaterial += material * triplanarWeights.y;
            blendNormal += normal * triplanarWeights.y;

            SampleSlopeRule(biome, 0, angle, mask, worldPos.xy / tilingFactor, albedo, material, normal);
            outAlbedo += albedo * triplanarWeights.z;
            outMaterial += material * triplanarWeights.z;
            blendNormal += normal * triplanarWeights.z;

            blendNormal.xy = blendNormal.xy * 2.0f - 1.0f;
            blendNormal.z = saturate(sqrt(1.0f - dot(blendNormal.xy, blendNormal.xy)));
            outNormal += (tbn * blendNormal) * mask;
        }
        else
        {
            vec3 albedo = vec3(0, 0, 0);
            vec3 normal = vec3(0, 0, 0);
            vec4 material = vec4(0, 0, 0, 0);

            SampleSlopeRule(biome, 2, angle, mask, worldPos.yz / tilingFactor, albedo, material, normal);
            outAlbedo += albedo * triplanarWeights.x * heightCutoff;
            outMaterial += material * triplanarWeights.x * heightCutoff;
            blendNormal += normal * triplanarWeights.x * heightCutoff;

            SampleSlopeRule(biome, 2, angle, mask, worldPos.xz / tilingFactor, albedo, material, normal);
            outAlbedo += albedo * triplanarWeights.y * heightCutoff;
            outMaterial += material * triplanarWeights.y * heightCutoff;
            blendNormal += normal * triplanarWeights.y * heightCutoff;

            SampleSlopeRule(biome, 2, angle, mask, worldPos.xy / tilingFactor, albedo, material, normal);
            outAlbedo += albedo * triplanarWeights.z * heightCutoff;
            outMaterial += material * triplanarWeights.z * heightCutoff;
            blendNormal += normal * triplanarWeights.z * heightCutoff;

            SampleSlopeRule(biome, 0, angle, mask, worldPos.yz / tilingFactor, albedo, material, normal);
            outAlbedo += albedo * triplanarWeights.x * (1.0f - heightCutoff);
            outMaterial += material * triplanarWeights.x * (1.0f - heightCutoff);
            blendNormal += normal * triplanarWeights.x * (1.0f - heightCutoff);

            SampleSlopeRule(biome, 0, angle, mask, worldPos.xz / tilingFactor, albedo, material, normal);
            outAlbedo += albedo * triplanarWeights.y * (1.0f - heightCutoff);
            outMaterial += material * triplanarWeights.y * (1.0f - heightCutoff);
            blendNormal += normal * triplanarWeights.y * (1.0f - heightCutoff);

            SampleSlopeRule(biome, 0, angle, mask, worldPos.xy / tilingFactor, albedo, material, normal);
            outAlbedo += albedo * triplanarWeights.z * (1.0f - heightCutoff);
            outMaterial += material * triplanarWeights.z * (1.0f - heightCutoff);
            blendNormal += normal * triplanarWeights.z * (1.0f - heightCutoff);

            blendNormal.xy = blendNormal.xy * 2.0f - 1.0f;
            blendNormal.z = saturate(sqrt(1.0f - dot(blendNormal.xy, blendNormal.xy)));
            outNormal += (tbn * blendNormal) * mask;
        }
    }
}
