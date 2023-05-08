//------------------------------------------------------------------------------
//  lighting_functions.fxh
//  (C) 2019 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "shadowbase.fxh"
#include "pbr.fxh"
#include "ltc.fxh"
#include "CSM.fxh"

// match these in lightcontext.cc
const uint USE_SHADOW_BITFLAG = 0x1;
const uint USE_PROJECTION_TEX_BITFLAG = 0x2;
const uint AREA_LIGHT_SHAPE_RECT = 0x4;
const uint AREA_LIGHT_SHAPE_DISK = 0x8;
const uint AREA_LIGHT_SHAPE_TUBE = 0x10;

#define FlagSet(x, flags) ((x & flags) == flags)


#define SPECULAR_SCALE 13
#define ROUGHNESS_TO_SPECPOWER(x) exp2(SPECULAR_SCALE * x + 1)

//---------------------------------------------------------------------------------------------------------------------------
/**
*/
float
GetInvertedOcclusionSpotLight(float receiverDepthInLightSpace,
    vec2 lightSpaceUv,
    uint Index,
    uint Texture)
{

    // offset and scale shadow lookup tex coordinates
    vec3 shadowUv = vec3(lightSpaceUv, Index);

    // calculate average of 4 closest pixels
    vec2 shadowSample = sample2DArray(Texture, SpotlightTextureSampler, shadowUv).rg;

    // get pixel size of shadow projection texture
    return ChebyshevUpperBound(shadowSample, receiverDepthInLightSpace, 0.000001f);
}

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

//------------------------------------------------------------------------------
/**
    Calculate point light for ambient, specular and shadows
*/
vec3
CalculatePointLight(
    in PointLight light, 
    in PointLightShadowExtension ext, 
    in vec3 viewPos,
    in vec3 viewVec, 
    in vec3 viewSpaceNormal, 
    in float depth, 
    in vec4 material, 
    in vec3 diffuseColor,
    in vec3 F0)
{
    vec3 lightDir = (light.position.xyz - viewPos);
    float lightDirLen = length(lightDir);

    float d2 = lightDirLen * lightDirLen;
    float factor = d2 / (light.position.w * light.position.w);
    float sf = saturate(1.0 - factor * factor);
    float att = (sf * sf) / max(d2, 0.0001);

    float oneDivLightDirLen = 1.0f / lightDirLen;
    lightDir = lightDir * oneDivLightDirLen;

    vec3 H = normalize(lightDir.xyz + viewVec);
    float NL = saturate(dot(lightDir, viewSpaceNormal));
    float NH = saturate(dot(viewSpaceNormal, H));
    float NV = saturate(dot(viewSpaceNormal, viewVec));
    float LH = saturate(dot(H, lightDir.xyz)); 

    vec3 brdf = EvaluateBRDF(diffuseColor, material, F0, H, NV, NL, NH, LH);

    vec3 radiance = light.color * att;
    vec3 irradiance = (brdf * radiance) * saturate(NL);

    float shadowFactor = 1.0f;
    if (FlagSet(light.flags, USE_SHADOW_BITFLAG))
    {
        vec3 projDir = (InvView * vec4(-lightDir, 0)).xyz;
        shadowFactor = GetInvertedOcclusionPointLight(depth, projDir, ext.shadowMap);
        shadowFactor = saturate(lerp(1.0f, saturate(shadowFactor), ext.shadowIntensity));
    }

    return irradiance * shadowFactor;
}

//------------------------------------------------------------------------------
/**
*/
vec4
CalculateSpotLightProjection(
    in SpotLight light
    , in SpotLightProjectionExtension projExt
    , in vec3 viewPos

)
{
    vec4 projLightPos = projExt.projection * vec4(viewPos, 1.0f);
    projLightPos.xy /= projLightPos.ww;
    vec2 lightSpaceUv = projLightPos.xy * vec2(0.5f, 0.5f) + 0.5f;
    return sample2DLod(projExt.projectionTexture, SpotlightTextureSampler, lightSpaceUv, 0);
}

