//------------------------------------------------------------------------------
//  hbao.fx
//  (C) 2011 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/util.fxh"
#include "lib/shared.fxh"

cbuffer AOVars
{
	float2 UVToViewA = {0.0f, 0.0f};
	float MaxRadiusPixels = {0.0f};
	float2 UVToViewB = {0.0f, 0.0f};
	float2 InvAOResolution = {0.0f, 0.0f};
	float TanAngleBias = {0.0f};
	float NegInvR2 = {0.0f};
	float2 AOResolution = {0.0f, 0.0f};
	float Strength = {0.0f};

	float R = {0.0f};
	float R2 = {0.0f};
}

/// Declaring used samplers
SamplerState PointClampSampler
{
	Filter = 0;
	AddressU = Clamp;
	AddressV = Clamp;
};


SamplerState PointWrapSampler
{
	Filter = 0;
	AddressU = 1;
	AddressV = 1;
};



Texture2D DepthBuffer;
Texture2D RandomMap;
#define RANDOM_TEXTURE_WIDTH 4
#define NUM_STEPS 6
#define NUM_DIRECTIONS 8
#define M_PI 3.14159265358979323846f
#define SAMPLE_FIRST_STEP 1

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

//----------------------------------------------------------------------------------
float2 RotateDirections(float2 Dir, float2 CosSin)
{
	return float2(Dir.x*CosSin.x - Dir.y*CosSin.y,
               Dir.x*CosSin.y + Dir.y*CosSin.x);
}
//----------------------------------------------------------------------------------
void ComputeSteps(inout float2 stepSizeUv, inout float numSteps, float rayRadiusPix, float rand)
{
	// Avoid oversampling if NUM_STEPS is greater than the kernel radius in pixels
	numSteps = min(NUM_STEPS, rayRadiusPix);
	
	// Divide by Ns+1 so that the farthest samples are not fully attenuated
	float stepSizePix = rayRadiusPix / (numSteps + 1);
	
	// Clamp numSteps if it is greater than the max kernel footprint
	float maxNumSteps = MaxRadiusPixels / stepSizePix;
	
	if (maxNumSteps < numSteps)
	{
		// Use dithering to avoid AO discontinuities
		numSteps = floor(maxNumSteps + rand);
		numSteps = max(numSteps, 1);
		stepSizePix = MaxRadiusPixels / numSteps;	
	}
	// Step size in uv space
	stepSizeUv = stepSizePix * InvAOResolution;
}
//----------------------------------------------------------------------------------
float InvLength(float2 v)
{
	return rsqrt(dot(v,v));
}
//----------------------------------------------------------------------------------
float Tangent(float3 P, float3 S)
{
	return (P.z - S.z) * InvLength(S.xy - P.xy);
}
//----------------------------------------------------------------------------------
float3 UVToEye(float2 uv, float eye_z)
{
	uv = UVToViewA * uv + UVToViewB;
	return float3(uv * eye_z, eye_z);
}
//----------------------------------------------------------------------------------
float3 FetchEyePos(float2 uv)
{
	float z = DepthBuffer.SampleLevel(PointClampSampler, uv, 0);
	return UVToEye(uv, z);
}
//----------------------------------------------------------------------------------
float Length2(float3 v)
{
    return dot(v, v);
}
//----------------------------------------------------------------------------------
float3 MinDiff(float3 P, float3 Pr, float3 Pl)
{
    float3 V1 = Pr - P;
    float3 V2 = P - Pl;
    return (Length2(V1) < Length2(V2)) ? V1 : V2;
}
//----------------------------------------------------------------------------------
float Falloff(float d2)
{
    // 1 scalar mad instruction
    return d2 * NegInvR2 + 1.0f;
}
//----------------------------------------------------------------------------------
float2 SnapUVOffset(float2 uv)
{
    return round(uv * AOResolution) * InvAOResolution;
}
//----------------------------------------------------------------------------------
float TanToSin(float x)
{
    return x * rsqrt(x*x + 1.0f);
}
//----------------------------------------------------------------------------------
float3 TangentVector(float2 deltaUV, float3 dPdu, float3 dPdv)
{
    return deltaUV.x * dPdu + deltaUV.y * dPdv;
}
//----------------------------------------------------------------------------------
float Tangent(float3 T)
{
    return -T.z * InvLength(T.xy);
}
//----------------------------------------------------------------------------------
float BiasedTangent(float3 T)
{
    // Do not use atan() because it gets expanded by fxc to many math instructions
    return Tangent(T) + TanAngleBias;
}
//----------------------------------------------------------------------------------
float IntegrateOcclusion(float2 UV0,
                          float2 snappedDUV,
                          float3 P,
                          float3 dPdu,
                          float3 dPdv,
                          inout float tanH)
{
    float ao = 0;
	
    // Compute a tangent vector for snappedDUV
    float3 T1 = TangentVector(snappedDUV, dPdu, dPdv);
    float tanT = BiasedTangent(T1);
    float sinT = TanToSin(tanT);
	
    float3 S = FetchEyePos(UV0 + snappedDUV);
    float tanS = Tangent(P, S);
    float sinS = TanToSin(tanS);
	
    float d2 = Length2(S - P);
    if ((d2 < R2) && (tanS > tanT))
    {
        // Compute AO between the tangent plane and the sample
        ao = Falloff(d2) * (sinS - sinT);
        // Update the horizon angle
        tanH = max(tanH, tanS);
    }
    return ao;
}
//----------------------------------------------------------------------------------
float HorizonOcclusion(float2 deltaUV,
		       float2 texelDeltaUV,
		       float2 UV0,
		       float3 P,
		       float numSteps,
		       float randStep,
		       float3 dPdu,
		       float3 dPdv)
{
	float ao = 0;
	
	// Randomize starting point within the first sample distance
	float2 UV = UV0 + SnapUVOffset(randStep * deltaUV);
	
	// Snap increments to pixels to avoid disparities between xy
	// and z sample locations and sample along a line
	deltaUV = SnapUVOffset(deltaUV);
	
	// Compute tangent vector using the tangent plane
	float3 T = deltaUV.x * dPdu + deltaUV.y * dPdv;
	float tanH = BiasedTangent(T);
	
#if SAMPLE_FIRST_STEP
	// Take a first sample between UV0 and UV0 + deltaUV
	float2 snappedUV = SnapUVOffset( randStep * deltaUV + texelDeltaUV);
	ao = IntegrateOcclusion(UV0, snappedUV, P, dPdu, dPdv, tanH);
	--numSteps;
#endif

	float sinH = tanH / sqrt(1.0f + tanH * tanH);
	for (float j = 1; j <= numSteps; ++j)
	{
		UV += deltaUV;
		float3 S = FetchEyePos(UV);
		float tanS = Tangent(P, S);
		float d2 = Length2(S - P);
		// Use a merged dynamic branch
		[branch]
		if ((d2 < R2) && (tanS > tanH))
		{
			// Accumulate AO between the horizon and the sample
			float sinS = tanS / sqrt(1.0f + tanS * tanS);
			ao += Falloff(d2) * (sinS - sinH);
			
			// Update the current horizon angle
			tanH = tanS;
			sinH = sinS;
		}
	}
	return ao;
}

