//------------------------------------------------------------------------------
//  water.fx
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "lib/shared.fxh"
#include "lib/util.fxh"

Texture2D NormalMap;
Texture2D ColorMap;
Texture2D FoamMap;
Texture2D DepthMap;
Texture2D DisplacementMap;
TextureCube ReflectionMap;

bool CheapReflections = false;
float ReflectionIntensity = {0.0f};
float4 GlobalLightDir;

cbuffer WaterVars
{
	float WaveSpeed = 0.0f;
	float4 WaterColor = {0.0f, 0.0f, 0.0f, 0.0f};
	float DeepDistance = 0.0f;
	float FoamDistance = 0.0f;
	float FoamIntensity = 0.0f;
	float FogDistance = 0.0f;
	float4 FogColor = {0.5, 0.5, 0.63, 0.0};
}


SamplerState DefaultSampler;
SamplerState WaveSampler
{
	AddressU = Wrap;
	AddressV = Wrap;
	AddressW = Wrap;
};

BlendState Blend 
{
	BlendEnable[1] = TRUE;
	SrcBlend[1] = 2;
	DestBlend[1] = 5;
};

RasterizerState Rasterizer
{
	CullMode = 1;
};

DepthStencilState DepthStencil
{
};

//------------------------------------------------------------------------------
/**
*/
void
vsDefault(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD0,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	out float3 ViewSpacePos : TEXCOORD0,
	out float3 Tangent : TANGENT0,
	out float3 Normal : NORMAL0,
	out float4 Position : SV_POSITION0,
	out float4 ProjPos : PROJPOS,
	out float3 Binormal : BINORMAL0,
	out float2 UV : TEXCOORD1,
	out float4 UV1 : TEXCOORD2,
	out float4 UV2 : TEXCOORD3,
	out float3 EyeVec : EYEVEC) 
{
	Position = mul(position, mul(Model, ViewProjection));
	ProjPos = Position;
	float3 worldPos = mul(position, Model).xyz;
	float4x4 modelView = mul(Model, View);
	ViewSpacePos = mul(position, modelView).xyz;    
	
	float2 uvTranslation = float2(fmod(Time, 100.0f) * 0.01f, 0);
	float2 texCoords = worldPos.xz * 0.001f;
	
	UV1.xy = texCoords.xy + uvTranslation * 2.0f * WaveSpeed;
	UV1.zw = texCoords.xy * 2.0 + uvTranslation * 4.0 * WaveSpeed;
	UV2.xy = texCoords.xy * 4.0 + uvTranslation * 2.0 * WaveSpeed;
	UV2.zw = texCoords.xy * 8.0 + uvTranslation;      
	UV = texCoords;
	
	Tangent  = mul(tangent, modelView).xyz;
	Normal   = mul(normal, modelView).xyz;
	Binormal = mul(binormal, modelView).xyz;
		
	EyeVec = EyePos - worldPos;        
}

//------------------------------------------------------------------------------
/**
*/
void
vsTessellated(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD0,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	out float3 Tangent : TANGENT,
	out float3 Normal : NORMAL,
	out float4 Position : POSITION,
	out float3 Binormal : BINORMAL,
	out float Distance : CAMERADIST) 
{
	Position = mul(position, Model);	  
	
	Tangent  = mul(tangent, Model).xyz;
	Normal   = mul(normal, Model).xyz;
	Binormal = mul(binormal, Model).xyz;
	
	float vertexDistance = distance( Position.xyz, EyePos.xyz );
	Distance = 1.0 - clamp( ( (vertexDistance - MinDistance) / (MaxDistance - MinDistance) ), 0.0, 1.0 - 1.0/TessellationFactor);
}

