//------------------------------------------------------------------------------
//  @file terrain_tile_write.fx
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "terrain_include.fxh"
#include "lib/compression/bccompression.fxh"

groupshared vec3 AlbedoCacheResult[8][8];
groupshared vec2 NormalCacheResult[8][8];
groupshared vec4 MaterialCacheResult[8][8];

//------------------------------------------------------------------------------
/**
*/
uint4
BlockCompressAlbedo(uvec2 blockStart)
{
    vec3 blockRGB[16];
    float blockA[16] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

    blockRGB[0] = AlbedoCacheResult[blockStart.x][blockStart.y];
    blockRGB[1] = AlbedoCacheResult[blockStart.x + 1][blockStart.y];
    blockRGB[2] = AlbedoCacheResult[blockStart.x + 2][blockStart.y];
    blockRGB[3] = AlbedoCacheResult[blockStart.x + 3][blockStart.y];

    blockRGB[4] = AlbedoCacheResult[blockStart.x][blockStart.y + 1];
    blockRGB[5] = AlbedoCacheResult[blockStart.x + 1][blockStart.y + 1];
    blockRGB[6] = AlbedoCacheResult[blockStart.x + 2][blockStart.y + 1];
    blockRGB[7] = AlbedoCacheResult[blockStart.x + 3][blockStart.y + 1];

    blockRGB[8] = AlbedoCacheResult[blockStart.x][blockStart.y + 2];
    blockRGB[9] = AlbedoCacheResult[blockStart.x + 1][blockStart.y + 2];
    blockRGB[10] = AlbedoCacheResult[blockStart.x + 2][blockStart.y + 2];
    blockRGB[11] = AlbedoCacheResult[blockStart.x + 3][blockStart.y + 2];

    blockRGB[12] = AlbedoCacheResult[blockStart.x][blockStart.y + 3];
    blockRGB[13] = AlbedoCacheResult[blockStart.x + 1][blockStart.y + 3];
    blockRGB[14] = AlbedoCacheResult[blockStart.x + 2][blockStart.y + 3];
    blockRGB[15] = AlbedoCacheResult[blockStart.x + 3][blockStart.y + 3];

    return CompressBC3Block(blockRGB, blockA, 1.0f);
}

//------------------------------------------------------------------------------
/**
*/
uint4
BlockCompressNormals(uvec2 blockStart)
{
    float blockU[16], blockV[16];
    blockU[0] = NormalCacheResult[blockStart.x][blockStart.y].x;
    blockU[1] = NormalCacheResult[blockStart.x + 1][blockStart.y].x;
    blockU[2] = NormalCacheResult[blockStart.x + 2][blockStart.y].x;
    blockU[3] = NormalCacheResult[blockStart.x + 3][blockStart.y].x;
    blockV[0] = NormalCacheResult[blockStart.x][blockStart.y].y;
    blockV[1] = NormalCacheResult[blockStart.x + 1][blockStart.y].y;
    blockV[2] = NormalCacheResult[blockStart.x + 2][blockStart.y].y;
    blockV[3] = NormalCacheResult[blockStart.x + 3][blockStart.y].y;

    blockU[4] = NormalCacheResult[blockStart.x][blockStart.y + 1].x;
    blockU[5] = NormalCacheResult[blockStart.x + 1][blockStart.y + 1].x;
    blockU[6] = NormalCacheResult[blockStart.x + 2][blockStart.y + 1].x;
    blockU[7] = NormalCacheResult[blockStart.x + 3][blockStart.y + 1].x;
    blockV[4] = NormalCacheResult[blockStart.x][blockStart.y + 1].y;
    blockV[5] = NormalCacheResult[blockStart.x + 1][blockStart.y + 1].y;
    blockV[6] = NormalCacheResult[blockStart.x + 2][blockStart.y + 1].y;
    blockV[7] = NormalCacheResult[blockStart.x + 3][blockStart.y + 1].y;

    blockU[8] = NormalCacheResult[blockStart.x][blockStart.y + 2].x;
    blockU[9] = NormalCacheResult[blockStart.x + 1][blockStart.y + 2].x;
    blockU[10] = NormalCacheResult[blockStart.x + 2][blockStart.y + 2].x;
    blockU[11] = NormalCacheResult[blockStart.x + 3][blockStart.y + 2].x;
    blockV[8] = NormalCacheResult[blockStart.x][blockStart.y + 2].y;
    blockV[9] = NormalCacheResult[blockStart.x + 1][blockStart.y + 2].y;
    blockV[10] = NormalCacheResult[blockStart.x + 2][blockStart.y + 2].y;
    blockV[11] = NormalCacheResult[blockStart.x + 3][blockStart.y + 2].y;

    blockU[12] = NormalCacheResult[blockStart.x][blockStart.y + 3].x;
    blockU[13] = NormalCacheResult[blockStart.x + 1][blockStart.y + 3].x;
    blockU[14] = NormalCacheResult[blockStart.x + 2][blockStart.y + 3].x;
    blockU[15] = NormalCacheResult[blockStart.x + 3][blockStart.y + 3].x;
    blockV[12] = NormalCacheResult[blockStart.x][blockStart.y + 3].y;
    blockV[13] = NormalCacheResult[blockStart.x + 1][blockStart.y + 3].y;
    blockV[14] = NormalCacheResult[blockStart.x + 2][blockStart.y + 3].y;
    blockV[15] = NormalCacheResult[blockStart.x + 3][blockStart.y + 3].y;

    return CompressBC5Block(blockU, blockV, 1.0f);
}

