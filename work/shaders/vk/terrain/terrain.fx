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
	uint NumLayers;
	uint Debug;
	float VirtualLodDistance;

	uint FrameIndex;
	uint UpdateIndex;

	textureHandle TerrainDataBuffer;
	textureHandle TerrainNormalBuffer;
	textureHandle TerrainPosBuffer;
	textureHandle IndirectionBuffer;
	textureHandle AlbedoPhysicalCacheBuffer;
	textureHandle NormalPhysicalCacheBuffer;
	textureHandle MaterialPhysicalCacheBuffer;

	vec4 BiomeRules[MAX_BIOMES];					// rules are x: slope, y: height, z: UV scaling factor, w: mip 
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
	uvec2 VirtualTerrainPhysicalTextureSize;
	uvec2 VirtualTerrainPhysicalTileSize;

	uvec4 VirtualTerrainTextureSize;
	uvec2 VirtualTerrainPageSize;
	uvec2 VirtualTerrainNumPages;
	uint VirtualTerrainNumMips;
	uint PageReadbackBufferSize;

	textureHandle HeightMap;
	textureHandle DecisionMap;
	textureHandle AlbedoMap;

	textureHandle VirtualAlbedoTexture;
	textureHandle VirtualNormalTexture;
	textureHandle VirtualMaterialTexture;
	textureHandle RedirectionTexture;

	uint VirtualPageBufferNumPages;
	uvec4 VirtualPageBufferMipOffsets[4];
	uvec4 VirtualPageBufferMipSizes[4];
};

group(DYNAMIC_OFFSET_GROUP) constant TerrainTileUpdateUniforms [ string Visbility = "CS"; ]
{
	vec2 SparseTileWorldOffset;
	uvec2 SparseTileIndirectionOffset;
	uvec2 SparseTileOutputOffset;
	uint Mip;
	float PixelsPerMeter;
	uint SubTextureTiles;
};

group(SYSTEM_GROUP) write r11g11b10f image2D VirtualAlbedoOutput [ string Visbility = "CS"; ];
group(SYSTEM_GROUP) write r11g11b10f image2D VirtualNormalOutput [ string Visbility = "CS"; ];
group(SYSTEM_GROUP) write rgba16f image2D VirtualMaterialOutput [ string Visbility = "CS"; ];
group(SYSTEM_GROUP) write r11g11b10f image2D LowresAlbedoOutput[12];
group(SYSTEM_GROUP) write r11g11b10f image2D LowresNormalOutput[12];
group(SYSTEM_GROUP) write rgba16f image2D LowresMaterialOutput[12];

group(SYSTEM_GROUP) readwrite rgba32f	image2DArray PageUpdateTexture [ string Visbility = "CS|PS"; ];

group(SYSTEM_GROUP) texture2D		MaterialMaskArray[MAX_BIOMES];
group(SYSTEM_GROUP) texture2DArray	MaterialAlbedoArray[MAX_BIOMES];
group(SYSTEM_GROUP) texture2DArray	MaterialNormalArray[MAX_BIOMES];
group(SYSTEM_GROUP) texture2DArray	MaterialPBRArray[MAX_BIOMES];

struct VirtualTerrainSubTexture
{
	vec2 worldCoordinate;
	uvec2 indirectionOffset;
	uint mips;
	uint size;
};

group(SYSTEM_GROUP) rw_buffer		VirtualTerrainSubTextures [ string Visibility = "PS|CS"; ]
{
	VirtualTerrainSubTexture SubTextures[];
};

const int MAX_PAGE_UPDATES = 1024;

struct PageUpdateEntry
{
	uvec4 indirection;
	vec4 coord;
};

struct PageUpdateList
{
	uint NumEntries;
	PageUpdateEntry Entry[MAX_PAGE_UPDATES];
};

group(SYSTEM_GROUP) rw_buffer		VirtualTerrainPageUpdateList [ string Visibility = "CS"; ]
{
	PageUpdateList PageList;
};

struct PageEntry
{
	uvec4 Coordinates;	// xy is page coord, xy is subtexture tile
	uvec2 PackedAndMip;
};

group(SYSTEM_GROUP) rw_buffer		PageUpdateBuffer [ string Visibility = "PS|CS"; ]
{
	PageEntry PageEntries[];
};

#define sampleBiomeAlbedo(biome, sampler, uv, layer)		texture(sampler2DArray(MaterialAlbedoArray[biome], sampler), vec3(uv, layer))
#define sampleBiomeNormal(biome, sampler, uv, layer)		texture(sampler2DArray(MaterialNormalArray[biome], sampler), vec3(uv, layer))
#define sampleBiomeMaterial(biome, sampler, uv, layer)		texture(sampler2DArray(MaterialPBRArray[biome], sampler), vec3(uv, layer))
#define sampleBiomeMask(biome, sampler, uv)					texture(sampler2D(MaterialMaskArray[biome], sampler), uv)
#define sampleBiomeMaskLod(biome, sampler, uv, lod)			textureLod(sampler2D(MaterialMaskArray[biome], sampler), uv, lod)

