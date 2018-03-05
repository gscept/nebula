//------------------------------------------------------------------------------
//  hbao.fx
//  (C) 2011 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"
#include "lib/techniques.fxh"


vec2 UVToViewA = vec2(0.0f, 0.0f);
vec2 AOResolution = vec2(0.0f, 0.0f);
vec2 UVToViewB = vec2(0.0f, 0.0f);
vec2 InvAOResolution = vec2(0.0f, 0.0f);
float MaxRadiusPixels = 0.0f;	
float TanAngleBias = 0.0f;
float NegInvR2 = 0.0f;	
float Strength = 0.0f;
float R = 0.0f;
float R2 = 0.0f;

sampler2D DepthBuffer;
sampler2D RandomMap;

samplerstate ClampSampler
{
	Samplers = { DepthBuffer };
	Filter = Point;
	AddressU = Clamp;
	AddressV = Clamp;
	//BorderColor = { 0, 0, 0, 0 };
};

samplerstate WrapSampler
{
	Samplers = { RandomMap };
	Filter = Point;
	AddressU = Wrap;
	AddressV = Wrap;
};

#define RANDOM_TEXTURE_WIDTH 4
#define NUM_STEPS 6.0f
#define NUM_DIRECTIONS 8
#define M_PI 3.14159265358979323846f
#define SAMPLE_FIRST_STEP 1

state HBAOState
{
	CullMode = Back;
	DepthEnabled = true;
	DepthWrite = false;
	DepthFunc = Always;
};

//------------------------------------------------------------------------------
/**
*/
shader
void
vsMain(in vec3 position,
	[slot=2] in vec2 uv,
	out vec2 UV) 
{
	gl_Position = vec4(position, 1);
	UV = FlipY(uv);
}

