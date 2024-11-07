//------------------------------------------------------------------------------
//  shared.gpuh
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#ifndef SHARED_GPUH
#define SHARED_GPUH

#include <lib/std.gpuh>
#include <lib/util.gpuh>

const MAX_TEXTURES = 65535;
const NUM_CASCADES = 4;
const SHADOW_CASTER_COUNT = 16;

#define FLT_MAX     3.40282347E+38F
#define FLT_MIN     -3.40282347E+38F

group(TICK_GROUP) binding(0) uniform Textures1D : [MAX_TEXTURES]* texture1D;
group(TICK_GROUP) binding(0) uniform Textures1DArray : [MAX_TEXTURES]* texture1DArray;
group(TICK_GROUP) binding(0) uniform Textures2D : [MAX_TEXTURES]* texture2DArray;
group(TICK_GROUP) binding(0) uniform Textures2DArray : [MAX_TEXTURES]* texture2DArray;
group(TICK_GROUP) binding(0) uniform Textures2DMS : [MAX_TEXTURES]* texture2DMS;
group(TICK_GROUP) binding(0) uniform Textures2DMSArray : [MAX_TEXTURES]* texture2DMSArray;
group(TICK_GROUP) binding(0) uniform Textures3D : [MAX_TEXTURES]* texture3D;
group(TICK_GROUP) binding(0) uniform TexturesCube : [MAX_TEXTURES]* textureCube;
group(TICK_GROUP) binding(0) uniform TexturesCubeArray : [MAX_TEXTURES]* textureCubeArray;
sampler_state Basic2DSampler{};
sampler_state PointSampler { Filter = Point; AddressU = Clamp; AddressV = Clamp; };
sampler_state LinearSampler { Filter = Linear; AddressU = Clamp; AddressV = Clamp; };
 

#define sample2D(handle, sampler, uv)						    textureSample(Textures2D[handle], sampler, uv)
#define sample2DLod(handle, sampler, uv, lod)				    textureSampleLod(Textures2D[handle], sampler, uv, lod)
#define sample2DGrad(handle, sampler, uv, ddx, ddy)			    textureSampleGrad(Textures2D[handle], sampler, uv, ddx, ddy)

#define sample2DMS(handle, sampler, uv)						    textureSample(Textures2DMS[handle], sampler, uv)
#define sample2DMSLod(handle, sampler, uv, lod)				    textureSampleLod(Textures2DMS[handle], sampler, uv, lod)
#define sample2DMSGrad(handle, sampler, uv, ddx, ddy)		    textureSampleGrad(Textures2D[handle], sampler, uv, ddx, ddy)

#define sampleCube(handle, sampler, uvw)					    textureSample(TexturesCube[handle], sampler, uvw)
#define sampleCubeLod(handle, sampler, uvw, lod)			    textureSampleLod(TexturesCube[handle], sampler, uvw, lod)

#define sample2DArray(handle, sampler, uvw)					    textureSample(Textures2DArray[handle], sampler, uvw)
#define sample2DArrayGrad(handle, sampler, uvw, ddx, ddy)	    textureSampleGrad(Textures2DArray[handle], sampler, uvw, ddx, ddy)
#define sample2DArrayLod(handle, sampler, uvw, lod)			    textureSampleLod(Textures2DArray[handle], sampler, uvw, lod)

#define sample3D(handle, sampler, uvw)						    textureSample(Textures3D[handle], sampler, uvw)
#define sample3DLod(handle, sampler, uvw, lod)				    textureSampleLod(Textures3D[handle], sampler, uvw, lod)

#define sample2DArrayShadow(handle, sampler, uv, index, depth)	textureSampleCompare(Textures2DArray[handle], sampler, f32x3(uv, index), depth)

#define fetch2D(handle, sampler, uv, lod)					    textureFetch(Textures2D[handle], uv, lod)
#define fetch2DMS(handle, sampler, uv, lod)					    textureFetch(Textures2DMS[handle], uv, lod)
#define fetchCube(handle, sampler, uvw, lod)				    textureFetch(Textures2DArray[handle], uvw, lod)
#define fetch2DArray(handle, sampler, uvw, lod)				    textureFetch(Textures2DArray[handle], uvw, lod)
#define fetch3D(handle, sampler, uvw, lod)					    textureFetch(Textures3D[handle], uvw, lod)
#define fetchStencil(handle, sampler, uv, lod)				    (castToU32(textureFetch(Textures2D[handle], sampler, uv, lod).r))

#define query_lod2D(handle, sampler, uv)                        textureQueryLod(sampler2D(Textures2D[handle], sampler), uv)

// these parameters are updated once per application tick
struct PerTickParams
{
    WindDirection : f32;

    WindWaveSize : f32;
    WindSpeed : f32;
    WindIntensity : f32;
    WindForce : f32;

    Saturation : f32;
    MaxLuminance : f32;
    FadeValue : f32;
    UseDof : u32;

    Balance : f32x4;

    DoFDistances : f32x3;

    BloomColor : f32x3;
    BloomIntensity : f32;
    
    FogDistances : f32x4;
    FogColor : f32x4;