//------------------------------------------------------------------------------
/**
*/
uint4
BlockCompressMaterials(uvec2 blockStart)
{
    vec3 blockRGB[16];
    float blockA[16];

    blockRGB[0] = MaterialCacheResult[blockStart.x][blockStart.y].rgb;
    blockA[0] = MaterialCacheResult[blockStart.x][blockStart.y].a;
    blockRGB[1] = MaterialCacheResult[blockStart.x + 1][blockStart.y].rgb;
    blockA[1] = MaterialCacheResult[blockStart.x + 1][blockStart.y].a;
    blockRGB[2] = MaterialCacheResult[blockStart.x + 2][blockStart.y].rgb;
    blockA[2] = MaterialCacheResult[blockStart.x + 2][blockStart.y].a;
    blockRGB[3] = MaterialCacheResult[blockStart.x + 3][blockStart.y].rgb;
    blockA[3] = MaterialCacheResult[blockStart.x + 3][blockStart.y].a;

    blockRGB[4] = MaterialCacheResult[blockStart.x][blockStart.y + 1].rgb;
    blockA[4] = MaterialCacheResult[blockStart.x][blockStart.y + 1].a;
    blockRGB[5] = MaterialCacheResult[blockStart.x + 1][blockStart.y + 1].rgb;
    blockA[5] = MaterialCacheResult[blockStart.x + 1][blockStart.y + 1].a;
    blockRGB[6] = MaterialCacheResult[blockStart.x + 2][blockStart.y + 1].rgb;
    blockA[6] = MaterialCacheResult[blockStart.x + 2][blockStart.y + 1].a;
    blockRGB[7] = MaterialCacheResult[blockStart.x + 3][blockStart.y + 1].rgb;
    blockA[7] = MaterialCacheResult[blockStart.x + 3][blockStart.y + 1].a;

    blockRGB[8] = MaterialCacheResult[blockStart.x][blockStart.y + 2].rgb;
    blockA[8] = MaterialCacheResult[blockStart.x][blockStart.y + 2].a;
    blockRGB[9] = MaterialCacheResult[blockStart.x + 1][blockStart.y + 2].rgb;
    blockA[9] = MaterialCacheResult[blockStart.x + 1][blockStart.y + 2].a;
    blockRGB[10] = MaterialCacheResult[blockStart.x + 2][blockStart.y + 2].rgb;
    blockA[10] = MaterialCacheResult[blockStart.x + 2][blockStart.y + 2].a;
    blockRGB[11] = MaterialCacheResult[blockStart.x + 3][blockStart.y + 2].rgb;
    blockA[11] = MaterialCacheResult[blockStart.x + 3][blockStart.y + 2].a;

    blockRGB[12] = MaterialCacheResult[blockStart.x][blockStart.y + 3].rgb;
    blockA[12] = MaterialCacheResult[blockStart.x][blockStart.y + 3].a;
    blockRGB[13] = MaterialCacheResult[blockStart.x + 1][blockStart.y + 3].rgb;
    blockA[13] = MaterialCacheResult[blockStart.x + 1][blockStart.y + 3].a;
    blockRGB[14] = MaterialCacheResult[blockStart.x + 2][blockStart.y + 3].rgb;
    blockA[14] = MaterialCacheResult[blockStart.x + 2][blockStart.y + 3].a;
    blockRGB[15] = MaterialCacheResult[blockStart.x + 3][blockStart.y + 3].rgb;
    blockA[15] = MaterialCacheResult[blockStart.x + 3][blockStart.y + 3].a;

    return CompressBC3Block(blockRGB, blockA, 1.0f);
}


