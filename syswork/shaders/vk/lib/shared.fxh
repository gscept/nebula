//------------------------------------------------------------------------------
//  shared.fxh
//  (C) 2018 gscept
//------------------------------------------------------------------------------

#ifndef SHARED_FXH
#define SHARED_FXH

#include <lib/std.fxh>
#include <lib/util.fxh>

const int MAX_TEXTURES = 65535;
const int NUM_CASCADES = 4;
const int SHADOW_CASTER_COUNT = 16;

#define FLT_MAX     3.40282347E+38F
#define FLT_MIN     -3.40282347E+38F
 
// the texture list can be updated once per tick (frame)
group(TICK_GROUP) binding(0) texture1D			Textures1D[MAX_TEXTURES];
group(TICK_GROUP) binding(0) texture1DArray 	Textures1DArray[MAX_TEXTURES];
group(TICK_GROUP) binding(0) texture2D			Textures2D[MAX_TEXTURES];
group(TICK_GROUP) binding(0) texture2DMS		Textures2DMS[MAX_TEXTURES];
group(TICK_GROUP) binding(0) textureCube		TexturesCube[MAX_TEXTURES];
group(TICK_GROUP) binding(0) textureCubeArray	TexturesCubeArray[MAX_TEXTURES];
group(TICK_GROUP) binding(0) texture3D			Textures3D[MAX_TEXTURES];
group(TICK_GROUP) binding(0) texture2DArray	    Textures2DArray[MAX_TEXTURES];
group(TICK_GROUP) sampler_state		Basic2DSampler {};
group(TICK_GROUP) sampler_state		PointSampler { Filter = Point; AddressU = Clamp; AddressV = Clamp; };
group(TICK_GROUP) sampler_state		LinearSampler { Filter = Linear; AddressU = Clamp; AddressV = Clamp; };

#define sample2D(handle, sampler, uv)						    texture(sampler2D(Textures2D[handle], sampler), uv)
#define sample2DLod(handle, sampler, uv, lod)				    textureLod(sampler2D(Textures2D[handle], sampler), uv, lod)
#define sample2DGrad(handle, sampler, uv, ddx, ddy)			    textureGrad(sampler2D(Textures2D[handle], sampler), uv, ddx, ddy)

#define sample2DMS(handle, sampler, uv)						    texture(sampler2DMS(Textures2DMS[handle], sampler), uv)
#define sample2DMSLod(handle, sampler, uv, lod)				    textureLod(sampler2DMS(Textures2DMS[handle], sampler), uv, lod)
#define sample2DMSGrad(handle, sampler, uv, ddx, ddy)		    textureGrad(sampler2DMS(Textures2D[handle], sampler), uv, ddx, ddy)

#define sampleCube(handle, sampler, uvw)					    texture(samplerCube(TexturesCube[handle], sampler), uvw)
#define sampleCubeLod(handle, sampler, uvw, lod)			    textureLod(samplerCube(TexturesCube[handle], sampler), uvw, lod)

#define sample2DArray(handle, sampler, uvw)					    texture(sampler2DArray(Textures2DArray[handle], sampler), uvw)
#define sample2DArrayGrad(handle, sampler, uvw, ddx, ddy)	    textureGrad(sampler2DArray(Textures2DArray[handle], sampler), uvw, ddx, ddy)
#define sample2DArrayLod(handle, sampler, uvw, lod)			    textureLod(sampler2DArray(Textures2DArray[handle], sampler), uvw, lod)

#define sample3D(handle, sampler, uvw)						    texture(sampler3D(Textures3D[handle], sampler), uvw)
#define sample3DLod(handle, sampler, uvw, lod)				    textureLod(sampler3D(Textures3D[handle], sampler), uvw, lod)

#define sample2DArrayShadow(handle, sampler, uv, index, depth)	texture(sampler2DArrayShadow(Textures2DArray[handle], sampler), vec4(uv, depth, index))

