//------------------------------------------------------------------------------
//  particle.fx
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/particles.fxh"
#include "lib/shared.fxh"
#include "lib/util.fxh"

/// Declaring used textures
Texture2D DepthBuffer;
Texture2D DiffuseMap;
Texture2D SpecularMap;
Texture2D EmissiveMap;
Texture2D NormalMap;

/// Declaring used samplers
SamplerState DefaultSampler;

BlendState LitBlend 
{
	BlendEnable[2] = TRUE;
	SrcBlend[2] = 5;
	DestBlend[2] = 6;
	SrcBlendAlpha[2] = 5;
	DestBlendAlpha[2] = 6;
	BlendOpAlpha[2] = 5;
	BlendEnable[5] = TRUE;
	SrcBlend[5] = 5;
	DestBlend[5] = 6;
};

BlendState UnlitBlend 
{
	BlendEnable[0] = TRUE;
	SrcBlend[0] = 5;
	DestBlend[0] = 6;
	BlendOp[0] = 1;
	SrcBlendAlpha[0] = 5;
	DestBlendAlpha[0] = 6;
	BlendOpAlpha[0] = 1;
	
	BlendEnable[1] = TRUE;
	SrcBlend[1] = 5;
	DestBlend[1] = 6;
	
};

// we use the same rasterizer for both states
RasterizerState Rasterizer
{
	CullMode = 1;
};

DepthStencilState DepthStencil
{
	DepthFunc = 2;
	DepthWriteMask = 0;
};

//------------------------------------------------------------------------------
/**
*/
#define numAlphaLayers 4
static const float2 stippleMasks[numAlphaLayers] = {0,0, 
                                             1,0, 
                                             0,1,
                                             1,1};


//------------------------------------------------------------------------------
/**
*/
void
vsUnlit(float2 corner : TEXCOORD,
	float4 position : POSITION0,
	float4 stretchPos : POSITION1,
	float4 color : COLOR,
	float4 uvMinMax : TEXCOORD1,
	float4 rotSize : TEXCOORD2,
	out float4 Position : SV_POSITION0,
	out float4 ViewSpacePos : VIEWPOS0,
	out float4 Color : COLOR0,
	out float2 UV : TEXCOORD0) 
{
	CornerVertex cornerVert = ComputeCornerVertex(true,
										corner,
										position,
										stretchPos,
										uvMinMax,
										rotSize.x,
										rotSize.y);
										
	UV = cornerVert.UV;
	Position = mul(cornerVert.worldPos, ViewProjection);
	ViewSpacePos = mul(cornerVert.worldPos, View);
	Color = color;
}

//------------------------------------------------------------------------------
/**
*/
void
vsLit(float2 corner : TEXCOORD,
	float4 position : POSITION0,
	float4 stretchPos : POSITION1,
	float4 color : COLOR,
	float4 uvMinMax : TEXCOORD1,
	float4 rotSize : TEXCOORD2,
	out float4 Position : SV_POSITION0,
	out float4 ViewSpacePos : VIEWPOS0,
	out float4 ProjPos : PROJPOS0,
	out float4 WorldPos : WORLDPOS0,
	out float3 Normal : NORMAL0,
	out float3 Tangent : TANGENT0,
	out float3 Binormal : BINORMAL0,
	out float3 WorldEyeVec : WORLDEYEVEC0,
	out float4 Color : COLOR0,
	out float2 UV : TEXCOORD0) 
{
	CornerVertex cornerVert = ComputeCornerVertex(false,
										corner,
										position,
										stretchPos,
										uvMinMax,
										rotSize.x,
										rotSize.y);
										
	float4x4 modelView = mul(Model, View);
	Normal = mul(cornerVert.worldNormal, modelView);
	Tangent = mul(cornerVert.worldTangent, modelView);
	Binormal = mul(cornerVert.worldBinormal, modelView);
	UV = cornerVert.UV;
	WorldEyeVec = normalize(EyePos - cornerVert.worldPos);
	WorldPos = cornerVert.worldPos;
	Position = mul(cornerVert.worldPos, ViewProjection);
	ViewSpacePos = mul(cornerVert.worldPos, View);
	ProjPos = Position;
	Color = color;
}

