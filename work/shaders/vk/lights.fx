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

const float specPower = float(32.0f);
const float rimLighting = float(0.2f);
const float exaggerateSpec = float(1.8f);
const vec3 luminanceValue = vec3(0.299f, 0.587f, 0.114f); 

group(BATCH_GROUP) shared varblock LocalLightBlock [ bool System = true; bool DynamicOffset = true; ]
{
	vec4 LightColor;
	vec4 LightPosRange;	
	vec4 ShadowOffsetScale = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	vec4 ShadowConstants = vec4(100.0f, 100.0f, 0.003f, 1024.0f);  

	float LightShadowBias;
	float ShadowIntensity = 1.0f;

	mat4 LightProjTransform;
	mat4 LightTransform;
	mat4 ShadowProjTransform;
	
	textureHandle SpotLightShadowAtlas;
	textureHandle SpotLightProjectionTexture;
	textureHandle PointLightShadowCube;
	textureHandle PointLightProjectionTexture;
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
	BorderColor = { 0.0f, 0.0f, 0.0f, 0.0f };
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
	[slot=2] in vec2 uv,
	out vec3 ViewSpacePosition,
	out vec2 UV) 
{
    gl_Position = vec4(position, 1);
    UV = uv;
    ViewSpacePosition = vec3(position.xy * FocalLength.xy, -1); 
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psGlob(
	in vec3 ViewSpacePosition,
	in vec2 UV,
	[color0] out vec4 Color) 
{
	vec3 ViewSpaceNormal = UnpackViewSpaceNormal(subpassLoad(InputAttachments[1]));
	float Depth = subpassLoad(InputAttachments[2]).r;
	
	//vec4 albedoColor = sample2DLod(AlbedoBuffer, PosteffectSampler, UV, 0);
	vec4 albedoColor = subpassLoad(InputAttachments[0]);
	if (Depth < 0) { Color = EncodeHDR(albedoColor); return; };
	
	float NL = saturate(dot(GlobalLightDir.xyz, ViewSpaceNormal));
	
	vec3 diff = GlobalAmbientLightColor.xyz;
	diff += GlobalLightColor.xyz * saturate(NL);
	diff += GlobalBackLightColor.xyz * saturate(-NL + GlobalBackLightOffset); 

	vec4 specColor = subpassLoad(InputAttachments[3]);
	float specPower = ROUGHNESS_TO_SPECPOWER(specColor.a);	
	
	vec3 viewVec = normalize(ViewSpacePosition);
	vec3 H = normalize(GlobalLightDir.xyz - viewVec);
	float NH = saturate(dot(ViewSpaceNormal, H));
	float NV = saturate(dot(ViewSpaceNormal, -viewVec));
	float HL = saturate(dot(H, GlobalLightDir.xyz));
	vec3 spec;
	BRDFLighting(NH, NL, NV, HL, specPower, specColor.rgb, spec);
	vec3 final = (albedoColor.rgb + spec) * diff;
	
	Color = EncodeHDR(vec4(final, 1));
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psGlobShadow(
	in vec3 ViewSpacePosition,
	in vec2 UV,
	[color0] out vec4 Color) 
{
	vec3 ViewSpaceNormal = UnpackViewSpaceNormal(subpassLoad(InputAttachments[1]));
	float Depth = subpassLoad(InputAttachments[2]).r;
		
	vec4 albedoColor = subpassLoad(InputAttachments[0]);
	if (Depth < 0) { Color = EncodeHDR(albedoColor); return; };
	
	float NL = saturate(dot(GlobalLightDir.xyz, ViewSpaceNormal));
	float shadowFactor = 1.0f;
	vec3 viewVec = normalize(ViewSpacePosition);
	vec4 debug;
		
	vec4 worldPos = vec4(viewVec * Depth, 1);
	vec4 texShadow;		
	CSMConvert(worldPos, texShadow);
	shadowFactor = CSMPS(texShadow,
						UV,
						GlobalLightShadowBuffer,
						debug);		
	shadowFactor = lerp(1.0f, shadowFactor, ShadowIntensity);

	// multiply specular with power of shadow factor, this makes shadowed areas not reflect specular light
	vec4 specColor = subpassLoad(InputAttachments[3]);
	float specPower = ROUGHNESS_TO_SPECPOWER(specColor.a);	

	vec3 diff = GlobalAmbientLightColor.xyz;
	diff += GlobalLightColor.xyz * NL;
	diff += GlobalBackLightColor.xyz * saturate(-NL + GlobalBackLightOffset);   
	
	vec3 H = normalize(GlobalLightDir.xyz - viewVec);
	float NH = saturate(dot(ViewSpaceNormal, H));
	float NV = saturate(dot(ViewSpaceNormal, -viewVec));
	float HL = saturate(dot(H, GlobalLightDir.xyz));
	vec3 spec;
	BRDFLighting(NH, NL, NV, HL, specPower, specColor.rgb, spec);
	vec3 final = (albedoColor.rgb + spec) * diff;
	Color = EncodeHDR(vec4(final * saturate(shadowFactor), 1));
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
	[slot=0] in vec3 position,
	out vec3 ViewSpacePosition) 
{
	vec4 modelSpace = LightTransform * vec4(position, 1);
	gl_Position = ViewProjection * modelSpace;
	ViewSpacePosition = (View * modelSpace).xyz;
}

//---------------------------------------------------------------------------------------------------------------------------
/**
*/
shader
void
psSpot(
	in vec3 ViewSpacePosition,
	[color0] out vec4 Color) 
{
	vec2 pixelSize = RenderTargetDimensions[0].zw;
	vec2 screenUV = psComputeScreenCoord(gl_FragCoord.xy, pixelSize.xy);
	vec3 ViewSpaceNormal = UnpackViewSpaceNormal(subpassLoad(InputAttachments[1]));
	float Depth = subpassLoad(InputAttachments[2]).r;
	
	vec3 viewVec = normalize(ViewSpacePosition);
	vec3 surfacePos = viewVec * Depth;    
	vec3 lightDir = (LightPosRange.xyz - surfacePos);
	
	float att = saturate(1.0 - length(lightDir) * LightPosRange.w);    
	if (att - 0.004 < 0) discard;
	lightDir = normalize(lightDir);
	
	vec4 projLightPos = LightProjTransform * vec4(surfacePos, 1.0f);
	if (projLightPos.z - 0.001 < 0) discard;
	float mipSelect = 0;
	vec2 lightSpaceUv = vec2(((projLightPos.xy / projLightPos.ww) * vec2(0.5f, 0.5f)) + 0.5f);
	
	vec4 lightModColor = sample2DLod(SpotLightProjectionTexture, SpotlightTextureSampler, lightSpaceUv, mipSelect);
	vec4 specColor = subpassLoad(InputAttachments[3]);
	vec4 albedoColor = subpassLoad(InputAttachments[0]);
	float specPower = ROUGHNESS_TO_SPECPOWER(specColor.a);	
	
	float NL = dot(lightDir, ViewSpaceNormal);
	vec3 diff = LightColor.xyz * saturate(NL) * att;
	
	vec3 H = normalize(lightDir - viewVec);
	float NH = saturate(dot(ViewSpaceNormal, H));
	float NV = saturate(dot(ViewSpaceNormal, -viewVec));
	float HL = saturate(dot(H, lightDir));
	vec3 spec;
	BRDFLighting(NH, NL, NV, HL, specPower, specColor.rgb, spec);
	vec3 final = (albedoColor.rgb + spec) * diff;
	
	vec4 oColor = vec4(lightModColor.rgb * final, lightModColor.a);
	
	Color = EncodeHDR(oColor); 
}

//---------------------------------------------------------------------------------------------------------------------------
/**
*/
shader
void
psSpotShadow(
	in vec3 ViewSpacePosition,
	[color0] out vec4 Color) 
{
	vec2 pixelSize = RenderTargetDimensions[0].zw;
	vec2 screenUV = psComputeScreenCoord(gl_FragCoord.xy, pixelSize.xy);
	vec3 ViewSpaceNormal = UnpackViewSpaceNormal(subpassLoad(InputAttachments[1]));
	float Depth = subpassLoad(InputAttachments[2]).r;
	
	vec3 viewVec = normalize(ViewSpacePosition);
	vec3 surfacePos = viewVec * Depth;    
	vec3 lightDir = (LightPosRange.xyz - surfacePos);
	
	float att = saturate(1.0 - length(lightDir) * LightPosRange.w);    
	if (att - 0.004 < 0) discard;
	lightDir = normalize(lightDir);
	
	vec4 projLightPos = LightProjTransform * vec4(surfacePos, 1.0f);
	if(projLightPos.z - 0.001 < 0) discard;
	float mipSelect = 0;
	vec2 lightSpaceUv = (projLightPos.xy / projLightPos.ww) * vec2(0.5f, -0.5f) + 0.5f;
	
	vec4 lightModColor = sample2DLod(SpotLightProjectionTexture, SpotlightTextureSampler, lightSpaceUv, mipSelect);
	vec4 specColor = subpassLoad(InputAttachments[3]);
	vec4 albedoColor = subpassLoad(InputAttachments[0]);
	float specPower = ROUGHNESS_TO_SPECPOWER(specColor.a);	
	
	float NL = dot(lightDir, ViewSpaceNormal);
	vec3 diff = LightColor.xyz * saturate(NL) * att;
	
	vec3 H = normalize(lightDir - viewVec);
	float NH = saturate(dot(ViewSpaceNormal, H));
	float NV = saturate(dot(ViewSpaceNormal, -viewVec));
	float HL = saturate(dot(H, lightDir));
	vec3 spec;
	BRDFLighting(NH, NL, NV, HL, specPower, specColor.rgb, spec);
	vec3 final = (albedoColor.rgb + spec) * diff;
	vec4 oColor = vec4(lightModColor.rgb * final, lightModColor.a);
	
	// shadows
	vec4 shadowProjLightPos = ShadowProjTransform * vec4(surfacePos, 1.0f);
	vec2 shadowLookup = (shadowProjLightPos.xy / shadowProjLightPos.ww) * vec2(0.5f, -0.5f) + 0.5f; 
	shadowLookup.y = 1 - shadowLookup.y;
	float receiverDepth = projLightPos.z / projLightPos.w;
	float shadowFactor = GetInvertedOcclusionSpotLight(receiverDepth, shadowLookup, SpotLightShadowAtlas);	
	//shadowFactor = smoothstep(0.0000001f, 1, shadowFactor);
	shadowFactor = saturate(lerp(1.0f, saturate(shadowFactor), ShadowIntensity));
	
	
	Color = EncodeHDR(oColor * shadowFactor);
}
//---------------------------------------------------------------------------------------------------------------------------
//											POINT LIGHT
//---------------------------------------------------------------------------------------------------------------------------

state PointLightStateStandard
{
	BlendEnabled[0] = true;
	SrcBlend[0] = One;
	DstBlend[0] = One;
	CullMode = Front;
	DepthEnabled = true;
	DepthWrite = false;
	DepthFunc = Greater;
};

state PointLightStateShadow
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
vsPoint(
	[slot=0] in vec3 position,
	out vec3 ViewSpacePosition,
	out vec3 WorldPosition,
	out vec4 ProjPosition) 
{
	vec4 modelSpace = LightTransform * vec4(position, 1);
	gl_Position = ViewProjection * modelSpace;
	WorldPosition = modelSpace.xyz;
	ViewSpacePosition = (View * modelSpace).xyz;
	ProjPosition = gl_Position;
}

//---------------------------------------------------------------------------------------------------------------------------
/**
*/
shader
void
psPoint(
	in vec3 ViewSpacePosition,	
	in vec3 WorldPosition,
	in vec4 ProjPosition,
	[color0] out vec4 Color) 
{
	vec2 pixelSize = RenderTargetDimensions[0].zw;
	vec2 screenUV = psComputeScreenCoord(gl_FragCoord.xy, pixelSize.xy);
	vec3 ViewSpaceNormal = UnpackViewSpaceNormal(subpassLoad(InputAttachments[1]));
	float Depth = subpassLoad(InputAttachments[2]).r;
	
	vec3 viewVec = normalize(ViewSpacePosition);
	vec3 surfacePos = viewVec * Depth;
	vec3 lightDir = (LightPosRange.xyz - surfacePos);
	vec3 projDir = (InvView * vec4(-lightDir, 0)).xyz;
	vec4 lightModColor = sampleCubeLod(PointLightProjectionTexture, PointLightTextureSampler, projDir, 0);
	
	vec4 specColor = subpassLoad(InputAttachments[3]);
	vec4 albedoColor = subpassLoad(InputAttachments[0]);
	float specPower = ROUGHNESS_TO_SPECPOWER(specColor.a);	// magic formulae to calculate specular power from color in the range [0..1]
	
	float att = saturate(1.0 - length(lightDir) * LightPosRange.w);
	att *= att;
	lightDir = normalize(lightDir);
	
	float NL = dot(lightDir, ViewSpaceNormal);
	vec3 diff = LightColor.xyz * saturate(NL) * att;
	
	vec3 H = normalize(lightDir - viewVec);
	float NH = saturate(dot(ViewSpaceNormal, H));
	float NV = saturate(dot(ViewSpaceNormal, -viewVec));
	float HL = saturate(dot(H, lightDir));
	vec3 spec;
	BRDFLighting(NH, NL, NV, HL, specPower, specColor.rgb, spec);
	vec3 final = (albedoColor.rgb + spec) * diff;
	
	vec4 oColor = vec4(lightModColor.rgb * final, lightModColor.a);
	              
	Color = EncodeHDR(oColor);
}

//---------------------------------------------------------------------------------------------------------------------------
/**
*/
shader
void
psPointShadow(
	in vec3 ViewSpacePosition,	
	in vec3 WorldPosition,
	in vec4 ProjPosition,
	[color0] out vec4 Color) 
{
	vec2 pixelSize = RenderTargetDimensions[0].zw;
	vec2 screenUV = psComputeScreenCoord(gl_FragCoord.xy, pixelSize.xy);
	vec3 ViewSpaceNormal = UnpackViewSpaceNormal(subpassLoad(InputAttachments[1]));
	float Depth = subpassLoad(InputAttachments[2]).r;
	
	vec3 viewVec = normalize(ViewSpacePosition);
	vec3 surfacePos = viewVec * Depth;
	vec3 lightDir = (LightPosRange.xyz - surfacePos);
	vec3 projDir = (InvView * vec4(-lightDir, 0)).xyz;
	float distToSurface = length(lightDir);
	
	vec4 lightModColor = sampleCubeLod(PointLightProjectionTexture, PointLightTextureSampler, projDir, 0);
	vec4 specColor = subpassLoad(InputAttachments[3]);
	vec4 albedoColor = subpassLoad(InputAttachments[0]);
	float specPower = ROUGHNESS_TO_SPECPOWER(specColor.a);	
	
	float att = saturate(1.0 - distToSurface * LightPosRange.w);
	att *= att;
	lightDir = normalize(lightDir);
	
	float NL = dot(lightDir, ViewSpaceNormal);
	vec3 diff = LightColor.xyz * saturate(NL) * att;
	
	vec3 H = normalize(lightDir - viewVec);
	float NH = saturate(dot(ViewSpaceNormal, H));
	float NV = saturate(dot(ViewSpaceNormal, -viewVec));
	float HL = saturate(dot(H, lightDir));
	vec3 spec;
	BRDFLighting(NH, NL, NV, HL, specPower, specColor.rgb, spec);
	vec3 final = (albedoColor.rgb + spec) * diff;
	
	vec4 oColor = vec4(lightModColor.rgb * final, lightModColor.a);
	
	// shadows
	float shadowFactor = GetInvertedOcclusionPointLight(gl_FragCoord.z,
						projDir,
						PointLightShadowCube);	
	shadowFactor = saturate(lerp(1.0f, saturate(shadowFactor), ShadowIntensity));      	
	
	Color = EncodeHDR(oColor * shadowFactor);
}

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(GlobalLight, "Global", vsGlob(), psGlob(), GlobalLightState);
SimpleTechnique(GlobalLightShadow, "Global|Alt0", vsGlob(), psGlobShadow(), GlobalLightState);
SimpleTechnique(SpotLight, "Spot", vsSpot(), psSpot(), SpotLightState);
SimpleTechnique(SpotLightShadow, "Spot|Alt0", vsSpot(), psSpotShadow(), SpotLightState);
SimpleTechnique(PointLight, "Point", vsPoint(), psPoint(), PointLightStateStandard);
SimpleTechnique(PointLightShadow, "Point|Alt0", vsPoint(), psPointShadow(), PointLightStateShadow);