#define fetch2D(handle, sampler, uv, lod)					    texelFetch(sampler2D(Textures2D[handle], sampler), uv, lod)
#define fetch2DMS(handle, sampler, uv, lod)					    texelFetch(sampler2DMS(Textures2DMS[handle], sampler), uv, lod)
#define fetchCube(handle, sampler, uvw, lod)				    texelFetch(sampler2DArray(Textures2DArray[handle], sampler), uvw, lod)
#define fetchArray(handle, sampler, uvw, lod)				    texelFetch(sampler2DArray(Textures2DArray[handle], sampler), uvw, lod)
#define fetch3D(handle, sampler, uvw, lod)					    texelFetch(sampler3D(Textures3D[handle], sampler), uvw, lod)
#define fetchStencil(handle, sampler, uv, lod)				    (floatBitsToUint(texelFetch(sampler2D(Textures2D[handle], sampler), uv, lod).r))

#define basic2D(handle)										    Textures2D[handle]
#define basic2DMS(handle)									    Textures2DMS[handle]
#define basicCube(handle)									    TexturesCube[handle]
#define basic3D(handle)										    Textures3D[handle]

#define make_sampler2D(handle, sampler)						    sampler2D(Textures2D[handle], sampler)
#define make_sampler2DMS(handle, sampler)					    sampler2DMS(Textures2DMS[handle], sampler)
#define make_sampler2DArray(handle, sampler)				    sampler2DArray(Textures2DArray[handle], sampler)
#define make_sampler3D(handle, sampler)						    sampler3D(Textures3D[handle], sampler)

#define query_lod2D(handle, sampler, uv)                        textureQueryLod(sampler2D(Textures2D[handle], sampler), uv)

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

    vec3 BloomColor;
    float BloomIntensity;
    
    vec4 FogDistances;
    vec4 FogColor;

    // global light stuff
    uint GlobalLightFlags;
    float GlobalLightShadowIntensity;
    vec2 GlobalLightShadowMapSize;
    vec4 GlobalLightDirWorldspace;
    vec4 GlobalLightDir;
    vec4 GlobalLightColor;
    vec4 GlobalBackLightColor;
    vec4 GlobalAmbientLightColor;
    mat4 CSMShadowMatrix;

    float GlobalBackLightOffset;
    textureHandle GlobalLightShadowBuffer;
    textureHandle TerrainShadowBuffer;
    int NumEnvMips;


    textureHandle EnvironmentMap;

    uvec2 TerrainShadowMapSize;
    vec2 InvTerrainSize;
    vec2 TerrainShadowMapPixelSize;

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

    float GlobalLightShadowBias;

    textureHandle NormalBuffer;
    textureHandle DepthBuffer;
    textureHandle SpecularBuffer;
    textureHandle IrradianceMap;
    textureHandle DepthBufferCopy;

    textureHandle ltcLUT0;
    textureHandle ltcLUT1;
};

// contains the render_state of the camera (and time)
group(FRAME_GROUP) shared constant ViewConstants
{
    mat4 View;
    mat4 InvView;
    mat4 ViewProjection;
    mat4 Projection;
    mat4 InvProjection;
    mat4 InvViewProjection;
    vec4 EyePos;
    vec4 FocalLengthNearFar; // x, y is focal length x/y, z, w is near/far planes
    vec4 Time_Random_Luminance_X; // x is time, y is random, z is luminance, w is unused
};

group(FRAME_GROUP) shared constant ShadowViewConstants[string Visibility = "VS|CS|PS|RGS";]
{
    vec4 CascadeOffset[NUM_CASCADES];
    vec4 CascadeScale[NUM_CASCADES];
    vec4 CascadeDistances;
    ivec4 ShadowTiles[SHADOW_CASTER_COUNT / 4];
    mat4 LightViewMatrix[SHADOW_CASTER_COUNT];
};

group(FRAME_GROUP) sampler_state ShadowSampler
{
    Comparison = true;
    AddressU = Clamp;
    AddressV = Clamp;
};

group(FRAME_GROUP) sampler_state PointLightTextureSampler
{
    Filter = MinMagLinearMipPoint;
};

group(FRAME_GROUP) sampler_state SpotlightTextureSampler
{
    Filter = MinMagLinearMipPoint;
    AddressU = Border;
    AddressV = Border;
    BorderColor = Transparent;
};

