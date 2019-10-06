//------------------------------------------------------------------------------
//  geometrybase.fxh
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#ifndef GEOMETRYBASE_FXH
#define GEOMETRYBASE_FXH

#include "lib/std.fxh"
#include "lib/skinning.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"
#include "lib/defaultsamplers.fxh"

//#define PN_TRIANGLES
state StandardState
{
};

state StandardNoCullState
{
	CullMode = None;
};

state AlphaState
{
	BlendEnabled[0] = true;
	SrcBlend[0] = SrcAlpha;
	DstBlend[0] = OneMinusSrcAlpha;
	DepthWrite = false;
	DepthEnabled = true;
};

float FresnelPower = 0.0f;
float FresnelStrength = 0.0f;

#include "lib/materialparams.fxh"
#include "lib/tessellationparams.fxh"

//---------------------------------------------------------------------------------------------------------------------------
//											DIFFUSE
//---------------------------------------------------------------------------------------------------------------------------
prototype vec4 CalculateColor(vec4 albedoColor, vec4 color, vec4 spec);
subroutine (CalculateColor) vec4 SimpleColor(
	in vec4 albedoColor,
	in vec4 color,
	in vec4 spec)
{
	return vec4(albedoColor.rgb * (1 - spec.rgb), albedoColor.a);
}

subroutine (CalculateColor) vec4 SimpleColorMultiply(
	in vec4 albedoColor,
	in vec4 color,
	in vec4 spec)
{
	vec4 col = albedoColor * color;
	return vec4(col.rgb * (1 - spec.rgb), albedoColor.a * col.a);
}

subroutine (CalculateColor) vec4 AlphaColor(
	in vec4 albedoColor,
	in vec4 color,
	in vec4 spec)
{
	return vec4(albedoColor.rgb * (1 - spec.rgb), albedoColor.a);
}

subroutine (CalculateColor) vec4 AlphaColorMultiply(
	in vec4 albedoColor,
	in vec4 color,
	in vec4 spec)
{
	vec4 col = albedoColor * color;
	return vec4(albedoColor.rgb * (1 - spec.rgb), albedoColor.a);
}

CalculateColor calcColor;


//---------------------------------------------------------------------------------------------------------------------------
//											NORMAL
//---------------------------------------------------------------------------------------------------------------------------
prototype vec3 CalculateBump(in vec3 tangent, in vec3 binormal, in vec3 normal, in vec4 bump);
subroutine (CalculateBump) vec3 NormalMapFunctor(
	in vec3 tangent,
	in vec3 binormal,
	in vec3 normal,
	in vec4 bumpData)
{
	mat3 tangentViewMatrix = mat3(normalize(tangent.xyz), normalize(binormal.xyz), normalize(normal.xyz));
	vec3 tNormal = vec3(0,0,0);
	tNormal.xy = (bumpData.ag * 2.0f) - 1.0f;
	tNormal.z = saturate(sqrt(1.0f - dot(tNormal.xy, tNormal.xy)));
	return tangentViewMatrix * tNormal;
}

subroutine (CalculateBump) vec3 FlatNormalFunctor(
	in vec3 tangent,
	in vec3 binormal,
	in vec3 normal,
	in vec4 bumpData)
{
	return normal;
}
CalculateBump calcBump;

//---------------------------------------------------------------------------------------------------------------------------
//											SPECULAR
//---------------------------------------------------------------------------------------------------------------------------
prototype vec4 CalculateSpecular(in vec3 specularColor, in float roughness);
subroutine (CalculateSpecular) vec4 NonReflectiveSpecularFunctor(
	in vec3 specularColor,
	in float roughness)
{
	//return vec4(pow(specularColor - 1, vec3(2)) / (pow(specularColor + 1, vec3(2))), roughness);
	return vec4(specularColor, roughness);
}

CalculateSpecular calcSpec;

//---------------------------------------------------------------------------------------------------------------------------
//											ENVIRONMENT
//
//	Note: We must return a mat2x3 (for 2 * vec3) since two outputs causes compilation issues, and subroutines cannot return structs.
//---------------------------------------------------------------------------------------------------------------------------
prototype mat2x3 CalculateEnvironment(
	in vec4 specularColor,
	in vec3 worldNormal,
	in vec3 worldViewVec,
	in float roughness);

