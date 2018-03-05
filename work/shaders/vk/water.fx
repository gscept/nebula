//------------------------------------------------------------------------------
//  water.fx
//
//	This shader is a GLSL derivative of the Island11.fx shader provided in the NVIDIA Direct3D SDK 11.
//
//  This software contains source code provided by NVIDIA Corporation.
//  Copyright (c) 2011 NVIDIA Corporation. All rights reserved.
//
//  (C) 2015 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/shared.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"
#include "lib/CSM.fxh"
#include "lib/tessellationparams.fxh"
#include "lib/defaultsamplers.fxh"

group(BATCH_GROUP) shared varblock WaterBlock [ bool DynamicOffset = true; ]
{

	float DynamicTessellationFactor = 32.0f;
	float WaveSpeed = 0.0f;
	vec4 WaterColor = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	vec4 WaterSpecular = vec4(1,1,1,1);
	vec4 WaterScatterColor = vec4(0.3f, 0.7f, 0.6f, 0.0f);
	vec2 WaterColorIntensity = vec2(0.1f, 0.2f);
	float FogDistance = 0.0f;

	vec4 FogBrightColor = vec4(1.0f, 1.1f, 1.4f, 0.0f);
	vec4 FogDarkColor = vec4(0.6f, 0.6f, 0.7f, 0.0f);
	float FogDensity = 1.0f;
	vec2 TileSize = vec2(512, 512);
	vec2 WaveScale = vec2(7, 7);
	vec2 WaveMicroScale = vec2(225, 225);
	float WaveHeight = 1.0f;
	vec2 DepthMapPixelSize;
	vec2 HeightMapPixelSize;
	vec2 ColorMapPixelSize;
	
	textureHandle ColorMap;
	textureHandle DepthMap;
	textureHandle WaveMap;
	textureHandle HeightMap;
};

samplerstate ScreenSampler
{
	Filter = Point;
	AddressU = Mirror;
	AddressV = Mirror;
};

samplerstate WaveSampler
{
	AddressU = Wrap;
	AddressV = Wrap;
};

state WaterState
{
	CullMode = Back;
	BlendEnabled[1] = true;
	SrcBlend[1] = One;
	DstBlend[1] = SrcAlpha;
	DepthWrite = true;
};

// calculating tessellation factor. It is either constant or hyperbolic depending on g_UseDynamicLOD switch
float CalculateTessellationFactor(float distance)
{
	return DynamicTessellationFactor * (1 / (0.015 * distance));
}

// to avoid vertex swimming while tessellation varies, one can use mipmapping for displacement maps
// it's not always the best choice, but it effificiently suppresses high frequencies at zero cost
float CalculateMIPLevelForDisplacementTextures(float distance)
{
	return log2(128/CalculateTessellationFactor(distance));
}

// primitive simulation of non-uniform atmospheric fog
float3 CalculateFogColor(vec3 pixel_to_light_vector, vec3 pixel_to_eye_vector)
{
	return lerp(FogDarkColor.rgb, FogBrightColor.rgb, 0.5f * dot(pixel_to_light_vector, pixel_to_eye_vector) + 0.5f);
}

float GetRefractionDepth(vec2 position)
{
	return sample2DLod(DepthMap, GeometryTextureSampler, position, 0).r;
}

float GetConservativeRefractionDepth(vec2 position)
{
	vec2 pixelSize = DepthMapPixelSize;
	float result =       sample2DLod(DepthMap, ScreenSampler, position + 2.0 * vec2(pixelSize.x, pixelSize.y), 0).r;
	result = min(result, sample2DLod(DepthMap, ScreenSampler, position + 2.0 * vec2(pixelSize.x, -pixelSize.y), 0).r);
	result = min(result, sample2DLod(DepthMap, ScreenSampler, position + 2.0 * vec2(-pixelSize.x, pixelSize.y), 0).r);
	result = min(result, sample2DLod(DepthMap, ScreenSampler, position + 2.0 * vec2(-pixelSize.x, -pixelSize.y), 0).r);
	return result;
}

#define ZNEAR 0.1f
#define ZFAR  100.0f

