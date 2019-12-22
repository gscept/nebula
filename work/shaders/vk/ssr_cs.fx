//------------------------------------------------------------------------------
//  ssr_cs.fx
//  (C) 2015 - 2019 Fredrik Lindahl & Gustav Sterbrant
//
// TraceScreenSpaceRay is inspired by the work by
// Morgan McGuire and Michael Mara, Efficient GPU Screen-Space Ray Tracing, Journal of Computer Graphics Techniques (JCGT), vol. 3, no. 4, 73-85, 2014
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"
#include "lib/shared.fxh"

write rgba16f image2D ReflectionBuffer;

varblock SSRBlock
{
	mat4 ViewToTextureSpace = mat4(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1);
};

const float zThickness = 0.35; // How thick is each depth fragment? higher value yields some wierd smudging at edges of reflection, but thinner z means we might miss some geometry. This should essentially be the average thickness of the geometry. to do dynamically would be a nightmare however...

// DDA tracing constants
const float pixelStride = 1; // lets you skip pixels during iteration. Larger stride means longer rays with same number of samples.
const float maxSteps = 16.0f; //Less steps means shorter reflections, but better performance
const float maxDistance = 100; //Not orthogonal to max steps. reflection of surfaces far away in world space are prone to rapid shifts with only a slight change in camera positioning, which can lead to objectionable temporal flicker. Setting this parameter can help mitigate that.
const float jitter = 0.30; // Number between 0 and 1 for how far to bump the ray in stride units to conceal banding artifacts

// Raymarching constants
const float maxMarchingSteps = 80;
const float maxMarchingDistance = 100;
const float marchingStepSize = 0.10;
const float rayOffset = 0.5;
const float distanceCutoff = 200; // Rays won't be evaluated beyond this distance from the camera

samplerstate LinearState
{
	//Samplers = {DepthBuffer, SpecularBuffer, NormalBuffer, AlbedoBuffer};
	Filter = Linear;
	BorderColor = {0,0,0,0};
	AddressU = Border;
	AddressV = Border;
};

samplerstate NoFilterState
{
	//Samplers = {DepthBuffer, SpecularBuffer, NormalBuffer, AlbedoBuffer};
	Filter = Point;
	BorderColor = {0,0,0,0};
	AddressU = Border;
	AddressV = Border;
};

float DistanceSquared(vec2 a, vec2 b)
{
	a -= b;
	return dot(a, a);
}

bool TraceScreenSpaceRay(in vec3 rayOrigin,
						 in vec3 rayDirection,
						 out vec2 hitTexCoord
						 )
{
	const float nearPlane = -FocalLengthNearFar.z;

	// Clip to the near plane
	const float rayLength = ((rayOrigin.z + rayDirection.z * maxDistance) > nearPlane) ? (nearPlane - rayOrigin.z) / rayDirection.z : maxDistance;

	const vec3 rayEnd = rayOrigin + rayDirection * rayLength;
	
	// TEMP: the screen-pixel-projection matrix should be precomputed
	const vec2 ScreenSize = imageSize(ReflectionBuffer);
	const vec2 InvScreenSize = vec2(1.0f) / ScreenSize;

	const float sx = ScreenSize[0] / 2.0f;
	const float sy = ScreenSize[1] / 2.0f;

	const mat4 scrScale = mat4(sx,   0.0,  0.0,  0.0,
							   0.0,  sy,   0.0,  0.0,
							   0.0,  0.0,  1.0,  0.0,
							   sx,   sy,   0.0,  1.0 );

	const mat4 proj = scrScale * Projection;

	// Project into homogeneous clip space
	const vec4 H0 = proj * vec4( rayOrigin, 1.0f);
	const vec4 H1 = proj * vec4( rayEnd, 1.0f);
	
	float k0 = 1.0 / H0.w;
	const float k1 = 1.0 / H1.w;

	// The interpolated homogeneous version of the camera-space points
	vec3 Q0 = rayOrigin * k0;
	vec3 Q1 = rayEnd * k1;

	// Screen-space endpoints
	vec2 P0 = (H0.xy * k0);
    vec2 P1 = (H1.xy * k1);

    // If the line is degenerate, make it cover at least one pixel
    // to avoid handling zero-pixel extent as a special case later
    P1 += (DistanceSquared(P0, P1) < 0.0001) ? 0.01 : 0.0;

	vec2 delta = P1 - P0;

	// Permute so that the primary iteration is in x to collapse
	// all quadrant-specific DDA cases later
	const bool permute = abs(delta.x) < abs(delta.y);
	
    if (permute)
	{
		// This is a more-vertical line
		delta = delta.yx;
		P0 = P0.yx;
		P1 = P1.yx;
	}

	const float stepDir = sign(delta.x);
	const float invdx = stepDir / delta.x;

	// Track the derivatives of Q and k
	vec3  dQ = (Q1 - Q0) * invdx;
	float dk = (k1 - k0) * invdx;
	vec2  dP = vec2(stepDir, delta.y * invdx);

	// Scale derivatives by the desired pixel stride and then
	// offset the starting values by the jitter fraction
    const float stride = 1.0f + pixelStride;
	dP *= stride;
	dQ *= stride;
	dk *= stride;

	P0 += dP * jitter;
	Q0 += dQ * jitter;
	k0 += dk * jitter;

	// Track ray step and derivatives in a float4 to parallelize
	vec4 PQk = vec4( P0.xy, Q0.z, k0);
	vec4 dPQk = vec4( dP.xy, dQ.z, dk);
	
    const float end = P1.x * stepDir;
	
    const float zBoundsMin = min(rayEnd.z, rayOrigin.z);
    const float zBoundsMax = max(rayEnd.z, rayOrigin.z);

	float i;
	float prevZMax = (dPQk.z * 0.5 + PQk.z) / (dPQk.w * 0.5 + PQk.w);
	float zMin = prevZMax;
    float zMax = prevZMax;
    float depth = prevZMax + 1e4;

	PQk += dPQk;

	for(i = 0;
	    ((PQk.x * stepDir) <= end) &&
	    (i < maxSteps) &&
	    ((zMax < depth - zThickness) || (zMin > depth));
	   i += 1.0f)
	{
		PQk += dPQk;
		
		zMin = prevZMax;
		zMax = (dPQk.z * 0.5 + PQk.z) / (dPQk.w * 0.5 + PQk.w);
		zMax = clamp(zMax, zBoundsMin, zBoundsMax);
		prevZMax = zMax;
		if (zMin > zMax)
		{
			//Swap
			float t = zMin;
			zMin = zMax;
			zMax = t;
		}

		hitTexCoord = permute ? PQk.yx : PQk.xy;

		float pixelDepth = fetch2D(DepthBuffer, Basic2DSampler, ivec2(hitTexCoord), 0).r;
        depth = -LinearizeDepth(pixelDepth);
	}

	// viewspace hitpoint
	// PQk -= dPQk;
	// i -= 1.0f;
	// Q0.xy += dQ.xy * i;
	// Q0.z = PQk.z;
	// hitPoint = Q0 / PQk.w;

	return (i < maxSteps) &&
		   (zMax >= depth - zThickness) && (zMin < depth);
}

