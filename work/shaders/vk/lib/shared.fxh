//------------------------------------------------------------------------------
//  shared.fxh
//  (C) 2018 gscept
//------------------------------------------------------------------------------

#ifndef SHARED_FXH
#define SHARED_FXH

#include <lib/std.fxh>
#include <lib/util.fxh>

const int MAX_2D_TEXTURES = 2048;
const int MAX_2D_MS_TEXTURES = 64;
const int MAX_2D_ARRAY_TEXTURES = 8;
const int MAX_CUBE_TEXTURES = 128;
const int MAX_3D_TEXTURES = 128;

// the texture list can be updated once per tick (frame)
group(TICK_GROUP) texture2D			Textures2D[MAX_2D_TEXTURES];
group(TICK_GROUP) texture2DMS		Textures2DMS[MAX_2D_MS_TEXTURES];
group(TICK_GROUP) textureCube		TexturesCube[MAX_CUBE_TEXTURES];
group(TICK_GROUP) texture3D			Textures3D[MAX_3D_TEXTURES];
group(TICK_GROUP) texture2DArray	Textures2DArray[MAX_2D_ARRAY_TEXTURES];
group(TICK_GROUP) sampler_state		Basic2DSampler {};
group(TICK_GROUP) sampler_state		PosteffectSampler { Filter = Point; };
group(TICK_GROUP) sampler_state		PosteffectUpscaleSampler { Filter = Linear; };

#define sample2D(handle, sampler, uv)						texture(sampler2D(Textures2D[handle], sampler), uv)
#define sample2DLod(handle, sampler, uv, lod)				textureLod(sampler2D(Textures2D[handle], sampler), uv, lod)
#define sample2DGrad(handle, sampler, uv, ddx, ddy)			textureGrad(sampler2D(Textures2D[handle], sampler), uv, ddx, ddy)

#define sample2DMS(handle, sampler, uv)						texture(sampler2DMS(Textures2DMS[handle], sampler), uv)
#define sample2DMSLod(handle, sampler, uv, lod)				textureLod(sampler2DMS(Textures2DMS[handle], sampler), uv, lod)
#define sample2DMSGrad(handle, sampler, uv, ddx, ddy)		textureGrad(sampler2DMS(Textures2D[handle], sampler), uv, ddx, ddy)

#define sampleCube(handle, sampler, uvw)					texture(samplerCube(TexturesCube[handle], sampler), uvw)
#define sampleCubeLod(handle, sampler, uvw, lod)			textureLod(samplerCube(TexturesCube[handle], sampler), uvw, lod)

#define sample2DArray(handle, sampler, uvw)					texture(sampler2DArray(Textures2DArray[handle], sampler), uvw)
#define sample2DArrayGrad(handle, sampler, uvw, ddx, ddy)	textureGrad(sampler2DArray(Textures2DArray[handle], sampler), uvw, ddx, ddy)
#define sample2DArrayLod(handle, sampler, uvw, lod)			textureLod(sampler2DArray(Textures2DArray[handle], sampler), uvw, lod)

#define sample3D(handle, sampler, uvw)						texture(sampler3D(Textures3D[handle], sampler), uvw)
#define sample3DLod(handle, sampler, uvw, lod)				textureLod(sampler3D(Textures3D[handle], sampler), uvw, lod)

#define fetch2D(handle, sampler, uv, lod)					texelFetch(sampler2D(Textures2D[handle], sampler), uv, lod)
#define fetch2DMS(handle, sampler, uv, lod)					texelFetch(sampler2DMS(Textures2DMS[handle], sampler), uv, lod)
#define fetchCube(handle, sampler, uvw, lod)				texelFetch(sampler2DArray(Textures2DArray[handle], sampler), uvw, lod)
#define fetchArray(handle, sampler, uvw, lod)				texelFetch(sampler2DArray(Textures2DArray[handle], sampler), uvw, lod)
#define fetch3D(handle, sampler, uvw, lod)					texelFetch(sampler3D(Textures3D[handle], sampler), uvw, lod)
#define fetchStencil(handle, sampler, uv, lod)				(floatBitsToUint(texelFetch(sampler2D(Textures2D[handle], sampler), uv, lod).r))

#define basic2D(handle)										Textures2D[handle]
#define basic2DMS(handle)									Textures2DMS[handle]
#define basicCube(handle)									TexturesCube[handle]
#define basic3D(handle)										Textures3D[handle]

#define make_sampler2D(handle, sampler)						sampler2D(Textures2D[handle], sampler)
#define make_sampler2DMS(handle, sampler)					sampler2DMS(Textures2DMS[handle], sampler)
#define make_sampler2DArray(handle, sampler)				sampler2DArray(Textures2DArray[handle], sampler)
#define make_sampler3D(handle, sampler)						sampler3D(Textures3D[handle], sampler)

