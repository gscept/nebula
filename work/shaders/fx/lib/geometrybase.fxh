//------------------------------------------------------------------------------
//  geometrybase.fxh
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "lib/skinning.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"

//#define PN_TRIANGLES

// textures
Texture2D SpecularMap;
Texture2D EmissiveMap;
Texture2D NormalMap;
Texture2D DiffuseMap;
Texture2D DisplacementMap;
TextureCube EnvironmentMap;

// simple sampler
SamplerState DefaultSampler
{
	AddressU = Wrap;
	AddressV = Wrap;
	AddressW = Wrap;
	Filter = MIN_MAG_MIP_LINEAR;
};

BlendState SolidBlend 
{
};

// useful blending option for foliage
BlendState AlphaTestBlend
{
	//AlphaToCoverageEnable = TRUE;
};

// we use the same rasterizer for both states
RasterizerState Rasterizer
{
	CullMode = 2;
};

RasterizerState NoCull
{
	CullMode = 1;
};


DepthStencilState SolidDepthStencil
{
};

BlendState AlphaBlend 
{
	BlendEnable[2] = TRUE;
	BlendEnable[3] = TRUE;
	BlendEnable[4] = TRUE;
	SrcBlend[2] = 5;
	DestBlend[2] = 6;
	SrcBlend[3] = 2;
	DestBlend[3] = 2;
	SrcBlend[4] = 2;
	DestBlend[4] = 2;
};

DepthStencilState AlphaDepthStencil
{
	DepthWriteMask = 0;
};

//------------------------------------------------------------------------------
/**
				STATIC GEOMETRY
*/
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
*/
void
vsStatic(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	out float3 ViewSpacePos : TEXCOORD0,
	out float3 Tangent : TANGENT0,
	out float3 Normal : NORMAL0,
	out float4 Position : SV_POSITION0,
	out float3 Binormal : BINORMAL0,
	out float2 UV : TEXCOORD1) 
{
    Position = mul(position, mul(Model, ViewProjection));
    UV = uv;
    float4x4 modelView = mul(Model, View);
    ViewSpacePos = mul(position, modelView).xyz;    
    
    Tangent  = mul(tangent, modelView).xyz;
    Normal   = mul(normal, modelView).xyz;
    Binormal = mul(binormal, modelView).xyz;
}

//------------------------------------------------------------------------------
/**
*/
void
vsStaticInstanced(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	uint ID : SV_InstanceID,
	out float3 ViewSpacePos : TEXCOORD0,
	out float3 Tangent : TANGENT0,
	out float3 Normal : NORMAL0,
	out float4 Position : SV_POSITION0,
	out float3 Binormal : BINORMAL0,
	out float2 UV : TEXCOORD1) 
{
    Position = mul(position, mul(ModelArray[ID], ViewProjection));
    UV = uv;
	
    ViewSpacePos = mul(position, mul(ModelArray[ID], View)).xyz;
    Tangent  = mul(tangent, mul(ModelArray[ID], View)).xyz;
    Normal   = mul(normal, mul(ModelArray[ID], View)).xyz;
    Binormal = mul(binormal, mul(ModelArray[ID], View)).xyz;
}

//------------------------------------------------------------------------------
/**
*/
void
vsStaticTessellated(float4 position : POSITION0,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	out float3 Tangent : TANGENT0,
	out float3 Normal : NORMAL0,
	out float4 Position : POSITION0,
	out float3 Binormal : BINORMAL0,
	out float2 UV : TEXCOORD0,
	out float Distance : CAMERADIST0) 
{
    Position = mul(position, Model);
    UV = uv;    
    
    Tangent  = mul(tangent, Model).xyz;
    Normal   = mul(normal, Model).xyz;
    Binormal = mul(binormal, Model).xyz;
	
	float vertexDistance = distance( Position.xyz, EyePos.xyz );
	Distance = 1.0 - clamp( ( (vertexDistance - MinDistance) / (MaxDistance - MinDistance) ), 0.0, 1.0 - 1.0/TessellationFactor);
}



