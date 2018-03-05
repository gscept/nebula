//------------------------------------------------------------------------------
//  reflectionprojector.fx
//  (C) 2015 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"
#include "lib/shared.fxh"

sampler2D NormalMap;
sampler2D SpecularMap;
sampler2D AlbedoMap;
sampler2D DepthMap;
readwrite r16f image2D DistanceFieldWeightMap;

shared varblock ReflectionProjectorBlock
{
	mat4 Transform;
	mat4 InvTransform;
	vec4 BBoxMin;
	vec4 BBoxMax;
	vec4 BBoxCenter;
	float FalloffDistance = 0.2f;
	float FalloffPower = 16.0f;
	int NumEnvMips = 9;
};

samplerCube EnvironmentMap;
samplerCube IrradianceMap;
samplerCube DepthConeMap;

samplerstate GeometrySampler
{
	Samplers = { NormalMap, DepthMap, SpecularMap, AlbedoMap };
	//Filter = Point;
	AddressU = Wrap;
	AddressV = Wrap;
	AddressW = Wrap;
	Filter = MinMagMipLinear;
};

samplerstate EnvironmentSampler
{
	Samplers = { EnvironmentMap, IrradianceMap, DepthConeMap };
	Filter = MinMagMipLinear;
	AddressU = Wrap;
	AddressV = Wrap;
	AddressW = Wrap;
};

state ProjectorState
{
	BlendEnabled[0] = true;
	
	// we use premultiplied alpha with the distance
	// these blend settings is just to pass the alpha value through
	// but still allow for the src and dst to be blended
	// however alpha is transfered directly
	SrcBlend[0] = SrcAlpha;
	DstBlend[0] = OneMinusSrcAlpha;
	//SrcBlendAlpha[0] = Zero;
	//DstBlendAlpha[0] = One;
	//SeparateBlend = true;
	CullMode = Front;
	DepthEnabled = true;
	DepthFunc = Greater;
	DepthWrite = false;
};

prototype float CalculateDistanceField(vec3 point, float falloff);
subroutine (CalculateDistanceField) float Box(
	in vec3 point,
	in float falloff)
{
	vec3 d = abs(point) - vec3(falloff);
	return min(max(d.x, max(d.y, d.z)), 0.0f) + length(max(d, 0.0f));
	//return length(max(abs(point) - d, 0.0f));
}

subroutine (CalculateDistanceField) float Sphere(
	in vec3 point,
	in float falloff)
{
	// use radius.x to get the blending factor
	return length(point) - falloff;
}

CalculateDistanceField calcDistanceField;

prototype vec3 CalculateCubemapCorrection(vec3 worldSpacePos, vec3 reflectVec, vec4 bboxmin, vec4 bboxmax, vec4 bboxcenter);
subroutine (CalculateCubemapCorrection) vec3 ParallaxCorrect(
	in vec3 worldSpacePos, in vec3 reflectVec, in vec4 bboxmin, in vec4 bboxmax, in vec4 bboxcenter)
{
	vec3 intersectMin = (bboxmin.xyz - worldSpacePos) / reflectVec;
	vec3 intersectMax = (bboxmax.xyz - worldSpacePos) / reflectVec;
	vec3 largestRay = max(intersectMin, intersectMax);
	float distToIntersection = min(min(largestRay.x, largestRay.y), largestRay.z);
	vec3 intersectPos = worldSpacePos + reflectVec * distToIntersection;
	return intersectPos - bboxcenter.xyz;		
}

subroutine (CalculateCubemapCorrection) vec3 NoCorrection(
	in vec3 worldSpacePos, in vec3 reflectVec, in vec4 bboxmin, in vec4 bboxmax, in vec4 bboxcenter)
{
	return reflectVec;
}

#define MAX_NUM_STEPS 12
#define DEPTH_THRESHOLD 1000
#define STEP_SIZE 0.01f
subroutine (CalculateCubemapCorrection) vec3 DepthCorrect(
	in vec3 worldSpacePos, in vec3 reflectVec, in vec4 bboxmin, in vec4 bboxmax, in vec4 bboxcenter)
{
	vec3 normed = normalize(reflectVec);
	vec3 localPos = worldSpacePos - bboxcenter.xyz;
	float rl = textureLod(DepthConeMap, normed, 0).r;
	float dp = rl - dot(localPos, normed);
	float3 p = localPos + normed * dp;
	float ppp = length(p) / textureLod(DepthConeMap, p, 0).r;
	float dun = 0, dov = 0, pun = ppp, pov = ppp;
	if (ppp < 1) dun = dp;
	else 		 dov = dp;
	float dl = max(dp + rl * (1 - ppp), 0.0f);
	float3 l = localPos + normed * dl;
	
	for (int i = 0; i < MAX_NUM_STEPS; i++)
	{
		float ddl;
		float llp = length(l) / textureLod(DepthConeMap, l, 0).r;
		if (llp < 1)
		{
			dun = dl; pun = llp;
			ddl = (dov == 0) ? rl * (1 - llp) : (dl - dov) * (1 - llp) / (llp-pov);
		}
		else
		{
			dov = dl; pov = llp;
			ddl = (dun == 0) ? rl * (1 - llp) : (dl - dun) * (1 - llp) / (llp-pun);
		}
		dl = max(dl + ddl, 0.0f);
		l = localPos + normed * dl;
	}
	return l;
	/*
	vec3 normed = normalize(reflectVec);
	float curr = textureLod(DepthConeMap, normed, 0).r;
	//vec3 depth = normed * curr;
	vec3 ray = normed;
	
	for (int i = 0; i < MAX_NUM_STEPS; i++)
	{
		if (length(ray) > curr) break;
		//if ((dot(ray, depth) / length(depth)) > 0) break;
		ray = normed * STEP_SIZE * i;
		curr = textureLod(DepthConeMap, ray, 0).r;
		//depth = ray * curr;
	}
	return (worldSpacePos + ray - bboxcenter.xyz);
	*/
}

