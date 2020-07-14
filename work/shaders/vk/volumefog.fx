//------------------------------------------------------------------------------
//  volumefog.fx
//  (C) 2020 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"
#include "lib/clustering.fxh"
#include "lib/lights_clustered.fxh"
#include "lib/preetham.fxh"

const uint VOLUME_FOG_STEPS = 32;

struct FogSphere
{
	vec3 position;
	float radius;
	vec3 absorption;
	float turbidity;
	float falloff;
};

struct FogBox
{
	vec3 bboxMin;
	float falloff;
	vec3 bboxMax;
	float turbidity;
	vec3 absorption;
	mat4 invTransform;
};

// increase if we need more decals in close proximity, for now, 128 is more than enough
#define MAX_FOGS_PER_CLUSTER 128

group(BATCH_GROUP) rw_buffer FogLists [ string Visibility = "CS"; ]
{
	FogSphere FogSpheres[128];
	FogBox FogBoxes[128];
};

// contains amount of lights, and the index of the light (pointing to the indices in PointLightList and SpotLightList), to output
group(BATCH_GROUP) rw_buffer FogIndexLists [ string Visibility = "CS"; ]
{
	uint FogSphereCountList[NUM_CLUSTER_ENTRIES];
	uint FogSphereIndexList[NUM_CLUSTER_ENTRIES * MAX_FOGS_PER_CLUSTER];
	uint FogBoxCountList[NUM_CLUSTER_ENTRIES];
	uint FogBoxIndexList[NUM_CLUSTER_ENTRIES * MAX_FOGS_PER_CLUSTER];
};

// this is used to keep track of how many lights we have active
group(BATCH_GROUP) constant VolumeFogUniforms [ string Visibility = "CS"; ]
{
	int Downscale;
	uint NumFogSpheres;
	uint NumFogBoxes;
	uint NumClusters;
	vec3 GlobalAbsorption;
	float GlobalTurbidity;
};

readwrite rgba16f image2D Lighting;
//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 64
shader
void csCull()
{
	uint index1D = gl_GlobalInvocationID.x;

	if (index1D > NumClusters)
		return;

	ClusterAABB aabb = AABBs[index1D];

	uint flags = 0;

	// update fog spheres
	uint numFogs = 0;
	for (uint i = 0; i < NumFogSpheres; i++)
	{
		const FogSphere fog = FogSpheres[i];
		if (TestAABBSphere(aabb, fog.position, fog.radius))
		{
			FogSphereIndexList[index1D * MAX_FOGS_PER_CLUSTER + numFogs] = i;
			numFogs++;
		}
	}
	FogSphereCountList[index1D] = numFogs;

	// update feature flags if we have any lights
	if (numFogs > 0)
		flags |= CLUSTER_FOG_SPHERE_BIT;

	// update fog boxes
	numFogs = 0;
	for (uint i = 0; i < NumFogBoxes; i++)
	{
		const FogBox fog = FogBoxes[i];
		if (TestAABBAABB(aabb, fog.bboxMin, fog.bboxMax))
		{
			FogBoxIndexList[index1D * MAX_FOGS_PER_CLUSTER + numFogs] = i;
			numFogs++;
		}
	}
	FogBoxCountList[index1D] = numFogs;

	// update feature flags if we have any lights
	if (numFogs > 0)
		flags |= CLUSTER_FOG_BOX_BIT;

	atomicOr(AABBs[index1D].featureFlags, flags);
}
//------------------------------------------------------------------------------
/**
*/
void
LocalFogVolumes(
	uint idx
	, vec3 viewPos
	, inout float turbidity
	, inout vec3 absorption)
{
	uint flag = AABBs[idx].featureFlags;
	if (CHECK_FLAG(flag, CLUSTER_FOG_SPHERE_BIT))
	{
		// add turbidity for local fogs
		uint count = FogSphereCountList[idx];
		for (int i = 0; i < count; i++)
		{
			uint lidx = FogSphereIndexList[idx * MAX_FOGS_PER_CLUSTER + i];
			FogSphere fog = FogSpheres[lidx];

			vec3 pos = fog.position - viewPos;
			float sd = (length(pos) - fog.radius);
			float falloff = pow(1.0f - sd, fog.falloff);

			if (falloff > fog.falloff)
			{
				turbidity += fog.turbidity;
				absorption *= fog.absorption;
			}
		}
	}

	if (CHECK_FLAG(flag, CLUSTER_FOG_BOX_BIT))
	{
		// add turbidity for local fogs
		uint count = FogBoxCountList[idx];
		for (int i = 0; i < count; i++)
		{
			uint lidx = FogBoxIndexList[idx * MAX_FOGS_PER_CLUSTER + i];
			FogBox fog = FogBoxes[lidx];

			vec3 localPos = (fog.invTransform * vec4(viewPos, 1.0f)).xyz;
			vec3 dist = vec3(0.5f) - abs(localPos.xyz);
			if (all(greaterThan(dist, vec3(0))))
			{
				vec3 q = abs(localPos) - vec3(1.0f);
				float sd = length(max(q, 0.0f)) + min(max(q.x, max(q.y, q.z)), 0.0f);
				float falloff = pow(1.0f - sd, fog.falloff);

				// todo, calculate distance field
				if (falloff > fog.falloff)
				{
					turbidity += fog.turbidity;
					absorption *= fog.absorption;
				}
			}
		}
	}
}


