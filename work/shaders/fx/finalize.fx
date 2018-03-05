//------------------------------------------------------------------------------
//  finalize.fx
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/util.fxh"
#include "lib/shared.fxh"

/// Declaring used textures
Texture2D BloomTexture;
Texture2D ColorTexture;
Texture2D DepthTexture;
Texture2D GodrayTexture;
Texture2D LuminanceTexture;

shared cbuffer PostVars
{
	float Saturation = {1.0f};
	float4 Balance = {1.0f, 1.0f, 1.0f, 1.0f};
	float FadeValue = {1.0f};
	float4 Luminance = {0.299f, 0.587f, 0.114f, 0.0f};
	float3 DoFDistances = {2.0f, 100.0f, 2.0f};
	float4 FogDistances = {0.0, 2500.0, 0.0, 1.0};
	float4 FogColor = {0.5, 0.5, 0.63, 0.0};
	bool UseDof = true;
}

/// Declaring used samplers
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
	DepthWriteMask = 0;
};

// depth of field samples
static const int MaxDofSamples = 23;
float2 DofSamples[MaxDofSamples] = {
    { 
        { 0.0, 0.0 },
        { -0.326212, -0.40581  },
        { -0.840144, -0.07358  },
        { -0.695914,  0.457137 },
        { -0.203345,  0.620716 },
        {  0.96234,  -0.194983 },
        {  0.473434, -0.480026 },
        {  0.519456,  0.767022 },
        {  0.185461, -0.893124 },
        {  0.507431,  0.064425 },
        {  0.89642,   0.412458 },
        { -0.32194,   0.93261f },
        {  0.326212,  0.40581  },
        {  0.840144,  0.07358  },
        {  0.695914, -0.457137 },
        {  0.203345, -0.620716 },
        { -0.96234,   0.194983 },
        { -0.473434,  0.480026 },
        { -0.519456, -0.767022 },
        { -0.185461,  0.893124 },
        { -0.507431, -0.064425 },
        { -0.89642,  -0.412458 },
        {  0.32194,  -0.93261f },
    }
};   

//------------------------------------------------------------------------------
/**
*/
void
vsMain(float4 position : POSITION,
	float2 uv0 : TEXCOORD0,
	out float2 UV : TEXCOORD0,
	out float4 Position : SV_POSITION0) 
{
	Position = position;
	UV = uv0;
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
	Get a depth-of-field blurred sample. Set all values to 0 in order to disable DoF
*/
float4 psDepthOfField(Texture2D tex, float2 uv)
{
    // get an averaged depth value        
    float depth = DepthTexture.Sample(DefaultSampler, uv);    
    
    // compute focus-blur param (0 -> no blur, 1 -> max blur)
    float focusDist = DoFDistances.x;
    float focusLength = DoFDistances.y;
    float filterRadius = DoFDistances.z;    
    float focus = saturate(abs(depth - focusDist) / focusLength);
    
    // perform a gaussian blur around uv
    float4 sample = 0.0f;
    float dofWeight = 1.0f / MaxDofSamples;
	float2 pixelSize = GetPixelSize(tex);
    float2 uvMul = focus * filterRadius * pixelSize.xy;
    int i;
    for (i = 0; i < MaxDofSamples; i++)
    {
        sample += tex.Sample(DefaultSampler, uv + (DofSamples[i] * uvMul));
    }
    sample *= dofWeight;
    return sample;
} 

//------------------------------------------------------------------------------
/**
*/
void
psMain(in float2 UV : TEXCOORD0,
	in float4 Position : SV_POSITION0,
	out float4 color : SV_TARGET0) 
{

	float2 normalMapPixelSize = GetPixelSize(ColorTexture);
	float2 screenUv = psComputeScreenCoord(Position.xy, normalMapPixelSize.xy);
	float4 c;
	c = DecodeHDR(psDepthOfField(ColorTexture, screenUv));	
	
	// Get the calculated average luminance 
	float fLumAvg = LuminanceTexture.Sample(DefaultSampler, float2(0.5f, 0.5f)).r;     
	
	c = ToneMap(c, fLumAvg, Luminance);
	float4 bloom = DecodeHDR(BloomTexture.Sample(DefaultSampler, screenUv));
	float4 godray = DecodeHDR(GodrayTexture.Sample(DefaultSampler, screenUv));
	c += bloom;    
	float depth = DepthTexture.Sample(DefaultSampler, screenUv);
	color = psFog(depth, c);
	color.rgb += godray.rgb;
	float4 grey = dot(color.xyz, Luminance.xyz) ;
	color = Balance * lerp(grey, color, Saturation);
	color.rgb *= FadeValue;
		
	// set alpha to 1
	color.a = 1.0f;
}

//------------------------------------------------------------------------------
/**
*/
VertexShader vs = CompileShader(vs_5_0, vsMain());
PixelShader ps = CompileShader(ps_5_0, psMain());

technique11 Default
{
	pass Main
	{
		SetBlendState(Blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(DepthStencil, 0);
		SetRasterizerState(Rasterizer);
		SetVertexShader(vs);
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(ps);
	}
}