//------------------------------------------------------------------------------
/**
*/
float
CalculateSpotLightShadow(
    in SpotLight light
    , in SpotLightShadowExtension shadowExt
    , in vec3 viewPos
)
{
    vec4 shadowProjLightPos = shadowExt.projection * vec4(viewPos, 1.0f);
    shadowProjLightPos.xyz /= shadowProjLightPos.www;
    vec2 shadowLookup = shadowProjLightPos.xy * vec2(0.5f, -0.5f) + 0.5f;
    shadowLookup.y = 1 - shadowLookup.y;
    float receiverDepth = shadowProjLightPos.z;
    float shadowFactor = GetInvertedOcclusionSpotLight(receiverDepth, shadowLookup, light.shadowExtension, shadowExt.shadowMap);
    return saturate(lerp(1.0f, saturate(shadowFactor), shadowExt.shadowIntensity));
}

//------------------------------------------------------------------------------
/**
*/
vec3
CalculateSpotLight(
    in SpotLight light, 
    in vec4 projection,
    in float shadow,    
    in vec3 viewPos,
    in vec3 viewVec, 
    in vec3 viewSpaceNormal, 
    in float depth, 
    in vec4 material, 
    in vec3 diffuseColor,
    in vec3 F0)
{
    vec3 lightDir = (light.position.xyz - viewPos);

    float lightDirLen = length(lightDir);

    float d2 = lightDirLen * lightDirLen;
    float factor = d2 / (light.position.w * light.position.w);
    float sf = saturate(1.0 - factor * factor);

    float att = (sf * sf) / max(d2, 0.0001);

    float oneDivLightDirLen = 1.0f / lightDirLen;
    lightDir = lightDir * oneDivLightDirLen;

    float theta = dot(light.forward.xyz, lightDir);
    float intensity = saturate((theta - light.angleSinCos.y) * light.forward.w);

    vec4 lightModColor = intensity.xxxx * att;
    lightModColor *= projection;

    vec3 H = normalize(lightDir.xyz + viewVec);
    float NL = saturate(dot(lightDir, viewSpaceNormal));
    float NH = saturate(dot(viewSpaceNormal, H));
    float NV = saturate(dot(viewSpaceNormal, viewVec));
    float LH = saturate(dot(H, lightDir.xyz)); 
    
    vec3 brdf = EvaluateBRDF(diffuseColor, material, F0, H, NV, NL, NH, LH);

    vec3 radiance = light.color;
    vec3 irradiance = (brdf * radiance) * saturate(NL);

    return irradiance * shadow * lightModColor.rgb;
}


//------------------------------------------------------------------------------
/**
*/
vec3
CalculateRectLight(
    in AreaLight li
    , in vec3 viewPos
    , in vec3 viewVec
    , in vec3 viewSpaceNormal
    , in vec4 material
)
{
    // Calculate LTC LUT uv
    float NV = saturate(dot(viewSpaceNormal, viewVec));
    vec2 uv = vec2(1.0f - material[MAT_ROUGHNESS], sqrt(1.0f - NV));
    uv = uv * LUT_SCALE + LUT_BIAS;

    // Sample LTC LUTs
    vec4 t1 = sample2D(ltcLUT0, LinearSampler, uv);
    vec4 t2 = sample2D(ltcLUT1, LinearSampler, uv);

    // Transform 4 rect points to light
    vec3 points[4];
    points[0] = (li.transform * vec4(-0.5, -0.5, 0, 1)).xyz;
    points[1] = (li.transform * vec4(-0.5, 0.5, 0, 1)).xyz;
    points[2] = (li.transform * vec4(0.5, 0.5, 0, 1)).xyz;
    points[3] = (li.transform * vec4(0.5, -0.5, 0, 1)).xyz;

    // Construct linear cosine transform
    mat3 minv = mat3
    (
        vec3(t1.x, 0, t1.y)
        , vec3(0, 1, 0)
        , vec3(t1.z, 0, t1.w)
    );

    // Integrate specular
    vec3 spec = vec3(LtcRectIntegrate(viewSpaceNormal, viewVec, viewPos, minv, points, true, false));
    spec *= li.color * t2.x + (1.0f - li.color) * t2.y;

    // Integrate diffuse
    vec3 diff = vec3(LtcRectIntegrate(viewSpaceNormal, viewVec, viewPos, mat3(1), points, false, false));

    return li.color * (spec + diff);
}