//------------------------------------------------------------------------------
/**
*/
void
psDefault(in float3 ViewSpacePos : TEXCOORD0,
	in float3 Tangent : TANGENT,
	in float3 Normal : NORMAL,
	in float4 Position : SV_POSITION,
	in float4 ProjPos : PROJPOS,
	in float3 Binormal : BINORMAL,
	in float2 UV : TEXCOORD1,
	in float4 UV1 : TEXCOORD2,
	in float4 UV2 : TEXCOORD3,
	in float3 EyeVec : EYEVEC,
	out float4 Albedo : SV_TARGET0,
	out float4 Unshaded : SV_TARGET1) 
{   	
	float2 bumpTexA = 2.0f * NormalMap.Sample(WaveSampler, UV1.xy).ag - 1.0f;
	float2 bumpTexB = 2.0f * NormalMap.Sample(WaveSampler, UV1.zw).ag - 1.0f;
	float2 bumpTexC = 2.0f * NormalMap.Sample(WaveSampler, UV2.xy).ag - 1.0f;
	float2 bumpTexD = 2.0f * NormalMap.Sample(WaveSampler, UV2.zw).ag - 1.0f;
	
	// get normal
	float3 tNormal = float3(0,0,0);
	tNormal.xy = (bumpTexA + bumpTexB + bumpTexC + bumpTexD) * 0.25f;
	tNormal.z = saturate(sqrt(1.0 - dot(tNormal.xy, tNormal.xy)));
	
	float3x3 tangentViewMatrix = float3x3(normalize(Tangent.xyz), normalize(Binormal.xyz), normalize(Normal.xyz));        
	float3 worldSpaceNormal = mul(tNormal, tangentViewMatrix);
	
	// calculate refraction
	float2 normalMapPixelSize = GetPixelSize(ColorMap);
	float2 screenUv = psComputeScreenCoord(Position.xy, normalMapPixelSize.xy);
	
	float3 refractiveBump = tNormal * float3(0.02f, 0.02f, 1.0f);
	float3 reflectiveBump = tNormal * float3(0.1f, 0.1f, 1.0f);
	
	// sample directly, and at offset
	float depth = DepthMap.Sample(DefaultSampler, screenUv.xy + reflectiveBump.xy).r;
	float4 refractionA = ColorMap.Sample(DefaultSampler, screenUv.xy + refractiveBump.xy);
	float4 refractionB = ColorMap.Sample(DefaultSampler, screenUv.xy);
	float4 refraction = refractionA;// * refractionA.w + refractionA * (1 - refractionA.w);
	
	float3 eyeVec = normalize(EyeVec);
	
	// cheat, here we should use the reflection map but instead we just use the same color map...
	float4 reflection;

	float3 reflectionSample = reflect(eyeVec, normalize(tNormal));
	if (CheapReflections)
	{
		reflection = ReflectionMap.Sample(DefaultSampler, reflectionSample) * ReflectionIntensity;
	}
	else
	{
		reflection = ColorMap.Sample(DefaultSampler, screenUv.xy + reflectiveBump.xy);
	}
	
	float3 H = normalize(GlobalLightDir.xyz - normalize(ViewSpacePos));
	float NH = saturate(dot(worldSpaceNormal, H));
	float NE = dot(eyeVec, normalize(worldSpaceNormal));
	float specular = pow(NH, 128);

	float fresnel = pow(abs(1.0f - abs(dot(normalize(ViewSpacePos), worldSpaceNormal))), 1.0f/5.0f);
	
	float pixelDepth = length(ViewSpacePos.xyz);
	float3 foamA = FoamMap.Sample(WaveSampler, UV1.xy).rgb;
	float3 foamB = FoamMap.Sample(WaveSampler, UV1.zw).rgb;
	float3 foamC = FoamMap.Sample(WaveSampler, UV2.xy).rgb;
	float3 foamD = FoamMap.Sample(WaveSampler, UV2.zw).rgb;
	float3 foamColor = (foamA + foamB + foamC + foamD) * 0.25;
	float waterDepth = DepthMap.Sample(DefaultSampler, screenUv.xy).r;
	float foamFactor = saturate(FoamDistance / (waterDepth - pixelDepth));

	float deepFactor = saturate(DeepDistance / (waterDepth - pixelDepth));
	float3 waterColor = lerp(WaterColor, refraction, deepFactor);
	float3 reflectionConstant = fresnel * reflection;
	
	float3 foam = foamColor * FoamIntensity * foamFactor;	
	waterColor += foam;
	float3 spec = float3(1,1,1) * specular;
	
	float4 finalColor = float4(waterColor + spec + reflectionConstant, 1);
	
	float fogFactor = saturate(FogDistance / max(depth - pixelDepth, 0));
	finalColor.rgb = lerp(FogColor.rgb, finalColor.rgb, fogFactor);
	
	// blend refraction values
	Albedo = EncodeHDR(finalColor);
	Unshaded = float4(0,0,0, NE + 0.5f);
}


//------------------------------------------------------------------------------
/**
*/
#define POINTS 3
struct HS_INPUT
{	
	float3 Tangent : TANGENT;
	float3 Normal : NORMAL;
	float4 Position : POSITION;
	float3 Binormal : BINORMAL;
	float Distance : CAMERADIST;
};

struct HS_OUTPUT
{
	float3 Tangent : TANGENT;
	float3 Normal : NORMAL;
	float4 Position : POSITION;
	float3 Binormal : BINORMAL;
};

struct HS_CONSTANT_OUTPUT
{
	float Edges[3] : SV_TessFactor;
	float Inside : SV_InsideTessFactor;
};