//------------------------------------------------------------------------------
/**
*/
vec3
LocalLightsFog(
	uint idx,
	vec3 viewPos,
	vec3 viewVec)
{
	vec3 light = vec3(0, 0, 0);
	uint flag = AABBs[idx].featureFlags;
	if (CHECK_FLAG(flag, CLUSTER_POINTLIGHT_BIT))
	{
		// shade point lights
		uint count = PointLightCountList[idx];
		for (int i = 0; i < count; i++)
		{
			uint lidx = PointLightIndexList[idx * MAX_LIGHTS_PER_CLUSTER + i];
			PointLight li = PointLights[lidx];
			
			vec3 lightDir = (li.position.xyz - viewPos);

			float lightDirLen = length(lightDir);
			float d2 = lightDirLen * lightDirLen;
			float factor = d2 / (li.position.w * li.position.w);
			float sf = saturate(1.0 - factor * factor);
			float att = (sf * sf) / max(d2, 0.0001);
			light += li.color * att;
		}
	}
	if (CHECK_FLAG(flag, CLUSTER_SPOTLIGHT_BIT))
	{
		uint count = SpotLightCountList[idx];
		for (int i = 0; i < count; i++)
		{
			uint lidx = SpotLightIndexList[idx * MAX_LIGHTS_PER_CLUSTER + i];
			SpotLightShadowExtension shadowExt;
			SpotLight li = SpotLights[lidx];

			if (li.shadowExtension != -1)
				shadowExt = SpotLightShadow[li.shadowExtension];

			// calculate attentuation and angle falloff, and just multiply by color
			vec3 lightDir = (li.position.xyz - viewPos);

			float lightDirLen = length(lightDir);
			float d2 = lightDirLen * lightDirLen;
			float factor = d2 / (li.position.w * li.position.w);
			float sf = saturate(1.0 - factor * factor);

			float att = (sf * sf) / max(d2, 0.0001);
			lightDir = lightDir * (1 / lightDirLen);

			float theta = dot(li.forward.xyz, lightDir);
			float intensity = saturate((theta - li.angleSinCos.y) * li.forward.w);

			float shadowFactor = 1.0f;
			if (FlagSet(li.flags, USE_SHADOW_BITFLAG))
			{
				// shadows
				vec4 shadowProjLightPos = shadowExt.projection * vec4(viewPos, 1.0f);
				shadowProjLightPos.xyz /= shadowProjLightPos.www;
				vec2 shadowLookup = shadowProjLightPos.xy * vec2(0.5f, -0.5f) + 0.5f;
				shadowLookup.y = 1 - shadowLookup.y;
				float receiverDepth = shadowProjLightPos.z;
				shadowFactor = GetInvertedOcclusionSpotLight(receiverDepth, shadowLookup, li.shadowExtension, shadowExt.shadowMap);
				shadowFactor = saturate(lerp(1.0f, saturate(shadowFactor), shadowExt.shadowIntensity));
			}

			light += intensity * att * li.color * shadowFactor;
		}
	}
	return light;
}

//------------------------------------------------------------------------------
/**
*/
vec3
GlobalLightFog(vec3 viewPos)
{
	float shadowFactor = 1.0f;
	if (FlagSet(GlobalLightFlags, USE_SHADOW_BITFLAG))
	{
		vec4 shadowPos = CSMShadowMatrix * vec4(viewPos, 1); // csm contains inversed view + csm transform
		shadowFactor = CSMPS(shadowPos, GlobalLightShadowBuffer);
		shadowFactor = lerp(1.0f, shadowFactor, 1);
	}

	// calculate 'global' fog
	vec3 atmo = Preetham(normalize(viewPos), GlobalLightDir.xyz, A, B, C, D, E, Z);
	return atmo * GlobalLightColor.rgb * shadowFactor;
}