//------------------------------------------------------------------------------
/**
*/
vec3
CalculateDiskLight(
    in AreaLight li
    , in vec3 viewPos
    , in vec3 viewVec
    , in vec3 viewSpaceNormal
    , in vec4 material
)
{
     // Calculate LTC LUT uv
    float NV = saturate(dot(viewSpaceNormal, viewVec));
    vec2 uv = vec2(1.0f - material[MAT_ROUGHNESS], sqrt(1.0f - NV));
    uv = uv * LUT_SCALE + LUT_BIAS;

    // Sample LTC LUTs
    vec4 t1 = sample2D(ltcLUT0, LinearSampler, uv);
    vec4 t2 = sample2D(ltcLUT1, LinearSampler, uv);

    // Transform 4 rect points to light
    vec3 points[3];
    points[0] = (li.transform * vec4(-0.5, -0.5, 0, 1)).xyz;
    points[1] = (li.transform * vec4(-0.5, 0.5, 0, 1)).xyz;
    points[2] = (li.transform * vec4(0.5, 0.5, 0, 1)).xyz;

    // Construct linear cosine transform
    mat3 minv = mat3
    (
        vec3(t1.x, 0, t1.y)
        , vec3(0, 1, 0)
        , vec3(t1.z, 0, t1.w)
    );

    // Integrate specular
    vec3 spec = vec3(LtcDiskIntegrate(viewSpaceNormal, viewVec, viewPos, minv, points, true, false));
    spec *= li.color * t2.x + (1.0f - li.color) * t2.y;

    // Integrate diffuse
    vec3 diff = vec3(LtcDiskIntegrate(viewSpaceNormal, viewVec, viewPos, mat3(1), points, false, false));

    return li.color * (spec + diff);
}

//------------------------------------------------------------------------------
/**
*/
vec3 CalculateTubeLight()
{
    return vec3(0.0f);
}

//------------------------------------------------------------------------------
/**
    @param diffuseColor		Material's diffuse reflectance color
    @param material			Material parameters (metallic, roughness, cavity)
    @param F0				Fresnel reflectance at 0 degree incidence angle
    @param viewVec			Unit vector from cameras worldspace position to fragments world space position.
    @param viewSpacePos		Fragments position in viewspace; used for shadowing.
*/
vec3
CalculateGlobalLight(vec3 diffuseColor, vec4 material, vec3 F0, vec3 viewVec, vec3 worldSpaceNormal, vec4 viewSpacePos, vec4 worldSpacePosition)
{
    float NL = saturate(dot(GlobalLightDirWorldspace.xyz, worldSpaceNormal));
    if (NL <= 0) { return vec3(0); }

#ifdef CSM_DEBUG
    vec4 csmDebug;
#endif

    float shadowFactor = 1.0f;
    if (FlagSet(GlobalLightFlags, USE_SHADOW_BITFLAG))
    {
        vec4 shadowPos = CSMShadowMatrix * viewSpacePos; // csm contains inversed view + csm transform
        shadowFactor = CSMPS(shadowPos,	GlobalLightShadowBuffer
#ifdef CSM_DEBUG
        , csmDebug  
#endif
        );

        vec2 terrainUv = mad(worldSpacePosition.xz, InvTerrainSize, vec2(0.5f));
        //shadowFactor *= sample2DLod(TerrainShadowBuffer, CSMTextureSampler, terrainUv, 0).r;
        shadowFactor *= PCFShadow(TerrainShadowBuffer, terrainUv, TerrainShadowMapPixelSize);

        shadowFactor = lerp(1.0f, shadowFactor, GlobalLightShadowIntensity);
    }

    vec3 H = normalize(GlobalLightDirWorldspace.xyz + viewVec);
    float NV = saturate(dot(worldSpaceNormal, viewVec));
    float NH = saturate(dot(worldSpaceNormal, H));
    float LH = saturate(dot(H, GlobalLightDirWorldspace.xyz));

    vec3 brdf = EvaluateBRDF(diffuseColor, material, F0, H, NV, NL, NH, LH);

    vec3 radiance = GlobalLightColor.xyz;
    vec3 irradiance = (brdf * radiance) * saturate(NL) + GlobalAmbientLightColor.xyz;

#ifdef CSM_DEBUG
    irradiance *= csmDebug.rgb;
#endif
    return irradiance * shadowFactor;
}

