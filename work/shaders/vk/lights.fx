//------------------------------------------------------------------------------
//  lights.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"
#include "lib/CSM.fxh"
#include "lib/techniques.fxh"
#include "lib/shadowbase.fxh"
#include "lib/preetham.fxh"
#include "lib/pbr.fxh"

const float specPower = float(32.0f);
const float rimLighting = float(0.2f);
const float exaggerateSpec = float(1.8f);
const vec3 luminanceValue = vec3(0.299f, 0.587f, 0.114f);

// match these in lightcontext.cc
const uint USE_SHADOW_BITFLAG				= 1;
const uint USE_PROJECTION_TEX_BITFLAG		= 2;

#define FlagSet(x, flags) ((x & flags) == flags)

group(INSTANCE_GROUP) shared varblock LocalLightBlock [ string Visibility = "VS|PS"; ]
{
	vec4 LightColor;
	vec4 LightPosRange;

	mat4 LightProjTransform;
	mat4 LightTransform;
	vec4 LightForward;
	vec2 LightCutoff; // only for spots
	uint Flags;

	textureHandle ProjectionTexture;
};

group(INSTANCE_GROUP) shared varblock LocalLightShadowBlock [string Visibility = "VS|PS"; ]
{
	vec4 ShadowOffsetScale = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	vec4 ShadowConstants = vec4(100.0f, 100.0f, 0.003f, 1024.0f);
	float ShadowBias;
	float ShadowIntensity = 1.0f;
	textureHandle ShadowMap; // for spot-lights, an atlas of shadows, for point-lights, a cube of shadows
	mat4 ShadowProjTransform;
};

samplerstate PointLightTextureSampler
{
	Filter = MinMagLinearMipPoint;
};

samplerstate SpotlightTextureSampler
{
	//Samplers = { LightProjMap, LightProjCube };
	Filter = MinMagLinearMipPoint;
	AddressU = Border;
	AddressV = Border;
	BorderColor = Transparent;
};

#define SPECULAR_SCALE 13
#define ROUGHNESS_TO_SPECPOWER(x) exp2(SPECULAR_SCALE * x + 1)

//---------------------------------------------------------------------------------------------------------------------------
//											GLOBAL LIGHT
//---------------------------------------------------------------------------------------------------------------------------

state GlobalLightState
{
	CullMode = None;
	DepthEnabled = false;
	DepthWrite = false;
	DepthClamp = false;
};