//------------------------------------------------------------------------------
/**
	Compute fogging given a sampled fog intensity value from the depth
	pass and a fog color.
*/
float
Fog(float fogDepth)
{
	return (FogDistances.y - fogDepth) / (FogDistances.y - FogDistances.x);
}


//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 64
shader
void csRender()
{
	vec2 coord = vec2(gl_GlobalInvocationID.xy);
	ivec2 upscaleCoord = ivec2(gl_GlobalInvocationID.xy * Downscale);
	float depth = fetch2D(DepthBuffer, PosteffectUpscaleSampler, upscaleCoord, 0).r;
	vec2 seed = coord * InvFramebufferDimensions;

	if (depth == 1)
		depth = 0.9999f;

	// find last point to march
	vec4 viewPos = PixelToView(seed, depth);
	vec3 eye = vec3(0, 0, 0);

	// construct a ray, beginning at eye and going from eye through worldPoint
	vec3 rayStart = eye;
	vec3 viewVec = eye - viewPos.xyz;
	vec3 rayDirection = normalize(viewVec);

	const float oneDivFogSteps = 1 / float(VOLUME_FOG_STEPS);

	// ray march!
	vec3 light = vec3(0, 0, 0);
	uint numSteps = 0;
	vec3 rnd = vec3(hash12(seed) + hash12(seed + 0.59374) - 0.5);
	float stepSize = (viewPos.z - (rnd.z + rnd.x + rnd.y)) * oneDivFogSteps;
	float stepLen = stepSize;
	vec3 rayOffset = rayDirection;

	// calculate global fog, which should be a factor of the distance and the global turbidity
	float fogModulate = ((FocalLengthNearFar.w + 0.001f) - length(viewVec)) / FocalLengthNearFar.w;
	float globalTurbidity = GlobalTurbidity * (1.0f - fogModulate);
	float totalTurbidity = 0.0f;

	/*
		Single scattering equation as presented here:

		- Li is the light, 
		- Tr is the transmission
		- Ls is the light at the surface
		- Lscat is the scattered light
		- S is the sample count
		- x is the camera point
		- xs is the sample point on the surface (at viewPos)
		- xt is the sample point when ray marching (at samplePos)
		- w0 is the incident angle at the surface point
		- wi is the incident angle for the sample point
		- ot(x) is the turbidity function at point x

		Li(x, wi) = Tr(x, xs) * Ls(xs, w0) + integral[0..S]{ Tr(x, xt) * ot(x) * Lscat(xt, wi) dt

		Tr(x, xs) = exp(- integral[0..S]{ ot(x)dt })
		Lscat is just a normal light function but for albedo surfaces in our case, also samples shadows

		The loop approximates the integral in the first expression, the integration with the surface Li happens in combine.fx
	*/

	for (int i = 0; i < VOLUME_FOG_STEPS; i++)
	{
		// construct sample position
		vec3 samplePos = rayStart + stepLen * rayDirection;

		// offset ray offset with some noise
		stepLen += stepSize;
		
		// break if we went too far
		if (samplePos.z < viewPos.z)
		{
			break;
		}

		// calculate cluster index
		uint3 index3D = CalculateClusterIndex(upscaleCoord / BlockSize, samplePos.z, InvZScale, InvZBias);
		uint idx = Pack3DTo1D(index3D, NumCells.x, NumCells.y);

		// sample local fog volumes
		float localTurbidity = 0.0f;
		vec3 localAbsorption = GlobalAbsorption;
		LocalFogVolumes(idx, samplePos, localTurbidity, localAbsorption);

		// local turbidity is the result of our volumes + global turbidity increment
		localTurbidity = (localTurbidity + globalTurbidity) * oneDivFogSteps;

		// calculate the total turbidity, required for Tr(x, xt)
		totalTurbidity += localTurbidity;

		// equation tells us Tr(x, xt) * ot(x) is the weight used for each scattered light sample
		float weight = exp(-totalTurbidity) * localTurbidity;

		// this is the Lscat calculation
		light += GlobalLightFog(samplePos) * weight * localAbsorption;
		light += LocalLightsFog(idx, samplePos, rayDirection) * weight * localAbsorption;
	}
	float weight = (exp(-totalTurbidity));

	// pass weight to combine pass so we can handle the Tr(x, xs) * Ls(xs, w0) 
	// part of the equation (how much of the surface is visible)
	imageStore(Lighting, ivec2(coord), vec4(light.xyz, weight));
}

//------------------------------------------------------------------------------
/**
*/
program Cull [ string Mask = "Cull"; ]
{
	ComputeShader = csCull();
};

//------------------------------------------------------------------------------
/**
*/
program Render [ string Mask = "Render"; ]
{
	ComputeShader = csRender();
};