bool RaymarchScreenSpace(in vec3 rayOrigin,
						 in vec3 rayDirection,
						 out vec2 hitTexCoord)
{
	if (rayOrigin.z < -distanceCutoff)
		return false;

	vec3 reflection = (rayDirection * rayOffset) + rayOrigin;
	float rayLength = length(((rayDirection * marchingStepSize) * maxMarchingSteps) + reflection);

	const vec2 ScreenSize = imageSize(ReflectionBuffer);

	// calculate amount of steps to take
	float numSteps = maxMarchingSteps;
	if( rayLength > maxMarchingDistance)
	{
		float ratio = min(rayLength, maxMarchingDistance) / maxMarchingDistance;
		numSteps *= ratio; // No need to floor!
	}
	
	vec4 clip;
	vec2 projCoord;
	float bufferDepth = 0;
	float reflectionDepth = 0;

	for (float i = 0; i < numSteps; i++)
	{
		reflection += rayDirection * marchingStepSize;
		clip = Projection * vec4(reflection, 1);
		clip /= clip.w;
		projCoord = (clip.xy + 1.0) / 2.0;
		bufferDepth = LinearizeDepth(fetch2D(DepthBuffer, Basic2DSampler, ivec2(projCoord * ScreenSize), 0).r);
		reflectionDepth = -reflection.z;


		if (bufferDepth < reflectionDepth - zThickness)
		{
			float delta = reflectionDepth - bufferDepth;
			if (delta < length(rayDirection))
			{
				hitTexCoord = projCoord * ScreenSize;
				return true;
			}
		}
	}
	return false;
}

//------------------------------------------------------------------------------
/**
*/

[localsizex] = 32
[localsizey] = 32
shader
void
csMain()
{
	ivec2 location = ivec2(gl_GlobalInvocationID.xy);
	const vec2 screenSize = imageSize(ReflectionBuffer);
	if (location.x >= screenSize.x || location.y > screenSize.y)
		return;

	vec2 invScreenSize = vec2(1.0/screenSize.x, 1.0/screenSize.y);

	vec2 UV = location * invScreenSize;

	vec4 n = sample2DLod(NormalBuffer, LinearState, UV, 0);
	vec3 viewSpaceNormal = normalize((transpose(inverse(mat3(View))) * normalize(n.xyz)).xyz);
	float pixelDepth = fetch2D(DepthBuffer, Basic2DSampler, location, 0).r;
	
	vec3 rayOrigin = PixelToView(UV, pixelDepth).xyz;

	vec3 viewDir = normalize(rayOrigin);
	vec3 reflectionDir = (reflect(viewDir, viewSpaceNormal));

	vec2 hitTexCoord = vec2(0.0,0.0);
	vec4 reflectionColor = vec4(0, 0, 0, 0);
	if (TraceScreenSpaceRay(rayOrigin, reflectionDir, hitTexCoord))
	{
		reflectionColor = sample2DLod(AlbedoBuffer, LinearState, (hitTexCoord * invScreenSize), 0);
	}
	else
	{
		//TODO: Get fall-back value.
	}
	
	imageStore(ReflectionBuffer, location, reflectionColor);
}

//------------------------------------------------------------------------------
/**
*/
program SSR [ string Mask = "Alt0"; ]
{
	ComputeShader = csMain();
};