subroutine (CalculateEnvironment) mat2x3 PBR(
	in vec4 specularColor,
	in vec3 worldNormal,
	in vec3 worldViewVec,
	in float roughness)
{
	mat2x3 ret;
	vec3 reflectVec = reflect(worldViewVec, worldNormal.xyz);
	float x = dot(worldNormal, normalize(worldViewVec));
	vec3 rim = FresnelSchlickGloss(specularColor.rgb, x, roughness);
	ret[1] = sampleCubeLod(EnvironmentMap, CubeSampler, reflectVec, (1.0f - roughness) * NumEnvMips).rgb * rim;
	ret[0] = sampleCubeLod(IrradianceMap, CubeSampler, worldNormal.xyz, 0).rgb;
	return ret;
}

subroutine (CalculateEnvironment) mat2x3 ReflectionOnly(
	in vec4 specularColor,
	in vec3 worldNormal,
	in vec3 worldViewVec,
	in float roughness)
{
	mat2x3 ret;
	vec3 reflectVec = reflect(worldViewVec, worldNormal.xyz);
	float x = dot(worldNormal, normalize(worldViewVec));
	vec3 rim = FresnelSchlickGloss(specularColor.rgb, x, roughness);
	ret[1] = sampleCubeLod(EnvironmentMap, CubeSampler, reflectVec, (1.0f - pow(roughness, 2)) * NumEnvMips).rgb * rim;
	ret[0] = vec3(0);
	return ret;
}

subroutine (CalculateEnvironment) mat2x3 IrradianceOnly(
	in vec4 specularColor,
	in vec3 worldNormal,
	in vec3 worldViewVec,
	in float roughness)
{
	mat2x3 ret;
	ret[1] = vec3(0);
	ret[0] = sampleCubeLod(IrradianceMap, CubeSampler, worldNormal.xyz, 0).rgb;
	return ret;
}

subroutine (CalculateEnvironment) mat2x3 NoEnvironment(
	in vec4 specularColor,
	in vec3 worldNormal,
	in vec3 worldViewVec,
	in float roughness)
{
	mat2x3 ret;
	ret[1] = vec3(0);
	ret[0] = vec3(0);
	return ret;
}

CalculateEnvironment calcEnv;


mat2x3 PBRSpec(
	in vec4 specularColor,
	in vec3 viewSpaceNormal,
	in vec3 viewSpacePos,
	in vec3 worldViewVec,
	in mat4 invView,
	in float roughness)
{
	mat2x3 ret;
	vec4 worldNorm = (invView * vec4(viewSpaceNormal, 0));
	vec3 reflectVec = reflect(worldViewVec, worldNorm.xyz);
	float x = dot(-viewSpaceNormal, normalize(viewSpacePos));
	vec3 rim = FresnelSchlickGloss(specularColor.rgb, x, roughness);
	ret[1] = sampleCubeLod(EnvironmentMap, CubeSampler, reflectVec, (1.0f - pow(roughness, 2)) * NumEnvMips).rgb * rim;
	ret[0] = sampleCubeLod(IrradianceMap, CubeSampler, worldNorm.xyz, 0).rgb;
	return ret;
}