// constructing the displacement amount and normal for water surface geometry
vec4 CombineWaterNormal(vec3 worldPos)
{
	vec4 water_normal = float4(0.0f, 4.0f, 0.0f, 0.0f);
	float water_miplevel;
	float distance_to_camera;
	float disp;
	vec4 norm;
	float texcoord_scale = 1.0f;
	float height_disturbance_scale = 1.0f;
	float normal_disturbance_scale = 1.0f;
	vec2 tc;
	vec2 variance = {1.0f, 1.0f};

	// calculating MIP level for water texture fetches
	distance_to_camera = distance(EyePos.xyz, worldPos);
	water_miplevel = CalculateMIPLevelForDisplacementTextures(distance_to_camera) / 2.0f - 2.0f;
	tc = (worldPos.xz * WaveScale / TileSize);
	
	vec2 translation = vec2(TimeAndRandom.x * 1.5f, TimeAndRandom.x * 0.75f) * WaveSpeed;

	// 0.65 + 0.4225 + 0.1785 + 0.0318 + 0.001
	// fetching water heightmap
	for (float i=0; i < 5; i++)
	{
		norm = sample2DLod(WaveMap, WaveSampler, tc * texcoord_scale + translation * 0.03f * variance, water_miplevel);
		//disp = textureLod(HeightMap, tc * texcoord_scale + translation * 0.03f * variance, water_miplevel).a;
		variance.x *= -1.0f;
		water_normal.xz += (2 * norm.xy - vec2(1.0f, 1.0f)) * normal_disturbance_scale;
		water_normal.w += (norm.w - 0.5f) * height_disturbance_scale;
		texcoord_scale *= 1.4f;
		height_disturbance_scale *= 0.65f;
		normal_disturbance_scale *= 0.65f;
	}
	water_normal.w *= WaveHeight;
	return vec4(normalize(water_normal.xyz), water_normal.w);
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsDefault(
	[slot=0] in vec3 position,
	[slot=1] in vec3 normal,
	[slot=2] in vec2 uv,
	[slot=3] in vec3 tangent,
	[slot=4] in vec3 binormal,
	out vec3 WorldPos,
	out vec3 ViewSpacePos,
	out vec3 Normal,
	out vec4 ProjPos,
	out vec2 UV,
	out vec3 EyeVec,
	out float DepthScale) 
{
	vec4 modelSpace = Model * vec4(position, 1);
	gl_Position = ViewProjection * modelSpace;
	ProjPos = gl_Position;
	WorldPos = position.xyz;
	mat4 modelView = View * Model;
	ViewSpacePos = (modelView * vec4(position, 1)).xyz;

	vec2 texcoord0to1 = modelSpace.xz/TileSize;
	vec2 depthCoord = (position.xz) * HeightMapPixelSize;
	DepthScale = sample2DLod(HeightMap, WaveSampler, vec2(depthCoord.x, 1 - depthCoord.y), 0).r;
	
	vec2 translation = vec2(TimeAndRandom.x * 1.5f, TimeAndRandom.x * 0.75f) * WaveSpeed;
	UV = texcoord0to1 * WaveMicroScale + translation * 0.07f;	
	/*
	UV1.xy = texCoords.xy + uvTranslation * 2.0f * WaveSpeed;
	UV1.zw = texCoords.xy * 2.0 + uvTranslation * 4.0 * WaveSpeed;
	UV2.xy = texCoords.xy * 4.0 + uvTranslation * 2.0 * WaveSpeed;
	UV2.zw = texCoords.xy * 8.0 + uvTranslation * WaveSpeed;      
	*/
	Normal  = normal;
	//Normal = CombineWaterNormal(WorldPos).xyz;
		
	EyeVec = EyePos.xyz - modelSpace.xyz;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsTessellated(
	[slot=0] in vec3 position,
	[slot=1] in vec3 normal,
	[slot=2] in vec2 uv,
	[slot=3] in vec3 tangent,
	[slot=4] in vec3 binormal,
	out vec4 Position,
	out vec3 Normal,
	out float Distance) 
{
	Position = vec4(position, 1);	
	Normal  = normal;
	
	float vertexDistance = distance( Position.xyz, EyePos.xyz );
	Distance = 1.0 - clamp( ( (vertexDistance - MinDistance) / (MaxDistance - MinDistance) ), 0.0, 1.0 - 1.0/TessellationFactor);
}

//------------------------------------------------------------------------------
/**
	This is a GLSL converted version of the Island11.fx shader found in the Nvidia SDK 11.
*/
shader
void
psDefault(
	in vec3 WorldPos,
	in vec3 ViewSpacePos,
	in vec3 Normal,
	in vec4 ProjPos,
	in vec2 UV,
	in vec3 EyeVec,
	in float DepthScale,
	[color0] out vec4 Albedo,
	[color1] out vec4 Unshaded) 
{   	
	vec2 translation = vec2(TimeAndRandom.x * 1.5f, TimeAndRandom.x * 0.75f) * WaveSpeed;
	
	vec3 tNormal = normalize(2 * sample2D(WaveMap, WaveSampler, UV - translation * 0.2f).gbr - vec3(1, -8, 1));
	tNormal += normalize(2 * sample2D(WaveMap, WaveSampler, UV * 0.5f + translation * 0.05f).gbr - vec3(1, -8, 1));

	// calculate bump mapping
	mat3x3 normal_rotation_matrix;
	normal_rotation_matrix[1] = Normal.xyz;
	normal_rotation_matrix[2] = normalize(cross(float3(0.0,0.0,-1.0), normal_rotation_matrix[1]));
	normal_rotation_matrix[0] = normalize(cross(normal_rotation_matrix[2], normal_rotation_matrix[1]));
	tNormal = normal_rotation_matrix * normalize(tNormal);
	
	// calculate screen space UV coordinate
	vec2 pixelSize = ColorMapPixelSize;
	vec2 screenUv = psComputeScreenCoord(gl_FragCoord.xy, pixelSize.xy);
	
	// calculating pixel position in light space
	/*
	vec4 texShadow;
	CSMConvert(vec4(WorldPos, 1.0f), texShadow);
	vec4 debug;
	float shadowFactor = CSMPS(texShadow, screenUv, debug);
	*/
	float shadowFactor = 1.0f;
	
	// calculate lighting vectors, global light position is in the direction of the light times ALOT
	float ssDepth = sample2DLod(DepthMap, ScreenSampler, screenUv, 0).r;
	vec3 pixel_to_eye_vector = normalize(EyeVec);
	vec3 pixel_to_light_vector = normalize(GlobalLightDir.xyz);
	
	// calculate MVP and MV
	mat4x4 MVP = View * Model;
	mat4x4 MV = View * Model;
			
	// simulating scattering/double refraction: light hits the side of wave, travels some distance in water, and leaves wave on the other side
	// it's difficult to do it physically correct without photon mapping/ray tracing, so using simple but plausible emulation below
	
	// only the crests of water waves generate double refracted light
	float scatter_factor = 2.5f * max(0.0f, WorldPos.y * 0.25f + 0.25f);
	
	// calculate distance to point
	float distanceToPoint = length(EyeVec);
	
	// calculate NL, clamp within reasonable range (negative values are only applicable if light is supposed to pass through the medium)
	float NL = saturate(dot(pixel_to_light_vector, tNormal));
	float NV = saturate(dot(pixel_to_eye_vector, tNormal));

	// the waves that lie between camera and light projection on water plane generate maximal amount of double refracted light 
	scatter_factor *= shadowFactor * pow(max(0.0f, dot(normalize(vec3(pixel_to_light_vector.x, 0.0f, pixel_to_light_vector.z)), -pixel_to_eye_vector)), 2.0f);
	
	// the slopes of waves that are oriented back to light generate maximal amount of double refracted light 
	scatter_factor *= pow(max(0.0f, 1.0f - NL), 8.0f);
	
	// water crests gather more light than lobes, so more light is scattered under the crests
	scatter_factor += shadowFactor * 1.5f * WaterColorIntensity.y * max(0.0f, WorldPos.y + 1) *
		// the scattered light is best seen if observing direction is normal to slope surface
		max(0.0f, NV) *
		// fading scattered light out at distance and if viewing direction is vertical to avoid unnatural look
		max(0.0f, 1.0f - pixel_to_eye_vector.y) * (300.0f / (300 + distanceToPoint));
		
	// fading scatter out by 90% near shores so it looks better
	scatter_factor *= 0.1 + 0.9 * DepthScale;

	// calculating fresnel factor 
	float r = (1.2f - 1.0f) / (1.2f + 1.0f);
	float fresnel_factor = max(0.0f, min(1.0f, r + (1.0f - r) * pow(1.0f - NV, 4)));

	// calculating specular factor
	vec3 reflected_eye_to_pixel_vector = reflect(-pixel_to_eye_vector, tNormal);
	//float specPower = exp2(10 * WaterSpecular.a + 1);
	float specular_factor = shadowFactor * fresnel_factor * pow(max(0.0f, dot(pixel_to_light_vector, reflected_eye_to_pixel_vector)), 1000.0f);

	// calculating diffuse intensity of water surface itself
	float diffuse_factor = WaterColorIntensity.x + WaterColorIntensity.y * max(0.0f, NL);

	// calculating disturbance which has to be applied to planar reflections/refractions to give plausible results
	vec4 disturbance_eyespace = MV * vec4(tNormal.x, 0, tNormal.z, 0);

	vec2 reflection_disturbance = vec2(disturbance_eyespace.x, disturbance_eyespace.z) * 0.03f;
	vec2 refraction_disturbance = vec2(-disturbance_eyespace.x, disturbance_eyespace.y) * 0.05f *
		// fading out reflection disturbance at distance so reflection doesn't look noisy at distance
		(20.0/(20 + distanceToPoint));
		
	// calculating correction that shifts reflection up/down according to water wave Y position
	float4 projected_waveheight = MVP * vec4(WorldPos, 1);
	float waveheight_correction = -0.5f * projected_waveheight.y / projected_waveheight.w;
	projected_waveheight = MVP * vec4(WorldPos.x, -0.8f, WorldPos.z, 1);
	waveheight_correction += 0.5f * projected_waveheight.y / projected_waveheight.w;
	reflection_disturbance.y = max(-0.15f, waveheight_correction + reflection_disturbance.y);
	
	// picking refraction depth at non-displaced point, need it to scale the refraction texture displacement amount according to water depth
	float refraction_depth = GetRefractionDepth(screenUv);
	
	// calculate world-space position of pixel
	float depth = length(ViewSpacePos.xyz);
	float water_depth = refraction_depth - depth;
	float nondisplaced_water_depth = water_depth;
	
	// scaling refraction texture displacement amount according to water depth, with some limit
	refraction_disturbance *= min(2.0f, water_depth);

	// picking refraction depth again, now at displaced point, need it to calculate correct water depth
	refraction_depth = GetRefractionDepth(screenUv + refraction_disturbance);
	water_depth = refraction_depth - depth;

	// zeroing displacement for points where displaced position points at geometry which is actually closer to the camera than the water surface
	float conservative_refraction_depth = GetConservativeRefractionDepth(screenUv + refraction_disturbance);
	float conservative_water_depth = conservative_refraction_depth - depth;

	if (conservative_water_depth < 0)
	{
		refraction_disturbance = vec2(0);
		water_depth = nondisplaced_water_depth;
	}
	
	// blend between depth and float max value, by multiplying depth with -1 and clamping between 0-1. This ensures negative depth values also shows the water
	//water_depth = lerp(water_depth, FLT_MAX, saturate(-1 * water_depth));
	water_depth = max(0.0f, water_depth);

	// getting reflection and refraction color at disturbed texture coordinates
	vec4 reflection_color = sampleCubeLod(EnvironmentMap, EnvironmentSampler, reflected_eye_to_pixel_vector + vec3(reflection_disturbance.x, 0, reflection_disturbance.y), 0); 
	//vec4 reflection_color = DecodeHDR(textureLod(ReflectionMap, reflected_eye_to_pixel_vector, 0));
	vec4 refraction_color = DecodeHDR(sample2DLod(ColorMap, ScreenSampler, screenUv + refraction_disturbance, 0));

	// calculating water surface color and applying atmospheric fog to it
	vec4 water_color = diffuse_factor * vec4(WaterColor.rgb, 1);
	water_color.rgb = lerp(CalculateFogColor(pixel_to_light_vector, pixel_to_eye_vector).rgb, water_color.rgb, min(1.0f, exp(-distanceToPoint * FogDensity)));
	
	// fading fresnel factor to 0 to soften water surface edges
	fresnel_factor *= min(1.0f, water_depth * 5.0f);

	// fading refraction color to water color according to distance that refracted ray travels in water 
	refraction_color = lerp(water_color, refraction_color, min(1.0f, 1.0f * exp(-water_depth / 8.0f)));
	
	// combining final water color
	vec4 color;
	color.rgb = lerp(refraction_color.rgb, reflection_color.rgb, fresnel_factor);
	color.rgb += 350.0f * specular_factor * WaterSpecular.rgb * fresnel_factor;
	color.rgb += WaterScatterColor.rgb * scatter_factor;
	color.a = 1;
	Albedo = EncodeHDR(color);
	Unshaded = vec4(color.rgb, fresnel_factor);
	/*
	
	// calculate refraction
	vec2 normalMapPixelSize = GetPixelSize(ColorMap);
	vec2 screenUv = psComputeScreenCoord(gl_FragCoord.xy, normalMapPixelSize.xy);
	
	vec3 refractiveBump = tNormal * vec3(0.02f, 0.02f, 1.0f);
	vec3 reflectiveBump = tNormal * vec3(0.1f, 0.1f, 1.0f);
	
	// sample directly, and at offset
	vec4 refractionA = textureLod(ColorMap, screenUv.xy + refractiveBump.xy, 0);
	vec4 refractionB = textureLod(ColorMap, screenUv.xy, 0);
	vec4 refraction = refractionA * refractionA.w + refractionA * (1 - refractionA.w);
	
	vec3 eyeVec = normalize(EyeVec);
	
	// cheat, here we should use the reflection map but instead we just use the same color map...
	vec4 reflection;

	if (CheapReflections)
	{
		vec3 reflectionSample = reflect(eyeVec, normalize(tNormal));
		reflection = texture(ReflectionMap, reflectionSample) * ReflectionIntensity;
	}
	else
	{
		reflection = texture(ColorMap, screenUv.xy + reflectiveBump.xy);
	}
	
	float specPower = exp2(10 * WaterSpecular.a + 1);
	
	float NL = dot(GlobalLightDir.xyz, ViewSpaceNormal);
	float x = dot(ViewSpaceNormal, normalize(ViewSpacePos));
	vec3 viewVec = normalize(ViewSpacePos);
	
	vec3 rim = (WaterSpecular.rgb + (1 - WaterSpecular.rgb) * (pow((1 - x), 5) / (4 - 3 * WaterSpecular.a)));
	vec3 reflectionColor = reflection.rgb * saturate(rim);
	
	vec3 H = normalize(GlobalLightDir.xyz - viewVec);
	float NH = saturate(dot(ViewSpaceNormal, H));
	float NV = saturate(dot(ViewSpaceNormal, viewVec));
	float HL = saturate(dot(H, GlobalLightDir.xyz));
	vec3 spec = BRDFLighting(NH, NL, NV, HL, specPower, GlobalLightColor.xyz, WaterSpecular.rgb * reflectionColor);
	
	// sample depths, the one for foam shouldn't be offset by refraction
	float depth = textureLod(DepthMap, screenUv.xy + refractiveBump.xy, 0).r;
	float foamDepth = textureLod(DepthMap, screenUv.xy, 0).r;
	
	float pixelDepth = length(ViewSpacePos.xyz);
	vec3 foamA = texture(FoamMap, UV1.xy).rgb;
	vec3 foamB = texture(FoamMap, UV1.zw).rgb;
	vec3 foamC = texture(FoamMap, UV2.xy).rgb;
	vec3 foamD = texture(FoamMap, UV2.zw).rgb;
	vec3 foamColor = (foamA + foamB + foamC + foamD) * 0.25;
	float foamFactor = saturate(FoamDistance * 1 / (foamDepth - pixelDepth));
	
	float depthDiff = 1 / (depth - pixelDepth);
	float deepFactor = saturate(DeepDistance * depthDiff);
	vec3 waterColor = lerp(WaterColor, refraction, deepFactor).rgb;
	
	vec3 foam = foamColor * FoamIntensity * foamFactor;	
	waterColor += foam;
	
	vec4 finalColor = vec4(waterColor + spec, 1);
	
	float fogFactor = saturate(FogDistance / max(depth - pixelDepth, 0));
	finalColor.rgb = lerp(FogColor.rgb, finalColor.rgb, fogFactor);
	
	// blend refraction values
	Albedo = EncodeHDR(finalColor);
	Unshaded = vec4(finalColor.rgb, 1 - (NL + 0.5f)) + UV1;
	*/
}

//------------------------------------------------------------------------------
/**
*/
[inputvertices] = 3
[outputvertices] = 3
shader
void 
hsDefault(
	in vec4 position[],
	in vec3 normal[],
	in float dist[],
	out vec4 Position[],
	out vec3 Normal[]
)
{	
	Position[gl_InvocationID] = position[gl_InvocationID];
	Normal[gl_InvocationID] = normal[gl_InvocationID];
	
	// perform per-patch operation
	if (gl_InvocationID == 0)
	{		
		vec4 EdgeTessFactors;
		EdgeTessFactors.x = 0.5f * (dist[1] + dist[2]);
		EdgeTessFactors.y = 0.5f * (dist[2] + dist[0]);
		EdgeTessFactors.z = 0.5f * (dist[0] + dist[1]);
		//EdgeTessFactors.w = distance(EyePos.xz, position[3].xz);
		
		// calculate factors
		EdgeTessFactors.x = CalculateTessellationFactor(EdgeTessFactors.x);
		EdgeTessFactors.y = CalculateTessellationFactor(EdgeTessFactors.y);
		EdgeTessFactors.z = CalculateTessellationFactor(EdgeTessFactors.z);
		//EdgeTessFactors.w = CalculateTessellationFactor(EdgeTessFactors.w);
	
		// set tessellation factors
		gl_TessLevelOuter[0] = EdgeTessFactors.x;
		gl_TessLevelOuter[1] = EdgeTessFactors.y;
		gl_TessLevelOuter[2] = EdgeTessFactors.z;
		//gl_TessLevelOuter[3] = EdgeTessFactors.w;
		gl_TessLevelInner[0] = (gl_TessLevelOuter[0] + gl_TessLevelOuter[1] + gl_TessLevelOuter[2]) / 3;
		//gl_TessLevelInner[0] = gl_TessLevelInner[1] = (gl_TessLevelOuter[0] + gl_TessLevelOuter[1] + gl_TessLevelOuter[2] + gl_TessLevelOuter[3]) * 0.25f;
	}
}

//------------------------------------------------------------------------------
/**
*/
[inputvertices] = 3
[winding] = ccw
[topology] = triangle
[partition] = odd
shader
void
dsDefault(
	in vec4 position[],
	in vec3 normal[],
	out vec3 WorldPos,
	out vec3 ViewSpacePos,
	out vec3 Normal,
	out vec4 ProjPos,
	out vec2 UV,	
	out vec3 EyeVec,
	out float DepthScale)
{

	// calculate barycentric coordinate for vertex
	vec3 pos = position[0].xyz * gl_TessCoord.x + position[1].xyz * gl_TessCoord.y + position[2].xyz * gl_TessCoord.z;
	vec3 norm = normal[0].xyz * gl_TessCoord.x + normal[1].xyz * gl_TessCoord.y + normal[2].xyz * gl_TessCoord.z;
	
	// calculate texture coordinate based on world space position
	vec4 worldSpace = (Model * vec4(pos, 1));
	vec2 texcoord0to1 = worldSpace.xz/TileSize;
	vec2 depthCoord = (pos.xz) * HeightMapPixelSize;
	depthCoord.y = 1 - depthCoord.y;
	
	// sample from texture the amount of displacement per pixel, this is done in the space of the texture, and not using the tile size
	DepthScale = sample2DLod(HeightMap, WaveSampler, depthCoord, 0).r;
	
	// this will be our final vertex position
	vec3 vertex;
	vertex.xz = pos.xz;
	vertex.y = pos.y - WaveHeight/2.0f;
	
	// calculate water normal based on the Model-based coordinates
	vec4 disp = CombineWaterNormal(vec3(texcoord0to1.x, vertex.y, texcoord0to1.y));
	disp.xyz = lerp(vec3(0,1,0), normalize(disp.xyz), 0.4 + 0.6 * DepthScale);
	
	// displace vertex
	vertex.y += disp.w * WaveHeight * (0.4 + 0.6 * DepthScale);
	vertex.xz -= (disp.xz) * 0.5f * (0.4 + 0.6 * DepthScale);

	// calculate model-view matrix
	mat4x4 modelView = View * Model;		
	
	// world position is NOT multiplied by the model matrix
	ViewSpacePos = (modelView * vec4(vertex, 1.0f)).xyz;	
	worldSpace = Model * vec4(vertex, 1.0f);
	WorldPos = vertex.xyz;
	gl_Position = ViewProjection * worldSpace;
	ProjPos = gl_Position;
		
	// texture coordinates is relative to the vertex position
	vec2 translation = vec2(TimeAndRandom.x * 1.5f, TimeAndRandom.x * 0.75f) * WaveSpeed;
	
	UV = texcoord0to1 * WaveMicroScale + translation * 0.07f;	
	EyeVec = EyePos.xyz - worldSpace.xyz;      	
	
	// write to Normal
	//Normal = normalize(norm.xyz);
	Normal = disp.xyz;
}

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(Default, "Static", vsDefault(), psDefault(), WaterState);
TessellationTechnique(Tessellated, "Static|Tessellated", vsTessellated(), psDefault(), hsDefault(), dsDefault(), WaterState);
