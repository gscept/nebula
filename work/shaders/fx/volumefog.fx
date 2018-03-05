//------------------------------------------------------------------------------
//  volumefog.fx
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "lib/shared.fxh"
#include "lib/util.fxh"

cbuffer VolumeFogVars
{
	float DepthDensity = 1.0f;
	float AlphaDensity = 0.0f;
	float LayerDistance = 0.0f;
	float2 Velocity = float2(0,0);
	float2 FogDistances = {0.0f, 50.0f};
	float4 FogColor = {0.5f, 0.5f, 0.63f, 0.0f};
}


Texture2D DiffuseMap;
Texture2D DepthMap;

SamplerState DefaultSampler;

BlendState Blend 
{
};

RasterizerState Rasterizer
{
	CullMode = 1;
};

DepthStencilState DepthStencil
{
	DepthWriteMask = 0;
};

static const float2 layerVelocities[4] = { {  1.0,   1.0 },
                                    { -0.9,  -0.8 },
                                    {  0.8,  -0.9 },
                                    { -0.4,   0.5 }};

//------------------------------------------------------------------------------
/**
*/
void SampleTexture(in float2 uv, in float4 vertexColor, inout float4 dstColor)
{
    float4 srcColor = DiffuseMap.Sample(DefaultSampler, uv) * vertexColor;
    dstColor.rgb = lerp(dstColor.rgb, srcColor.rgb, srcColor.a);
    dstColor.a += srcColor.a * 0.25;
}

//------------------------------------------------------------------------------
/**
    Compute fogging given a sampled fog intensity value from the depth
    pass and a fog color.
*/
float4 psFog(float fogDepth, float4 color)
{
    float fogIntensity = clamp((FogDistances.y - fogDepth) / (FogDistances.y - FogDistances.x), FogColor.a, 1.0);
    return float4(lerp(FogColor.rgb, color.rgb, fogIntensity), color.a);
}

//------------------------------------------------------------------------------
/**
*/
void
vsMain(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD,
	float4 color : COLOR,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	out float4 Position : SV_POSITION,
	out float3 ViewSpacePos : TEXCOORD0,
	out float4 UVSet1 : TEXCOORD1,
	out float4 UVSet2 : TEXCOORD2,
	out float4 Color : COLOR) 
{
	Position = mul(position, mul(Model, ViewProjection));
	ViewSpacePos = mul(position, mul(Model, View));
	
	float4 worldPos = mul(position, Model);
	float3 worldEyeVec = normalize(EyePos - worldPos);
	float3 worldNormal = normalize(mul(normal, Model));
	
	// compute animated uvs
	float2 uvs[4];
	float2 uvDiff = worldEyeVec * LayerDistance;
	float2 uvOffset = -4 * uvDiff;
	
	[unroll]
	for (int i = 0; i < 4; i++)
	{
		uvOffset += uvDiff;
		uvs[i] = uv + uvOffset + Velocity.xy * Time * layerVelocities[i];
	}
	
	UVSet1.xy = uvs[0];
	UVSet1.zw = uvs[1];
	UVSet2.xy = uvs[2];
	UVSet2.zw = uvs[3];
	
	Color = color;
	float dotP = dot(worldNormal, worldEyeVec);
	Color.a = dotP * dotP;
}


//------------------------------------------------------------------------------
/**
*/
void
psMain(float4 Position : SV_POSITION,
	float3 ViewSpacePos : TEXCOORD0,
	float4 UVSet1 : TEXCOORD1,
	float4 UVSet2 : TEXCOORD2,
	float4 Color : COLOR,
	out float4 Result : SV_TARGET0,
	out float4 Unshaded : SV_TARGET1)
{
	float2 normalMapPixelSize = GetPixelSize(DiffuseMap);
	float2 screenUv = psComputeScreenCoord(Position.xy, normalMapPixelSize.xy);
	
	float4 dstColor = float4(0,0,0,0);
	SampleTexture(UVSet1.xy, Color, dstColor);
	SampleTexture(UVSet1.zw, Color, dstColor);
	SampleTexture(UVSet2.xy, Color, dstColor);
	SampleTexture(UVSet2.zw, Color, dstColor);
	
	float3 viewVec = normalize(ViewSpacePos);
	float depth = DepthMap.Sample(DefaultSampler, screenUv);
	float3 surfacePos = viewVec * depth;
	
	float depthDiff = ViewSpacePos.z - surfacePos.z;
	
	// modulate alpha by depth
	float modAlpha = saturate(depthDiff * 5.0f) * DepthDensity * AlphaDensity * 2;
	Result.rgb = dstColor.rgb;
	Result.a = dstColor.a * modAlpha;
	Result = psFog(Position.z, Result);
}

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(Default, "Static", vsMain(), psMain(), Blend, DepthStencil, Rasterizer)
