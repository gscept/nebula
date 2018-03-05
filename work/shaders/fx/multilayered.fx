//------------------------------------------------------------------------------
//  multilayered.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/util.fxh"
#include "lib/shared.fxh"

Texture2D MLPDiff1;
Texture2D MLPDiff2;
Texture2D MLPDiff3;
Texture2D MLPDiff4;
Texture2D MLPEmsv1;
Texture2D MLPEmsv2;
Texture2D MLPEmsv3;
Texture2D MLPEmsv4;
Texture2D MLPSpec1;
Texture2D MLPSpec2;
Texture2D MLPSpec3;
Texture2D MLPSpec4;
Texture2D MLPNorm1;
Texture2D MLPNorm2;
Texture2D MLPNorm3;
Texture2D MLPNorm4;
Texture2D MLPDisp1;
Texture2D MLPDisp2;
Texture2D MLPDisp3;
Texture2D MLPDisp4;

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

// we use the same rasterizer for both states
RasterizerState Rasterizer
{
	CullMode = 2;
};

DepthStencilState SolidDepthStencil
{
};

//------------------------------------------------------------------------------
/**
	Used for multi-layered painting
*/
void
vsColored(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD,
	float4 color : COLOR,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	out float3 ViewSpacePos : TEXCOORD0,
	out float3 Tangent : TANGENT,
	out float3 Normal : NORMAL,
	out float4 Position : SV_POSITION,
	out float3 Binormal : BINORMAL,
	out float2 UV : TEXCOORD1,
	out float4 Color : COLOR) 
{
	Position = mul(position, mul(Model, ViewProjection));
	UV = float2(uv.x * NumXTiles, uv.y * NumYTiles);
	Color = color;
	float4x4 modelView = mul(Model, View);
	ViewSpacePos = mul(position, modelView).xyz;    
	
	Tangent  = mul(tangent, modelView).xyz;
	Normal   = mul(normal, modelView).xyz;
	Binormal = mul(binormal, modelView).xyz;
}

//------------------------------------------------------------------------------
/**
	Used for multi-layered painting
*/
void
vsColoredTessellated(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD,
	float4 color : COLOR,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	out float3 Tangent : TANGENT,
	out float3 Normal : NORMAL,
	out float4 Position : POSITION,
	out float3 Binormal : BINORMAL,
	out float2 UV : TEXCOORD,
	out float4 Color : COLOR,
	out float Distance : CAMERADIST) 
{	
	Position = mul(position, Model);
	UV = uv;
	Color = color; 
	
	Tangent  = mul(tangent, Model).xyz;
	Normal   = mul(normal, Model).xyz;
	Binormal = mul(binormal, Model).xyz;
	
	float vertexDistance = distance( Position.xyz, EyePos.xyz );
	Distance = 1.0 - clamp( ( (vertexDistance - MinDistance) / (MaxDistance - MinDistance) ), 0.0, 1.0 - 1.0/TessellationFactor);
}

//------------------------------------------------------------------------------
/**
	Pixel shader for multilayered painting
*/
void
psMultiLayered(in float3 ViewSpacePos : TEXCOORD0,
	in float3 Tangent : TANGENT0,
	in float3 Normal : NORMAL0,
	in float4 Position : SV_POSITION0,
	in float3 Binormal : BINORMAL0,
	in float2 UV : TEXCOORD1,
	in float4 Color : COLOR,
	out float4 Albedo : SV_TARGET0,
	out float4 Normals : SV_TARGET1,
	out float Depth : SV_TARGET2,	
	out float3 Specular : SV_TARGET3,
	out float3 Emissive : SV_TARGET4,
	out float4 Unshaded : SV_TARGET5) 
{
	float4 diffColor1 = MLPDiff1.Sample(DefaultSampler, UV.xy);
	float4 diffColor2 = MLPDiff2.Sample(DefaultSampler, UV.xy);
	float4 diffColor3 = MLPDiff3.Sample(DefaultSampler, UV.xy);
	float4 diffColor4 = MLPDiff4.Sample(DefaultSampler, UV.xy);
	float4 diffColor = diffColor1 * Color.r + diffColor2 * Color.g + diffColor3 * Color.b + diffColor4 * (1-Color.a);
		
	float4 emsvColor1 = MLPEmsv1.Sample(DefaultSampler, UV.xy);
	float4 emsvColor2 = MLPEmsv2.Sample(DefaultSampler, UV.xy);
	float4 emsvColor3 = MLPEmsv3.Sample(DefaultSampler, UV.xy);
	float4 emsvColor4 = MLPEmsv4.Sample(DefaultSampler, UV.xy);
	float4 emsvColor = emsvColor1 * Color.r + emsvColor2 * Color.g + emsvColor3 * Color.b + emsvColor4 * (1-Color.a);
	
	float4 specColor1 = MLPSpec1.Sample(DefaultSampler, UV.xy);
	float4 specColor2 = MLPSpec2.Sample(DefaultSampler, UV.xy);
	float4 specColor3 = MLPSpec3.Sample(DefaultSampler, UV.xy);
	float4 specColor4 = MLPSpec4.Sample(DefaultSampler, UV.xy);
	float4 specColor = specColor1 * Color.r + specColor2 * Color.g + specColor3 * Color.b + specColor4 * (1-Color.a);	
	
	Emissive = emsvColor.rgb * MatEmissiveIntensity;
	Specular = specColor.rgb * MatSpecularIntensity;
	Albedo = diffColor;
	Unshaded = float4(0,0,0,1);
	Depth = length(ViewSpacePos.xyz);
	float3x3 tangentViewMatrix = float3x3(normalize(Tangent.xyz), normalize(Binormal.xyz), normalize(Normal.xyz));        
	float3 tNormal = float3(0,0,0);
	
	float2 normColor1 = (MLPNorm1.Sample(DefaultSampler, UV).ag * 2.0) - 1.0;
	float2 normColor2 = (MLPNorm2.Sample(DefaultSampler, UV).ag * 2.0) - 1.0;
	float2 normColor3 = (MLPNorm3.Sample(DefaultSampler, UV).ag * 2.0) - 1.0;
	float2 normColor4 = (MLPNorm4.Sample(DefaultSampler, UV).ag * 2.0) - 1.0;
	tNormal.xy = normColor1 * Color.r + normColor2 * Color.g + normColor3 * Color.b + normColor4 * (1-Color.a);
	tNormal.z = saturate(sqrt(1.0 - dot(tNormal.xy, tNormal.xy)));
	
	Normals = PackViewSpaceNormal(mul(tNormal, tangentViewMatrix).xyz);
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
	float4 Color : COLOR;
	float Distance : CAMERADIST;
};