//------------------------------------------------------------------------------
/**
				STATIC GEOMETRY
*/
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
*/
shader
void
vsStatic(
	[slot=0] in vec3 position,
	[slot=1] in vec3 normal,
	[slot=2] in vec2 uv,
	[slot=3] in vec3 tangent,
	[slot=4] in vec3 binormal,
	out vec3 Tangent,
	out vec3 Normal,
	out vec3 Binormal,
	out vec2 UV,
	out vec3 WorldViewVec)
{
	vec4 modelSpace = Model * vec4(position, 1);
    gl_Position = ViewProjection * modelSpace;
    UV = uv;

	Tangent 	= (Model * vec4(tangent, 0)).xyz;
	Normal 		= (Model * vec4(normal, 0)).xyz;
	Binormal 	= (Model * vec4(binormal, 0)).xyz;
	WorldViewVec = modelSpace.xyz - EyePos.xyz;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsStaticInstanced(
	[slot=0] in vec3 position,
	[slot=1] in vec3 normal,
	[slot=2] in vec2 uv,
	[slot=3] in vec3 tangent,
	[slot=4] in vec3 binormal,
	out vec3 Tangent,
	out vec3 Normal,
	out vec3 Binormal,
	out vec2 UV,
	out vec3 WorldViewVec)
{
	vec4 modelSpace = ModelArray[gl_InstanceID] * vec4(position, 1);
    gl_Position = ViewProjection * modelSpace;
    UV = uv;

	Tangent = (Model * vec4(tangent, 0)).xyz;
	Normal = (Model * vec4(normal, 0)).xyz;
	Binormal = (Model * vec4(binormal, 0)).xyz;
	WorldViewVec = modelSpace.xyz - EyePos.xyz;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsStaticTessellated(
	[slot=0] in vec3 position,
	[slot=1] in vec3 normal,
	[slot=2] in vec2 uv,
	[slot=3] in vec3 tangent,
	[slot=4] in vec3 binormal,
	out vec3 Tangent,
	out vec3 Normal,
	out vec4 Position,
	out vec3 Binormal,
	out vec2 UV,
	out float Distance)
{
    Position = Model * vec4(position, 1);
    UV = uv;

	Tangent = (Model * vec4(tangent, 0)).xyz;
	Normal = (Model * vec4(normal, 0)).xyz;
	Binormal = (Model * vec4(binormal, 0)).xyz;

	float vertexDistance = distance( Position.xyz, EyePos.xyz );
	Distance = 1.0 - clamp( ( (vertexDistance - MinDistance) / (MaxDistance - MinDistance) ), 0.0, 1.0 - 1.0/TessellationFactor);
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsStaticColored(
	[slot=0] in vec3 position,
	[slot=1] in vec3 normal,
	[slot=2] in vec2 uv,
	[slot=3] in vec3 tangent,
	[slot=4] in vec3 binormal,
	[slot=5] in vec4 color,
	out vec3 Tangent,
	out vec3 Normal,
	out vec3 Binormal,
	out vec2 UV,
	out vec4 Color,
	out vec3 WorldViewVec)
{
	vec4 modelSpace = Model * vec4(position, 1);
    gl_Position = ViewProjection * modelSpace;
	UV = uv;
	Color = color;

	Tangent = (Model * vec4(tangent, 0)).xyz;
	Normal = (Model * vec4(normal, 0)).xyz;
	Binormal = (Model * vec4(binormal, 0)).xyz;
	WorldViewVec = modelSpace.xyz - EyePos.xyz;
}

//------------------------------------------------------------------------------
/**
				SKINNED GEOMETRY
*/
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
	Transform feedback for skinned vertex buffers
*/
/*
shader
void
vsTransformSkinned(in vec3 position,
	in vec3 normal,
	in vec2 uv,
	in vec3 tangent,
	in vec3 binormal,
	[slot=7] in vec4 weights,
	[slot=8] in uvec4 indices,
	[feedback=(0,0)] out vec3 OutPos,
	[feedback=(0,12)] out vec3 OutNormal,
	[feedback=(0,24)] out vec2 UV,
	[feedback=(0,32)] out vec3 OutTangent,
	[feedback=(0,44)] out vec3 OutBinormal)
{
	OutPos      = SkinnedPosition(position, weights, indices).xyz;
	OutNormal   = SkinnedNormal(normal, weights, indices).xyz;
	OutTangent  = SkinnedNormal(tangent, weights, indices).xyz;
	OutBinormal = SkinnedNormal(binormal, weights, indices).xyz;
	UV = uv;
}
*/

//------------------------------------------------------------------------------
/**
*/
shader
void
vsSkinned(
	[slot=0] in vec3 position,
	[slot=1] in vec3 normal,
	[slot=2] in vec2 uv,
	[slot=3] in vec3 tangent,
	[slot=4] in vec3 binormal,
	[slot=7] in vec4 weights,
	[slot=8] in uvec4 indices,
	out vec3 Tangent,
	out vec3 Normal,
	out vec3 Binormal,
	out vec2 UV,
	out vec3 WorldViewVec)
{
	vec4 skinnedPos      = SkinnedPosition(position, weights, indices);
	vec4 skinnedNormal   = SkinnedNormal(normal, weights, indices);
	vec4 skinnedTangent  = SkinnedNormal(tangent, weights, indices);
	vec4 skinnedBinormal = SkinnedNormal(binormal, weights, indices);

	vec4 modelSpace = Model * skinnedPos;
    gl_Position = ViewProjection * modelSpace;
	UV = uv;

	Tangent = (Model * skinnedTangent).xyz;
	Normal = (Model * skinnedNormal).xyz;
	Binormal = (Model * skinnedBinormal).xyz;
	WorldViewVec = modelSpace.xyz - EyePos.xyz;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsSkinnedTessellated(
	[slot=0] in vec3 position,
	[slot=1] in vec3 normal,
	[slot=2] in vec2 uv,
	[slot=3] in vec3 tangent,
	[slot=4] in vec3 binormal,
	[slot=7] in vec4 weights,
	[slot=8] in uvec4 indices,
	out vec3 Tangent,
	out vec3 Normal,
	out vec4 Position,
	out vec3 Binormal,
	out vec2 UV,
	out float Distance)
{
	vec4 skinnedPos      = SkinnedPosition(position, weights, indices);
	vec4 skinnedNormal   = SkinnedNormal(normal, weights, indices);
	vec4 skinnedTangent  = SkinnedNormal(tangent, weights, indices);
	vec4 skinnedBinormal = SkinnedNormal(binormal, weights, indices);

	Position = Model * skinnedPos;
	UV = uv;

	Tangent = (Model * skinnedTangent).xyz;
	Normal = (Model * skinnedNormal).xyz;
	Binormal = (Model * skinnedBinormal).xyz;

	float vertexDistance = distance( Position.xyz, EyePos.xyz );
	Distance = 1.0 - clamp( ( (vertexDistance - MinDistance) / (MaxDistance - MinDistance) ), 0.0, 1.0 - 1.0/TessellationFactor);
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsBillboard(
	[slot=0] in vec3 position,
	[slot=1] in vec3 normal,
	[slot=2] in vec2 uv,
	[slot=3] in vec3 tangent,
	[slot=4] in vec3 binormal,
	out vec2 UV)
{
	gl_Position = ViewProjection * Model * vec4(position, 0);
	UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
[inputvertices] = 3
[outputvertices] = 6
shader
void
hsDefault(in vec3 tangent[],
		  in vec3 normal[],
		  in vec4 position[],
		  in vec3 binormal[],
		  in vec2 uv[],
		  in float distance[],
		  out vec3 Tangent[],
		  out vec3 Normal[],
		  out vec4 Position[],
		  out vec3 Binormal[],
		  out vec2 UV[]
#ifdef PN_TRIANGLES
,
		  patch out vec3 f3B210,
		  patch out vec3 f3B120,
		  patch out vec3 f3B021,
		  patch out vec3 f3B012,
		  patch out vec3 f3B102,
		  patch out vec3 f3B201,
		  patch out vec3 f3B111
#endif
		  )
{
	Tangent[gl_InvocationID] 	= tangent[gl_InvocationID];
	Normal[gl_InvocationID] 	= normal[gl_InvocationID];
	Position[gl_InvocationID]	= position[gl_InvocationID];
	Binormal[gl_InvocationID] 	= binormal[gl_InvocationID];
	UV[gl_InvocationID]			= uv[gl_InvocationID];

	// perform per-patch operation
	if (gl_InvocationID == 0)
	{
		vec4 EdgeTessFactors;
		EdgeTessFactors.x = 0.5 * (distance[1] + distance[2]);
		EdgeTessFactors.y = 0.5 * (distance[2] + distance[0]);
		EdgeTessFactors.z = 0.5 * (distance[0] + distance[1]);
		EdgeTessFactors *= TessellationFactor;

#ifdef PN_TRIANGLES
		// compute the cubic geometry control points
		// edge control points
		f3B210 = ( ( 2.0f * position[0] ) + position[1] - ( dot( ( position[1] - position[0] ), normal[0] ) * normal[0] ) ) / 3.0f;
		f3B120 = ( ( 2.0f * position[1] ) + position[0] - ( dot( ( position[0] - position[1] ), normal[1] ) * normal[1] ) ) / 3.0f;
		f3B021 = ( ( 2.0f * position[1] ) + position[2] - ( dot( ( position[2] - position[1] ), normal[1] ) * normal[1] ) ) / 3.0f;
		f3B012 = ( ( 2.0f * position[2] ) + position[1] - ( dot( ( position[1] - position[2] ), normal[2] ) * normal[2] ) ) / 3.0f;
		f3B102 = ( ( 2.0f * position[2] ) + position[0] - ( dot( ( position[0] - position[2] ), normal[2] ) * normal[2] ) ) / 3.0f;
		f3B201 = ( ( 2.0f * position[0] ) + position[2] - ( dot( ( position[2] - position[0] ), normal[0] ) * normal[0] ) ) / 3.0f;
		// center control point
		vec3 f3E = ( f3B210 + f3B120 + f3B021 + f3B012 + f3B102 + f3B201 ) / 6.0f;
		vec3 f3V = ( indata[0].Position + indata[1].Position + indata[2].Position ) / 3.0f;
		f3B111 = f3E + ( ( f3E - f3V ) / 2.0f );
#endif

		gl_TessLevelOuter[0] = EdgeTessFactors.x;
		gl_TessLevelOuter[1] = EdgeTessFactors.y;
		gl_TessLevelOuter[2] = EdgeTessFactors.z;
		gl_TessLevelInner[0] = (gl_TessLevelOuter[0] + gl_TessLevelOuter[1] + gl_TessLevelOuter[2]) / 3;
	}
}

//------------------------------------------------------------------------------
/**
*/
[inputvertices] = 6
[winding] = ccw
[topology] = triangle
[partition] = odd
shader
void
dsDefault(
	in vec3 tangent[],
	in vec3 normal[],
	in vec4 position[],
	in vec3 binormal[],
	in vec2 uv[],
	out vec3 Tangent,
	out vec3 Normal,
	out vec3 Binormal,
	out vec2 UV
#ifdef PN_TRIANGLES
	,
	patch in vec3 f3B210,
	patch in vec3 f3B120,
	patch in vec3 f3B021,
	patch in vec3 f3B012,
	patch in vec3 f3B102,
	patch in vec3 f3B201,
	patch in vec3 f3B111
#endif
	)
{
	// The barycentric coordinates
	float fU = gl_TessCoord.z;
	float fV = gl_TessCoord.x;
	float fW = gl_TessCoord.y;

	// Precompute squares and squares * 3
	float fUU = fU * fU;
	float fVV = fV * fV;
	float fWW = fW * fW;
	float fUU3 = fUU * 3.0f;
	float fVV3 = fVV * 3.0f;
	float fWW3 = fWW * 3.0f;

#ifdef PN_TRIANGLES
	// Compute position from cubic control points and barycentric coords
	vec3 Position = position[0] * fWW * fW + position[1] * fUU * fU + position[2] * fVV * fV +
					  f3B210 * fWW3 * fU + f3B120 * fW * fUU3 + f3B201 * fWW3 * fV + f3B021 * fUU3 * fV +
					  f3B102 * fW * fVV3 + f3B012 * fU * fVV3 + f3B111 * 6.0f * fW * fU * fV;
#else
	vec3 Position = gl_TessCoord.x * position[0].xyz + gl_TessCoord.y * position[1].xyz + gl_TessCoord.z * position[2].xyz;
#endif

	UV = gl_TessCoord.x * uv[0] + gl_TessCoord.y * uv[1] + gl_TessCoord.z * uv[2];
	float Height = 2.0f * texture(sampler2D(Textures2D[DisplacementMap], MaterialSampler), UV).r - 1.0f;
	vec3 VectorNormalized = normalize( Normal );

	Position += VectorNormalized.xyz * HeightScale * SceneScale * Height;

	gl_Position = vec4(Position, 1);
	gl_Position = ViewProjection * gl_Position;

	Normal = gl_TessCoord.x * normal[0] + gl_TessCoord.y * normal[1] + gl_TessCoord.z * normal[2];
	Binormal = gl_TessCoord.x * binormal[0] + gl_TessCoord.y * binormal[1] + gl_TessCoord.z * binormal[2];
	Tangent = gl_TessCoord.x * tangent[0] + gl_TessCoord.y * tangent[1] + gl_TessCoord.z * tangent[2];
}

//------------------------------------------------------------------------------
/**
	Ubershader for standard geometry
*/
[earlydepth]
shader
void
psUber(
	in vec3 Tangent,
	in vec3 Normal,
	in vec3 Binormal,
	in vec2 UV,
	in vec3 WorldViewVec,
	[color0] out vec4 Albedo,
	[color1] out vec3 Normals,
	[color2] out vec4 Specular,
	[color3] out vec4 Emissive)
{
	vec4 diffColor = 	sample2D(AlbedoMap, MaterialSampler, UV) * MatAlbedoIntensity;
	float roughness =	sample2D(RoughnessMap, MaterialSampler, UV).r * MatRoughnessIntensity;
	vec4 specColor =	sample2D(SpecularMap, MaterialSampler, UV) * MatSpecularIntensity;
	float cavity = 		sample2D(CavityMap, MaterialSampler, UV).r;

	vec4 normals = 		sample2D(NormalMap, NormalSampler, UV);
	vec3 bumpNormal = normalize(calcBump(Tangent, Binormal, Normal, normals));

	mat2x3 env = calcEnv(specColor, bumpNormal, WorldViewVec, roughness);
	vec4 spec = calcSpec(specColor.rgb, roughness);
	vec4 albedo = calcColor(diffColor, vec4(1), spec);
	vec4 emissive = vec4((env[0] * albedo.rgb + env[1]) * cavity, -1);

	Specular = spec;
	Albedo = albedo;
	Emissive = emissive;
	Normals = bumpNormal;
}

//------------------------------------------------------------------------------
/**
	Ubershader for standard geometry.
	Tests for alpha clipping
*/
shader
void
psUberAlphaTest(
	in vec3 Tangent,
	in vec3 Normal,
	in vec3 Binormal,
	in vec2 UV,
	in vec3 WorldViewVec,
	[color0] out vec4 Albedo,
	[color1] out vec3 Normals,
	[color2] out vec4 Specular,
	[color3] out vec4 Emissive)
{
	vec4 diffColor = 	sample2D(AlbedoMap, MaterialSampler, UV) * MatAlbedoIntensity;
	if (diffColor.a < AlphaSensitivity) { discard; return; }
	float roughness = 	sample2D(RoughnessMap, MaterialSampler, UV).r * MatRoughnessIntensity;
	vec4 specColor = 	sample2D(SpecularMap, MaterialSampler, UV) * MatSpecularIntensity;
	float cavity = 		sample2D(CavityMap, MaterialSampler, UV).r;

	vec4 normals = 		sample2D(NormalMap, NormalSampler, UV);
	vec3 bumpNormal = calcBump(Tangent, Binormal, Normal, normals);

	mat2x3 env = calcEnv(specColor, bumpNormal, WorldViewVec, roughness);
	vec4 spec = calcSpec(specColor.rgb, roughness);
	vec4 albedo = calcColor(diffColor, vec4(1), spec);
	vec4 emissive = vec4((env[0] * albedo.rgb + env[1]) * cavity, -1);

	Specular = spec;
	Albedo = albedo;
	Emissive = emissive;
	Normals = bumpNormal;
}

//------------------------------------------------------------------------------
/**
	Ubershader for standard geometry
*/
shader
void
psUberVertexColor(
	in vec3 Tangent,
	in vec3 Normal,
	in vec3 Binormal,
	in vec2 UV,
	in vec4 Color,
	in vec3 WorldViewVec,
	[color0] out vec4 Albedo,
	[color1] out vec3 Normals,
	[color2] out vec4 Specular,
	[color3] out vec4 Emissive)
{
	vec4 diffColor = 	sample2D(AlbedoMap, MaterialSampler, UV) * MatAlbedoIntensity * Color;
	float roughness = 	sample2D(RoughnessMap, MaterialSampler, UV).r * MatRoughnessIntensity;
	vec4 specColor = 	sample2D(SpecularMap, MaterialSampler, UV) * MatSpecularIntensity;
	float cavity = 		sample2D(CavityMap, MaterialSampler, UV).r;

	vec4 normals = 		sample2D(NormalMap, NormalSampler, UV);
	vec3 bumpNormal = calcBump(Tangent, Binormal, Normal, normals);

	mat2x3 env = calcEnv(specColor, bumpNormal, WorldViewVec, roughness);
	vec4 spec = calcSpec(specColor.rgb, roughness);
	vec4 albedo = calcColor(diffColor, Color, spec);
	vec4 emissive = vec4((env[0] * albedo.rgb + env[1]) * cavity, -1);

	Specular = spec;
	Albedo = albedo;
	Emissive = emissive;
	Normals = bumpNormal;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psDefault(in vec3 ViewSpacePos,
	in vec3 Tangent,
	in vec3 Normal,
	in vec3 Binormal,
	in vec2 UV,
	[color0] out vec4 Albedo,
	[color1] out vec3 Normals,
	[color2] out vec4 Specular,
	[color3] out vec4 Emissive)
{
	vec4 diffColor = 	sample2D(AlbedoMap, MaterialSampler, UV);
	if (diffColor.a < AlphaSensitivity) { discard; return; }
	vec4 emsvColor = 	sample2D(EmissiveMap, MaterialSampler, UV);
	vec4 specColor = 	sample2D(SpecularMap, MaterialSampler, UV);
	float roughness = 	sample2D(RoughnessMap, MaterialSampler, UV).r;

	Specular = vec4(specColor.rgb * MatSpecularIntensity.rgb, roughness);
	Albedo = diffColor;
	Emissive = emsvColor;
	mat3 tangentViewMatrix = mat3(normalize(Tangent), normalize(Binormal), normalize(Normal));
	vec3 tNormal = vec3(0,0,0);
	tNormal.xy = (sample2D(NormalMap, NormalSampler, UV).ag * 2.0f) - 1.0f;
	tNormal.z = saturate(sqrt(1.0f - dot(tNormal.xy, tNormal.xy)));

	Normals = tangentViewMatrix * tNormal;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psTranslucent(in vec3 ViewSpacePos,
	in vec3 Tangent,
	in vec3 Normal,
	in vec3 Binormal,
	in vec2 UV,
	[color0] out vec4 Albedo,
	[color1] out vec3 Normals,
	[color2] out vec4 Specular)
{
	vec4 diffColor = 	sample2D(AlbedoMap, MaterialSampler, UV);
	if (diffColor.a < AlphaSensitivity) { discard; return; }
	vec4 emsvColor = 	sample2D(EmissiveMap, MaterialSampler, UV);
	vec4 specColor = 	sample2D(SpecularMap, MaterialSampler, UV);
	float roughness = 	sample2D(RoughnessMap, MaterialSampler, UV).r;

	Specular = vec4(specColor.rgb * MatSpecularIntensity, roughness);
	Albedo = diffColor;

	vec3 normal = Normal;
	mat3 tangentViewMatrix = mat3(normalize(Tangent), normalize(Binormal), normalize(normal));
	vec3 tNormal = vec3(0,0,0);
	tNormal.xy = (sample2D(NormalMap, NormalSampler, UV).ag * 2.0f) - 1.0f;

	// fake z to look at camera, gives a semi-decent translucent effect
	tNormal.z = 1;
	tNormal = normalize(tNormal);

	Normals = tangentViewMatrix * tNormal;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psAlpha(in vec3 ViewSpacePos,
	in vec3 Tangent,
	in vec3 Normal,
	in vec3 Binormal,
	in vec2 UV,
	[color0] out vec4 Albedo,
	[color1] out vec3 Normals,
	[color2] out vec4 Specular)
{
	vec4 diffColor = 	sample2D(AlbedoMap, MaterialSampler, UV);
	if (diffColor.a < AlphaSensitivity) { discard; return; }
	vec4 emsvColor = 	sample2D(EmissiveMap, MaterialSampler, UV);
	vec4 specColor = 	sample2D(SpecularMap, MaterialSampler, UV);
	float roughness = 	sample2D(RoughnessMap, MaterialSampler, UV).r;

	Specular = vec4(specColor.rgb * MatSpecularIntensity, roughness);
	Albedo = vec4(diffColor.rgb, diffColor.a * AlphaBlendFactor);
	mat3 tangentViewMatrix = mat3(normalize(Tangent.xyz), normalize(Binormal.xyz), normalize(Normal.xyz));
	vec3 tNormal = vec3(0,0,0);
	tNormal.xy = (sample2D(NormalMap, NormalSampler, UV).ag * 2.0f) - 1.0f;
	tNormal.z = saturate(sqrt(1.0f - dot(tNormal.xy, tNormal.xy)));

	Normals = tangentViewMatrix * tNormal;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psEnvironment(in vec3 ViewSpacePos,
	in vec3 Tangent,
	in vec3 Normal,
	in vec3 Binormal,
	in vec2 UV,
	[color0] out vec4 Albedo,
	[color1] out vec3 Normals,
	[color2] out vec4 Specular)
{
	vec4 diffColor = 	sample2D(AlbedoMap, MaterialSampler, UV);
	if (diffColor.a < AlphaSensitivity) { discard; return; }
	vec4 emsvColor = 	sample2D(EmissiveMap, MaterialSampler, UV);
	vec4 specColor = 	sample2D(SpecularMap, MaterialSampler, UV);
	float roughness = 	sample2D(RoughnessMap, MaterialSampler, UV).r;

	mat3 tangentViewMatrix = mat3(normalize(Tangent.xyz), normalize(Binormal.xyz), normalize(Normal.xyz));
	vec3 tNormal = vec3(0,0,0);
	tNormal.xy = (sample2D(NormalMap, NormalSampler, UV).ag * 2.0f) - 1.0f;
	tNormal.z = saturate(sqrt(1.0f - dot(tNormal.xy, tNormal.xy)));

	vec3 viewSpaceNormal = (tangentViewMatrix * tNormal).xyz;
	vec3 worldViewVec = normalize(EyePos.xyz - (InvView * vec4(ViewSpacePos, 1)).xyz);
	vec3 reflectVec = reflect(-worldViewVec, (InvView * vec4(viewSpaceNormal, 0)).xyz);
	float x = dot(viewSpaceNormal, normalize(ViewSpacePos.xyz));
	vec4 rim = vec4(specColor.rgb + (1 - specColor.rgb) * (pow((1 - x), 5) / (4 - 3 * roughness)), 1) * 1000;
	vec4 envColor = sampleCubeLod(EnvironmentMap, CubeSampler, reflectVec, (1 - roughness) * 9) * saturate(rim);

	Specular = vec4(specColor.rgb * MatSpecularIntensity * envColor.rgb, roughness);
	Albedo = diffColor * lerp(vec4(1), envColor, roughness);
	Normals = viewSpaceNormal;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psVertexColor(in vec3 ViewSpacePos,
	in vec3 Tangent,
	in vec3 Normal,
	in vec3 Binormal,
	in vec2 UV,
	in vec4 Color,
	[color0] out vec4 Albedo,
	[color1] out vec3 Normals,
	[color2] out vec4 Specular)
{
	vec4 diffColor = 	sample2D(AlbedoMap, MaterialSampler, UV);
	vec4 emsvColor = 	sample2D(EmissiveMap, MaterialSampler, UV);
	vec4 specColor = 	sample2D(SpecularMap, MaterialSampler, UV);
	float roughness = 	sample2D(RoughnessMap, MaterialSampler, UV).r;

	mat3 tangentViewMatrix = mat3(normalize(Tangent.xyz), normalize(Binormal.xyz), normalize(Normal.xyz));
	vec3 tNormal = vec3(0,0,0);
	tNormal.xy = (sample2D(NormalMap, NormalSampler, UV).ag * 2.0f) - 1.0f;
	tNormal.z = saturate(sqrt(1.0f - dot(tNormal.xy, tNormal.xy)));
	Normals = tangentViewMatrix * tNormal;

	vec3 worldViewVec = normalize(EyePos.xyz - (InvView * vec4(ViewSpacePos, 1)).xyz);
	vec3 reflectVec = reflect(-worldViewVec, (InvView * vec4(tNormal, 0)).xyz);
	float x = dot(tNormal, normalize(ViewSpacePos.xyz));
	vec3 rim = specColor.rgb + (1 - specColor.rgb) * (pow((1 - x), 5) / (4 - 3 * roughness));
	vec3 environment = sampleCubeLod(EnvironmentMap, CubeSampler, reflectVec, (1 - roughness) * 9).rgb * saturate(rim);

	Specular = vec4(specColor.rgb * MatSpecularIntensity * environment, roughness);
	Albedo = vec4(diffColor.rgb * lerp(vec3(1), environment, roughness), diffColor.a);
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psBillboard(in vec2 UV,
			[color0] out vec4 Albedo)
{
	// get diffcolor
	vec4 diffColor = sample2D(AlbedoMap, MaterialSampler, UV);

	Albedo = diffColor;
}

#endif