//------------------------------------------------------------------------------
/**
*/
HS_CONSTANT_OUTPUT 
HSPatchFunc(InputPatch<HS_INPUT, POINTS> patch, uint patchID : SV_PrimitiveID)
{
	HS_CONSTANT_OUTPUT output;
	float4 EdgeTessFactors;
	EdgeTessFactors.x = 0.5 * (patch[1].Distance + patch[2].Distance);
	EdgeTessFactors.y = 0.5 * (patch[2].Distance + patch[0].Distance);
	EdgeTessFactors.z = 0.5 * (patch[0].Distance + patch[1].Distance);
	EdgeTessFactors *= TessellationFactor;

	output.Edges[0] = EdgeTessFactors.x;
	output.Edges[1] = EdgeTessFactors.y;
	output.Edges[2] = EdgeTessFactors.z;
	output.Inside = (EdgeTessFactors.x + EdgeTessFactors.y + EdgeTessFactors.z) / 3;
	return output;
}

//------------------------------------------------------------------------------
/**
*/
[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[maxtessfactor(32)]
[patchconstantfunc("HSPatchFunc")]
HS_OUTPUT 
hsDefault(InputPatch<HS_INPUT, POINTS> patch, uint i : SV_OutputControlPointID)
{
	HS_OUTPUT output;
	output.Tangent = patch[i].Tangent;
	output.Normal = patch[i].Normal;
	output.Position = patch[i].Position;
	output.Binormal = patch[i].Binormal;
	return output;
}

//------------------------------------------------------------------------------
/**
*/
struct DS_OUTPUT
{
	float3 ViewSpacePos : TEXCOORD0;
	float3 Tangent : TANGENT;
	float3 Normal : NORMAL;
	float4 Position : SV_POSITION;
	float4 ProjPos : PROJPOS;
	float3 Binormal : BINORMAL;
	float2 UV : TEXCOORD1;
	float4 UV1 : TEXCOORD2;
	float4 UV2 : TEXCOORD3;
	float3 EyeVec : EYEVEC;
};

//------------------------------------------------------------------------------
/**
*/
[domain("tri")]
DS_OUTPUT
dsDefault( HS_CONSTANT_OUTPUT input, float3 Coordinate : SV_DomainLocation, const OutputPatch<HS_OUTPUT, POINTS> Patch)
{
	DS_OUTPUT output;

	float4 Position = Coordinate.x * Patch[0].Position + Coordinate.y * Patch[1].Position + Coordinate.z * Patch[2].Position;
	
	float2 uvTranslation = float2(fmod(Time, 100.0f) * 0.01f, 0);
	float2 texCoords = Position.xz * 0.001f;
	
	float4 UV1, UV2;
	UV1.xy = texCoords.xy + uvTranslation * 2.0 * WaveSpeed;
	UV1.zw = texCoords.xy * 2.0 + uvTranslation * 4.0 * WaveSpeed;
	UV2.xy = texCoords.xy * 4.0 + uvTranslation * 2.0 * WaveSpeed;
	UV2.zw = texCoords.xy * 8.0 + uvTranslation;      	
	
	output.UV1 = UV1;
	output.UV2 = UV2;
	
	float heightA = 2.0f * DisplacementMap.SampleLevel(WaveSampler, UV1.xy, 0).x - 1.0f;
	float heightB = 2.0f * DisplacementMap.SampleLevel(WaveSampler, UV1.zw, 0).x - 1.0f;
	float heightC = 2.0f * DisplacementMap.SampleLevel(WaveSampler, UV2.xy, 0).x - 1.0f;
	float heightD = 2.0f * DisplacementMap.SampleLevel(WaveSampler, UV2.zw, 0).x - 1.0f;
	
	float Height = (heightA + heightB + heightC + heightD) * 0.25f;
	output.Normal = Coordinate.x * Patch[0].Normal + Coordinate.y * Patch[1].Normal + Coordinate.z * Patch[2].Normal;
	float3 VectorNormalized = normalize( output.Normal );
	
	Position.xyz += VectorNormalized.xyz * HeightScale * SceneScale * Height;
	output.EyeVec = EyePos - Position.xyz;      
	
	output.Position = float4(Position.xyz, 1);
	output.ViewSpacePos = mul(output.Position, View).xyz;	
	output.Position = mul(output.Position, ViewProjection);	
	output.ProjPos = output.Position;
	
	output.Normal = mul(output.Normal, View);
	output.Binormal = Coordinate.x * Patch[0].Binormal + Coordinate.y * Patch[1].Binormal + Coordinate.z * Patch[2].Binormal;
	output.Binormal = mul(output.Binormal, View);
	output.Tangent = Coordinate.x * Patch[0].Tangent + Coordinate.y * Patch[1].Tangent + Coordinate.z * Patch[2].Tangent;
	output.Tangent = mul(output.Tangent, View);
	
	output.UV = texCoords;

	return output;
}

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(Default, "Static", vsDefault(), psDefault(), Blend, DepthStencil, Rasterizer)
TessellationTechnique(Tessellated, "Static|Tessellated", vsTessellated(), psDefault(), hsDefault(), dsDefault(), Blend, DepthStencil, Rasterizer)