//------------------------------------------------------------------------------
/**
    @param clusterIndex		The 1D cluster/bucket index to evaluate
    @param diffuseColor		Material's diffuse reflectance color
    @param material			Material parameters (metallic, roughness, cavity)
    @param F0				Fresnel reflectance at 0 degree incidence angle
    @param viewPos			The fragments position in view space
    @param viewSpaceNormal	The fragments view space normal
    @param depth			The fragments depth (gl_FragCoord.z)
*/
vec3
LocalLights(uint clusterIndex, vec3 diffuseColor, vec4 material, vec3 F0, vec4 viewPos, vec3 viewSpaceNormal, float depth)
{
    vec3 light = vec3(0, 0, 0);
    uint flag = AABBs[clusterIndex].featureFlags;
    vec3 viewVec = -normalize(viewPos.xyz);
    if (CHECK_FLAG(flag, CLUSTER_POINTLIGHT_BIT))
    {
        // shade point lights
        uint count = PointLightCountList[clusterIndex];
        PointLightShadowExtension ext;
        for (int i = 0; i < count; i++)
        {
            uint lidx = PointLightIndexList[clusterIndex * MAX_LIGHTS_PER_CLUSTER + i];
            PointLight li = PointLights[lidx];
            light += CalculatePointLight(
                li,
                ext,
                viewPos.xyz,
                viewVec,
                viewSpaceNormal,
                depth,
                material,
                diffuseColor,
                F0
            );
        }
    }
    if (CHECK_FLAG(flag, CLUSTER_SPOTLIGHT_BIT))
    {
        uint count = SpotLightCountList[clusterIndex];
        SpotLightShadowExtension shadowExt;
        SpotLightProjectionExtension projExt;
        for (int i = 0; i < count; i++)
        {
            uint lidx = SpotLightIndexList[clusterIndex * MAX_LIGHTS_PER_CLUSTER + i];
            SpotLight li = SpotLights[lidx];
            vec4 projection = vec4(1.0f);
            float shadow = 1.0f;

            // if we have extensions, load them from their respective buffers
            if (li.shadowExtension != -1)
                shadow = CalculateSpotLightShadow(li, SpotLightShadow[li.shadowExtension], viewPos.xyz);
            if (li.projectionExtension != -1)
                projection = CalculateSpotLightProjection(li, SpotLightProjection[li.projectionExtension], viewPos.xyz);

            light += CalculateSpotLight(
                li,
                projection,
                shadow,
                viewPos.xyz,
                viewVec,
                viewSpaceNormal,
                depth,
                material,
                diffuseColor,
                F0
            );
        }
    }

    if (CHECK_FLAG(flag, CLUSTER_AREALIGHT_BIT))
    {
        uint count = AreaLightCountList[clusterIndex];
        AreaLightShadowExtension shadowExt;
        for (int i = 0; i < count; i++)
        {
            uint lidx = AreaLightIndexList[clusterIndex * MAX_LIGHTS_PER_CLUSTER + i];
            AreaLight li = AreaLights[lidx];

            if (li.shadowExtension != -1)
                shadowExt = AreaLightShadow[li.shadowExtension];

            if (CHECK_FLAG(li.flags, AREA_LIGHT_SHAPE_RECT))
            {
                light += CalculateRectLight(
                    li
                    , viewPos.xyz
                    , viewVec
                    , viewSpaceNormal
                    , material
                );
            }
            else if (CHECK_FLAG(li.flags, AREA_LIGHT_SHAPE_DISK))
            {
                light += CalculateDiskLight(
                    li
                    , viewPos.xyz
                    , viewVec
                    , viewSpaceNormal
                    , material
                );
            }
            else if (CHECK_FLAG(li.flags, AREA_LIGHT_SHAPE_TUBE))
            {
                light += CalculateTubeLight(

                );
            }
        }
    }
    return light;
}