#define fetchBiomeMask(biome, sampler, uv, lod)				texelFetch(sampler2D(MaterialMaskArray[biome], sampler), uv, lod)


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
	out vec3 Normal) 
{
	vec3 offsetPos = position + vec3(OffsetPatchPos.x, 0, OffsetPatchPos.y);
	vec4 modelSpace = Transform * vec4(offsetPos, 1);
	Position = modelSpace;
	UV = uv + OffsetPatchUV;
	LocalUV = uv;

	float vertexDistance = distance( Position.xyz, EyePos.xyz);
	float factor = 1.0f - saturate((MinLODDistance - vertexDistance) / (MinLODDistance - MaxLODDistance));
	float decision = 1.0f - sample2D(DecisionMap, TextureSampler, UV).r;

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
	out vec2 UV[],
	out vec2 LocalUV[],
	out vec4 Position[],
	out vec3 Normal[]) 
{
	UV[gl_InvocationID]			= uv[gl_InvocationID];
	LocalUV[gl_InvocationID]	= localUv[gl_InvocationID];
	Position[gl_InvocationID]	= position[gl_InvocationID];
	Normal[gl_InvocationID]		= normal[gl_InvocationID];

	// provoking vertex gets to decide tessellation factors
	if (gl_InvocationID == 0)
	{
		vec4 EdgeTessFactors;
		EdgeTessFactors.x = TessellationFactorScreenSpace(Position[2], Position[0]);
		EdgeTessFactors.y = TessellationFactorScreenSpace(Position[0], Position[1]);
		EdgeTessFactors.z = TessellationFactorScreenSpace(Position[1], Position[3]);
		EdgeTessFactors.w = TessellationFactorScreenSpace(Position[3], Position[2]);


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
	Pixel shader for Z pass
*/
[early_depth]
shader
void
psTerrainZ(
	in vec2 uv,
	in vec2 localUv,
	in vec3 viewPos,
	in vec3 normal,
	in vec3 worldPos,
	[color0] out vec2 Data,
	[color1] out vec4 Normal,
	[color3] out vec4 Pos)
{
	// go through the masks and figure out which ids we want to use
	int finalMask = 0x0;
	Data.x = saturate(length(viewPos) / VirtualLodDistance);

	uint bucketX = uint(worldPos.x + WorldSizeX * 0.5f) / uint(TileWidth);
	uint bucketZ = uint(worldPos.z + WorldSizeZ * 0.5f) / uint(TileHeight);
	uint lodIndex = bucketX + bucketZ * NumTilesX;
	int lod = 1000;

	vec2 uvuv = worldPos.xz / vec2(WorldSizeX, WorldSizeZ);

	// figure out which lod we should pick for this pixel
	if (NumBiomes > 0)
		lod = max(0, int(textureQueryLod(sampler2DArray(MaterialAlbedoArray[0], PointSampler), uvuv).y * 1000.0f));

	for (uint i = 0; i < NumBiomes; i++)
	{
		float mask = sampleBiomeMask(i, TextureSampler, uvuv).r;
		if (mask > 0.0f)
		{
			finalMask |= 1 << i;
		}
	}

	Data.y = float(finalMask); // use max 256 materials
	Normal.xyz = normal;
	Normal.a = worldPos.y;
	Pos.xyz = worldPos.xyz;
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
	outAlbedo = sampleBiomeAlbedo(i, TextureSampler, uv, baseArrayIndex).rgb * mask * (1.0f - angle);
	outAlbedo += sampleBiomeAlbedo(i, TextureSampler, uv, baseArrayIndex + 1).rgb * mask * angle;
	outMaterial = sampleBiomeMaterial(i, TextureSampler, uv, baseArrayIndex).rgb * mask * (1.0f - angle);
	outMaterial += sampleBiomeMaterial(i, TextureSampler, uv, baseArrayIndex + 1).rgb * mask * angle;
	outNormal = sampleBiomeNormal(i, TextureSampler, uv, baseArrayIndex).rgb * (1.0f - angle);
	outNormal += sampleBiomeNormal(i, TextureSampler, uv, baseArrayIndex + 1).rgb * angle;
}

//------------------------------------------------------------------------------
/**
	Calculate pixel light contribution
*/
shader
void 
psScreenSpace(
	in vec2 ScreenUV,
	[color0] out vec4 Albedo,
	[color1] out vec3 Normal,
	[color2] out vec4 Material)
{
	vec2 data = sample2DLod(TerrainDataBuffer, PointSampler, ScreenUV, 0).rg;

	// get the mask back
	uint mask = uint(data.y);
	if (mask == 0)
		discard;

	vec4 normal = sample2DLod(TerrainNormalBuffer, PointSampler, ScreenUV, 0);
	vec3 triplanarWeights = abs(normal.xyz);
	triplanarWeights = normalize(max(triplanarWeights * triplanarWeights, 0.00001f));
	float norm = (triplanarWeights.x + triplanarWeights.y + triplanarWeights.z);
	triplanarWeights /= vec3(norm, norm, norm);

	vec4 worldPos = sample2DLod(TerrainPosBuffer, PointSampler, ScreenUV, 0).rgba;
	vec2 globalUV = worldPos.xz;
	vec2 tileUV = worldPos.xz / 64.0f;
	vec3 totalAlbedo = vec3(0, 0, 0);
	vec3 totalMaterial = vec3(0, 0, 0);
	vec3 totalNormal = vec3(0, 0, 0);
	vec3 tangent = cross(normal.xyz, vec3(0, 0, 1));
	tangent = normalize(cross(normal.xyz, tangent));
	vec3 binormal = normalize(cross(normal.xyz, tangent));
	mat3 tbn = mat3(tangent, binormal, normal.xyz);

	//SampleSlopeRule(0, 0, 0, 1.0f, uvs.zw, tbn, totalAlbedo, totalMaterial, totalNormal);

	for (int i = 0; i < NumBiomes; i++)
	{
		if ((mask & 1) == 1)
		{
			// first get the mask value
			float maskValue = sampleBiomeMask(i, TextureSampler, globalUV).r;
			vec4 rules = BiomeRules[i];

			float angle = saturate((1.0f - dot(normal.xyz, vec3(0, 1, 0))) / 0.5f);
			float height = saturate(max(0, normal.a - rules.y) / 25.0f);
			//float angle = 0.5f;
			//float height = 0.0f;
			vec3 blendNormal = vec3(0, 0, 0);
			if (height == 0.0f)
			{
				vec3 albedo = vec3(0, 0, 0);
				vec3 normal = vec3(0, 0, 0);
				vec3 material = vec3(0, 0, 0);

				SampleSlopeRule(i, 0, angle, maskValue, worldPos.yz / 64.0f, albedo, material, normal);
				totalAlbedo += albedo * triplanarWeights.x;
				totalMaterial += material * triplanarWeights.x;
				blendNormal += normal * triplanarWeights.x;

				SampleSlopeRule(i, 0, angle, maskValue, worldPos.xz / 64.0f, albedo, material, normal);
				totalAlbedo += albedo * triplanarWeights.y;
				totalMaterial += material * triplanarWeights.y;
				blendNormal += normal * triplanarWeights.y;

				SampleSlopeRule(i, 0, angle, maskValue, worldPos.xy / 64.0f, albedo, material, normal);
				totalAlbedo += albedo * triplanarWeights.z;
				totalMaterial += material * triplanarWeights.z;
				blendNormal += normal * triplanarWeights.z;

				blendNormal.xy = blendNormal.xy * 2.0f - 1.0f;
				blendNormal.z = saturate(sqrt(1.0f - dot(blendNormal.xy, blendNormal.xy)));
				totalNormal += (tbn * blendNormal) * maskValue;
			}
			else
			{
				vec3 albedo = vec3(0, 0, 0);
				vec3 normal = vec3(0, 0, 0);
				vec3 material = vec3(0, 0, 0);
				SampleSlopeRule(i, 2, angle, maskValue, worldPos.yz / 64.0f, albedo, material, normal);
				totalAlbedo += albedo * triplanarWeights.x * height;
				totalMaterial += material * triplanarWeights.x * height;
				blendNormal += normal * triplanarWeights.x * height;
				SampleSlopeRule(i, 2, angle, maskValue, worldPos.xz / 64.0f, albedo, material, normal);
				totalAlbedo += albedo * triplanarWeights.y * height;
				totalMaterial += material * triplanarWeights.y * height;
				blendNormal += normal * triplanarWeights.y * height;
				SampleSlopeRule(i, 2, angle, maskValue, worldPos.xy / 64.0f, albedo, material, normal);
				totalAlbedo += albedo * triplanarWeights.z * height;
				totalMaterial += material * triplanarWeights.z * height;
				blendNormal += normal * triplanarWeights.z * height;
				SampleSlopeRule(i, 0, angle, maskValue, worldPos.yz / 64.0f, albedo, material, normal);
				totalAlbedo += albedo * triplanarWeights.x * (1.0f - height);
				totalMaterial += material * triplanarWeights.x * (1.0f - height);
				blendNormal += normal * triplanarWeights.x * (1.0f - height);
				SampleSlopeRule(i, 0, angle, maskValue, worldPos.xz / 64.0f, albedo, material, normal);
				totalAlbedo += albedo * triplanarWeights.y * (1.0f - height);
				totalMaterial += material * triplanarWeights.y * (1.0f - height);
				blendNormal += normal * triplanarWeights.y * (1.0f - height);
				SampleSlopeRule(i, 0, angle, maskValue, worldPos.xy / 64.0f, albedo, material, normal);
				totalAlbedo += albedo * triplanarWeights.z * (1.0f - height);
				totalMaterial += material * triplanarWeights.z * (1.0f - height);
				blendNormal += normal * triplanarWeights.z * (1.0f - height);

				blendNormal.xy = blendNormal.xy * 2.0f - 1.0f;
				blendNormal.z = saturate(sqrt(1.0f - dot(blendNormal.xy, blendNormal.xy)));
				totalNormal += (tbn * blendNormal) * maskValue;
			}
		}
		mask = mask >> 1;
	}


	Albedo = vec4(totalAlbedo, 1.0f);
	Material = vec4(totalMaterial, 0.0f);
	Normal = normalize(totalNormal);
}

//------------------------------------------------------------------------------
/**
	Pixel shader for shading
*/
[early_depth]
shader
void
psTerrain(
	in vec2 uv,
	in vec2 localUv,
	in vec3 viewPos,
	in vec3 normal,
	in vec3 worldPos,
	[color0] out vec4 Albedo,
	[color1] out vec3 Normal,
	[color2] out vec4 Material)
{
	vec2 worldSize = vec2(WorldSizeX, WorldSizeZ);
	vec2 worldUv = vec2(worldPos.xz + worldSize * 0.5f);
	vec2 redirect = fetch2D(RedirectionTexture, TextureSampler, ivec2(worldUv / VirtualTerrainPageSize), 0).xy;
	vec2 relativeUv = localUv / PatchUvScale + redirect / worldSize;
	//relativeUv.x = 1.0f - relativeUv.x;
	vec4 albedo = sample2D(VirtualAlbedoTexture, TextureSampler, relativeUv);
	vec4 normal = sample2D(VirtualNormalTexture, TextureSampler, relativeUv);
	vec4 material = sample2D(VirtualMaterialTexture, TextureSampler, relativeUv);
	Albedo = vec4(albedo.rgb, 1.0f);
	Normal = normal.xyz;// normal.xyz;
	Material = material;
}

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 8
[local_size_y] = 8
[local_size_z] = 1
shader
void
csTerrainPageClearUpdateTexture()
{
	uint mip = gl_GlobalInvocationID.z;
	uint x = gl_GlobalInvocationID.x;
	uint y = gl_GlobalInvocationID.y;
	if ((gl_GlobalInvocationID.x < (VirtualTerrainNumPages.x >> mip)) &&
		(gl_GlobalInvocationID.y < (VirtualTerrainNumPages.y >> mip)))
	{
		ivec3 coord = ivec3(x, y, mip);
		
		// read result from last frame
		uvec4 resident = uvec4(imageLoad(PageUpdateTexture, coord));
		uint status = resident.x & 0x3;

		// if the page was resident last frame, clear to 2.0f
		// this means that if the pixel shader does not write to this pixel
		// then we can find which pages to free up next frame
		if (status == 1)
			imageStore(PageUpdateTexture, coord, vec4(2.0f, 0, 0, 0));
		else
			imageStore(PageUpdateTexture, coord, vec4(0.0f, 0, 0, 0));
	}

	// clear page entries
	if (gl_GlobalInvocationID.x == 0 && gl_GlobalInvocationID.y == 0 && gl_GlobalInvocationID.z == 0)
	{
		PageList.NumEntries = 0u;
	}
}


//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 64
shader
void
csTerrainPageClearUpdateBuffer()
{
	uint index = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * 2048;
	if (index >= VirtualPageBufferNumPages)
		return;

	PageEntry entry = PageEntries[index]; 
	uint status = entry.PackedAndMip.x & 0x3;
	if (status == 1)
	{
		entry.PackedAndMip = uvec2(2, 0);
		entry.Coordinates = uvec4(0, 0, 0, 0);
		PageEntries[index] = entry;
	}
	else
	{
		entry.PackedAndMip = uvec2(0, 0);
		entry.Coordinates = uvec4(0, 0, 0, 0);
		PageEntries[index] = entry;
	}

	// clear page entries
	if (index == 0)
	{
		PageList.NumEntries = 0u;
	}
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
	// figure out which subtexture this pixel is in
	vec2 worldSize = vec2(WorldSizeX, WorldSizeZ);

	Pos.xy = worldPos.xz;
	Pos.w = 0.0f;
	Pos.z = 0.0f;

	uvec2 subTextureCoord = uvec2(Pos.xy + worldSize * 0.5f) / VirtualTerrainSubTextureSize;

	// calculate subtexture index
	uint subTextureIndex = subTextureCoord.x + subTextureCoord.y * VirtualTerrainNumSubTextures.x;
	if (subTextureIndex >= VirtualTerrainNumSubTextures.x * VirtualTerrainNumSubTextures.y)
		return;
	VirtualTerrainSubTexture subTexture = SubTextures[FrameIndex * VirtualTerrainNumSubTextures.x * VirtualTerrainNumSubTextures.y + subTextureIndex];

	// if this subtexture is bound on the CPU side, use it
	if (subTexture.size != 0xFFFFFFFF)
	{
		// calculate LOD
		vec2 dx = dFdx(worldPos.xz * subTexture.size * 4.0f);
		vec2 dy = dFdy(worldPos.xz * subTexture.size * 4.0f);
		float d = max(dot(dx, dx), dot(dy, dy));
		const float clampRange = pow(2, float(subTexture.mips - 1));
		d = clamp(sqrt(d), 1.0f, clampRange);
		float lod = log2(d);

		uint upperMip = uint(ceil(lod));
		vec2 metersPerTileUpper = VirtualTerrainSubTextureSize / float(max(1, subTexture.size >> upperMip));
		vec2 tileCoordUpper = (worldPos.xz - subTexture.worldCoordinate) / metersPerTileUpper;
		uvec2 subTextureTileUpper = uvec2(tileCoordUpper);
		uvec2 pageCoordUpper = (subTexture.indirectionOffset >> upperMip) + subTextureTileUpper;

		uint lowerMip = uint(floor(lod));
		vec2 metersPerTileLower = VirtualTerrainSubTextureSize / float(max(1, subTexture.size >> lowerMip));
		vec2 tileCoordLower = (worldPos.xz - subTexture.worldCoordinate) / metersPerTileLower;
		uvec2 subTextureTileLower = uvec2(tileCoordLower);
		uvec2 pageCoordLower = (subTexture.indirectionOffset >> lowerMip) + subTextureTileLower;

		uint packedData = 1;
		packedData |= subTextureIndex << 2; // we will use the first 2 bits to control the update state of the pixel

		// update texture (change to buffer later) for both upper and lower
		if (upperMip != lowerMip)
		{
			uint upperIndex = VirtualPageBufferMipOffsets[upperMip / 4][upperMip % 4] + pageCoordUpper.x + pageCoordUpper.y * VirtualPageBufferMipSizes[upperMip / 4][upperMip % 4];
			PageEntry upperEntry;
			upperEntry.PackedAndMip = uvec2(packedData, upperMip);
			upperEntry.Coordinates = uvec4(pageCoordUpper.x, pageCoordUpper.y, subTextureTileUpper.x, subTextureTileUpper.y);
			PageEntries[upperIndex] = upperEntry;

			uint lowerIndex = VirtualPageBufferMipOffsets[lowerMip / 4][lowerMip % 4] + pageCoordLower.x + pageCoordLower.y * VirtualPageBufferMipSizes[lowerMip / 4][lowerMip % 4];
			PageEntry lowerEntry;
			lowerEntry.PackedAndMip = uvec2(packedData, lowerMip);
			lowerEntry.Coordinates = uvec4(pageCoordLower.x, pageCoordLower.y, subTextureTileLower.x, subTextureTileLower.y);
			PageEntries[lowerIndex] = lowerEntry;
			//imageStore(PageUpdateTexture, ivec3(pageCoordUpper, upperMip), vec4(packedData, subTextureTileUpper.x, subTextureTileUpper.y, 0));
			//imageStore(PageUpdateTexture, ivec3(pageCoordLower, lowerMip), vec4(packedData, subTextureTileLower.x, subTextureTileLower.y, 0));
		}
		else
		{
			uint lowerIndex = VirtualPageBufferMipOffsets[lowerMip / 4][lowerMip % 4] + pageCoordLower.x + pageCoordLower.y * VirtualPageBufferMipSizes[lowerMip / 4][lowerMip % 4];
			PageEntry lowerEntry;
			lowerEntry.PackedAndMip = uvec2(packedData, lowerMip);
			lowerEntry.Coordinates = uvec4(pageCoordLower.x, pageCoordLower.y, subTextureTileLower.x, subTextureTileLower.y);
			PageEntries[lowerIndex] = lowerEntry;
			//imageStore(PageUpdateTexture, ivec3(pageCoordLower, lowerMip), vec4(packedData, subTextureTileLower.x, subTextureTileLower.y, 0));
		}

		// if the position has w == 1, it means we found a page
		//Pos.xy = vec2(subTextureTile) / float(subTexture.size >> upperMip);
		Pos.z = lod;
		Pos.w = 1.0f;
	}
}

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 8
[local_size_y] = 8
[local_size_z] = 1
shader
void
csExtractPageUpdateTexture()
{
	uint mip = gl_GlobalInvocationID.z;
	uint x = gl_GlobalInvocationID.x;
	uint y = gl_GlobalInvocationID.y;
	if ((gl_GlobalInvocationID.x < (VirtualTerrainNumPages.x >> mip)) &&
		(gl_GlobalInvocationID.y < (VirtualTerrainNumPages.y >> mip)))
	{
		// if there is a difference between the buffers, 
		vec4 resident = imageLoad(PageUpdateTexture, ivec3(x, y, mip));
		uint residency = uint(resident.x) & 0x3;

		if (residency != 0)
		{
			// add one item to NumPageEntries, it's an atomic operation, so the index returned is the one we can use 
			uint entryIndex = atomicAdd(PageList.NumEntries, 1u);

			// the pixel can be 1 if written to by the shader, or 2 if cleared in the clear pass
			PageUpdateEntry entry;
			entry.indirection = uvec4(x, y, mip, resident.x);
			entry.coord.x = resident.y;
			entry.coord.y = resident.z;
			PageList.Entry[entryIndex] = entry;
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 64
shader
void
csExtractPageUpdateBuffer()
{
	uint index = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * 2048;
	if (index >= VirtualPageBufferNumPages)
		return;

	PageEntry entry = PageEntries[index];
	uint residency = entry.PackedAndMip.x & 0x3;
	if (residency != 0)
	{
		// add one item to NumPageEntries, it's an atomic operation, so the index returned is the one we can use 
		uint entryIndex = atomicAdd(PageList.NumEntries, 1u);

		// the pixel can be 1 if written to by the shader, or 2 if cleared in the clear pass
		PageUpdateEntry updateEntry;
		updateEntry.indirection = uvec4(entry.Coordinates.x, entry.Coordinates.y, entry.PackedAndMip.y, entry.PackedAndMip.x);
		updateEntry.coord.x = entry.Coordinates.z;
		updateEntry.coord.y = entry.Coordinates.w;
		PageList.Entry[entryIndex] = updateEntry;
	}
}

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 8
[local_size_y] = 8
shader
void
csTerrainTileUpdate()
{
	// calculate 
	vec2 worldSize = vec2(WorldSizeX, WorldSizeZ);
	vec2 worldPos2D = vec2(SparseTileWorldOffset + gl_GlobalInvocationID.xy * PixelsPerMeter) + worldSize * 0.5f;
	vec2 inputUv = worldPos2D;
	ivec2 outputUv = ivec2(SparseTileOutputOffset + gl_GlobalInvocationID.xy);

	// update redirect texture if we are on the first thread
	/*
	if (gl_GlobalInvocationID.x == 0 &&
		gl_GlobalInvocationID.y == 0)
	{
		// output indirection pixels if it's first pixel in the tile
		imageStore(IndirectionTextureOutput[Mip], ivec2(SparseTileIndirectionOffset), vec4(SparseTileOutputOffset.x, SparseTileOutputOffset.y, 0, 0));
	}
	*/

	float heightValue = sample2DLod(HeightMap, TextureSampler, inputUv / worldSize, 0).r;
	float height = MinHeight + heightValue * (MaxHeight - MinHeight);

	vec3 worldPos = vec3(worldPos2D.x, height, worldPos2D.y);

	// calculate normals by grabbing pixels around our UV
	ivec3 offset = ivec3(1, 1, 0.0f);
	float hl = MinHeight + sample2DLod(HeightMap, TextureSampler, (inputUv - offset.xz) / worldSize, 0).r * (MaxHeight - MinHeight);
	float hr = MinHeight + sample2DLod(HeightMap, TextureSampler, (inputUv + offset.xz) / worldSize, 0).r * (MaxHeight - MinHeight);
	float ht = MinHeight + sample2DLod(HeightMap, TextureSampler, (inputUv - offset.zy) / worldSize, 0).r * (MaxHeight - MinHeight);
	float hb = MinHeight + sample2DLod(HeightMap, TextureSampler, (inputUv + offset.zy) / worldSize, 0).r * (MaxHeight - MinHeight);
	vec3 normal = vec3(0, 0, 0);
	normal.x = hl - hr;
	normal.y = 2.0f;
	normal.z = ht - hb;
	normal = normalize(normal.xyz);

	// setup the TBN
	vec3 tangent = cross(normal.xyz, vec3(0, 0, 1));
	tangent = normalize(cross(normal.xyz, tangent));
	vec3 binormal = normalize(cross(normal.xyz, tangent));
	mat3 tbn = mat3(tangent, binormal, normal.xyz);

	// calculate weights for triplanar mapping
	vec3 triplanarWeights = abs(normal.xyz);
	triplanarWeights = normalize(max(triplanarWeights * triplanarWeights, 0.00001f));
	float norm = (triplanarWeights.x + triplanarWeights.y + triplanarWeights.z);
	triplanarWeights /= vec3(norm, norm, norm);

	vec3 totalAlbedo = vec3(0, 0, 0);
	vec3 totalMaterial = vec3(0, 0, 0);
	vec3 totalNormal = vec3(0, 0, 0);

	for (uint i = 0; i < NumBiomes; i++)
	{
		// get biome data
		float mask = sampleBiomeMaskLod(i, TextureSampler, inputUv / worldSize, 0).r;
		vec4 rules = BiomeRules[i];

		// calculate rules
		float angle = saturate((1.0f - dot(normal, vec3(0, 1, 0))) / 0.5f);
		float heightCutoff = saturate(max(0, height - rules.y) / 25.0f);

		const vec2 tilingFactor = vec2(4.0f);

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

		//totalAlbedo.r = heightCutoff;
		//totalAlbedo.gb = vec2(0);
	}


	// write output to virtual textures
	imageStore(VirtualAlbedoOutput, outputUv, vec4(totalAlbedo, 1.0f));
	imageStore(VirtualNormalOutput, outputUv, vec4(totalNormal, 0.0f));
	imageStore(VirtualMaterialOutput, outputUv, vec4(totalMaterial, 0.0f));
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
	Calculate pixel light contribution
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
	vec2 worldPos = pos.xy;
	ivec2 worldSize = ivec2(WorldSizeX, WorldSizeZ);
	uvec2 subTextureCoord = uvec2(worldPos + worldSize * 0.5f) / VirtualTerrainSubTextureSize;

	// get subtexture
	uint subTextureIndex = subTextureCoord.x + subTextureCoord.y * VirtualTerrainNumSubTextures.x;
	VirtualTerrainSubTexture subTexture = SubTextures[FrameIndex * VirtualTerrainNumSubTextures.x * VirtualTerrainNumSubTextures.y + subTextureIndex];

	if (subTexture.size != 0xFFFFFFFF)
	{
		int lowerMip = int(floor(pos.z));
		int upperMip = int(ceil(pos.z));

		vec2 metersPerTileUpper = VirtualTerrainSubTextureSize / float(max(1, subTexture.size >> upperMip));
		vec2 tileCoordUpper = (worldPos - subTexture.worldCoordinate) / metersPerTileUpper;
		uvec2 subTextureTileUpper = uvec2(tileCoordUpper);
		uvec2 pageCoordUpper = (subTexture.indirectionOffset >> upperMip) + subTextureTileUpper;
		vec2 physicalUvUpper = fract(tileCoordUpper) * 256.0f;

		vec2 metersPerTileLower = VirtualTerrainSubTextureSize / float(max(1, subTexture.size >> lowerMip));
		vec2 tileCoordLower = (worldPos - subTexture.worldCoordinate) / metersPerTileLower;
		uvec2 subTextureTileLower = uvec2(tileCoordLower);
		uvec2 pageCoordLower = (subTexture.indirectionOffset >> lowerMip) + subTextureTileLower;
		vec2 physicalUvLower = fract(tileCoordLower) * 256.0f;

		if (pos.w == 2.0f)
			discard;
		else if (pos.w == 1.0f)
		{
			vec2 uv = pos.xy;

			// if we need to sample two lods, do bilinear interpolation ourselves
			if (upperMip != lowerMip)
			{
				// get the indirection coord and normalize it to the physical space
				vec3 indirectionUpper = UnpackIndirection(floatBitsToUint(fetch2D(IndirectionBuffer, TextureSampler, ivec2(pageCoordUpper), upperMip).x));
				indirectionUpper.xy = (indirectionUpper.xy + physicalUvUpper) / vec2(VirtualTerrainPhysicalTextureSize);
				vec3 indirectionLower = UnpackIndirection(floatBitsToUint(fetch2D(IndirectionBuffer, TextureSampler, ivec2(pageCoordLower), lowerMip).x));
				indirectionLower.xy = (indirectionLower.xy + physicalUvLower) / vec2(VirtualTerrainPhysicalTextureSize);

				vec4 albedo0 = sample2DLod(AlbedoPhysicalCacheBuffer, TextureSampler, indirectionUpper.xy, 0);
				vec4 normal0 = sample2DLod(NormalPhysicalCacheBuffer, TextureSampler, indirectionUpper.xy, 0);
				vec4 material0 = sample2DLod(MaterialPhysicalCacheBuffer, TextureSampler, indirectionUpper.xy, 0);
				vec4 albedo1 = sample2DLod(AlbedoPhysicalCacheBuffer, TextureSampler, indirectionLower.xy, 0);
				vec4 normal1 = sample2DLod(NormalPhysicalCacheBuffer, TextureSampler, indirectionLower.xy, 0);
				vec4 material1 = sample2DLod(MaterialPhysicalCacheBuffer, TextureSampler, indirectionLower.xy, 0);
				float weight = fract(pos.z);
				Albedo = lerp(albedo1, albedo0, weight);
				Normal = lerp(normal1, normal0, weight).xyz;
				Material = lerp(material1, material0, weight);

				//Albedo.rg = lerp(indirectionUpper, indirectionLower, weight);
			}
			else
			{
				vec3 indirection = UnpackIndirection(floatBitsToUint(fetch2D(IndirectionBuffer, TextureSampler, ivec2(pageCoordLower), lowerMip).x));
				indirection.xy = (indirection.xy + physicalUvLower) / vec2(VirtualTerrainPhysicalTextureSize);
				Albedo = sample2DLod(AlbedoPhysicalCacheBuffer, TextureSampler, indirection.xy, 0);
				Normal = sample2DLod(NormalPhysicalCacheBuffer, TextureSampler, indirection.xy, 0).xyz;
				Material = sample2DLod(MaterialPhysicalCacheBuffer, TextureSampler, indirection.xy, 0);

				//Albedo.rg = (indirection.xy + physicalUv) / vec2(VirtualTerrainPhysicalTextureSize);

			}
		}
		else
		{
			Albedo = vec4(0, 0, 0, 0);
			Normal = vec3(0, 0, 0);
			Material = vec4(0, 0, 0, 0);
		}
		//Albedo.rg = physicalUv / 256.0f;
		//Albedo.b = 0.0f;
		//Albedo.rg = vec2(0.0f);
		//Albedo.b = pos.z / VirtualTerrainNumMips;
	}
	else
	{
		Albedo = vec4(0, 0, 0, 0);
		Normal = vec3(0, 0, 0);
		Material = vec4(0, 0, 0, 0);
	}

	//Albedo.b = 0.0f;
}

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 8
[local_size_y] = 8
[local_size_z] = 1
shader
void
csGenerateLowresFallback()
{
	// calculate 
	vec2 worldSize = vec2(WorldSizeX, WorldSizeZ);
	vec2 worldPos2D = vec2(gl_GlobalInvocationID.xy);
	vec2 inputUv = worldPos2D;
	ivec2 outputUv = ivec2(gl_GlobalInvocationID.xy);

	float heightValue = sample2DLod(HeightMap, TextureSampler, inputUv / worldSize, 0).r;
	float height = MinHeight + heightValue * (MaxHeight - MinHeight);

	vec3 worldPos = vec3(worldPos2D.x, height, worldPos2D.y);

	// calculate normals by grabbing pixels around our UV
	ivec3 offset = ivec3(1, 1, 0.0f);
	float hl = MinHeight + sample2DLod(HeightMap, TextureSampler, (inputUv - offset.xz) / worldSize, 0).r * (MaxHeight - MinHeight);
	float hr = MinHeight + sample2DLod(HeightMap, TextureSampler, (inputUv + offset.xz) / worldSize, 0).r * (MaxHeight - MinHeight);
	float ht = MinHeight + sample2DLod(HeightMap, TextureSampler, (inputUv - offset.zy) / worldSize, 0).r * (MaxHeight - MinHeight);
	float hb = MinHeight + sample2DLod(HeightMap, TextureSampler, (inputUv + offset.zy) / worldSize, 0).r * (MaxHeight - MinHeight);
	vec3 normal = vec3(0, 0, 0);
	normal.x = hl - hr;
	normal.y = 2.0f;
	normal.z = ht - hb;
	normal = normalize(normal.xyz);

	// setup the TBN
	vec3 tangent = cross(normal.xyz, vec3(0, 0, 1));
	tangent = normalize(cross(normal.xyz, tangent));
	vec3 binormal = normalize(cross(normal.xyz, tangent));
	mat3 tbn = mat3(tangent, binormal, normal.xyz);

	// calculate weights for triplanar mapping
	vec3 triplanarWeights = abs(normal.xyz);
	triplanarWeights = normalize(max(triplanarWeights * triplanarWeights, 0.00001f));
	float norm = (triplanarWeights.x + triplanarWeights.y + triplanarWeights.z);
	triplanarWeights /= vec3(norm, norm, norm);

	vec3 totalAlbedo = vec3(0, 0, 0);
	vec3 totalMaterial = vec3(0, 0, 0);
	vec3 totalNormal = vec3(0, 0, 0);

	for (uint i = 0; i < NumBiomes; i++)
	{
		// get biome data
		float mask = sampleBiomeMaskLod(i, TextureSampler, inputUv / worldSize, 0).r;
		vec4 rules = BiomeRules[i];

		// calculate rules
		float angle = saturate((1.0f - dot(normal, vec3(0, 1, 0))) / 0.5f);
		float heightCutoff = saturate(max(0, height - rules.y) / 25.0f);

		const vec2 tilingFactor = vec2(4.0f);

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
	imageStore(LowresAlbedoOutput[Mip], outputUv, vec4(totalAlbedo, 1.0f));
	imageStore(LowresNormalOutput[Mip], outputUv, vec4(totalNormal, 0.0f));
	imageStore(LowresMaterialOutput[Mip], outputUv, vec4(totalMaterial, 0.0f));
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

TessellationTechnique(TerrainZ, "TerrainZ", vsTerrain(), psTerrainZ(), hsTerrain(), dsTerrain(), TerrainState);
TessellationTechnique(Terrain, "Terrain", vsTerrain(), psTerrain(), hsTerrain(), dsTerrain(), TerrainState);
TessellationTechnique(TerrainPrepass, "TerrainPrepass", vsTerrain(), psTerrainPrepass(), hsTerrain(), dsTerrain(), TerrainState);
SimpleTechnique(TerrainScreenSpace, "TerrainScreenSpace", vsScreenSpace(), psScreenSpace(), FinalState);

SimpleTechnique(TerrainVirtualScreenSpace, "TerrainVirtualScreenSpace", vsScreenSpace(), psScreenSpaceVirtual(), FinalState);

program TerrainTileSplatting [ string Mask = "TerrainTileSplatting"; ]
{
	ComputeShader = csTerrainTileUpdate();
};

program TerrainPageClearUpdateTexture [ string Mask = "TerrainPageClearUpdateTexture"; ]
{
	ComputeShader = csTerrainPageClearUpdateTexture();
};

program TerrainExtractPageTexture [ string Mask = "TerrainExtractPageTexture"; ]
{
	ComputeShader = csExtractPageUpdateTexture();
};

program TerrainPageClearUpdateBuffer [ string Mask = "TerrainPageClearUpdateBuffer"; ]
{
	ComputeShader = csTerrainPageClearUpdateBuffer();
};

program TerrainExtractPageBuffer [ string Mask = "TerrainExtractPageBuffer"; ]
{
	ComputeShader = csExtractPageUpdateBuffer();
};

program TerrainGenerateLowresFallback [ string Mask = "TerrainGenerateLowresFallback"; ]
{
	ComputeShader = csGenerateLowresFallback();
};

program TerrainTileUpdate [ string Mask = "TerrainTileUpdate"; ]
{
	ComputeShader = csTerrainTileUpdate();
};
