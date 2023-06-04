//------------------------------------------------------------------------------
//  terrain.fx
//  (C) 2020 Individual contributors, see AUTHORS file
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

group(SYSTEM_GROUP) constant MaterialLayers[string Visibility = "PS|CS";]
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

group(DYNAMIC_OFFSET_GROUP) constant TerrainTileUpdateUniforms [ string Visbility = "PS|CS"; ]
{
    vec2 SparseTileWorldOffset;
    float MetersPerTile;
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

group(SYSTEM_GROUP) rw_buffer TerrainSubTexturesBuffer [ string Visibility = "PS|CS"; ]
{
    TerrainSubTexture SubTextures[];
};

const uint MAX_PAGE_UPDATES = 4096;

#define fetchIndirection(coords, mip, bias) UnpackIndirection(floatBitsToUint(fetch2D(IndirectionBuffer, PointSampler, coords, int(mip + bias)).x));

struct PageUpdateList
{
    uint NumEntries;
    uvec4 Entry[MAX_PAGE_UPDATES];
    uint PageStatuses[MAX_PAGE_UPDATES];
};

group(SYSTEM_GROUP) rw_buffer       PageUpdateListBuffer [ string Visibility = "PS|CS"; ]
{
    PageUpdateList PageList;
};

group(SYSTEM_GROUP) rw_buffer       PageStatusBuffer [ string Visibility = "PS|CS"; ]
{
    uint PageStatuses[];
};

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

group(SYSTEM_GROUP) sampler_state AnisoSampler
{
    Filter = Anisotropic;
    MaxAnisotropic = 4;
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
    Tessellation terrain vertex shader
*/
shader
void
vsTerrain(
    [slot=0] in vec3 position
    , [slot=1] in vec2 uv
    , out vec4 Position
    , out vec3 Normal
    , out float Tessellation
) 
{
    vec3 offsetPos = position + vec3(OffsetPatchPos.x, 0, OffsetPatchPos.y);
    vec4 modelSpace = Transform * vec4(offsetPos, 1);
    Position = vec4(offsetPos, 1);
    vec2 UV = uv + OffsetPatchUV;

    float vertexDistance = distance( Position.xyz, EyePos.xyz);
    float factor = 1.0f - saturate((MinLODDistance - vertexDistance) / (MinLODDistance - MaxLODDistance));
    float decision = 1.0f - sample2D(DecisionMap, TextureSampler, UV).r;
    Tessellation = MinTessellation + factor * (MaxTessellation - MinTessellation) * decision;

    vec2 sampleUV = (Position.xz / vec2(WorldSizeX, WorldSizeZ)) - 0.5f;

    vec2 pixelSize = textureSize(basic2D(HeightMap), 0);
    pixelSize = vec2(1.0f) / pixelSize;

    vec3 offset = vec3(-pixelSize.x, pixelSize.x, 0.0f);
    Normal = CalculateNormalFromHeight(UV, offset);

    gl_Position = modelSpace;
}

//------------------------------------------------------------------------------
/**
*/
float
TessellationFactorScreenSpace(vec4 p0, vec4 p1)
{
    /*
    mat4 mvp = Transform * ViewProjection;
    vec4 p0Proj = mvp * p0;
    vec4 p1Proj = mvp * p1;
    float screen = max(RenderTargetDimensions[0].x, RenderTargetDimensions[0].y);
    float dist = distance(p0Proj.xy / p0Proj.w, p1Proj.xy / p1Proj.w);
    return clamp(dist * screen, MinTessellation, MaxTessellation);
    */
    // Calculate edge mid point
    vec4 midPoint = 0.5 * (p0 + p1);
    // Sphere radius as distance between the control points
    float radius = distance(p0, p1) * 0.5f;

    // View space
    vec4 v0 = Transform * View * midPoint;

    // Project into clip space
    vec4 clip0 = (Projection * (v0 - vec4(radius, radius, 0.0f, 0.0f)));
    vec4 clip1 = (Projection * (v0 + vec4(radius, radius, 0.0f, 0.0f)));

    // Get normalized device coordinates
    clip0 /= clip0.w;
    clip1 /= clip1.w;

    // Convert to viewport coordinates
    clip0.xy *= RenderTargetDimensions[0].xy;
    clip1.xy *= RenderTargetDimensions[0].xy;

    // Return the tessellation factor based on the screen size 
    // given by the distance of the two edge control points in screen space
    // and a reference (min.) tessellation size for the edge set by the application
    return clamp(distance(clip0, clip1) / 24.0f, MinTessellation, MaxTessellation);
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
    in vec4 position[]
    , in vec3 normal[]
    , in float tessellation[]
    , out vec4 Position[]
    , out vec3 Normal[]
)
{
    Position[gl_InvocationID]   = Transform * position[gl_InvocationID];
    Normal[gl_InvocationID]     = normal[gl_InvocationID];

    // provoking vertex gets to decide tessellation factors
    if (gl_InvocationID == 0)
    {
        vec4 EdgeTessFactors;
        EdgeTessFactors.x = TessellationFactorScreenSpace(position[2], position[0]);
        EdgeTessFactors.y = TessellationFactorScreenSpace(position[0], position[1]);
        EdgeTessFactors.z = TessellationFactorScreenSpace(position[1], position[3]);
        EdgeTessFactors.w = TessellationFactorScreenSpace(position[3], position[2]);
        //EdgeTessFactors.x = 0.5f * (tessellation[2] + tessellation[0]);
        //EdgeTessFactors.y = 0.5f * (tessellation[0] + tessellation[1]);
        //EdgeTessFactors.z = 0.5f * (tessellation[1] + tessellation[3]);
        //EdgeTessFactors.w = 0.5f * (tessellation[3] + tessellation[2]);


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
    in vec4 position[],
    in vec3 normal[],
    out vec2 UV,
    out vec3 ViewPos,
    out vec3 Normal,
    out vec3 Position)
{
    Position = mix(
        mix(position[0].xyz, position[1].xyz, gl_TessCoord.x),
        mix(position[2].xyz, position[3].xyz, gl_TessCoord.x),
        gl_TessCoord.y);
        
    Normal = mix(
        mix(normal[0], normal[1], gl_TessCoord.x),
        mix(normal[2], normal[3], gl_TessCoord.x),
        gl_TessCoord.y);

    UV = (Position.xz / vec2(WorldSizeX, WorldSizeZ)) - 0.5f;

    float heightValue = sample2DLod(HeightMap, TextureSampler, UV, 0).r;
    Position.y = MinHeight + heightValue * (MaxHeight - MinHeight);

    // when we have height adjusted, calculate the view position
    ViewPos = EyePos.xyz - Position.xyz;

    gl_Position = ViewProjection * vec4(Position, 1);
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
*/
[local_size_x] = 64
shader
void
csTerrainPageClearUpdateBuffer()
{
    // Clear page statuses
    if (gl_GlobalInvocationID.x < PageList.NumEntries)
        PageStatuses[PageList.PageStatuses[gl_GlobalInvocationID.x]] = 0x0;

    // Then reset the number of entries to 0
    if (gl_GlobalInvocationID.x == 0 && gl_GlobalInvocationID.y == 0)
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
    in vec3 viewPos,
    in vec3 normal,
    in vec3 worldPos,
    [color0] out vec4 Pos)
{
    Pos.xy = worldPos.xz;
    Pos.z = 0.0f;
    Pos.w = query_lod2D(AlbedoLowresBuffer, TextureSampler, uv).y;

    // convert world space to positive integer interval [0..WorldSize]
    vec2 worldSize = vec2(WorldSizeX, WorldSizeZ);
    vec2 unsignedPos = worldPos.xz + worldSize * 0.5f;
    ivec2 subTextureCoord = ivec2(unsignedPos / VirtualTerrainSubTextureSize);

    if (any(lessThan(subTextureCoord, ivec2(0, 0))) || any(greaterThanEqual(subTextureCoord, VirtualTerrainNumSubTextures)))
        return;

    // calculate subtexture index
    uint subTextureIndex = subTextureCoord.x + subTextureCoord.y * VirtualTerrainNumSubTextures.x;
    TerrainSubTexture subTexture = SubTextures[subTextureIndex];

    uvec2 dummydummy, indirectionOffset;
    uint maxMip, tiles, mipBias;
    UnpackSubTexture(subTexture, dummydummy, indirectionOffset, maxMip, mipBias, tiles);

    // if this subtexture is bound on the CPU side, use it
    if (tiles != 1)
    {
        // calculate pixel position relative to the world coordinate for the subtexture
        vec2 relativePos = worldPos.xz - subTexture.worldCoordinate;
        //float lod = (Pos.w / IndirectionNumMips) * maxMip;
        
        const float lodScale = 4 * tiles;
        vec2 dy = dFdyFine(worldPos.xz * lodScale);
        vec2 dx = dFdxFine(worldPos.xz * lodScale);
        float d = max(1.0f, max(dot(dx, dx), dot(dy, dy)));
        d = clamp(sqrt(d), 1.0f, pow(2, maxMip));
        float lod = log2(d);

        // the mip levels would be those rounded up, and down from the lod value we receive
        uint upperMip = uint(ceil(lod));
        uint lowerMip = uint(floor(lod));

        // calculate tile coords
        uvec2 subTextureTile;
        uvec2 pageCoord;
        vec2 dummy;
        CalculateTileCoords(lowerMip, tiles, relativePos, indirectionOffset, pageCoord, subTextureTile, dummy);

        // since we have a buffer, we must find the appropriate offset and size into the buffer for this mip
        uint mipOffset = VirtualPageBufferMipOffsets[lowerMip / 4][lowerMip % 4];
        uint mipSize = VirtualPageBufferMipSizes[lowerMip / 4][lowerMip % 4];

        uint index = mipOffset + pageCoord.x + pageCoord.y * mipSize;
        uint status = atomicExchange(PageStatuses[index], 1u);
        if (status == 0x0)
        {
            uvec4 entry = PackPageDataEntry(1u, subTextureIndex, lowerMip, maxMip, subTextureTile.x, subTextureTile.y);

            uint entryIndex = atomicAdd(PageList.NumEntries, 1u);
            PageList.Entry[entryIndex] = entry;
            PageList.PageStatuses[entryIndex] = index;
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
            CalculateTileCoords(upperMip, tiles, relativePos, indirectionOffset, pageCoord, subTextureTile, dummy);

            mipOffset = VirtualPageBufferMipOffsets[upperMip / 4][upperMip % 4];
            mipSize = VirtualPageBufferMipSizes[upperMip / 4][upperMip % 4];

            index = mipOffset + pageCoord.x + pageCoord.y * mipSize;
            uint status = atomicExchange(PageStatuses[index], 1u);
            if (status == 0x0)
            {
                uvec4 entry = PackPageDataEntry(1u, subTextureIndex, upperMip, maxMip, subTextureTile.x, subTextureTile.y);

                uint entryIndex = atomicAdd(PageList.NumEntries, 1u);
                PageList.Entry[entryIndex] = entry;
                PageList.PageStatuses[entryIndex] = index;
            }
        }

        // if the position has w == 1, it means we found a page
        Pos.z = lod + mipBias;
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
shader
void
psTerrainTileUpdate(
    in vec2 uv,
    [color0] out vec3 Albedo,
    [color1] out vec3 Normal,
    [color2] out vec4 Material)
{
    // calculate 
    vec2 worldSize = vec2(WorldSizeX, WorldSizeZ);
    vec2 invWorldSize = 1.0f / worldSize;
    vec2 worldPos2D = vec2(SparseTileWorldOffset + uv * MetersPerTile) + worldSize * 0.5f;
    vec2 inputUv = worldPos2D;

    vec3 normal = sample2DLod(NormalLowresBuffer, TextureSampler, inputUv * invWorldSize, 0).xyz;
    float heightValue = sample2DLod(HeightMap, TextureSampler, inputUv * invWorldSize, 0).r;
    float height = MinHeight + heightValue * (MaxHeight - MinHeight);

    vec3 worldPos = vec3(worldPos2D.x, height, worldPos2D.y);

    // calculate normals by grabbing pixels around our UV
    //ivec3 offset = ivec3(-1, 1, 0.0f);
    //vec3 normal = CalculateNormalFromHeight(inputUv, offset, invWorldSize);

    // setup the TBN
    vec3 tangent = cross(normal.xyz, vec3(0, 0, 1));
    tangent = normalize(cross(normal.xyz, tangent));
    vec3 binormal = normalize(cross(normal.xyz, tangent));
    mat3 tbn = mat3(tangent, binormal, normal.xyz);

    // calculate weights for triplanar mapping
    vec3 triplanarWeights = abs(normal.xyz);
    triplanarWeights /= (triplanarWeights.x + triplanarWeights.y + triplanarWeights.z);

    vec3 totalAlbedo = vec3(0, 0, 0);
    vec4 totalMaterial = vec4(0, 0, 0, 0);
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
                vec4 material = vec4(0, 0, 0, 0);

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
                vec4 material = vec4(0, 0, 0, 0);

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
    Albedo = totalAlbedo;
    Normal = totalNormal;
    Material = totalMaterial;
}

//------------------------------------------------------------------------------
/**
    Calculate pixel color
*/
shader
void
psScreenSpaceVirtual(
    in vec2 ScreenUV,
    [color0] out vec4 Color)
{
    // sample position, lod and texture sampling mode from screenspace buffer
    vec4 pos = sample2DLod(TerrainPosBuffer, TextureSampler, ScreenUV, 0);
    if (pos.w == 255.0f)
        discard;

    // calculate the subtexture coordinate
    vec2 worldSize = vec2(WorldSizeX, WorldSizeZ);
    vec2 worldPos = pos.xy;
    vec2 worldUv = (worldPos + worldSize * 0.5f) / worldSize;
    vec2 unsignedPos = worldPos + worldSize * 0.5f;
    ivec2 subTextureCoord = ivec2(unsignedPos / VirtualTerrainSubTextureSize);

    vec3 albedo = sample2DLod(AlbedoLowresBuffer, TextureSampler, worldUv, pos.w).rgb;
    vec3 normal = sample2DLod(NormalLowresBuffer, TextureSampler, worldUv, pos.w).xyz;
    vec4 material = sample2DLod(MaterialLowresBuffer, TextureSampler, worldUv, pos.w);

    if (any(lessThan(subTextureCoord, ivec2(0, 0))) || any(greaterThanEqual(subTextureCoord, VirtualTerrainNumSubTextures)))
    {
        vec3 viewVec = normalize(EyePos.xyz - pos.xyz);
        vec3 F0 = CalculateF0(albedo.rgb, material[MAT_METALLIC], vec3(0.04));
        vec3 viewNormal = (View * vec4(normal.xyz, 0)).xyz;
        vec4 viewPos = (View * vec4(pos.xyz, 1));

        uint3 index3D = CalculateClusterIndex(gl_FragCoord.xy / BlockSize, pos.z, InvZScale, InvZBias);
        uint idx = Pack3DTo1D(index3D, NumCells.x, NumCells.y);

        vec3 light = vec3(0, 0, 0);
        light += CalculateGlobalLight(albedo.rgb, material, F0, viewVec, normal, pos.xxy);
        light += LocalLights(idx, albedo.rgb, material, F0, pos.xyz, normal, gl_FragCoord.z);
        //light += IBL(albedo, F0, normal, viewVec, material);
        light += albedo.rgb * material[MAT_EMISSIVE];
        Color = vec4(light, 1);
        return;
    }

    // get subtexture
    uint subTextureIndex = subTextureCoord.x + subTextureCoord.y * VirtualTerrainNumSubTextures.x;
    TerrainSubTexture subTexture = SubTextures[subTextureIndex];

    uvec2 dummydummy, indirectionOffset;
    uint maxMip, tiles, mipBias;
    UnpackSubTexture(subTexture, dummydummy, indirectionOffset, maxMip, mipBias, tiles);

    vec2 debugCoord = uvec2(0,0);

    if (tiles != 1)
    {
        int lowerMip = int(floor(pos.z));
        int upperMip = int(ceil(pos.z));

        vec2 cameraRelativePos = worldPos.xy - EyePos.xz;
        float distSquared = dot(cameraRelativePos, cameraRelativePos);
        float blendWeight = (distSquared - LowresFadeStart) * LowresFadeDistance;
        blendWeight = clamp(blendWeight, 0, 1);

        vec2 relativePos = worldPos - subTexture.worldCoordinate;

        // calculate lower mip page coord, page tile coord, and the fractional of the page tile
        uvec2 pageCoordLower;
        uvec2 dummy;
        vec2 subTextureTileFractLower;
        CalculateTileCoords(lowerMip, tiles, relativePos, indirectionOffset, pageCoordLower, dummy, subTextureTileFractLower);

        // physicalUv represents the pixel offset for this pixel into that page, add padding to account for anisotropy
        vec2 physicalUvLower = subTextureTileFractLower * PhysicalTileSize + PhysicalTilePadding;

        vec3 albedo0;
        vec3 normal0;
        vec4 material0;

        // if we need to sample two lods, do bilinear interpolation ourselves
        if (upperMip != lowerMip)
        {
            uvec2 pageCoordUpper;
            uvec2 dummy;
            vec2 subTextureTileFractUpper;
            CalculateTileCoords(upperMip, tiles, relativePos, indirectionOffset, pageCoordUpper, dummy, subTextureTileFractUpper);
            vec2 physicalUvUpper = subTextureTileFractUpper * (PhysicalTileSize) + PhysicalTilePadding;

            // get the indirection coord and normalize it to the physical space
            uvec3 indirectionUpper = fetchIndirection(ivec2(pageCoordUpper), int(upperMip), 0);
            uvec3 indirectionLower = fetchIndirection(ivec2(pageCoordLower), int(lowerMip), 0);
            debugCoord = indirectionLower.xy / vec2(2048.0f);


            vec3 albedo1;
            vec3 normal1;
            vec4 material1;

            // if valid mip, sample from physical cache
            if (indirectionUpper.z != 0xF)
            {
                // convert from texture space to normalized space
                vec2 indirection = (indirectionUpper.xy + physicalUvUpper) * vec2(PhysicalInvPaddedTextureSize);
                albedo0 = sample2DLod(AlbedoPhysicalCacheBuffer, TextureSampler, indirection.xy, 0).rgb;
                normal0 = sample2DLod(NormalPhysicalCacheBuffer, TextureSampler, indirection.xy, 0).xyz;
                material0 = sample2DLod(MaterialPhysicalCacheBuffer, TextureSampler, indirection.xy, 0);
            }
            else
            {
                // otherwise, pick fallback texture
                albedo0 = albedo;
                normal0 = normal;
                material0 = material;
            }

            // same here
            if (indirectionLower.z != 0xF)
            {
                // convert from texture space to normalized space
                vec2 indirection = (indirectionLower.xy + physicalUvLower) * vec2(PhysicalInvPaddedTextureSize);
                albedo1 = sample2DLod(AlbedoPhysicalCacheBuffer, TextureSampler, indirection.xy, 0).rgb;
                normal1 = sample2DLod(NormalPhysicalCacheBuffer, TextureSampler, indirection.xy, 0).xyz;
                material1 = sample2DLod(MaterialPhysicalCacheBuffer, TextureSampler, indirection.xy, 0);
            }
            else
            {
                albedo1 = albedo;
                normal1 = normal;
                material1 = material;
            }

            float weight = fract(pos.z);
            albedo0 = lerp(albedo1, albedo0, weight);
            normal0 = lerp(normal1, normal0, weight);
            material0 = lerp(material1, material0, weight);
        }
        else
        {
            // do the cheap path and just do a single lookup
            uvec3 indirection = fetchIndirection(ivec2(pageCoordLower), int(lowerMip), 0);
            debugCoord = indirection.xy;

            // use physical cache if indirection is valid
            if (indirection.z != 0xF)
            {
                vec2 indir = (indirection.xy + physicalUvLower) * vec2(PhysicalInvPaddedTextureSize);
                albedo0 = sample2DLod(AlbedoPhysicalCacheBuffer, TextureSampler, indir.xy, 0).rgb;
                normal0 = sample2DLod(NormalPhysicalCacheBuffer, TextureSampler, indir.xy, 0).xyz;
                material0 = sample2DLod(MaterialPhysicalCacheBuffer, TextureSampler, indir.xy, 0);
            }
            else
            {
                // otherwise, pick fallback texture
                albedo0 = albedo;
                normal0 = normal;
                material0 = material;
            }
        }

        if (blendWeight >= 0.0f)
        {
            albedo = lerp(albedo0, albedo, blendWeight);
            normal = lerp(normal0, normal, blendWeight);
            material = lerp(material0, material, blendWeight);
        }
        else
        {
            albedo = albedo0;
            normal = normal0;
            material = material0;
        }
    }

    vec3 viewVec = normalize(EyePos.xyz - pos.xyz);
    vec3 F0 = CalculateF0(albedo.rgb, material[MAT_METALLIC], vec3(0.04));
    vec3 viewNormal = (View * vec4(normal.xyz, 0)).xyz;
    vec4 viewPos = (View * vec4(pos.xyz, 1));

    uint3 index3D = CalculateClusterIndex(gl_FragCoord.xy / BlockSize, pos.z, InvZScale, InvZBias);
    uint idx = Pack3DTo1D(index3D, NumCells.x, NumCells.y);

    vec3 light = vec3(0, 0, 0);
    light += CalculateGlobalLight(albedo.rgb, material, F0, viewVec, normal, pos.xxy);
    light += LocalLights(idx, albedo.rgb, material, F0, pos.xyz, viewNormal, gl_FragCoord.z);
    //light += IBL(albedo, F0, normal, viewVec, material);
    light += albedo.rgb * material[MAT_EMISSIVE];

    //Color = vec4(debugCoord.xy, 0, 1);
    Color = vec4(light.rgb, 1);
    //Color.rgb = vec3(debugCoord.xy, 0) + light.rgb;
    //Color.a = 1.0f;
    //Color = albedo;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psGenerateLowresFallback(
    in vec2 dummy,
    [color0] out vec3 Albedo,
    [color1] out vec3 Normal,
    [color2] out vec4 Material)
{
    // calculate 
    vec2 texelSize = RenderTargetDimensions[0].zw;
    vec2 pixel = vec2(gl_FragCoord.xy);
    vec2 uv = pixel * texelSize;
    vec2 pixelToWorldScale = vec2(WorldSizeX, WorldSizeZ) * texelSize;

    float heightValue = sample2DLod(HeightMap, TextureSampler, uv, 0).r;
    float height = MinHeight + heightValue * (MaxHeight - MinHeight);

    vec3 worldPos = vec3(pixel.x * pixelToWorldScale.x, height, pixel.y * pixelToWorldScale.y);

    // calculate normals by grabbing pixels around our UV
    ivec3 offset = ivec3(-1, 1, 0.0f);
    vec3 normal = CalculateNormalFromHeight(pixel, offset, texelSize);

    // setup the TBN
    vec3 tangent = cross(normal.xyz, vec3(0, 0, 1));
    tangent = normalize(cross(normal.xyz, tangent));
    vec3 binormal = normalize(cross(normal.xyz, tangent));
    mat3 tbn = mat3(tangent, binormal, normal.xyz);

    // calculate weights for triplanar mapping
    vec3 triplanarWeights = abs(normal.xyz);
    triplanarWeights /= (triplanarWeights.x + triplanarWeights.y + triplanarWeights.z);

    vec3 totalAlbedo = vec3(0, 0, 0);
    vec4 totalMaterial = vec4(0, 0, 0, 0);
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
                vec4 material = vec4(0, 0, 0, 0);

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
                vec4 material = vec4(0, 0, 0, 0);

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

    Albedo = totalAlbedo;
    Normal = totalNormal;
    Material = totalMaterial;
}

group(SYSTEM_GROUP) readwrite r32f image2D TerrainShadowMap;
//------------------------------------------------------------------------------
/**
    Copy between indirection textures
*/
[local_size_x] = 64
shader
void
csTerrainShadows()
{
    if (any(greaterThan(gl_GlobalInvocationID.xy, TerrainShadowMapSize)))
        return;

    vec2 uv = gl_GlobalInvocationID.xy * TerrainShadowMapPixelSize;
    // Take a sample for the pixel we are dealing with right now
    float heightAtPixel = MinHeight + sample2DLod(HeightMap, ShadowSampler, uv, 0).r * (MaxHeight - MinHeight);

    vec3 startCoord = vec3(gl_GlobalInvocationID.x, heightAtPixel, gl_GlobalInvocationID.y);
    vec3 endCoord = startCoord + GlobalLightDirWorldspace.xyz * vec3(100000, 100000, 100000);

    // Adjust these parameters per game
    // For terrain with a lot of small details, use a smaller MaxDistance for more precision
    // For large terrains where you need long shadows, use a bigger MaxDistance for longer casting shadows
    const uint NumSamples = 32;
    const float MaxDistance = 256.0f;
    const float InitialStepSize = MaxDistance / NumSamples;
    float stepSize = InitialStepSize;
    vec3 coord = startCoord + GlobalLightDirWorldspace.xyz * stepSize;

    float smallestDistance = 10000000.0f;
    vec4 plane = vec4(GlobalLightDirWorldspace.xyz, 0);
    for (uint i = 0; i < NumSamples; i++)
    {
        // Sample height at current position
        float heightAlongRay = MinHeight + sample2DLod(HeightMap, ShadowSampler, coord.xz * TerrainShadowMapPixelSize, 0).r * (MaxHeight - MinHeight);

        // This is the world space position of the point
        vec3 sampleCoord = vec3(coord.x, heightAlongRay, coord.z);

        // Construct a plane and do a ray-plane intersection
        plane.w = dot(sampleCoord, GlobalLightDirWorldspace.xyz);
        vec3 intersection;
        IntersectLineWithPlane(startCoord, endCoord, plane, intersection);

        // If the intersection point with the plane is above the sample
        // it means the point is in shadow
        if (coord.y <= intersection.y)
        {
            // Calculate distance for contact hardening shadows
            float dist = distance(startCoord, sampleCoord);
            smallestDistance = min(smallestDistance, dist);

            // Half step size in preparation of the next sample
            stepSize *= 0.5f;

            // Move coord back half the distance traveled in search fo the closest point of intersection
            coord -= GlobalLightDirWorldspace.xyz * dist * 0.5f;
        }
        else
        {
            // Progress coord
            coord += GlobalLightDirWorldspace.xyz * stepSize;
        }
    }
    float shadow;
    if (smallestDistance == 10000000.0f)
        shadow = 1.0f;
    else
        // Do a bit of dirty inverse square falloff
        shadow = (smallestDistance * smallestDistance) / (MaxDistance * MaxDistance);

    imageStore(TerrainShadowMap, ivec2(gl_GlobalInvocationID.xy), vec4(shadow));
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
    DepthWrite = false;
    DepthEnabled = false;
};

TessellationTechnique(TerrainPrepass, "TerrainPrepass", vsTerrain(), hsTerrain(), dsTerrain(), psTerrainPrepass(), TerrainState);
SimpleTechnique(TerrainVirtualScreenSpace, "TerrainVirtualScreenSpace", vsScreenSpace(), psScreenSpaceVirtual(), FinalState);
SimpleTechnique(TerrainTileUpdate, "TerrainTileUpdate", vsScreenSpace(), psTerrainTileUpdate(), FinalState);
SimpleTechnique(TerrainLowresFallback, "TerrainLowresFallback", vsScreenSpace(), psGenerateLowresFallback(), FinalState);

program TerrainPageClearUpdateBuffer [ string Mask = "TerrainPageClearUpdateBuffer"; ]
{
    ComputeShader = csTerrainPageClearUpdateBuffer();
};

program TerrainShadows [ string Mask = "TerrainShadows"; ]
{
    ComputeShader = csTerrainShadows();
};