//------------------------------------------------------------------------------
/**
        CLUSTERS
*/
//------------------------------------------------------------------------------
const uint CLUSTER_POINTLIGHT_BIT = 0x1u;
const uint CLUSTER_SPOTLIGHT_BIT = 0x2u;
const uint CLUSTER_AREALIGHT_BIT = 0x4u;
const uint CLUSTER_LIGHTPROBE_BIT = 0x8u;
const uint CLUSTER_PBR_DECAL_BIT = 0x10u;
const uint CLUSTER_EMISSIVE_DECAL_BIT = 0x20u;
const uint CLUSTER_FOG_SPHERE_BIT = 0x40u;
const uint CLUSTER_FOG_BOX_BIT = 0x80u;

// set a fixed number of cluster entries
const uint NUM_CLUSTER_ENTRIES = 16384;

struct ClusterAABB
{
    vec4 maxPoint;
    vec4 minPoint;
    uint featureFlags;
};

group(FRAME_GROUP) rw_buffer ClusterAABBs [ string Visibility = "CS|VS|PS|RGS"; ]
{
    ClusterAABB AABBs[];
};

// this is used to keep track of how many lights we have active
group(FRAME_GROUP) shared constant ClusterUniforms [ string Visibility = "CS|VS|PS|RGS"; ]
{
    vec2 FramebufferDimensions;
    vec2 InvFramebufferDimensions;
    uvec2 BlockSize;
    float InvZScale;
    float InvZBias;

    uvec3 NumCells;
    float ZDistribution;
};


//------------------------------------------------------------------------------
/**
        LIGHTS
*/
//------------------------------------------------------------------------------

// increase if we need more lights in close proximity, for now, 128 is more than enough
const uint MAX_LIGHTS_PER_CLUSTER = 128;

struct SpotLight
{
    vec3 position;				// view space position of light
    float range;
    vec3 forward;				// forward vector of light (spotlight and arealights)
    float angleFade;

    vec2 angleSinCos;			// angle cutoffs
    int projectionExtension;	// projection extension index
    int shadowExtension;		// projection extension index

    vec3 color;					// light color
    uint flags;					// feature flags (shadows, projection texture, etc)
};

struct SpotLightProjectionExtension
{
    mat4 projection;					// projection transform
    textureHandle projectionTexture;	// projection texture
};

struct SpotLightShadowExtension
{
    mat4 projection;
    float shadowIntensity;				// intensity of shadows
    uint shadowSlice;
    textureHandle shadowMap;			// shadow map
};

struct PointLight
{
    vec3 position;				// view space position of light, w is range
    float range;

    vec3 color;					// light color
    uint flags;					// feature flags (shadows, projection texture, etc)
};

struct PointLightShadowExtension
{
    float shadowIntensity;		// intensity of shadows
    uint shadowMap;				// shadow map
};

struct AreaLight
{
    vec3 bboxMin;               // Bounding box min point
    float range;
    vec3 bboxMax;               // Bounding box max point
    float radius;

    vec3 xAxis;
    float width;
    vec3 yAxis;
    float height;
    vec3 position;
    uint flags;

    vec3 color;					// light color
    int shadowExtension;		// projection extension index
};

struct AreaLightShadowExtension
{
    mat4 projection;
    float shadowIntensity;				// intensity of shadows
    uint shadowSlice;
    textureHandle shadowMap;			// shadow map
};

group(FRAME_GROUP) shared constant LightUniforms [ string Visibility = "CS|VS|PS"; ]
{
    textureHandle SSAOBuffer;
    uint NumPointLights;
    uint NumSpotLights;
    uint NumAreaLights;
    uint NumLightClusters;
};

// contains amount of lights, and the index of the light (pointing to the indices in PointLightList and SpotLightList), to output
group(FRAME_GROUP) rw_buffer LightIndexLists[string Visibility = "CS|VS|PS|RGS";]
{
    uint PointLightCountList[NUM_CLUSTER_ENTRIES];
    uint PointLightIndexList[NUM_CLUSTER_ENTRIES * MAX_LIGHTS_PER_CLUSTER];
    uint SpotLightCountList[NUM_CLUSTER_ENTRIES];
    uint SpotLightIndexList[NUM_CLUSTER_ENTRIES * MAX_LIGHTS_PER_CLUSTER];
    uint AreaLightCountList[NUM_CLUSTER_ENTRIES];
    uint AreaLightIndexList[NUM_CLUSTER_ENTRIES * MAX_LIGHTS_PER_CLUSTER];
};