//------------------------------------------------------------------------------
/**
				SKINNED GEOMETRY
*/
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
*/
void
vsSkinned(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	float4 weights : BLENDWEIGHT,
	uint4 indices : BLENDINDICES,
	out float3 ViewSpacePos : TEXCOORD0,
	out float3 Tangent : TANGENT0,
	out float3 Normal : NORMAL0,
	out float4 Position : SV_POSITION0,
	out float3 Binormal : BINORMAL0,	
	out float2 UV : TEXCOORD1)	
{	
	float4 skinnedPos      = SkinnedPosition(position, weights, indices);
	float3 skinnedNormal   = SkinnedNormal(normal, weights, indices);
	float3 skinnedTangent  = SkinnedNormal(tangent, weights, indices);
	float3 skinnedBinormal = SkinnedNormal(binormal, weights, indices);
	Position = mul(skinnedPos, mul(Model, ViewProjection));
	UV = uv;
	
	float4x4 modelView = mul(Model, View);
	ViewSpacePos = mul(skinnedPos, modelView).xyz;
	
	
	Tangent  = mul(skinnedTangent, modelView).xyz;
	Normal   = mul(skinnedNormal, modelView).xyz;
	Binormal = mul(skinnedBinormal, modelView).xyz;
}

//------------------------------------------------------------------------------
/**
*/
void
vsSkinnedTessellated(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	float4 weights : BLENDWEIGHT,
	uint4 indices : BLENDINDICES,
	out float3 Tangent : TANGENT0,
	out float3 Normal : NORMAL0,
	out float4 Position : POSITION0,
	out float3 Binormal : BINORMAL0,
	out float2 UV : TEXCOORD0,
	out float Distance : CAMERADIST0) 
{
	float4 skinnedPos      = SkinnedPosition(position, weights, indices);
	float3 skinnedNormal   = SkinnedNormal(normal, weights, indices);
	float3 skinnedTangent  = SkinnedNormal(tangent, weights, indices);
	float3 skinnedBinormal = SkinnedNormal(binormal, weights, indices);
	
	Position = mul(skinnedPos, Model);
	UV = uv;    
	
	Tangent  = mul(skinnedTangent, Model).xyz;
	Normal   = mul(skinnedNormal, Model).xyz;
	Binormal = mul(skinnedBinormal, Model).xyz;
	
	float vertexDistance = distance( Position.xyz, EyePos.xyz );
	Distance = 1.0 - clamp( ( (vertexDistance - MinDistance) / (MaxDistance - MinDistance) ), 0.0, 1.0 - 1.0/TessellationFactor);
}

//------------------------------------------------------------------------------
/**
*/
void
vsBillboard(float4 position : POSITION0,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	out float4 Position : SV_POSITION0,
	out float2 UV : TEXCOORD0)
{
	float4 newPos = float4(position.x, position.y, 0, 1);
	Position = mul(float4(position.xyz, 0), mul(Model, ViewProjection));
	UV = uv;
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
	float2 UV : TEXCOORD;	
	float Distance : CAMERADIST;
};

struct HS_OUTPUT
{
	float3 Tangent : TANGENT;
	float3 Normal : NORMAL;
	float4 Position : POSITION;
	float3 Binormal : BINORMAL;
	float2 UV : TEXCOORD0;
};

struct HS_CONSTANT_OUTPUT
{
	float Edges[3] : SV_TessFactor;
	float Inside : SV_InsideTessFactor;

#ifdef PN_TRIANGLES
	// Geometry cubic generated control points
    float3 f3B210   : POSITION3;
    float3 f3B120   : POSITION4;
    float3 f3B021   : POSITION5;
    float3 f3B012   : POSITION6;
    float3 f3B102   : POSITION7;
    float3 f3B201   : POSITION8;
    float3 f3B111   : CENTER;
#endif
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
	
#ifdef PN_TRIANGLES
	// compute the cubic geometry control points
    // edge control points
    output.f3B210 = ( ( 2.0f * patch[0].Position ) + patch[1].Position - ( dot( ( patch[1].Position - patch[0].Position ), patch[0].Normal ) * patch[0].Normal ) ) / 3.0f;
    output.f3B120 = ( ( 2.0f * patch[1].Position ) + patch[0].Position - ( dot( ( patch[0].Position - patch[1].Position ), patch[1].Normal ) * patch[1].Normal ) ) / 3.0f;
    output.f3B021 = ( ( 2.0f * patch[1].Position ) + patch[2].Position - ( dot( ( patch[2].Position - patch[1].Position ), patch[1].Normal ) * patch[1].Normal ) ) / 3.0f;
    output.f3B012 = ( ( 2.0f * patch[2].Position ) + patch[1].Position - ( dot( ( patch[1].Position - patch[2].Position ), patch[2].Normal ) * patch[2].Normal ) ) / 3.0f;
    output.f3B102 = ( ( 2.0f * patch[2].Position ) + patch[0].Position - ( dot( ( patch[0].Position - patch[2].Position ), patch[2].Normal ) * patch[2].Normal ) ) / 3.0f;
    output.f3B201 = ( ( 2.0f * patch[0].Position ) + patch[2].Position - ( dot( ( patch[2].Position - patch[0].Position ), patch[0].Normal ) * patch[0].Normal ) ) / 3.0f;
    // center control point
    float3 f3E = ( output.f3B210 + output.f3B120 + output.f3B021 + output.f3B012 + output.f3B102 + output.f3B201 ) / 6.0f;
    float3 f3V = ( patch[0].Position + patch[1].Position + patch[2].Position ) / 3.0f;
    output.f3B111 = f3E + ( ( f3E - f3V ) / 2.0f );
#endif

