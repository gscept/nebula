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

// this is used to keep track of how many lights we have active
group(BATCH_GROUP) constant VolumeFogUniforms [ string Visibility = "CS"; ]
{
	int Downscale;
	uint NumLocalVolumes;
	uint NumClusters;
	vec3 GlobalAbsorption;
	float GlobalTurbidity;
};

// contains amount of lights, and the index of the light (pointing to the indices in PointLightList and SpotLightList), to output
group(BATCH_GROUP) rw_buffer LightIndexLists [ string Visibility = "CS"; ]
{
	uint PointLightCountList[16384];
	uint PointLightIndexList[16384 * MAX_LIGHTS_PER_CLUSTER];
	uint SpotLightCountList[16384];
	uint SpotLightIndexList[16384 * MAX_LIGHTS_PER_CLUSTER];
};

group(BATCH_GROUP) rw_buffer LightLists [ string Visibility = "CS"; ]
{
	SpotLight SpotLights[1024];
	SpotLightProjectionExtension SpotLightProjection[256];
	SpotLightShadowExtension SpotLightShadow[16];
	PointLight PointLights[1024];
};

readwrite rgba16f image2D Lighting;
//------------------------------------------------------------------------------
/**
*/
vec3
LocalLightsFog(
	uint idx,
	float globalTurbidity,
	vec3 globalFogColor,
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
			light += li.color * att * globalTurbidity * globalFogColor;
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

			light += intensity * att * li.color * shadowFactor * globalTurbidity * globalFogColor;
		}
	}
	return light;
}

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 64
shader
void csVolumeFog()
{
	vec2 coord = vec2(gl_GlobalInvocationID.xy);
	ivec2 upscaleCoord = ivec2(gl_GlobalInvocationID.xy * Downscale);
	float depth = fetch2D(DepthBuffer, PosteffectUpscaleSampler, upscaleCoord, 0).r;

	// find last point to march
	vec4 viewPos = PixelToView(coord * InvFramebufferDimensions, depth);
	vec3 eye = vec3(0, 0, 0);

	// construct a ray, beginning at eye and going from eye through worldPoint
	vec3 rayStart = eye;
	vec3 rayDirection = normalize(viewPos.xyz - eye);

	const float oneDivFogSteps = 1 / float(VOLUME_FOG_STEPS);

	// ray march!
	vec3 light = vec3(0, 0, 0);
	uint numSteps = 0;
	vec2 seed = coord * InvFramebufferDimensions;
	vec3 rnd = vec3(hash12(seed) + hash12(seed + 0.59374) - 0.5);
	float stepSize = (viewPos.z + (rnd.z * 1.0f))  / VOLUME_FOG_STEPS;
	vec3 rayOffset = vec3(0, 0, 0);
	
	for (int i = 0; i < VOLUME_FOG_STEPS; i++, numSteps++)
	{
		// construct sample position
		vec3 samplePos = rayStart - rayOffset;

		// offset ray offset with some noise
		rayOffset += stepSize * (rayDirection);
		if (viewPos.z > samplePos.z)
		{
			break;
		}

		float shadowFactor = 1.0f;
		if (FlagSet(GlobalLightFlags, USE_SHADOW_BITFLAG))
		{
			vec4 shadowPos = CSMShadowMatrix * vec4(samplePos, 1); // csm contains inversed view + csm transform
			shadowFactor = CSMPS(shadowPos,	GlobalLightShadowBuffer);
			shadowFactor = lerp(1.0f, shadowFactor, 1);
		}

		// calculate 'global' fog
		vec3 atmo = Preetham(normalize(samplePos), -GlobalLightDirWorldspace.xyz, A, B, C, D, E, Z) * GlobalLightColor.rgb;
		light += atmo * shadowFactor * GlobalAbsorption * GlobalTurbidity;

		// calculate cluster index
		uint3 index3D = CalculateClusterIndex(upscaleCoord / BlockSize, samplePos.z, InvZScale, InvZBias);
		uint idx = Pack3DTo1D(index3D, NumCells.x, NumCells.y);
		light += LocalLightsFog(idx, GlobalTurbidity, GlobalAbsorption, samplePos, rayDirection);
	}
	light /= numSteps;
	light = min(light, vec3(100));
	imageStore(Lighting, ivec2(coord), light.xyzx);
}

//------------------------------------------------------------------------------
/**
*/
program Cull [ string Mask = "Cull"; ]
{
	ComputeShader = csVolumeFog();
};

//------------------------------------------------------------------------------
/**
*/
program Render [ string Mask = "Render"; ]
{
	ComputeShader = csVolumeFog();
};