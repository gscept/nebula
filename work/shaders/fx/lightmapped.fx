//------------------------------------------------------------------------------
//  lightmapped.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/shared.fxh"
#include "lib/util.fxh"
#include "lib/csm.fxh"

/// Declaring used textures
Texture2D SpecularMap;
Texture2D EmissiveMap;
Texture2D NormalMap;
Texture2D LightMap;
Texture2D DiffuseMap;

SamplerState DefaultSampler;
BlendState Blend 
{
};

// we use the same rasterizer for both states
RasterizerState Rasterizer
{
	CullMode = 2;
};

DepthStencilState DepthStencil
{
};

//------------------------------------------------------------------------------
/**
	Used for lightmapped geometry
*/
void
vsLightmap(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv1 : TEXCOORD0,
	float2 uv2 : TEXCOORD1,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	out float3 ViewSpacePos : TEXCOORD0,
	out float3 Tangent : TANGENT0,
	out float3 Normal : NORMAL0,
	out float4 Position : SV_POSITION0,
	out float3 Binormal : BINORMAL0,
	out float2 UV1 : TEXCOORD1,
	out float2 UV2 : TEXCOORD2) 
{
	Position = mul(position, mul(Model, ViewProjection));
	UV1 = uv1;
	UV2 = uv2;
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
vsShadow(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv1 : TEXCOORD0,
	float2 uv2 : TEXCOORD1,
	float2 uv : TEXCOORD,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	out float4 Position : SV_POSITION,
	out float2 UV : TEXCOORD) 
{
	Position = mul(position, mul(Model, ViewProjection));
	UV = uv1;
}

//------------------------------------------------------------------------------
/**
*/
void
vsShadowCSM(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv1 : TEXCOORD0,
	float2 uv2 : TEXCOORD1,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	out float4 Position : SV_POSITION,
	out float2 UV : TEXCOORD) 
{
	Position = mul(position, Model);
	UV = uv1;
}

//------------------------------------------------------------------------------
/**
*/
struct VS_OUT
{
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD;
};

struct GS_OUT
{
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD;
	uint ViewportIndex : SV_ViewPortArrayIndex;
};

//------------------------------------------------------------------------------
/**
	Geometry shader for CSM shadow instancing.
	We copy the geometry and project into each frustum.
*/
[maxvertexcount(CASCADE_COUNT_FLAG * 3)]
void 
gsMain(triangle VS_OUT In[3], inout TriangleStream<GS_OUT> triStream)
{
	for (int split = 0; split < CASCADE_COUNT_FLAG; split++)
	{
		GS_OUT output;
		
		output.ViewportIndex = split;
		
		for (int vertex = 0; vertex < 3; vertex++)
		{
			output.Position = mul(In[vertex].Position, CSMSplitMatrices[split]);
			output.UV = In[vertex].UV;
			triStream.Append(output);
		}
		
		triStream.RestartStrip();
	}
}

//------------------------------------------------------------------------------
/**
	Pixel shader for lightmapped lit geometry
*/
void
psLightMappedLit(in float3 ViewSpacePos : TEXCOORD0,
	in float3 Tangent : TANGENT0,
	in float3 Normal : NORMAL0,
	in float4 Position : SV_POSITION0,
	in float3 Binormal : BINORMAL0,
	in float2 UV1 : TEXCOORD1,
	in float2 UV2 : TEXCOORD2,
	out float4 Albedo : SV_TARGET0,
	out float4 Normals : SV_TARGET1,
	out float Depth : SV_TARGET2,	
	out float3 Specular : SV_TARGET3,
	out float3 Emissive : SV_TARGET4,
	out float4 Unshaded : SV_TARGET5) 
{
	float4 diffColor = DiffuseMap.Sample(DefaultSampler, UV1.xy);
	float4 emsvColor = EmissiveMap.Sample(DefaultSampler, UV1.xy);
	float4 specColor = SpecularMap.Sample(DefaultSampler, UV1.xy);
	float4 lightMapColor = float4(((LightMap.Sample(DefaultSampler, UV2.xy) - 0.5f) * 2.0f * LightMapIntensity).rgb, 1);
	
	Emissive = emsvColor.rgb * MatEmissiveIntensity + diffColor * lightMapColor;
	Specular = specColor.rgb * MatSpecularIntensity;
	Albedo = diffColor;
	Unshaded = float4(0,0,0,1);
	Depth = length(ViewSpacePos.xyz);
	float3x3 tangentViewMatrix = float3x3(normalize(Tangent.xyz), normalize(Binormal.xyz), normalize(Normal.xyz));        
    float3 tNormal = float3(0,0,0);
	tNormal.xy = (NormalMap.Sample(DefaultSampler, UV1).ag * 2.0) - 1.0;
    tNormal.z = saturate(sqrt(1.0 - dot(tNormal.xy, tNormal.xy)));
	
	Normals = PackViewSpaceNormal(mul(tNormal, tangentViewMatrix).xyz);

	float alpha = diffColor.a;
	clip( alpha < AlphaSensitivity ? -1:1 );
}

//------------------------------------------------------------------------------
/**
	Pixel shader for lightmapped unlit geometry
*/
void
psLightMappedUnlit(in float3 ViewSpacePos : TEXCOORD0,
	in float3 Tangent : TANGENT0,
	in float3 Normal : NORMAL0,
	in float4 Position : SV_POSITION0,
	in float3 Binormal : BINORMAL0,
	in float2 UV1 : TEXCOORD1,
	in float2 UV2 : TEXCOORD2,
	out float4 Albedo : SV_TARGET0,
	out float4 Unshaded : SV_TARGET1) 
{
	float4 diffColor = DiffuseMap.Sample(DefaultSampler, UV1.xy);
	float4 lightMapColor = float4(((LightMap.Sample(DefaultSampler, UV2.xy) - 0.5f) * 2.0f * LightMapIntensity).rgb, 1);
	
	Albedo = diffColor * lightMapColor;
	Unshaded = float4(0,0,0,1);
	
	float alpha = diffColor.a;
	clip( alpha < AlphaSensitivity ? -1:1 );
}

//------------------------------------------------------------------------------
/**
*/
void
psShadow(in float4 Position : SV_POSITION0,
	in float2 UV : TEXCOORD0,
	out float ShadowColor : SV_TARGET0) 
{
	float alpha = DiffuseMap.Sample( DefaultSampler, UV).a;
	clip( alpha < AlphaSensitivity ? -1:1 );
	ShadowColor = (Position.z/Position.w) * ShadowConstant;
}


//------------------------------------------------------------------------------
/**
*/
VertexShader geometryVs = CompileShader(vs_5_0, vsLightmap());
VertexShader shadowVs = CompileShader(vs_5_0, vsShadow());
VertexShader shadowVsCSM = CompileShader(vs_5_0, vsShadowCSM());
GeometryShader csmGs = CompileShader(gs_5_0, gsMain());
PixelShader psLit = CompileShader(ps_5_0, psLightMappedLit());
PixelShader psUnlit = CompileShader(ps_5_0, psLightMappedUnlit());
PixelShader shadowPs = CompileShader(ps_5_0, psShadow());


technique11 Lit < string Mask = "Static|Lit"; >
{
		pass Main
	{
		SetBlendState(Blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(DepthStencil, 0);
		SetRasterizerState(Rasterizer);
		SetVertexShader(geometryVs);
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(psLit);
	}
}

technique11 Unlit < string Mask = "Static|Unlit"; >
{
		pass Main
	{
		SetBlendState(Blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(DepthStencil, 0);
		SetRasterizerState(Rasterizer);
		SetVertexShader(geometryVs);
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(psUnlit);
	}
}

technique11 ShadowLit < string Mask = "Static|Shadow"; >
{
	pass Main
	{
		SetBlendState(Blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(DepthStencil, 0);
		SetRasterizerState(Rasterizer);
		SetVertexShader(shadowVs);
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(shadowPs);
	}
}

technique11 ShadowLitCSM < string Mask = "Static|Shadow|CSM"; >
{
	pass Main
	{
		SetBlendState(Blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(DepthStencil, 0);
		SetRasterizerState(Rasterizer);
		SetVertexShader(shadowVsCSM);
		SetGeometryShader(csmGs);
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetPixelShader(shadowPs);
	}
}