//------------------------------------------------------------------------------
/**
*/
void
psMain(in float4 Position : SV_POSITION0,
	in float2 UV : TEXCOORD0,
	out float2 AO : SV_TARGET0) 
{

	float2 pixelSize = GetPixelSize(DepthBuffer);
	float2 screenUv = psComputeScreenCoord(Position.xy, pixelSize.xy);
	float3 P = FetchEyePos(screenUv);
	
	float3 rand = RandomMap.Sample(PointWrapSampler, Position.xy / RANDOM_TEXTURE_WIDTH);
	
	float2 rayRadiusUv = 0.5 * 	R * FocalLength / P.z;
	float rayRadiusPix = rayRadiusUv.x * AOResolution.x;
	if (rayRadiusPix < 1) AO = 1.0f;
	float numSteps;
	float2 stepSize;
	ComputeSteps(stepSize, numSteps, rayRadiusPix, rand.z);
	
	float3 Pr, Pl, Pt, Pb;
	Pr = FetchEyePos(screenUv + float2(InvAOResolution.x, 0));
	Pl = FetchEyePos(screenUv + float2(-InvAOResolution.x, 0));
	Pt = FetchEyePos(screenUv + float2(0, InvAOResolution.y));
	Pb = FetchEyePos(screenUv + float2(0, -InvAOResolution.y));
	
	float3 dPdu = MinDiff(P, Pr, Pl);
	float3 dPdv = MinDiff(P, Pt, Pb) * (AOResolution.y * InvAOResolution.x);
	float ao = 0;
	float d;
	float alpha = 2.0 * M_PI / NUM_DIRECTIONS;
	
	for (d = 0; d < NUM_DIRECTIONS; ++d)
	{
		float angle = alpha * d;
		float2 dir = RotateDirections(float2(cos(angle), sin(angle)), rand.xy);
		float2 deltaUV = dir * stepSize.xy;
		float2 texelDeltaUV = dir * InvAOResolution;
		ao += HorizonOcclusion(deltaUV, texelDeltaUV, screenUv, P, numSteps, rand.z, dPdu, dPdv);
	}
	
	ao = ao / NUM_DIRECTIONS * Strength;
	AO = float2(ao, P.z);
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