//------------------------------------------------------------------------------
/**
*/
vec3
CalculatePointLightAmbientTransmission(
    in PointLight light,
    in PointLightShadowExtension ext,
    in vec3 viewPos,
    in vec3 viewVec,
    in vec3 normal,
    in float depth,
    in vec4 material,
    in vec4 albedo,
    in float transmission)
{
    vec3 lightDir = (light.position.xyz - viewPos);
    float lightDirLen = length(lightDir);

    float d2 = lightDirLen * lightDirLen;
    float factor = d2 / (light.position.w * light.position.w);
    float sf = saturate(1.0 - factor * factor);
    float att = (sf * sf) / max(d2, 0.0001);
    lightDir = lightDir / lightDirLen;

    float NL = saturate(dot(lightDir, normal));
    float TNL = saturate(dot(-lightDir, normal)) * transmission;
    vec3 radiance = light.color * att * saturate(NL + TNL) * albedo.rgb;

    float shadowFactor = 1.0f;
    if (FlagSet(light.flags, USE_SHADOW_BITFLAG))
    {
        vec3 projDir = (InvView * vec4(-lightDir, 0)).xyz;
        shadowFactor = GetInvertedOcclusionPointLight(depth, projDir, ext.shadowMap);
        shadowFactor = saturate(lerp(1.0f, saturate(shadowFactor), ext.shadowIntensity));
    }

    return radiance * shadowFactor;
}

//------------------------------------------------------------------------------
/**
*/
vec3
CalculateSpotLightAmbientTransmission(
    in SpotLight light,
    in vec4 projection,
    in float shadow,
    in vec3 viewPos,
    in vec3 viewVec,
    in vec3 normal,
    in float depth,
    in vec4 material,
    in vec4 albedo,
    in float transmission)
{
    vec3 lightDir = (light.position.xyz - viewPos);

    float lightDirLen = length(lightDir);

    float d2 = lightDirLen * lightDirLen;
    float factor = d2 / (light.position.w * light.position.w);
    float sf = saturate(1.0 - factor * factor);

    float att = (sf * sf) / max(d2, 0.0001);
    lightDir = lightDir / lightDirLen;

    float theta = dot(light.forward.xyz, lightDir);
    float intensity = saturate((theta - light.angleSinCos.y) * light.forward.w);

    vec4 lightModColor = intensity.xxxx * att * projection;

    float NL = saturate(dot(lightDir, normal));
    float TNL = saturate(dot(-lightDir, normal)) * transmission;
    vec3 radiance = light.color * saturate(NL + TNL) * albedo.rgb;

    return radiance * shadow * lightModColor.rgb;
}

//------------------------------------------------------------------------------
/**
*/
vec3
CalculateGlobalLightAmbientTransmission(vec4 viewPos, vec3 viewVec, vec3 normal, float depth, vec4 material, vec4 albedo, in float transmission)
{
    float NL = saturate(dot(GlobalLightDirWorldspace.xyz, normal));
    float TNL = saturate(dot(-GlobalLightDirWorldspace.xyz, normal)) * transmission;

    if ((NL + TNL) <= 0) { return vec3(0); }

#ifdef CSM_DEBUG
    vec4 csmDebug;
#endif

    float shadowFactor = 1.0f;
    if (FlagSet(GlobalLightFlags, USE_SHADOW_BITFLAG))
    {
        vec4 shadowPos = CSMShadowMatrix * viewPos; // csm contains inversed view + csm transform
        shadowFactor = CSMPS(shadowPos, GlobalLightShadowBuffer
#ifdef CSM_DEBUG
            , csmDebug
#endif
        
        );
        shadowFactor = lerp(1.0f, shadowFactor, GlobalLightShadowIntensity);
    }

    vec3 radiance = GlobalLightColor.xyz * saturate(NL + TNL);

#ifdef CSM_DEBUG
    radiance *= csmDebug.rgb;
#endif
    return radiance * shadowFactor * albedo.rgb;
}

