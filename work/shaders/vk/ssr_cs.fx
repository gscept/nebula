//------------------------------------------------------------------------------
//  ssr_cs.fx
//  (C) 2019 Fredrik Lindahl
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"
#include "lib/shared.fxh"

// sampler2D ColorBuffer;
write rgba16f image2D ReflectionBuffer;

varblock SSRBlock
{
	mat4 ViewToTextureSpace = mat4(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1);
};

const float zThickness = 0.25; // How thick is each depth fragment? higher value yields some wierd smudging at edges of reflection, but thinner z means we might miss some geometry. This should essentially be the average thickness of the geometry. to do dynamically would be a nightmare however...
const float pixelStride = 0; // lets you skip pixels during iteration. Larger stride means longer rays with same number of samples.
const float maxSteps = 100; //Less steps means shorter reflections, but better performance
const float maxDistance = 1000; //Not orthogonal to max steps. reflection of surfaces far away in world space are prone to rapid shifts with only a slight change in camera positioning, which can lead to objectionable temporal flicker. Setting this parameter can help mitigate that.
const float jitter = 1.0f;

samplerstate LinearState
{
	//Samplers = {DepthBuffer, SpecularBuffer, NormalBuffer, AlbedoBuffer};
	Filter = Point;
	BorderColor = {0,0,0,0};
	AddressU = Border;
	AddressV = Border;
};

float distanceSquared(vec2 a, vec2 b)
{
	a -= b;
	return dot(a, a);
}