//----------------------------------------------------------------------------------
vec2 RotateDirections(vec2 Dir, vec2 CosSin)
{
	return vec2(Dir.x*CosSin.x - Dir.y*CosSin.y,
               Dir.x*CosSin.y + Dir.y*CosSin.x);
}
//----------------------------------------------------------------------------------
void ComputeSteps(inout vec2 stepSizeUv, inout float numSteps, float rayRadiusPix, float rand)
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
		numSteps = max(numSteps, 1.0f);
		stepSizePix = MaxRadiusPixels / numSteps;	
	}
	// Step size in uv space
	stepSizeUv = stepSizePix * InvAOResolution;
}
//----------------------------------------------------------------------------------
float InvLength(vec2 v)
{
	return rsqrt(dot(v,v));
}
//----------------------------------------------------------------------------------
float Tangent(vec3 P, vec3 S)
{
	return (P.z - S.z) * InvLength(S.xy - P.xy);
}
//----------------------------------------------------------------------------------
vec3 UVToEye(vec2 uv, float eye_z)
{
	uv = UVToViewA * uv + UVToViewB;
	return vec3(uv * eye_z, eye_z);
}
//----------------------------------------------------------------------------------
vec3 FetchEyePos(vec2 uv)
{
	float z = textureLod(DepthBuffer, uv, 0).r;
	return UVToEye(uv, z);
}
//----------------------------------------------------------------------------------
float Length2(vec3 v)
{
    return dot(v, v);
}
//----------------------------------------------------------------------------------
vec3 MinDiff(vec3 P, vec3 Pr, vec3 Pl)
{
    vec3 V1 = Pr - P;
    vec3 V2 = P - Pl;
    return (Length2(V1) < Length2(V2)) ? V1 : V2;
}
//----------------------------------------------------------------------------------
float Falloff(float d2)
{
    // 1 scalar mad instruction
    return d2 * NegInvR2 + 1.0f;
}
//----------------------------------------------------------------------------------
vec2 SnapUVOffset(vec2 uv)
{
    return round(uv * AOResolution) * InvAOResolution;
}
//----------------------------------------------------------------------------------
float TanToSin(float x)
{
    return x * rsqrt(x*x + 1.0f);
}
//----------------------------------------------------------------------------------
vec3 TangentVector(vec2 deltaUV, vec3 dPdu, vec3 dPdv)
{
    return deltaUV.x * dPdu + deltaUV.y * dPdv;
}
//----------------------------------------------------------------------------------
float TangentSingle(vec3 T)
{
    return -T.z * InvLength(T.xy);
}
//----------------------------------------------------------------------------------
float BiasedTangent(vec3 T)
{
    // Do not use atan() because it gets expanded by fxc to many math instructions
    return TangentSingle(T) + TanAngleBias;
}
//----------------------------------------------------------------------------------
float IntegrateOcclusion(vec2 UV0,
                          vec2 snappedDUV,
                          vec3 P,
                          vec3 dPdu,
                          vec3 dPdv,
                          inout float tanH)
{
    float ao = 0;
	
    // Compute a tangent vector for snappedDUV
    vec3 T1 = TangentVector(snappedDUV, dPdu, dPdv);
    float tanT = BiasedTangent(T1);
    float sinT = TanToSin(tanT);
	
    vec3 S = FetchEyePos(UV0 + snappedDUV);
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
float HorizonOcclusion(vec2 deltaUV,
		       vec2 texelDeltaUV,
		       vec2 UV0,
		       vec3 P,
		       float numSteps,
		       float randStep,
		       vec3 dPdu,
		       vec3 dPdv)
{
	float ao = 0;
	
	// Randomize starting point within the first sample distance
	vec2 UV = UV0 + SnapUVOffset(randStep * deltaUV);
	
	// Snap increments to pixels to avoid disparities between xy
	// and z sample locations and sample along a line
	deltaUV = SnapUVOffset(deltaUV);
	
	// Compute tangent vector using the tangent plane
	vec3 T = deltaUV.x * dPdu + deltaUV.y * dPdv;
	float tanH = BiasedTangent(T);
	
#if SAMPLE_FIRST_STEP
	// Take a first sample between UV0 and UV0 + deltaUV
	vec2 snappedUV = SnapUVOffset( randStep * deltaUV + texelDeltaUV);
	ao = IntegrateOcclusion(UV0, snappedUV, P, dPdu, dPdv, tanH);
	--numSteps;
#endif

	float sinH = tanH / sqrt(1.0f + tanH * tanH);
	for (float j = 1; j <= numSteps; ++j)
	{
		UV += deltaUV;
		vec3 S = FetchEyePos(UV);
		float tanS = Tangent(P, S);
		float d2 = Length2(S - P);
		// Use a merged dynamic branch
		
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
shader
void
psMain(in vec2 UV,	
	[color0] out vec2 AO) 
{	
	vec3 P = FetchEyePos(UV);
	vec4 rand = textureLod(RandomMap, gl_FragCoord.xy / RANDOM_TEXTURE_WIDTH, 0);
	
	vec2 rayRadiusUv = 0.5 * R * FocalLength.xy / P.z;
	float rayRadiusPix = rayRadiusUv.x * AOResolution.x;
	if (rayRadiusPix < 1)
	{
		AO = vec2(0);
		return;
	}
	float numSteps;
	vec2 stepSize;
	ComputeSteps(stepSize, numSteps, rayRadiusPix, rand.z);
	
	vec3 Pr, Pl, Pt, Pb;
	Pr = FetchEyePos(UV + vec2(InvAOResolution.x, 0));
	Pl = FetchEyePos(UV + vec2(-InvAOResolution.x, 0));
	Pt = FetchEyePos(UV + vec2(0, InvAOResolution.y));
	Pb = FetchEyePos(UV + vec2(0, -InvAOResolution.y));
	
	vec3 dPdu = MinDiff(P, Pr, Pl);
	vec3 dPdv = MinDiff(P, Pt, Pb) * (AOResolution.y * InvAOResolution.x);
	float ao = 0;
	float d;
	float alpha = 2.0 * M_PI / NUM_DIRECTIONS;
	
	for (d = 0; d < NUM_DIRECTIONS; ++d)
	{
		float angle = alpha * d;
		vec2 dir = RotateDirections(vec2(cos(angle), sin(angle)), rand.xy);
		vec2 deltaUV = dir * stepSize.xy;
		vec2 texelDeltaUV = dir * InvAOResolution;
		ao += HorizonOcclusion(deltaUV, texelDeltaUV, UV, P, numSteps, rand.z, dPdu, dPdv);
	}
	
	ao = ao / NUM_DIRECTIONS * Strength;
	AO = vec2(ao, P.z);	
}

//------------------------------------------------------------------------------
/**
*/
PostEffect(vsMain(), psMain(), HBAOState);