//------------------------------------------------------------------------------
/**
*/
vec3
LocalLightsAmbientTransmission(
    uint idx,
    vec4 viewPos,
    vec3 viewVec,
    vec3 normal,
    float depth,
    vec4 material,
    vec4 albedo,
    float transmission)
{
    vec3 light = vec3(0, 0, 0);
    uint flag = AABBs[idx].featureFlags;
    if (CHECK_FLAG(flag, CLUSTER_POINTLIGHT_BIT))
    {
        // shade point lights
        uint count = PointLightCountList[idx];
        PointLightShadowExtension ext;
        for (int i = 0; i < count; i++)
        {
            uint lidx = PointLightIndexList[idx * MAX_LIGHTS_PER_CLUSTER + i];
            PointLight li = PointLights[lidx];
            light += CalculatePointLightAmbientTransmission(
                li,
                ext,
                viewPos.xyz,
                viewVec,
                normal,
                depth,
                material,
                albedo,
                transmission
            );
        }
    }
    if (CHECK_FLAG(flag, CLUSTER_SPOTLIGHT_BIT))
    {
        uint count = SpotLightCountList[idx];
        SpotLightShadowExtension shadowExt;
        SpotLightProjectionExtension projExt;
        for (int i = 0; i < count; i++)
        {
            uint lidx = SpotLightIndexList[idx * MAX_LIGHTS_PER_CLUSTER + i];
            SpotLight li = SpotLights[lidx];

            // if we have extensions, load them from their respective buffers
            vec4 projection = vec4(1.0f);
            float shadow = 1.0f;

            // if we have extensions, load them from their respective buffers
            if (li.shadowExtension != -1)
                shadow = CalculateSpotLightShadow(li, SpotLightShadow[li.shadowExtension], viewPos.xyz);
            if (li.projectionExtension != -1)
                projection = CalculateSpotLightProjection(li, SpotLightProjection[li.projectionExtension], viewPos.xyz);

            light += CalculateSpotLightAmbientTransmission(
                li,
                projection,
                shadow,
                viewPos.xyz,
                viewVec,
                normal,
                depth,
                material,
                albedo,
                transmission
            );
        }
    }
    return light;
}

#define USE_SCALARIZATION_LOOP 1
//------------------------------------------------------------------------------
/**
*/
vec3
CalculateLight(vec3 worldSpacePos, vec3 clipXYZ, vec3 viewSpacePos, vec3 albedo, vec4 material, vec3 normal)
{
    uint3 index3D = CalculateClusterIndex(clipXYZ.xy / BlockSize, viewSpacePos.z, InvZScale, InvZBias);
    uint idx = Pack3DTo1D(index3D, NumCells.x, NumCells.y);

    vec3 light = vec3(0, 0, 0);
    vec3 viewVec = normalize(EyePos.xyz - worldSpacePos.xyz);
    vec3 F0 = CalculateF0(albedo.rgb, material[MAT_METALLIC], vec3(0.04));
    light += CalculateGlobalLight(albedo, material, F0, viewVec, normal, vec4(viewSpacePos, 1), vec4(worldSpacePos, 1));

#ifdef USE_SCALARIZATION_LOOP
    // Get the mask for the invocation
    uint4 laneMask = gl_SubgroupEqMask;

    // Get the execution mask for the wavefront
    uint4 execMask = subgroupBallot(true);
    uint firstWaveIndex = subgroupBroadcastFirst(idx);

    // Check if all waves use the same index and do this super cheaply
    if (subgroupBallot(firstWaveIndex == idx) == execMask)
    {
        vec3 viewNormal = (View * vec4(normal.xyz, 0)).xyz;
        light += LocalLights(firstWaveIndex, albedo, material, F0, vec4(viewSpacePos, 1), viewNormal, clipXYZ.z);        
    }
    else
    {
        // Scalarization loop
        while ((laneMask & execMask) != uint4(0))
        {
            uint scalarIdx = subgroupBroadcastFirst(idx);
            uint4 currentMask = subgroupBallot(scalarIdx == idx);
            execMask &= ~currentMask;

            // If this wave uses the cell we loaded into SGPR, use it
            // this will effectively scalarize the light lists
            if (scalarIdx == idx)
            {
                vec3 viewNormal = (View * vec4(normal.xyz, 0)).xyz;
                light += LocalLights(scalarIdx, albedo, material, F0, vec4(viewSpacePos, 1), viewNormal, clipXYZ.z);        
            }
        }
    }
#else
    vec3 viewNormal = (View * vec4(normal.xyz, 0)).xyz;
    light += LocalLights(idx, albedo, material, F0, vec4(viewSpacePos, 1), viewNormal, clipXYZ.z);  
#endif

    //light += IBL(albedo, F0, normal, viewVec, material);
    light += albedo.rgb * material[MAT_EMISSIVE];
    return light;
}