bool TraceScreenSpaceRay(in vec3 rayOrigin,
						 in vec3 rayDirection,
						 out vec2 hitTexCoord,
						 out vec3 hitPoint
						 )
{
	float nearPlane = -0.05f;

	// Clip to the near plane
	float rayLength = ((rayOrigin.z + rayDirection.z * maxDistance) > nearPlane) ? (nearPlane - rayOrigin.z) / rayDirection.z : maxDistance;

	vec3 rayEnd = rayOrigin + rayDirection * rayLength;

	// Project into homogeneous clip space
	vec4 H0 = ViewToTextureSpace * vec4( rayOrigin, 1.0f);
	vec4 H1 = ViewToTextureSpace * vec4( rayEnd, 1.0f);

	float k0 = 1.0 / H0.w;
	float k1 = 1.0 / H1.w;

	// The interpolated homogeneous version of the camera-space points
	vec3 Q0 = rayOrigin * k0;
	vec3 Q1 = rayEnd * k1;

	// Screen-space endpoints
	vec2 P0 = H0.xy * k0;
	vec2 P1 = H1.xy * k1;

	// If the line is degenerate, make it cover at least one pixel
	// to avoid handling zero-pixel extent as a special case later
	P1 += (distanceSquared(P0, P1) < 0.0001) ? 0.01 : 0.0;

	vec2 delta = P1 - P0;

	// Permute so that the primary iteration is in x to collapse
	// all quadrant-specific DDA cases later
	bool permute = false;

	if (abs(delta.x) < abs(delta.y))
	{
		// This is a more-vertical line
		permute = true;
		delta = delta.yx;
		P0 = P0.yx;
		P1 = P1.yx;
	}

	float stepDir = sign(delta.x);
	float invdx = stepDir / delta.x;

	// Track the derivatives of Q and k
	vec3  dQ = (Q1 - Q0) * invdx;
	float dk = (k1 - k0) * invdx;
	vec2  dP = vec2(stepDir, delta.y * invdx);

	// Scale derivatives by the desired pixel stride and then
	// offset the starting values by the jitter fraction
	dP *= pixelStride;
	dQ *= pixelStride;
	dk *= pixelStride;
	P0 += dP * jitter;
	Q0 += dQ * jitter;
	k0 += dk * jitter;

	float i;
	float zA = 0.0;
	float zB = 0.0;

	// Track ray step and derivatives in a float4 to parallelize
	vec4 pqk = vec4( P0, Q0.z, k0);
	vec4 dPQK = vec4( dP, dQ.z, dk);
	bool intersect = false;

	float end = P1.x * stepDir;

	for(i = 0; i < maxSteps && intersect == false; i++)
	{
		pqk += dPQK;

		zA = zB;
		zB = (dPQK.z * 0.5 + pqk.z) / (dPQK.w * 0.5 + pqk.w);
		if (zB > zA)
		{
			//Swap
			float t = zA;
			zA = zB;
			zB = t;
        }

		hitTexCoord = permute ? pqk.yx : pqk.xy;

		// TEMP
		vec2 ScreenSize = vec2(1680, 1050);// imageSize(AlbedoBuffer);

		//This doesn't seem necessary anymore...
		// || pqk.x * stepDir >= end
		if(hitTexCoord.x > ScreenSize.x || hitTexCoord.y > ScreenSize.y || hitTexCoord.x < 0 || hitTexCoord.y < 0)
		{
			break;
		}

		float depth = -sample2DLod(DepthBuffer, LinearState, hitTexCoord, 0).r;
		intersect = (zB >= depth - zThickness) && (zA <= depth);
	}
	
	/*
	// Binary search refinement
	if( stride > 1.0 && intersect)
	{
		pqk -= dPQK;
		dPQK /= stride;

		float originalStride = stride * 0.5;
		float stride = originalStride;
		zA = pqk.z / pqk.w;
		zB = zA;
		float j;
		for(j=0; j< 64.0f; j++)
		{
			pqk += dPQK * stride;
			zA = zB;
			zB = (dPQK.z * -0.5 + pqk.z) / (dPQK.w * -0.5 + pqk.w);
			if (zB > zA)
			{
				//Swap
				float t = zA;
				zA = zB;
				zB = t;
			}

			hitTexCoord = permute ? pqk.yx : pqk.xy;

			originalStride *= 0.5;
			stride = (zB <= (-texelFetch(depthMap, ivec2(hitTexCoord), 0).r)) ? -originalStride : originalStride;
		}
	}
	*/

	Q0.xy += dQ.xy * i;
	Q0.z = pqk.z;
	hitPoint = Q0 / pqk.w;
	//iterationCount = i;

	return intersect;
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
	const vec2 screenSize = vec2(1680, 1050);
	if (location.x >= screenSize.x || location.y > screenSize.y)
		return;

	vec2 invScreenSize = vec2(1.0/screenSize.x, 1.0/screenSize.y);

	vec2 ScreenCoord = location * invScreenSize;
	vec2 UV = ScreenCoord + vec2(0.5f) * invScreenSize;
	
	vec3 viewSpaceNormal = normalize((View * sample2DLod(NormalBuffer, LinearState, UV, 0)).xyz);
	float pixelDepth = sample2DLod(DepthBuffer, LinearState, UV, 0).r;

	//Calculate world pos
	vec4 clipSpaceLocation;
	clipSpaceLocation.xy = UV * 2.0f - 1.0f;
	clipSpaceLocation.z = 1.0f;
	clipSpaceLocation.w = 1.0f;
	vec4 homogenousLocation = InvProjection * clipSpaceLocation;
	vec3 viewSpacePosition = homogenousLocation.xyz;

	vec3 rayOrigin = viewSpacePosition * pixelDepth;

	vec3 viewDir = normalize(rayOrigin.xyz);

	//Reflect vector against normal
	vec3 reflectionDir = (reflect(viewDir, viewSpaceNormal));

	vec2 hitTexCoord = vec2(0.0,0.0);
	vec3 hitPoint = vec3(0.0,0.0,0.0);

	vec4 specular = sample2DLod(SpecularBuffer, LinearState, UV, 0);
	// float roughness = specularAndRoughness.w;
	vec4 color = sample2DLod(AlbedoBuffer, LinearState, UV, 0);

	vec3 reflectionColor = vec3(1.0, 0, 0);
	
	if (TraceScreenSpaceRay(rayOrigin.xyz, reflectionDir, hitTexCoord, hitPoint))
	{
		reflectionColor = sample2DLod(AlbedoBuffer, LinearState, hitTexCoord, 0).xyz;
	}
	else
	{
		//Get fall-back value.
		//GetBlendedCubeMapColor(reflectionDir, reflectionColor);
		
		// vec3 worldPosition = (InvView * vec4(rayOrigin, 1.0f)).xyz;
		// vec3 worldDirection = normalize(mat3(InvView) * reflectionDir);
		// GetBlendedAndParallaxCorrectedCubeMapColor(roughness, worldPosition, worldDirection, reflectionColor);
	}
	//vec4 finalColor = vec4((color.rgb * (1-specular.r)) + (reflectionColor.rgb * specular.r), color.a);
	vec4 finalColor = vec4(reflectionColor.xyz, 1.0f);
	imageStore(ReflectionBuffer, location, finalColor);
}

//------------------------------------------------------------------------------
/**
*/
program SSR [ string Mask = "Alt0"; ]
{
	ComputeShader = csMain();
};