// The number of CSM cascades
#ifndef CASCADE_COUNT_FLAG
#define CASCADE_COUNT_FLAG 4
#endif

// these parameters are updated once per application tick
group(TICK_GROUP) shared constant PerTickParams
{
	vec4 WindDirection;

	float WindWaveSize;
	float WindSpeed;
	float WindIntensity;
	float WindForce;

	float Saturation;
	float MaxLuminance;
	float FadeValue;
	uint UseDof;

	vec4 Balance;

	vec3 DoFDistances;
	float HDRBrightPassThreshold;

	vec4 HDRBloomColor;
	vec4 FogDistances;
	vec4 FogColor;

	// global light stuff
	uint GlobalLightFlags;
	float GlobalLightShadowIntensity;
	vec4 GlobalLightDirWorldspace;
	vec4 GlobalLightDir;
	vec4 GlobalLightColor;
	vec4 GlobalBackLightColor;
	vec4 GlobalAmbientLightColor;
	mat4 CSMShadowMatrix;

	float GlobalBackLightOffset;
	textureHandle GlobalLightShadowBuffer;
	int NumEnvMips;
	textureHandle EnvironmentMap;

	// these params are for the Preetham sky model
	vec4 A;
	vec4 B;
	vec4 C;
	vec4 D;
	vec4 E;
	vec4 Z;

	float RayleighFactor;
	float RayleighZenithLength;
	float RefractiveIndex;
	float DepolarizationFactor;
	float Molecules;
	float MieV;
	float MieCoefficient;
	float MieDirectionalG;
	float MieZenithLength;
	float Turbidity;
	float SunIntensityFactor;
	float SunIntensityFalloff;
	float SunDiscSize;
	vec3 MieKCoefficient;
	vec3 PrimaryColors;
	float TonemapWeight;
	float Lum;

	// CSM params
	vec4 CascadeOffset[CASCADE_COUNT_FLAG];
	vec4 CascadeScale[CASCADE_COUNT_FLAG];
	vec4 CascadeDistances; 
	float MinBorderPadding;
	float MaxBorderPadding;
	float ShadowPartitionSize;
	float GlobalLightShadowBias;

	textureHandle NormalBuffer;
	textureHandle DepthBuffer;
	textureHandle SpecularBuffer;
	textureHandle IrradianceMap;
	textureHandle DepthBufferCopy;
};

// contains the render_state of the camera (and time)
group(FRAME_GROUP) shared constant FrameBlock
{
	mat4 View;
	mat4 InvView;
	mat4 ViewProjection;
	mat4 Projection;
	mat4 InvProjection;
	mat4 InvViewProjection;
	vec4 EyePos;
	vec4 FocalLengthNearFar; // x, y is focal length x/y, z, w is near/far planes
	vec4 TimeAndRandom;
};

group(FRAME_GROUP) shared constant ShadowMatrixBlock[string Visibility = "VS";]
{
	mat4 CSMViewMatrix[CASCADE_COUNT_FLAG];
	mat4 LightViewMatrix[SHADOW_CASTER_COUNT];
};

const int SHADOW_CASTER_COUNT = 16;

#define FLT_MAX     3.40282347E+38F
#define FLT_MIN     -3.40282347E+38F


group(PASS_GROUP) inputAttachment InputAttachment0;
group(PASS_GROUP) inputAttachment InputAttachment1;
group(PASS_GROUP) inputAttachment InputAttachment2;
group(PASS_GROUP) inputAttachment InputAttachment3;
group(PASS_GROUP) inputAttachment InputAttachment4;
group(PASS_GROUP) inputAttachment InputAttachment5;
group(PASS_GROUP) inputAttachment InputAttachment6;
group(PASS_GROUP) inputAttachment InputAttachment7;
group(PASS_GROUP) inputAttachment InputAttachment8;
group(PASS_GROUP) inputAttachment InputAttachment9;
group(PASS_GROUP) inputAttachment InputAttachment10;
group(PASS_GROUP) inputAttachment InputAttachment11;
group(PASS_GROUP) inputAttachment InputAttachment12;
group(PASS_GROUP) inputAttachment InputAttachment13;
group(PASS_GROUP) inputAttachment InputAttachment14;
group(PASS_GROUP) inputAttachment InputAttachment15;
group(PASS_GROUP) inputAttachment DepthAttachment;

group(PASS_GROUP) shared constant PassBlock
{
	vec4 RenderTargetDimensions[16]; // render target dimensions are size (xy) inversed size (zw)
};

#endif // SHARED_H