	output.Edges[0] = EdgeTessFactors.x;
	output.Edges[1] = EdgeTessFactors.y;
	output.Edges[2] = EdgeTessFactors.z;
	output.Inside = (output.Edges[0] + output.Edges[1] + output.Edges[2]) / 3;

	return output;
}

//------------------------------------------------------------------------------
/**
*/
[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[maxtessfactor(32.0)]
[patchconstantfunc("HSPatchFunc")]
HS_OUTPUT 
hsDefault(InputPatch<HS_INPUT, POINTS> patch, uint i : SV_OutputControlPointID)
{
	HS_OUTPUT output;
	output.Tangent = patch[i].Tangent;
	output.Normal = patch[i].Normal;
	output.Position = patch[i].Position;
	output.Binormal = patch[i].Binormal;
	output.UV = patch[i].UV;	
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
	float3 Binormal : BINORMAL;
	float2 UV : TEXCOORD1;
};

//------------------------------------------------------------------------------
/**
*/
[domain("tri")]
DS_OUTPUT
dsDefault( HS_CONSTANT_OUTPUT input, float3 Coordinate : SV_DomainLocation, const OutputPatch<HS_OUTPUT, POINTS> Patch)
{
	DS_OUTPUT output;
	
	// The barycentric coordinates
	float fU = Coordinate.z;
	float fV = Coordinate.x;
	float fW = Coordinate.y;
	
	// Precompute squares and squares * 3 
	float fUU = fU * fU;
	float fVV = fV * fV;
	float fWW = fW * fW;
	float fUU3 = fUU * 3.0f;
	float fVV3 = fVV * 3.0f;
	float fWW3 = fWW * 3.0f;
	
#ifdef PN_TRIANGLES
	// Compute position from cubic control points and barycentric coords
	float3 Position = Patch[0].Position * fWW * fW + Patch[1].Position * fUU * fU + Patch[2].Position * fVV * fV +
					  input.f3B210 * fWW3 * fU + input.f3B120 * fW * fUU3 + input.f3B201 * fWW3 * fV + input.f3B021 * fUU3 * fV +
					  input.f3B102 * fW * fVV3 + input.f3B012 * fU * fVV3 + input.f3B111 * 6.0f * fW * fU * fV;
#else
	float3 Position = Coordinate.x * Patch[0].Position + Coordinate.y * Patch[1].Position + Coordinate.z * Patch[2].Position;
#endif
	output.UV = Coordinate.x * Patch[0].UV + Coordinate.y * Patch[1].UV + Coordinate.z * Patch[2].UV;
	float Height = 2.0f * DisplacementMap.SampleLevel(DefaultSampler, output.UV, 0).x - 1.0f;
	output.Normal = Coordinate.x * Patch[0].Normal + Coordinate.y * Patch[1].Normal + Coordinate.z * Patch[2].Normal;
	float3 VectorNormalized = normalize( output.Normal );
	
	Position.xyz += VectorNormalized.xyz * HeightScale * SceneScale * Height;
	
	output.Position = float4(Position.xyz, 1);
	output.ViewSpacePos = mul(output.Position, View).xyz;	
	output.Position = mul(output.Position, ViewProjection);	
	
	output.Normal = mul(output.Normal, View);
	output.Binormal = Coordinate.x * Patch[0].Binormal + Coordinate.y * Patch[1].Binormal + Coordinate.z * Patch[2].Binormal;
	output.Binormal = mul(output.Binormal, View);
	output.Tangent = Coordinate.x * Patch[0].Tangent + Coordinate.y * Patch[1].Tangent + Coordinate.z * Patch[2].Tangent;
	output.Tangent = mul(output.Tangent, View);

	return output;
}

//------------------------------------------------------------------------------
/**
*/
void
psDefault(in float3 ViewSpacePos : TEXCOORD0,
	in float3 Tangent : TANGENT0,
	in float3 Normal : NORMAL0,
	in float4 Position : SV_POSITION0,
	in float3 Binormal : BINORMAL0,
	in float2 UV : TEXCOORD1,
	out float4 Albedo : SV_TARGET0,
	out float4 Normals : SV_TARGET1,
	out float Depth : SV_TARGET2,	
	out float3 Specular : SV_TARGET3,
	out float3 Emissive : SV_TARGET4,
	out float4 Unshaded : SV_TARGET5) 
{
	float4 diffColor = DiffuseMap.Sample(DefaultSampler, UV.xy);
	float4 emsvColor = EmissiveMap.Sample(DefaultSampler, UV.xy);
	float4 specColor = SpecularMap.Sample(DefaultSampler, UV.xy);

	Emissive = emsvColor.rgb * MatEmissiveIntensity;
	Specular = specColor.rgb * MatSpecularIntensity;
	Albedo = diffColor;
	Unshaded = float4(0,0,0,1);
	Depth = length(ViewSpacePos.xyz);
	float3x3 tangentViewMatrix = float3x3(normalize(Tangent.xyz), normalize(Binormal.xyz), normalize(Normal.xyz));        
	float3 tNormal = float3(0,0,0);
	tNormal.xy = (NormalMap.Sample(DefaultSampler, UV).ag * 2.0) - 1.0;
	tNormal.z = saturate(sqrt(1.0 - dot(tNormal.xy, tNormal.xy)));
	
	Normals = PackViewSpaceNormal(mul(tNormal, tangentViewMatrix).xyz);

	float alpha = Albedo.a;
	clip( alpha < AlphaSensitivity ? -1:1 );
}

//------------------------------------------------------------------------------
/**
*/
void
psTransulcent(in float3 ViewSpacePos : TEXCOORD0,
	in float3 Tangent : TANGENT0,
	in float3 Normal : NORMAL0,
	in float4 Position : SV_POSITION0,
	in float3 Binormal : BINORMAL0,
	in float2 UV : TEXCOORD1,
	bool frontFace : SV_IsFrontFace,
	out float4 Albedo : SV_TARGET0,
	out float4 Normals : SV_TARGET1,
	out float Depth : SV_TARGET2,	
	out float3 Specular : SV_TARGET3,
	out float3 Emissive : SV_TARGET4,
	out float4 Unshaded : SV_TARGET5) 
{
	float4 diffColor = DiffuseMap.Sample(DefaultSampler, UV.xy);
	float4 emsvColor = EmissiveMap.Sample(DefaultSampler, UV.xy);
	float4 specColor = SpecularMap.Sample(DefaultSampler, UV.xy);

	Emissive = emsvColor.rgb * MatEmissiveIntensity;
	Specular = specColor.rgb * MatSpecularIntensity;
	Albedo = diffColor;
	Unshaded = float4(0,0,0,1);
	Depth = length(ViewSpacePos.xyz);
	
	float3 normal = Normal;
	if (frontFace) normal = -normal;
	float3x3 tangentViewMatrix = float3x3(normalize(Tangent.xyz), normalize(Binormal.xyz), normalize(normal.xyz));        
	float3 tNormal = float3(0,0,0);
	tNormal.xy = (NormalMap.Sample(DefaultSampler, UV).ag * 2.0) - 1.0;
	tNormal.z = saturate(sqrt(1.0 - dot(tNormal.xy, tNormal.xy)));
	
	Normals = PackViewSpaceNormal(mul(tNormal, tangentViewMatrix).xyz);

	float alpha = Albedo.a;
	clip( alpha < AlphaSensitivity ? -1:1 );
}

//------------------------------------------------------------------------------
/**
*/
void
psAlpha(in float3 ViewSpacePos : TEXCOORD0,
	in float3 Tangent : TANGENT0,
	in float3 Normal : NORMAL0,
	in float4 Position : SV_POSITION0,
	in float3 Binormal : BINORMAL0,
	in float2 UV : TEXCOORD1,
	out float4 Albedo : SV_TARGET0,
	out float4 Normals : SV_TARGET1,
	out float Depth : SV_TARGET2,	
	out float3 Specular : SV_TARGET3,
	out float3 Emissive : SV_TARGET4,
	out float4 Unshaded : SV_TARGET5) 
{
	float4 diffColor = DiffuseMap.Sample(DefaultSampler, UV.xy);
	float4 emsvColor = EmissiveMap.Sample(DefaultSampler, UV.xy);
	float4 specColor = SpecularMap.Sample(DefaultSampler, UV.xy);
	Emissive = emsvColor.rgb * MatEmissiveIntensity;
	Specular = specColor.rgb * MatSpecularIntensity;
	Albedo = diffColor * AlphaBlendFactor;
	Unshaded = float4(0,0,0,1);
	Depth = length(ViewSpacePos.xyz);
	float3x3 tangentViewMatrix = float3x3(normalize(Tangent.xyz), normalize(Binormal.xyz), normalize(Normal.xyz));        
	float3 tNormal = float3(0,0,0);
	tNormal.xy = (NormalMap.Sample(DefaultSampler, UV).ag * 2.0) - 1.0;
	tNormal.z = saturate(sqrt(1.0 - dot(tNormal.xy, tNormal.xy)));
	
	Normals = PackViewSpaceNormal(mul(tNormal, tangentViewMatrix).xyz);

	float alpha = Albedo.a;
	clip( alpha < AlphaSensitivity ? -1:1 );
}

//------------------------------------------------------------------------------
/**
*/
void
psEnvironment(in float3 ViewSpacePos : TEXCOORD0,
	in float3 Tangent : TANGENT0,
	in float3 Normal : NORMAL0,
	in float4 Position : SV_POSITION0,
	in float3 Binormal : BINORMAL0,
	in float2 UV : TEXCOORD1,
	out float4 Albedo : SV_TARGET0,
	out float4 Normals : SV_TARGET1,
	out float Depth : SV_TARGET2,	
	out float3 Specular : SV_TARGET3,
	out float3 Emissive : SV_TARGET4,
	out float4 Unshaded : SV_TARGET5) 
{
	float4 diffColor = DiffuseMap.Sample(DefaultSampler, UV.xy);
	float4 emsvColor = EmissiveMap.Sample(DefaultSampler, UV.xy);
	float4 specColor = SpecularMap.Sample(DefaultSampler, UV.xy);

	Emissive = emsvColor.rgb * MatEmissiveIntensity;
	Specular = specColor.rgb * MatSpecularIntensity;

	Depth = length(ViewSpacePos.xyz);
	float3x3 tangentViewMatrix = float3x3(normalize(Tangent.xyz), normalize(Binormal.xyz), normalize(Normal.xyz));        
	float3 tNormal = float3(0,0,0);
	tNormal.xy = (NormalMap.Sample(DefaultSampler, UV).ag * 2.0) - 1.0;
	tNormal.z = saturate(sqrt(1.0 - dot(tNormal.xy, tNormal.xy)));
	
	float3 worldSpaceNormal = mul(tNormal, tangentViewMatrix).xyz;
	float3 worldViewVec = normalize(EyePos.xyz - mul(ViewSpacePos, InvView).xyz);
	float3 reflectVec = reflect(worldViewVec, tNormal);
	float rim = saturate(pow(abs(1.0f - abs(dot(normalize(ViewSpacePos.xyz), worldSpaceNormal))), 1/FresnelPower) * FresnelStrength);
	float4 envColor = saturate(EnvironmentMap.Sample(DefaultSampler, reflectVec));
	Albedo = float4(lerp(diffColor.rgb, envColor.rgb, EnvironmentStrength), diffColor.a) + float4(rim, rim, rim, 0);
	Unshaded = float4(0,0,0,1);
	Normals = PackViewSpaceNormal(worldSpaceNormal.xyz);

	float alpha = Albedo.a;
	clip( alpha < AlphaSensitivity ? -1:1 );
}


//------------------------------------------------------------------------------
/**
*/
void
psBillboard(in float4 Position : SV_POSITION0,
			in float2 UV : TEXCOORD0,
			out float4 Albedo : SV_TARGET0)
{
	// get diffcolor
	float4 diffColor = DiffuseMap.Sample(DefaultSampler, UV.xy);
	
	Albedo = diffColor;
}