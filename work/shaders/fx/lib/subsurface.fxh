//------------------------------------------------------------------------------
//  subsurface.fxh
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "lib/defaultsampler.fxh"
#include "lib/util.fxh"

/// Declaring used samplers
SamplerState DefaultSampler
{
	AddressU = Mirror;
	AddressV = Mirror;
	Filter = MIN_MAG_MIP_LINEAR;
};

Texture2D LightTexture;
Texture2D DepthTexture;
Texture2D AbsorptionTexture;
Texture2D ScatterTexture;
Texture2D Mask;

const float SubsurfaceStrength  = {1.45f};
const float SubsurfaceWidth = {0.20f};
const float SubsurfaceCorrection = {50.0f};

#ifdef VERTICAL
float2 SubsurfaceDirection = {1, 0};
#else
float2 SubsurfaceDirection = {0, 1};
#endif

const float4 kernel[] = {
    float4(0.530605, 0.613514, 0.739601, 0),
    float4(0.000973794, 1.11862e-005, 9.43437e-007, -3),
    float4(0.00333804, 7.85443e-005, 1.2945e-005, -2.52083),
    float4(0.00500364, 0.00020094, 5.28848e-005, -2.08333),
    float4(0.00700976, 0.00049366, 0.000151938, -1.6875),
    float4(0.0094389, 0.00139119, 0.000416598, -1.33333),
    float4(0.0128496, 0.00356329, 0.00132016, -1.02083),
    float4(0.017924, 0.00711691, 0.00347194, -0.75),
    float4(0.0263642, 0.0119715, 0.00684598, -0.520833),
    float4(0.0410172, 0.0199899, 0.0118481, -0.333333),
    float4(0.0493588, 0.0367726, 0.0219485, -0.1875),
    float4(0.0402784, 0.0657244, 0.04631, -0.0833333),
    float4(0.0211412, 0.0459286, 0.0378196, -0.0208333),
    float4(0.0211412, 0.0459286, 0.0378196, 0.0208333),
    float4(0.0402784, 0.0657244, 0.04631, 0.0833333),
    float4(0.0493588, 0.0367726, 0.0219485, 0.1875),
    float4(0.0410172, 0.0199899, 0.0118481, 0.333333),
    float4(0.0263642, 0.0119715, 0.00684598, 0.520833),
    float4(0.017924, 0.00711691, 0.00347194, 0.75),
    float4(0.0128496, 0.00356329, 0.00132016, 1.02083),
    float4(0.0094389, 0.00139119, 0.000416598, 1.33333),
    float4(0.00700976, 0.00049366, 0.000151938, 1.6875),
    float4(0.00500364, 0.00020094, 5.28848e-005, 2.08333),
    float4(0.00333804, 7.85443e-005, 1.2945e-005, 2.52083),
    float4(0.000973794, 1.11862e-005, 9.43437e-007, 3),
};

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


//------------------------------------------------------------------------------
/**
*/
void
vsMain(float4 position : POSITION,
	float2 uv0 : TEXCOORD0,
	out float4 Position : SV_POSITION0,
	out float2 UV : TEXCOORD0) 
{
	Position = position;
	UV = uv0;
}

//------------------------------------------------------------------------------
/**
	Calculates subsurface approximation based on screen-space criteria.
	Should only be applied to skins!
*/
void
psMain(float4 Position : SV_POSITION0,
	float2 UV : TEXCOORD0,
	out float4 result : SV_TARGET0)
{
	float2 pixelSize = GetPixelSize(LightTexture);
	float2 screenUv = psComputeScreenCoord(Position.xy, pixelSize.xy);
	
	float4 mask = Mask.Sample(DefaultSampler, screenUv);
	
	// clip pixel if the mask is 0
	if (mask.a == 0)
	{
		discard;
	}

	// sample data textures
	float4 absorption = AbsorptionTexture.Sample(DefaultSampler, screenUv);
	float4 scatter = ScatterTexture.Sample(DefaultSampler, screenUv);
	
	// get color and depth
	float4 light = DecodeHDR(LightTexture.Sample(DefaultSampler, screenUv));
	float3 color = light.rgb * scatter.rgb;
	
	float depth = DepthTexture.Sample(DefaultSampler, screenUv);
	
	// create blurred color
	float3 colorBlurred = color;
	colorBlurred *= 0.382f;
	
	// calculate final step
	float2 finalStep = mask.r * mask.g * SubsurfaceDirection / depth;
	
	// get other samples
	[unroll]
	for (int i = 0; i < 16; i++)
	{
		float2 offset = screenUv + kernel[i].a * finalStep;
		float3 lightSample = DecodeHDR(LightTexture.SampleLevel(DefaultSampler, offset, 0)).rgb;
		float depthSample = DepthTexture.SampleLevel(DefaultSampler, offset, 0);
		
		float s = min(0.0125 * mask.b * abs(depth - depthSample), 1.0f);
		lightSample = lerp(lightSample, color, s);
		
		// accumulate
		colorBlurred += kernel[i].rgb * lightSample;
	}
	
	// multiply by absorption
	colorBlurred *= absorption;
	
	// finally return color
	result = EncodeHDR(float4(colorBlurred, 1));
}