    // global light stuff
    GlobalLightFlags : u32;
    GlobalLightShadowIntensity : f32;
    GlobalLightShadowMapSize : f32x2;
    GlobalLightDirWorldspace : f32x4;
    GlobalLightColor : f32x4;
    GlobalBackLightColor : f32x4;
    GlobalAmbientLightColor : f32x4;
    CSMShadowMatrix : f32x4x4;

    GlobalBackLightOffset : f32;
    GlobalLightShadowBuffer : u32;
    TerrainShadowBuffer : u32;
    NumEnvMips : i32;

    EnvironmentMap : u32;

    TerrainShadowMapSize : u32x2;
    InvTerrainSize : u32x2;
    TerrainShadowMapPixelSize : u32x2;

    // these params are for the Preetham sky model
    A : f32x4;
    B : f32x4;
    C : f32x4;
    D : f32x4;
    E : f32x4;
    Z : f32x4;

    RayleighFactor : f32;
    RayleighZenithLength : f32;
    RefractiveIndex : f32;
    DepolarizationFactor : f32;
    Molecules : f32;
    MieV : f32;
    MieCoefficient : f32;
    MieDirectionalG : f32;
    MieZenithLength : f32;
    Turbidity : f32;
    SunIntensityFactor : f32;
    SunIntensityFalloff : f32;
    SunDiscSize : f32;
    MieKCoefficient : f32x3;
    PrimaryColors : f32x3;
    TonemapWeight : f32;
    Lum : f32;

    // CSM params

    GlobalLightShadowBias : f32;

    NormalBuffer : u32;
    DepthBuffer : u32;
    SpecularBuffer : u32;
    IrradianceMap : u32;
    DepthBufferCopy : u32;

    ltcLUT0 : u32;
    ltcLUT1 : u32;
};
group(TICK_GROUP) uniform PerTickParams : *TickParams;

// contains the render_state of the camera (and time)
struct ViewParams
{
    View : f32x4x4;
    InvView : f32x4x4;
    ViewProjection : f32x4x4;
    Projection : f32x4x4;
    InvProjection : f32x4x4;
    InvViewProjection : f32x4x4;
    EyePos : f32x4;
    FocalLengthNearFar : f32x4; // x, y is focal length x/y, z, w is near/far planes
    Time_Random_Luminance_X : f32x4; // x is time, y is random, z is luminance, w is unused
};
group(FRAME_GROUP) uniform ViewConstants : *ViewParams;

struct ShadowViewParams
{
    CascadeOffset : [NUM_CASCADES]f32x4;
    CascadeScale : [NUM_CASCADES]f32x4;
    CascadeDistances : f32x4;
    ShadowTiles : [SHADOW_CASTER_COUNT / 4] i32x4;
    LightViewMatrix : [SHADOW_CASTER_COUNT] f32x4x4;
};
group(FRAME_GROUP) uniform ShadowViewConstants : *ShadowViewParams;

sampler_state ShadowSampler
{
    Comparison = true;
    AddressU = Clamp;
    AddressV = Clamp;
};

sampler_state PointLightTextureSampler
{
    Filter = MinMagLinearMipPoint;
};

sampler_state SpotlightTextureSampler
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
const CLUSTER_POINTLIGHT_BIT = 0x1u;
const CLUSTER_SPOTLIGHT_BIT = 0x2u;
const CLUSTER_AREALIGHT_BIT = 0x4u;
const CLUSTER_LIGHTPROBE_BIT = 0x8u;
const CLUSTER_PBR_DECAL_BIT = 0x10u;
const CLUSTER_EMISSIVE_DECAL_BIT = 0x20u;
const CLUSTER_FOG_SPHERE_BIT = 0x40u;
const CLUSTER_FOG_BOX_BIT = 0x80u;

// set a fixed number of cluster entries
const NUM_CLUSTER_ENTRIES = 16384;

struct ClusterAABB
{
    maxPoint : f32x4;
    minPoint : f32x4;
    featureFlags : u32;
};
group(FRAME_GROUP) uniform ClusterAABBs : [] *mutable ClusterAABB; 

// this is used to keep track of how many lights we have active
struct ClusterParams
{
    FramebufferDimensions : f32x2;
    InvFramebufferDimensions : f32x2;
    BlockSize : u32x2;
    InvZScale : f32;
    InvZBias : f32;

    NumCells : u32x3;
    ZDistribution : f32;
};
group(FRAME_GROUP) uniform ClusterUniforms : *ClusterParams;

//------------------------------------------------------------------------------
/**
        LIGHTS
*/
//------------------------------------------------------------------------------

// increase if we need more lights in close proximity, for now, 128 is more than enough
const MAX_LIGHTS_PER_CLUSTER = 128;

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

group(FRAME_GROUP) constant LightUniforms [ string Visibility = "CS|VS|PS"; ]
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
group(FRAME_GROUP) constant DecalUniforms [ string Visibility = "CS|PS"; ]
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
group(FRAME_GROUP) constant VolumeFogUniforms [ string Visibility = "CS|VS|PS"; ]
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

struct RenderTargetParameters
{
    vec4 Dimensions;    // render target dimensions are size (xy) inversed size (zw)
    vec2 Scale;         // dimensions / viewport
};

group(PASS_GROUP) constant PassBlock
{
    RenderTargetParameters RenderTargetParameter[16]; // render target dimensions are size (xy) inversed size (zw)
};

#endif // SHARED_H
