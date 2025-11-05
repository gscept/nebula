//------------------------------------------------------------------------------
//  terrain.fx
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "terrain_include.fxh"



//------------------------------------------------------------------------------
/**
    Tessellation terrain vertex shader
*/
shader
void
vsTerrain(
    [slot=0] in vec3 position
    , [slot=1] in ivec2 uv
    , out vec4 Position
    , out vec3 Normal
    , out float Tessellation
) 
{
    TerrainPatch terrainPatch = Patches[gl_InstanceIndex];
    vec3 offsetPos = position + vec3(terrainPatch.PosOffset.x, 0, terrainPatch.PosOffset.y);
    vec4 modelSpace = Transform * vec4(offsetPos, 1);
    Position = vec4(offsetPos, 1);
    vec2 UV = UnpackUV(uv) + terrainPatch.UvOffset;

    float vertexDistance = distance( Position.xyz, EyePos.xyz);
    float factor = 1.0f - saturate((TerrainSystemUniforms.MinLODDistance - vertexDistance) / (TerrainSystemUniforms.MinLODDistance - TerrainSystemUniforms.MaxLODDistance));
    float decision = 1.0f - sample2D(DecisionMap, TextureSampler, UV).r;
    Tessellation = MinTessellation + factor * (MaxTessellation - MinTessellation) * decision;

    vec2 sampleUV = (Position.xz / vec2(WorldSizeX, WorldSizeZ)) - 0.5f;

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
    float screen = max(RenderTargetParameter[0].Dimensionsx, RenderTargetParameter[0].Dimensions.y);
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
    clip0.xy *= RenderTargetParameter[0].Dimensions.xy;
    clip1.xy *= RenderTargetParameter[0].Dimensions.xy;

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
    Position[gl_InvocationID]   = position[gl_InvocationID];
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
    out vec3 Normal,
    out vec3 Position)
{
    Position = mix(
        mix(position[0].xyz, position[1].xyz, gl_TessCoord.x),
        mix(position[2].xyz, position[3].xyz, gl_TessCoord.x),
        gl_TessCoord.y);

    /*
    Normal = mix(
        mix(normal[0], normal[1], gl_TessCoord.x),
        mix(normal[2], normal[3], gl_TessCoord.x),
        gl_TessCoord.y);
        */
        
    UV = (Position.xz / vec2(WorldSizeX, WorldSizeZ)) - 0.5f;

    
    vec2 pixelSize = textureSize(basic2D(HeightMap), 0);
    pixelSize = vec2(1.0f) / pixelSize;

    vec3 offset = vec3(-pixelSize.x, pixelSize.x, 0.0f);
    Normal = CalculateNormalFromHeight(UV, offset);

    float heightValue = sample2DLod(HeightMap, TextureSampler, UV, 0).r;
    Position.y = MinHeight + heightValue * (MaxHeight - MinHeight);

    gl_Position = ViewProjection * vec4(Position, 1);
}

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 64
shader
void
csTerrainPageClearUpdateBuffer()
{
    // Early out if this thread is slow and all the other ones have finished
    if (PageList.NumEntries == 0)
        return;

    // Decrement the entries list
    int numLeft = atomicAdd(PageList.NumEntries, -1);

    // If we get 0 or lower, it means the last item was dealt with already
    if (numLeft > 0)
        PageStatuses[PageList.PageStatuses[numLeft - 1]] = 0x0;

    // Ensure we can't get negative NumEntries
    atomicMax(PageList.NumEntries, 0);
}

