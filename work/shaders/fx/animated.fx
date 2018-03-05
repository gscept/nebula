//------------------------------------------------------------------------------
//  animated.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/geometrybase.fxh"


//------------------------------------------------------------------------------
/**
*/
void
vsAnimated(float4 position : POSITION,
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
	
	// create rotation matrix
	float angle = AnimationAngle * Time * AnimationAngularSpeed;
	float rotSin, rotCos;
	sincos(angle, rotSin, rotCos);
	float2x2 rotationMat = { rotCos, -rotSin,
						     rotSin, rotCos };
							 
	// compute 2d extruded corner position
	float2 uvCenter = ((uv * 2.0f) - 1.0f) / 2.0f;
	float2 uvOffset = AnimationDirection * Time * AnimationLinearSpeed;
	UV = (mul(uvCenter, rotationMat) + float2(0.5f, 0.5f)) * float2(NumXTiles, NumYTiles) + uvOffset;
	
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
vsAnimatedTessellated(float4 position : POSITION,
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
	
	// create rotation matrix
	float angle = AnimationAngle * Time * AnimationAngularSpeed;
	float rotSin, rotCos;
	sincos(angle, rotSin, rotCos);
	float2x2 rotationMat = { rotCos, -rotSin,
						     rotSin, rotCos };
							 
	// compute 2d extruded corner position
	float2 uvCenter = ((uv * 2.0f) - 1.0f) / 2.0f;
	float2 uvOffset = AnimationDirection * Time * AnimationLinearSpeed;
	UV = (mul(uvCenter, rotationMat) + float2(0.5f, 0.5f)) * float2(NumXTiles, NumYTiles) + uvOffset;
	
	Tangent  = mul(tangent, Model).xyz;
	Normal   = mul(normal, Model).xyz;
	Binormal = mul(binormal, Model).xyz;
		
	float vertexDistance = distance( Position.xyz, EyePos.xyz );
	Distance = 1.0 - clamp( ( (vertexDistance - MinDistance) / (MaxDistance - MinDistance) ), 0.0, 1.0 - 1.0/TessellationFactor);
}

//------------------------------------------------------------------------------
/**
*/
VertexShader vs = CompileShader(vs_5_0, vsAnimated());
VertexShader vsTess = CompileShader(vs_5_0, vsAnimatedTessellated());
HullShader hs = CompileShader(hs_5_0, hsDefault());
DomainShader ds = CompileShader(ds_5_0, dsDefault());
PixelShader ps = CompileShader(ps_5_0, psDefault());

technique11 Solid < string Mask = "Static"; >
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

technique11 Tessellated < string Mask = "Static|Tessellated"; >
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

technique11 Alpha < string Mask = "Alpha"; >
{
	pass Main
	{
		SetBlendState(AlphaBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(AlphaDepthStencil, 0);
		SetRasterizerState(Rasterizer);
		SetVertexShader(vs);
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(ps);
	}
}