//------------------------------------------------------------------------------
/**
*/
void
psUnlit(in float4 Position : SV_POSITION0,
	in float4 ViewSpacePosition : VIEWPOS0,
	in float4 Color : COLOR0,
	in float2 UV : TEXCOORD0,
	out float4 FinalColor : SV_TARGET0,
	out float4 Unshaded : SV_TARGET1) 
{
	float2 pixelSize = GetPixelSize(DiffuseMap);
	float2 ScreenUV = psComputeScreenCoord(Position.xy, pixelSize.xy);
	float4 diffColor = DiffuseMap.Sample( DefaultSampler, UV );
	float4 emsColor = EmissiveMap.Sample( DefaultSampler, UV );
	
	float4 color = diffColor + float4(Color.rgb, 0) + emsColor * MatEmissiveIntensity;
	float Alpha = diffColor.a * Color.a;
	float depth = DepthBuffer.Sample(DefaultSampler, ScreenUV);
	float particleDepth = length(ViewSpacePosition);
	float AlphaMod = saturate(abs(depth - particleDepth));
	color.a = Alpha * AlphaMod;
	FinalColor = EncodeHDR(color);
	Unshaded = float4(0,0,0,color.a);
}

//------------------------------------------------------------------------------
/**
*/
void
psLit(in float4 Position : SV_POSITION0,
	in float4 ViewSpacePosition : VIEWPOS0,
	in float4 ProjPos : PROJPOS0,
	in float4 WorldPos : WORLDPOS0,
	in float3 Normal : NORMAL0,
	in float3 Tangent : TANGENT0,
	in float3 Binormal : BINORMAL0,
	in float3 WorldEyeVec : WORLDEYEVEC0,
	in float4 Color : COLOR0,
	in float2 UV : TEXCOORD0,
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
	Albedo = diffColor + float4(Color.rgb, 0);
	
	Depth = length(ViewSpacePosition.xyz);
	float3x3 tangentViewMatrix = float3x3(normalize(Tangent.xyz), normalize(Binormal.xyz), normalize(Normal.xyz));        
	float3 tNormal = float3(0,0,0);
	tNormal.xy = (NormalMap.Sample(DefaultSampler, UV).ag * 2.0) - 1.0;
	tNormal.z = saturate(sqrt(1.0 - dot(tNormal.xy, tNormal.xy)));
	
	Normals = PackViewSpaceNormal(mul(tNormal, tangentViewMatrix).xyz);
	
	float depth = Depth;
	float particleDepth = length(ViewSpacePosition);
	float alphaMod = saturate(abs(depth - particleDepth));
	Albedo.a = diffColor.a * Color.a;
	Unshaded = float4(0,0,0,Albedo.a);
}

//------------------------------------------------------------------------------
/**
*/
VertexShader vs = CompileShader(vs_5_0, vsUnlit());
VertexShader vsLight = CompileShader(vs_5_0, vsLit());
PixelShader ps = CompileShader(ps_5_0, psUnlit());
PixelShader psLight = CompileShader(ps_5_0, psLit());


technique11 Unlit < string Mask = "Particle|Unlit"; >
{
	pass Main
	{
		SetBlendState(UnlitBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(DepthStencil, 0);
		SetRasterizerState(Rasterizer);
		SetVertexShader(vs);
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(ps);
	}
}

technique11 Lit < string Mask = "Particle|Lit"; >
{
	pass Main
	{
		SetBlendState(LitBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(DepthStencil, 0);
		SetRasterizerState(Rasterizer);
		SetVertexShader(vsLight);
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(psLight);
	}
}