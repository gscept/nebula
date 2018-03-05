//------------------------------------------------------------------------------
//  copy.fx
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

Texture2D CopyBuffer;
SamplerState DefaultSampler;

BlendState Blend 
{
};

// we use the same rasterizer for both states
RasterizerState Rasterizer
{
	CullMode = 2;
};

DepthStencilState Depth
{
	DepthEnable = FALSE;
	DepthWriteMask = 0;
};

//------------------------------------------------------------------------------
/**
*/
void
vsMain(float4 position : POSITION,
	float2 uv : TEXCOORD,
	out float2 UV : TEXCOORD,
	out float4 Position : SV_POSITION) 
{
    Position = position;
    UV = uv; 
}

//------------------------------------------------------------------------------
/**
*/
void
psMain(float2 UV : TEXCOORD,
	float4 Position : SV_POSITION,
	out float4 Color : SV_TARGET0)
{
	Color = CopyBuffer.Sample(DefaultSampler, UV);
}

//------------------------------------------------------------------------------
/**
*/
VertexShader vs = CompileShader(vs_5_0, vsMain());
PixelShader ps = CompileShader(ps_5_0, psMain());

technique11
{
	pass Main
	{
		SetBlendState(Blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(Depth, 0);
		SetRasterizerState(Rasterizer);
		SetVertexShader(vs);
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(ps);
	}
}