CalculateCubemapCorrection calcCubemapCorrection;
//------------------------------------------------------------------------------
/**
*/
shader
void
vsMain(in vec3 position,
	in vec3 normal,
	in vec2 uv,
	out vec3 ViewSpacePosition,
	out vec3 WorldViewVec,
	out vec2 UV) 
{
	vec4 modelSpace = Transform * vec4(position, 1);
	ViewSpacePosition = (View * modelSpace).xyz;
    gl_Position = ViewProjection * modelSpace;
    UV = uv;
	WorldViewVec = modelSpace.xyz - EyePos.xyz;
}

#define USE_DISTANCE_IMAGE 0
//------------------------------------------------------------------------------
/**
	Calculate reflection projection using a box, basically the same as circle except we are using a signed distance function to determine falloff.
*/
[earlydepth]
shader
void
psMain(in vec3 viewSpacePosition,	
	in vec3 worldViewVec,
	in vec2 uv,
	[color0] out vec4 Color)
{	
	vec2 pixelSize = GetPixelSize(DepthMap);
	vec2 screenUV = psComputeScreenCoord(gl_FragCoord.xy, pixelSize.xy);
	float depth = textureLod(DepthMap, screenUV, 0).r;
		
	// get view space position
	vec3 viewVec = normalize(viewSpacePosition);
	vec3 surfacePos = viewVec * depth;
	vec4 worldPosition = InvView * vec4(surfacePos, 1);
	vec4 localPos = InvTransform * worldPosition;	

	// eliminate pixels outside of the object space (-0.5, -0.5, -0.5) and (0.5, 0.5, 0.5)
	vec3 dist = vec3(0.5f) - abs(localPos.xyz);
	if (all(greaterThan(dist, vec3(0))))
	{
		// calculate distance field and falloff
		float d = calcDistanceField(localPos.xyz, FalloffDistance);	
		float distanceFalloff = pow(1-d, FalloffPower);
		//float distanceFalloff = exp(1-d) * FalloffPower;
		//float distanceFalloff = 1 / (pow(d, FalloffPower));
		
		// load biggest distance from texture, basically solving the distance field blending
//		float weight = imageLoad(DistanceFieldWeightMap, ivec2(gl_FragCoord.xy)).r;
		memoryBarrierImage();
#if USE_DISTANCE_IMAGE
		float diff = saturate(distanceFalloff - weight);
#else
		float diff = saturate(distanceFalloff);
#endif
		if (diff <= 0.001f) discard;

//		imageStore(DistanceFieldWeightMap, ivec2(gl_FragCoord.xy), vec4(max(weight, distanceFalloff)));

		
		//float distanceFalloff = pow(1 - diff, FalloffPower);
	
		// sample normal and specular, do some pseudo-PBR energy balance between albedo and spec
		vec3 viewSpaceNormal = UnpackViewSpaceNormal(textureLod(NormalMap, screenUV, 0));
		vec4 spec = textureLod(SpecularMap, screenUV, 0);
		vec4 oneMinusSpec = 1 - spec;
		vec3 albedo = textureLod(AlbedoMap, screenUV, 0).rgb * oneMinusSpec.rgb;
	
		// calculate reflection
		vec3 viewVecWorld = worldViewVec;
		vec3 worldNormal = (InvView * vec4(viewSpaceNormal, 0)).xyz;
		vec3 reflectVec = reflect(viewVecWorld, worldNormal);
		
		// perform cubemap correction method if required
		reflectVec = calcCubemapCorrection(worldPosition.xyz, reflectVec, BBoxMin, BBoxMax, BBoxCenter);
		
		// calculate reflection and irradiance
		float x =  dot(viewSpaceNormal, -viewVec);
		vec3 rim = FresnelSchlickGloss(spec.rgb, x, spec.a);
		vec3 refl = textureLod(EnvironmentMap, reflectVec, oneMinusSpec.a * NumEnvMips).rgb * rim;
		vec3 irr = textureLod(IrradianceMap, worldNormal.xyz, 0).rgb;
		Color = vec4((irr * albedo + refl), diff);
	}
	else
	{
		discard;
	}
}

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(BoxProjector, "Alt0", vsMain(), psMain(calcDistanceField = Box, calcCubemapCorrection = NoCorrection), ProjectorState);
SimpleTechnique(SphereProjector, "Alt1", vsMain(), psMain(calcDistanceField = Sphere, calcCubemapCorrection = NoCorrection), ProjectorState);
SimpleTechnique(CorrectedBoxProjector, "Alt0|Alt2", vsMain(), psMain(calcDistanceField = Box, calcCubemapCorrection = ParallaxCorrect), ProjectorState);
SimpleTechnique(CorrectedSphereProjector, "Alt1|Alt2", vsMain(), psMain(calcDistanceField = Sphere, calcCubemapCorrection = ParallaxCorrect), ProjectorState);
SimpleTechnique(ExactBoxProjector, "Alt0|Alt3", vsMain(), psMain(calcDistanceField = Box, calcCubemapCorrection = DepthCorrect), ProjectorState);
SimpleTechnique(ExactSphereProjector, "Alt1|Alt3", vsMain(), psMain(calcDistanceField = Sphere, calcCubemapCorrection = DepthCorrect), ProjectorState);