struct HS_OUTPUT
{
	float3 Tangent : TANGENT;
	float3 Normal : NORMAL;
	float4 Position : POSITION;
	float3 Binormal : BINORMAL;
	float2 UV : TEXCOORD;
	float4 Color : COLOR;
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
HSPatchFunc(InputPatch<HS_INPUT, 3> patch, uint patchID : SV_PrimitiveID)
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
hsDefault(InputPatch<HS_INPUT, 3> patch, uint i : SV_OutputControlPointID)
{
	HS_OUTPUT output;
	output.Tangent = patch[i].Tangent;
	output.Normal = patch[i].Normal;
	output.Position = patch[i].Position;
	output.Binormal = patch[i].Binormal;
	output.UV = patch[i].UV;	
	output.Color = patch[i].Color;
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
	float4 Color : COLOR;
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
    float fU = Coordinate.x;
    float fV = Coordinate.y;
    float fW = Coordinate.z;

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

	output.Color = Coordinate.x * Patch[0].Color + Coordinate.y * Patch[1].Color + Coordinate.z * Patch[2].Color;

	output.UV = (Coordinate.x * Patch[0].UV + Coordinate.y * Patch[1].UV + Coordinate.z * Patch[2].UV) * float2(NumXTiles, NumYTiles);
	float height1 = 2.0f * MLPDisp1.SampleLevel(DefaultSampler, output.UV, 0).x - 1.0f;
	float height2 = 2.0f * MLPDisp2.SampleLevel(DefaultSampler, output.UV, 0).x - 1.0f;
	float height3 = 2.0f * MLPDisp3.SampleLevel(DefaultSampler, output.UV, 0).x - 1.0f;
	float height4 = 2.0f * MLPDisp4.SampleLevel(DefaultSampler, output.UV, 0).x - 1.0f;
	float height = height1 * output.Color.r + height2 * output.Color.g + height3 * output.Color.b + height4 * (1 - output.Color.a);
	output.Normal = Coordinate.x * Patch[0].Normal + Coordinate.y * Patch[1].Normal + Coordinate.z * Patch[2].Normal;
	float3 VectorNormalized = normalize( output.Normal );
	
	Position.xyz += VectorNormalized.xyz * HeightScale * SceneScale * height;
	
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
VertexShader vs = CompileShader(vs_5_0, vsColored());
VertexShader vsTess = CompileShader(vs_5_0, vsColoredTessellated());
PixelShader ps = CompileShader(ps_5_0, psMultiLayered());
HullShader hs = CompileShader(hs_5_0, hsDefault());
DomainShader ds = CompileShader(ds_5_0, dsDefault());

technique11 Multilayered < string Mask = "Static"; >
{
	pass Main
	{
		SetBlendState(SolidBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(SolidDepthStencil, 0);
		SetRasterizerState(Rasterizer);
		SetVertexShader(vs);
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(ps);
	}
}

technique11 MultilayeredTessellated < string Mask = "Static|Tessellated"; >
{
	pass Main
	{
		SetBlendState(SolidBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(SolidDepthStencil, 0);
		SetRasterizerState(Rasterizer);
		SetVertexShader(vsTess);
		SetHullShader(hs);
		SetDomainShader(ds);
		SetGeometryShader(NULL);
		SetPixelShader(ps);
	}
}