group(FRAME_GROUP) rw_buffer LightLists[string Visibility = "CS|VS|PS|RGS";]
{
    SpotLight SpotLights[1024];
    SpotLightProjectionExtension SpotLightProjection[256];
    SpotLightShadowExtension SpotLightShadow[16];
    PointLight PointLights[1024];
    PointLightShadowExtension PointLightShadow[16];
    AreaLight AreaLights[1024];
    AreaLightShadowExtension AreaLightShadow[16];
};

//------------------------------------------------------------------------------
/**
        DECALS
*/
//------------------------------------------------------------------------------
// increase if we need more decals in close proximity, for now, 128 is more than enough
#define MAX_DECALS_PER_CLUSTER 128

struct PBRDecal
{
    textureHandle albedo;
    vec4 bboxMin;
    vec4 bboxMax;
    mat4 invModel;
    vec3 direction;
    textureHandle material;
    vec3 tangent;
    textureHandle normal;
};

struct EmissiveDecal
{
    vec4 bboxMin;
    vec4 bboxMax;
    mat4 invModel;
    vec3 direction;
    textureHandle emissive;
};


// this is used to keep track of how many lights we have active
group(FRAME_GROUP) shared constant DecalUniforms [ string Visibility = "CS|PS"; ]
{
    uint NumPBRDecals;
    uint NumEmissiveDecals;
    uint NumDecalClusters;
    textureHandle NormalBufferCopy;
    textureHandle StencilBuffer;
};

// contains amount of lights, and the index of the light (pointing to the indices in PointLightList and SpotLightList), to output
group(FRAME_GROUP) rw_buffer DecalIndexLists [ string Visibility = "CS|PS"; ]
{
    uint EmissiveDecalCountList[NUM_CLUSTER_ENTRIES];
    uint EmissiveDecalIndexList[NUM_CLUSTER_ENTRIES * MAX_DECALS_PER_CLUSTER];
    uint PBRDecalCountList[NUM_CLUSTER_ENTRIES];
    uint PBRDecalIndexList[NUM_CLUSTER_ENTRIES * MAX_DECALS_PER_CLUSTER];
};

group(FRAME_GROUP) rw_buffer DecalLists [ string Visibility = "CS|PS"; ]
{
    EmissiveDecal EmissiveDecals[128];
    PBRDecal PBRDecals[128];
};

//------------------------------------------------------------------------------
/**
        VOLUME FOG
*/
//------------------------------------------------------------------------------


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


// this is used to keep track of how many lights we have active
group(FRAME_GROUP) shared constant VolumeFogUniforms [ string Visibility = "CS|VS|PS"; ]
{
    int DownscaleFog;
    uint NumFogSpheres;
    uint NumFogBoxes;
    uint NumVolumeFogClusters;
    vec3 GlobalAbsorption;
    float GlobalTurbidity;
};

// contains amount of lights, and the index of the light (pointing to the indices in PointLightList and SpotLightList), to output
group(FRAME_GROUP) rw_buffer FogIndexLists [ string Visibility = "CS|VS|PS"; ]
{
    uint FogSphereCountList[NUM_CLUSTER_ENTRIES];
    uint FogSphereIndexList[NUM_CLUSTER_ENTRIES * MAX_FOGS_PER_CLUSTER];
    uint FogBoxCountList[NUM_CLUSTER_ENTRIES];
    uint FogBoxIndexList[NUM_CLUSTER_ENTRIES * MAX_FOGS_PER_CLUSTER];
};

group(FRAME_GROUP) rw_buffer FogLists [ string Visibility = "CS|VS|PS"; ]
{
    FogSphere FogSpheres[128];
    FogBox FogBoxes[128];
};

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