//------------------------------------------------------------------------------
/**
    Pixel shader for outputting our Terrain GBuffer, and update the mip requirement buffer
*/
shader
void
psTerrainPrepass(
    in vec2 UV,
    in vec3 Normal,
    in vec3 WorldPos,
    [color0] out vec4 Pos)
{
    Pos.x = 0.0f;
    Pos.y = query_lod2D(AlbedoLowresBuffer, TextureSampler, UV).y;

    // convert world space to positive integer interval [0..WorldSize]
    vec2 worldSize = vec2(WorldSizeX, WorldSizeZ);
    vec2 unsignedPos = WorldPos.xz + worldSize * 0.5f;
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
        vec2 relativePos = WorldPos.xz - subTexture.worldCoordinate;
        //float lod = (Pos.w / IndirectionNumMips) * maxMip;
        
        const float lodScale = 4 * tiles;
        vec2 dy = dFdyFine(WorldPos.xz * lodScale);
        vec2 dx = dFdxFine(WorldPos.xz * lodScale);
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

            uint entryIndex = atomicAdd(PageList.NumEntries, 1);
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

                uint entryIndex = atomicAdd(PageList.NumEntries, 1);
                PageList.Entry[entryIndex] = entry;
                PageList.PageStatuses[entryIndex] = index;
            }
        }

        // if the position has w == 1, it means we found a page
        Pos.x = lod + mipBias;
        //Pos.y += mipBias;
    }
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
[early_depth]
shader
void
psTerrainResolve(
    in vec2 UV
    , in vec3 Normal
    , in vec3 WorldPos
    , [color0] out vec4 OutColor)
{
    // sample position, lod and texture sampling mode from screenspace buffer
    vec2 screenUv = gl_FragCoord.xy / DataBufferSize;
    vec2 posBuf = sample2DLod(TerrainPosBuffer, TextureSampler, screenUv, 0).xy;

    // calculate the subtexture coordinate
    vec2 worldSize = vec2(WorldSizeX, WorldSizeZ);
    vec2 worldUv = (WorldPos.xz + worldSize * 0.5f) / worldSize;
    vec2 unsignedPos = WorldPos.xz + worldSize * 0.5f;
    ivec2 subTextureCoord = ivec2(unsignedPos / VirtualTerrainSubTextureSize);

    vec3 albedo = sample2DLod(AlbedoLowresBuffer, TextureSampler, worldUv, posBuf.y).rgb;
    vec3 normal = sample2DLod(NormalLowresBuffer, TextureSampler, worldUv, posBuf.y).xyz;
    vec4 material = sample2DLod(MaterialLowresBuffer, TextureSampler, worldUv, posBuf.y);

    if (any(lessThan(subTextureCoord, ivec2(0, 0))) || any(greaterThanEqual(subTextureCoord, VirtualTerrainNumSubTextures)))
    {
        // Skip virtual texture lookup
    }
    else
    {
        // get subtexture
        uint subTextureIndex = subTextureCoord.x + subTextureCoord.y * VirtualTerrainNumSubTextures.x;
        TerrainSubTexture subTexture = SubTextures[subTextureIndex];

        uvec2 dummydummy, indirectionOffset;
        uint maxMip, tiles, mipBias;
        UnpackSubTexture(subTexture, dummydummy, indirectionOffset, maxMip, mipBias, tiles);

        if (tiles != 1)
        {
            int lowerMip = int(floor(posBuf.x));
            int upperMip = int(ceil(posBuf.x));

            vec2 cameraRelativePos = WorldPos.xz - EyePos.xz;
            float distSquared = dot(cameraRelativePos, cameraRelativePos);
            float blendWeight = (distSquared - LowresFadeStart) * LowresFadeDistance;
            blendWeight = clamp(blendWeight, 0, 1);

            vec2 relativePos = WorldPos.xz - subTexture.worldCoordinate;

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
                vec2 physicalUvUpper = subTextureTileFractUpper * (PhysicalTileSize)+PhysicalTilePadding;

                // get the indirection coord and normalize it to the physical space
                uvec3 indirectionUpper = fetchIndirection(ivec2(pageCoordUpper), int(upperMip), 0);
                uvec3 indirectionLower = fetchIndirection(ivec2(pageCoordLower), int(lowerMip), 0);

                vec3 albedo1;
                vec3 normal1;
                vec4 material1;

                // if valid mip, sample from physical cache
                if (indirectionUpper.z != 0xF)
                {
                    // convert from texture space to normalized space
                    vec2 indirection = (indirectionUpper.xy + physicalUvUpper) * vec2(PhysicalInvPaddedTextureSize);
                    albedo0 = sample2DLod(AlbedoPhysicalCacheBuffer, TextureSampler, indirection.xy, 0).rgb;
                    normal0 = UnpackBC5Normal(sample2DLod(NormalPhysicalCacheBuffer, TextureSampler, indirection.xy, 0).xy);
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
                    normal1 = UnpackBC5Normal(sample2DLod(NormalPhysicalCacheBuffer, TextureSampler, indirection.xy, 0).xy);
                    material1 = sample2DLod(MaterialPhysicalCacheBuffer, TextureSampler, indirection.xy, 0);
                }
                else
                {
                    albedo1 = albedo;
                    normal1 = normal;
                    material1 = material;
                }

                float weight = fract(posBuf.x);
                albedo0 = lerp(albedo1, albedo0, weight);
                normal0 = lerp(normal1, normal0, weight);
                material0 = lerp(material1, material0, weight);
            }
            else
            {
                // do the cheap path and just do a single lookup
                uvec3 indirection = fetchIndirection(ivec2(pageCoordLower), int(lowerMip), 0);

                // use physical cache if indirection is valid
                if (indirection.z != 0xF)
                {
                    vec2 indir = (indirection.xy + physicalUvLower) * vec2(PhysicalInvPaddedTextureSize);
                    albedo0 = sample2DLod(AlbedoPhysicalCacheBuffer, TextureSampler, indir.xy, 0).rgb;
                    normal0 = UnpackBC5Normal(sample2DLod(NormalPhysicalCacheBuffer, TextureSampler, indir.xy, 0).xy);
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
    }

    vec3 viewVec = normalize(EyePos.xyz - WorldPos);
    vec3 F0 = CalculateF0(albedo.rgb, material[MAT_METALLIC], vec3(0.04));

    vec3 light = vec3(0, 0, 0);
    light += CalculateLight(WorldPos, gl_FragCoord.xyz, albedo.rgb, material, normal);
    //light += CalculateGlobalLight(albedo.rgb, material, F0, viewVec, normal, pos.xxy);
    //light += LocalLights(idx, albedo.rgb, material, F0, pos.xyz, normal, gl_FragCoord.z);
    //light += IBL(albedo, F0, normal, viewVec, material);
    light += albedo.rgb * material[MAT_EMISSIVE];
    OutColor = vec4(light.rgb, 1);
}

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
    float highestPoint = 100000.0f;
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
            highestPoint = intersection.y;

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
    float shadow = smallestDistance == 10000000.0f ? 1.0f : (smallestDistance * smallestDistance) / (MaxDistance * MaxDistance);

    imageStore(TerrainShadowMap, ivec2(gl_GlobalInvocationID.xy), vec4(shadow, highestPoint, 0, 0));
}

//------------------------------------------------------------------------------
/**
*/
render_state TerrainState
{
    CullMode = Back;
    //FillMode = Line;
};

render_state ResolveState
{
    DepthEnabled = true;
    DepthFunc = Equal;
};

render_state FinalState
{
    DepthWrite = false;
    DepthEnabled = false;
};

TessellationTechnique(TerrainPrepass, "TerrainPrepass", vsTerrain(), hsTerrain(), dsTerrain(), psTerrainPrepass(), TerrainState);
TessellationTechnique(TerrainResolve, "TerrainResolve", vsTerrain(), hsTerrain(), dsTerrain(), psTerrainResolve(), ResolveState);

program TerrainPageClearUpdateBuffer [ string Mask = "TerrainPageClearUpdateBuffer"; ]
{
    ComputeShader = csTerrainPageClearUpdateBuffer();
};

program TerrainShadows [ string Mask = "TerrainShadows"; ]
{
    ComputeShader = csTerrainShadows();
};