//------------------------------------------------------------------------------
/**
*/
shader
void
vsGlob(
	[slot=0] in vec3 position,
	[slot=2] in vec2 uv)
{
    gl_Position = vec4(position, 1);
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psGlob([color0] out vec4 Color)
{
	vec3 rawNormal = subpassLoad(InputAttachment1).xyz;
	vec4 albedoColor = subpassLoad(InputAttachment0);
	if (rawNormal.z == -1.0f) { Color = albedoColor; return; };

	vec3 normal = rawNormal;
	float Depth = subpassLoad(DepthAttachment).r;

	float NL = saturate(dot(GlobalLightDir.xyz, normal));
	if (NL <= 0) { Color = albedoColor; return; };
	vec4 worldPos = PixelToWorld((gl_FragCoord.xy + vec2(0.5f, 0.5f)) * RenderTargetDimensions[0].zw, Depth);
	vec3 viewVec = normalize(EyePos.xyz - worldPos.xyz);

	float shadowFactor = 1.0f;
	vec4 debug = vec4(1,1,1,1);
	if (FlagSet(GlobalLightFlags, USE_SHADOW_BITFLAG))
	{
		
		vec4 shadowPos = CSMShadowMatrix * worldPos; // csm contains inversed view + csm transform
		shadowFactor = CSMPS(shadowPos,
			GlobalLightShadowBuffer,
			debug);
		//debug = saturate(worldPos);
		shadowFactor = lerp(1.0f, shadowFactor, GlobalLightShadowIntensity);
	}
	else
	{
		debug = vec4(0, 0, 0, 0);
	}

	vec3 diff = GlobalAmbientLightColor.xyz;
	diff += GlobalLightColor.xyz * saturate(NL);
	//diff += GlobalBackLightColor.xyz * saturate(-NL + GlobalBackLightOffset);

	vec4 specColor = subpassLoad(InputAttachment2);
	float specPower = ROUGHNESS_TO_SPECPOWER(specColor.a);

	vec3 H = normalize(GlobalLightDir.xyz + viewVec);
	float NH = saturate(dot(normal, H));
	float NV = saturate(dot(normal, viewVec));
	float HL = saturate(dot(H, GlobalLightDir.xyz));
	vec3 spec;  
	BRDFLighting(NH, NL, NV, HL, specPower, specColor.rgb, spec);

	// add sky light
	vec3 skyLight = Preetham(normal, GlobalLightDir.xyz, A, B, C, D, E, Z) * GlobalLightColor.xyz;
	diff += skyLight;

	vec3 final = (albedoColor.rgb + spec) * diff;

	Color = vec4(final * shadowFactor, 1);
}

//---------------------------------------------------------------------------------------------------------------------------
//											SPOT LIGHT
//---------------------------------------------------------------------------------------------------------------------------

state SpotLightState
{
	BlendEnabled[0] = true;
	SrcBlend[0] = One;
	DstBlend[0] = One;
	CullMode = Front;
	DepthEnabled = true;
	DepthWrite = false;
	DepthFunc = Greater;
};

//---------------------------------------------------------------------------------------------------------------------------
/**
*/
float
GetInvertedOcclusionSpotLight(float receiverDepthInLightSpace,
                     vec2 lightSpaceUv,
					 uint Texture)
{

    // offset and scale shadow lookup tex coordinates
	vec2 shadowUv = lightSpaceUv;
    shadowUv.xy *= ShadowOffsetScale.zw;
    shadowUv.xy += ShadowOffsetScale.xy;

	// calculate average of 4 closest pixels
	vec2 shadowSample = sample2D(Texture, SpotlightTextureSampler, shadowUv).rg;

	// get pixel size of shadow projection texture
	return ChebyshevUpperBound(shadowSample, receiverDepthInLightSpace, 0.000001f);
}

//---------------------------------------------------------------------------------------------------------------------------
/**
*/
shader
void
vsSpot(
	[slot=0] in vec3 position)
{
	vec4 modelSpace = LightTransform * vec4(position, 1);
	gl_Position = ViewProjection * modelSpace;
}

//---------------------------------------------------------------------------------------------------------------------------
/**
*/
shader
void
psSpot([color0] out vec4 Color)
{
	vec3 normal = subpassLoad(InputAttachment1).xyz;
	float Depth = subpassLoad(DepthAttachment).r;

	vec3 worldPos = PixelToWorld((gl_FragCoord.xy + vec2(0.5f, 0.5f)) * RenderTargetDimensions[0].zw, Depth).xyz;
	vec3 viewVec = worldPos - EyePos.xyz;
	vec3 lightDir = (LightPosRange.xyz - worldPos);

	float att = saturate(1.0 - length(lightDir) * LightPosRange.w);
	if (att - 0.004 < 0) discard;
	lightDir = normalize(lightDir);
	
	float theta = dot(LightForward.xyz, lightDir);
	const float innerCutoff = LightCutoff.x;
	const float outerCutoff = LightCutoff.y;
	float epsilon = innerCutoff - outerCutoff;
	float intensity = clamp((outerCutoff - theta) / epsilon, 0.0f, 1.0f);

	vec4 projLightPos = LightProjTransform * vec4(worldPos, 1.0f);
	if (projLightPos.z - 0.001 < 0) discard;
	float mipSelect = 0;
	vec2 lightSpaceUv = vec2(((projLightPos.xy / projLightPos.ww) * vec2(0.5f, 0.5f)) + 0.5f);

	vec4 lightModColor;

	if (FlagSet(Flags, USE_PROJECTION_TEX_BITFLAG))
		lightModColor = sample2DLod(ProjectionTexture, SpotlightTextureSampler, lightSpaceUv, mipSelect);
	else
		lightModColor = intensity.xxxx * att;

	float shadowFactor = 1.0f;
	if (FlagSet(Flags, USE_SHADOW_BITFLAG))
	{
		// shadows
		vec4 shadowProjLightPos = ShadowProjTransform * vec4(worldPos, 1.0f);
		vec2 shadowLookup = (shadowProjLightPos.xy / shadowProjLightPos.ww) * vec2(0.5f, -0.5f) + 0.5f;
		shadowLookup.y = 1 - shadowLookup.y;
		float receiverDepth = projLightPos.z / projLightPos.w;
		shadowFactor = GetInvertedOcclusionSpotLight(receiverDepth, shadowLookup, ShadowMap);
		shadowFactor = saturate(lerp(1.0f, saturate(shadowFactor), ShadowIntensity));
	}

	vec4 specColor = subpassLoad(InputAttachment2);
	vec4 albedoColor = subpassLoad(InputAttachment0);
	float specPower = ROUGHNESS_TO_SPECPOWER(specColor.a);

	float NL = dot(lightDir, normal);
	vec3 diff = LightColor.xyz * saturate(NL) * att;

	vec3 H = normalize(-lightDir + viewVec);
	float NH = saturate(dot(normal, H));
	float NV = saturate(dot(normal, -viewVec));
	float HL = saturate(dot(H, -lightDir));
	vec3 spec;
	BRDFLighting(NH, NL, NV, HL, specPower, specColor.rgb, spec);
	vec3 final = (albedoColor.rgb + spec) * diff;

	vec4 oColor = vec4(lightModColor.rgb * final, lightModColor.a);

	Color = oColor * shadowFactor;
}

//---------------------------------------------------------------------------------------------------------------------------
//											POINT LIGHT
//---------------------------------------------------------------------------------------------------------------------------

state PointLightState
{
	BlendEnabled[0] = true;
	SrcBlend[0] = One;
	DstBlend[0] = One;
	CullMode = Front;
	DepthEnabled = true;
	DepthWrite = false;
	DepthFunc = Greater;
};

//---------------------------------------------------------------------------------------------------------------------------
/**
*/
float
GetInvertedOcclusionPointLight(
					 float receiverDepthInLightSpace,
                     vec3 lightSpaceUv,
					 uint Texture)
{

    // offset and scale shadow lookup tex coordinates
	vec3 shadowUv = lightSpaceUv;

	// get pixel size of shadow projection texture
	vec2 shadowSample = sampleCube(Texture, PointLightTextureSampler, shadowUv).rg;

	// get pixel size of shadow projection texture
	return ChebyshevUpperBound(shadowSample, receiverDepthInLightSpace, 0.00000001f);
}

//---------------------------------------------------------------------------------------------------------------------------
/**
*/
shader
void
vsPoint([slot=0] in vec3 position)
{
	vec4 modelSpace = LightTransform * vec4(position, 1);
	gl_Position = ViewProjection * modelSpace;
}

//---------------------------------------------------------------------------------------------------------------------------
/**
*/
shader
void
psPoint([color0] out vec4 Color)
{
	vec3 normal = subpassLoad(InputAttachment1).xyz;
	float Depth = subpassLoad(DepthAttachment).r;

	vec3 worldPos = PixelToWorld((gl_FragCoord.xy + vec2(0.5f, 0.5f)) * RenderTargetDimensions[0].zw, Depth).xyz;
	vec3 viewVec = worldPos - EyePos.xyz;
	vec3 lightDir = (LightPosRange.xyz - worldPos);
	vec3 projDir = (InvView * vec4(-lightDir, 0)).xyz;

	vec4 lightModColor = vec4(1, 1, 1, 1);
	if (FlagSet(Flags, USE_PROJECTION_TEX_BITFLAG))
		lightModColor = sampleCubeLod(ProjectionTexture, PointLightTextureSampler, projDir, 0);

	vec4 specColor = subpassLoad(InputAttachment2);
	vec4 albedoColor = subpassLoad(InputAttachment0);
	float specPower = ROUGHNESS_TO_SPECPOWER(specColor.a);	// magic formulae to calculate specular power from color in the range [0..1]

	float att = saturate(1.0 - length(lightDir) * LightPosRange.w);
	att *= att;
	lightDir = normalize(lightDir);

	float NL = dot(lightDir, normal);
	vec3 diff = LightColor.xyz * saturate(NL) * att;

	vec3 H = normalize(lightDir - viewVec);
	float NH = saturate(dot(normal, H));
	float NV = saturate(dot(normal, -viewVec));
	float HL = saturate(dot(H, lightDir));
	vec3 spec;
	BRDFLighting(NH, NL, NV, HL, specPower, specColor.rgb, spec);
	vec3 final = (albedoColor.rgb + spec) * diff;

	vec4 oColor = vec4(lightModColor.rgb * final, lightModColor.a);

	// shadows
	float shadowFactor = 1.0f;
	if (FlagSet(Flags, USE_SHADOW_BITFLAG))
	{
		shadowFactor = GetInvertedOcclusionPointLight(gl_FragCoord.z,
			projDir,
			ShadowMap);
		shadowFactor = saturate(lerp(1.0f, saturate(shadowFactor), ShadowIntensity));
	}

	Color = oColor * shadowFactor;
}


//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(GlobalLight, "Global", vsGlob(), psGlob(), GlobalLightState);
SimpleTechnique(SpotLight, "Spot", vsSpot(), psSpot(), SpotLightState);
SimpleTechnique(PointLight, "Point", vsPoint(), psPoint(), PointLightState);