//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 8
[local_size_y] = 8
shader
void
csWriteTile()
{
    // calculate 
    ivec2 location = ivec2(gl_GlobalInvocationID.xy);
    if (location.x >= PhysicalTilePaddedSize || location.y >= PhysicalTilePaddedSize)
        return;

    ivec2 writeOffset = ivec2(unpack_2u16(TileWrites[gl_GlobalInvocationID.z].WriteOffset_MetersPerTile.x));
    float metersPerTile = uintBitsToFloat(TileWrites[gl_GlobalInvocationID.z].WriteOffset_MetersPerTile.y);
    vec2 UV = (vec2(location) + vec2(0.5f)) / vec2(PhysicalTilePaddedSize, PhysicalTilePaddedSize);
    vec2 worldSize = vec2(WorldSizeX, WorldSizeZ);
    vec2 invWorldSize = 1.0f / worldSize;
    vec2 worldPos2D = vec2(TileWrites[gl_GlobalInvocationID.z].WorldOffset + UV * metersPerTile) + worldSize * 0.5f;
    vec2 inputUv = worldPos2D;

    //vec3 normal = sample2DLod(NormalLowresBuffer, TextureSampler, inputUv * invWorldSize, 0).xyz;
    float heightValue = sample2DLod(HeightMap, TextureSampler, inputUv * invWorldSize, 0).r;
    float height = MinHeight + heightValue * (MaxHeight - MinHeight);

    vec3 worldPos = vec3(worldPos2D.x, height, worldPos2D.y);

    // calculate normals by grabbing pixels around our UV
    ivec3 offset = ivec3(-1, 1, 0.0f);
    vec3 normal = CalculateNormalFromHeight(inputUv, offset, invWorldSize);

    // setup the TBN
    mat3 tbn = PlaneTBN(normal);

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

        vec2 tilingFactor = vec2(rules.z);

        SampleTerrain(i, tbn, angle, heightCutoff, mask, tilingFactor, worldPos, triplanarWeights, totalAlbedo, totalMaterial, totalNormal);
    }

    // Prepare for BC compression
    AlbedoCacheResult[gl_LocalInvocationID.x][gl_LocalInvocationID.y] = totalAlbedo;
    NormalCacheResult[gl_LocalInvocationID.x][gl_LocalInvocationID.y] = (totalNormal.xy + vec2(1, 1)) * vec2(0.5f);
    MaterialCacheResult[gl_LocalInvocationID.x][gl_LocalInvocationID.y] = totalMaterial;
    barrier();

    // only perform write for each invocation in a 8x8 work group
    if (gl_LocalInvocationID.x < 2 && gl_LocalInvocationID.y < 2)
    {
        uvec2 blockStart = gl_LocalInvocationID.xy * 4;

        uint4 albedoRes = BlockCompressAlbedo(blockStart);
        uint4 materialRes = BlockCompressMaterials(blockStart);
        uint4 normalRes = BlockCompressNormals(blockStart);
        
        // Since we work in 4x4 tiles, the offset to the 264 tile is 66
        ivec2 bcTileOffset = writeOffset / 4;
        // Since each thread samples 4x4, we have 2x2 threads that write, so gl_LocalInvocationID.xy will be [0..1]
        ivec2 bcTileLocalOffset = ivec2(gl_LocalInvocationID.xy);
        // And since each work group is responsible for writing 2x2 pixels BC compressed, the work group o
        ivec2 bcTileWorkgroupOffset = ivec2(gl_WorkGroupID.xy * 2);

        // The offset within the workgroup should just be invocation id which ranges between 0..1
        ivec2 workgroupOffset = ivec2(gl_LocalInvocationID.xy);
        imageStore(AlbedoCacheOutputBC, bcTileOffset + bcTileWorkgroupOffset + bcTileLocalOffset, uintBitsToFloat(albedoRes));
        imageStore(MaterialCacheOutputBC, bcTileOffset + bcTileWorkgroupOffset + bcTileLocalOffset, uintBitsToFloat(materialRes));
        imageStore(NormalCacheOutputBC, bcTileOffset + bcTileWorkgroupOffset + bcTileLocalOffset, uintBitsToFloat(normalRes));
    }
}

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 8
[local_size_y] = 8
shader
void
csWriteLowres()
{
    // calculate 
    vec2 worldSize = vec2(WorldSizeX, WorldSizeZ);
    vec2 invWorldSize = 1.0f / worldSize;

    vec2 texelSize = 1 / vec2(gl_NumWorkGroups.xy * gl_WorkGroupSize.xy);
    vec2 pixel = vec2(gl_GlobalInvocationID.xy);
    vec2 uv = (pixel + vec2(0.5f)) * texelSize;
    vec2 pixelToWorldScale = vec2(WorldSizeX, WorldSizeZ) * texelSize;

    float heightValue = sample2DLod(HeightMap, TextureSampler, uv, 0).r;
    float height = MinHeight + heightValue * (MaxHeight - MinHeight);

    vec3 worldPos = vec3(pixel.x * pixelToWorldScale.x, height, pixel.y * pixelToWorldScale.y);

    // calculate normals by grabbing pixels around our UV
    ivec3 offset = ivec3(-1, 1, 0.0f);
    vec3 normal = CalculateNormalFromHeight(worldPos.xz, offset, invWorldSize);

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

        vec2 tilingFactor = vec2(rules.z);

        SampleTerrain(i, tbn, angle, heightCutoff, mask, tilingFactor, worldPos, triplanarWeights, totalAlbedo, totalMaterial, totalNormal);
    }

    imageStore(AlbedoLowresOutput, ivec2(pixel), vec4(totalAlbedo, 0));
    imageStore(NormalLowresOutput, ivec2(pixel), vec4(totalNormal, 0));
    imageStore(MaterialLowresOutput, ivec2(pixel), totalMaterial);

    /* Enable when we can figure out how to mipmap BC textures easily
    // Prepare for BC compression
    AlbedoCacheResult[gl_LocalInvocationID.x][gl_LocalInvocationID.y] = totalAlbedo;
    NormalCacheResult[gl_LocalInvocationID.x][gl_LocalInvocationID.y] = (totalNormal.xy + vec2(1, 1)) * vec2(0.5f);
    MaterialCacheResult[gl_LocalInvocationID.x][gl_LocalInvocationID.y] = totalMaterial;
    barrier();

    // only perform write for each invocation in a 8x8 work group
    if (gl_LocalInvocationID.x < 2 && gl_LocalInvocationID.y < 2)
    {
        uvec2 blockStart = gl_LocalInvocationID.xy * 4;

        uint4 albedoRes = BlockCompressAlbedo(blockStart);
        uint4 materialRes = BlockCompressMaterials(blockStart);
        uint4 normalRes = BlockCompressNormals(blockStart);

        // Since we work in 4x4 tiles, the offset to the 264 tile is 66
        ivec2 bcTileOffset = writeOffset / 4;
        // Since each thread samples 4x4, we have 2x2 threads that write, so gl_LocalInvocationID.xy will be [0..1]
        ivec2 bcTileLocalOffset = ivec2(gl_LocalInvocationID.xy);
        // And since each work group is responsible for writing 2x2 pixels BC compressed, the work group o
        ivec2 bcTileWorkgroupOffset = ivec2(gl_WorkGroupID.xy * 2);

        // The offset within the workgroup should just be invocation id which ranges between 0..1
        ivec2 workgroupOffset = ivec2(gl_LocalInvocationID.xy);
        imageStore(AlbedoLowresOutputBC, bcTileOffset + bcTileWorkgroupOffset + bcTileLocalOffset, uintBitsToFloat(albedoRes));
        imageStore(MaterialLowresOutputBC, bcTileOffset + bcTileWorkgroupOffset + bcTileLocalOffset, uintBitsToFloat(materialRes));
        imageStore(NormalLowresOutputBC, bcTileOffset + bcTileWorkgroupOffset + bcTileLocalOffset, uintBitsToFloat(normalRes));
    }
    */
}


program TerrainTileWrite[string Mask = "TerrainTileWrite";]
{
    ComputeShader = csWriteTile();
};

program TerrailLowresWrite[string Mask = "TerrainLowresWrite";]
{
    ComputeShader = csWriteLowres();
};
