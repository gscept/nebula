//------------------------------------------------------------------------------
//  gui.fx
//
//	Basic GUI shader for use with LibRocket
//
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

/// Declaring used textures
Texture2D Texture;

float4x4 ModelViewProjection;

/// Declaring used samplers
SamplerState TextureSampler;

BlendState Blend 
{
	BlendEnable[0] = TRUE;
	SrcBlend[0] = 5;
	DestBlend[0] = 6;
};

RasterizerState DefaultRasterizer
{
	CullMode = 1;
};

RasterizerState ScissorRasterizer
{
	CullMode = 1;
	ScissorEnable = 1;
};

DepthStencilState DepthStencil
{
	DepthEnable = FALSE;
};

//------------------------------------------------------------------------------
/**
*/
void
vsMain(float4 position : POSITION,
	float2 uv : TEXCOORD,		
	float4 color : COLOR,
	out float4 Position : SV_POSITION,
	out float2 UV : TEXCOORD,
	out float4 Color : COLOR) 
{
	Position = mul(position, ModelViewProjection);
	Color = color;
	UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
void
psMain(in float4 Position : SV_POSITION,
	in float2 UV : TEXCOORD,
	in float4 Color : COLOR,		
	out float4 FinalColor : SV_TARGET0) 
{
	float4 texColor = Texture.Sample(TextureSampler, UV);
	FinalColor = texColor * Color;
}

//------------------------------------------------------------------------------
/**
*/
VertexShader vs = CompileShader(vs_5_0, vsMain());
PixelShader ps = CompileShader(ps_5_0, psMain());

technique11 Default < string Mask = "Static"; >
{
	pass Main
	{
		SetBlendState(Blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(DepthStencil, 0);
		SetRasterizerState(DefaultRasterizer);
		SetVertexShader(vs);
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(ps);
	}
}

technique11 Scissor < string Mask = "Static|Scissor"; >
{
	pass Main
	{
		SetBlendState(Blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(DepthStencil, 0);
		SetRasterizerState(ScissorRasterizer);
		SetVertexShader(vs);
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(ps);
	}
}