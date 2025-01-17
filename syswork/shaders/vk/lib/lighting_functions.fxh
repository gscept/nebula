//------------------------------------------------------------------------------
//  lighting_functions.fxh
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "shadowbase.fxh"
#include "pbr.fxh"
#include "ltc.fxh"
#include "CSM.fxh"
#include "clustering.fxh"
#include "ddgi.fxh"

// match these in lightcontext.cc
const uint USE_SHADOW_BITFLAG = 0x1;
const uint USE_PROJECTION_TEX_BITFLAG = 0x2;
const uint AREA_LIGHT_SHAPE_RECT = 0x4;
const uint AREA_LIGHT_SHAPE_DISK = 0x8;
const uint AREA_LIGHT_SHAPE_TUBE = 0x10;
const uint AREA_LIGHT_TWOSIDED = 0x20;

#define FlagSet(x, flags) ((x & flags) == flags)


#define SPECULAR_SCALE 13
#define ROUGHNESS_TO_SPECPOWER(x) exp2(SPECULAR_SCALE * x + 1)

//------------------------------------------------------------------------------
/**
    GLTF recommended inverse square falloff
    https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_lights_punctual/README.md#range-property
*/
float 
InvSquareFalloff(float maxRange, float currentDistance, vec3 lightDir)
{
    return max(min(1.0f - pow(currentDistance / maxRange, 4.0f), 1.0f), 0.0f) / pow(currentDistance, 2);
}

//------------------------------------------------------------------------------
/**
*/
float
FalloffWindow(float radius, vec3 lightDir)
{
    float dist2 = dot(lightDir, lightDir);
    return saturate(1.0f - dist2 / sqr(radius));
}

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
    in vec3 pos,
    in vec3 viewVec, 
    in vec3 normal, 
    in float depth, 
    in vec4 material, 
    in vec3 diffuseColor,
    in vec3 F0)
{
    vec3 lightDir = (light.position.xyz - pos);
    float lightDirLen = length(lightDir);

    float d2 = lightDirLen * lightDirLen;
    float factor = d2 / (light.range * light.range);
    float sf = saturate(1.0 - factor * factor);
    float att = (sf * sf) / max(d2, 0.0001);

    float oneDivLightDirLen = 1.0f / lightDirLen;
    lightDir = lightDir * oneDivLightDirLen;

    vec3 H = normalize(lightDir.xyz + viewVec);
    float NL = saturate(dot(lightDir, normal));
    float NH = saturate(dot(normal, H));
    float NV = saturate(dot(normal, viewVec));
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
    , in vec3 pos
)
{
    vec4 projLightPos = projExt.projection * vec4(pos, 1.0f);
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
    , in vec3 pos
)
{
    vec4 shadowProjLightPos = shadowExt.projection * vec4(pos, 1.0f);
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
    in vec3 pos,
    in vec3 viewVec, 
    in vec3 normal, 
    in vec4 material, 
    in vec3 diffuseColor,
    in vec3 F0)
{
    vec3 lightDir = (light.position - pos);
    float lightDirLen = length(lightDir);

    float att = InvSquareFalloff(light.range, lightDirLen, lightDir);

    float oneDivLightDirLen = 1.0f / lightDirLen;
    lightDir = lightDir * oneDivLightDirLen;

    float theta = dot(light.forward.xyz, lightDir);
    float intensity = saturate((theta - light.angleSinCos.y) * light.angleFade);

    vec4 lightModColor = intensity.xxxx * att;
    lightModColor *= projection;

    vec3 H = normalize(lightDir.xyz + viewVec);
    float NL = saturate(dot(lightDir, normal));
    float NH = saturate(dot(normal, H));
    float NV = saturate(dot(normal, viewVec));
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
    , in vec3 pos
    , in vec3 viewVec
    , in vec3 normal
    , in vec4 material
    , in vec3 albedo
    , in bool twoSided
)
{
    vec3 lightDir = (li.position.xyz - pos);
    float attenuation = FalloffWindow(li.range, lightDir);
    if (attenuation < 0.0001f)
        return vec3(0);

    // Calculate LTC LUT uv
    float NV = saturate(dot(normal, viewVec));
    float ltcRoughness = sqr(material[MAT_ROUGHNESS]);
    vec2 uv = vec2(clamp(0.01f, 0.98f, ltcRoughness), sqrt(1.0f - NV));
    uv = uv * LUT_SCALE + LUT_BIAS;

    // Sample LTC LUTs
    vec4 t1 = sample2D(ltcLUT0, LinearSampler, uv);

    // Transform 4 rect points to light
    vec3 points[4];
    vec3 dx = li.xAxis * li.width;
    vec3 dy = li.yAxis * li.height;

    points[0] = li.position - dx - dy;
    points[1] = li.position + dx - dy;
    points[2] = li.position + dx + dy;
    points[3] = li.position - dx + dy;

    // Construct linear cosine transform
    mat3 minv = mat3
    (
        vec3(t1.x, 0, t1.y)
        , vec3(0, 1, 0)
        , vec3(t1.z, 0, t1.w)
    );

    // Integrate specular
    vec3 spec = vec3(LtcRectIntegrate(normal, viewVec, pos, minv, points, true, twoSided));

    // Integrate diffuse
    vec3 diff = vec3(LtcRectIntegrate(normal, viewVec, pos, mat3(1), points, false, twoSided)) * albedo;

    return li.color * (spec + diff) * attenuation;
}

//------------------------------------------------------------------------------
/**
*/
vec3
CalculateDiskLight(
    in AreaLight li
    , in vec3 pos
    , in vec3 viewVec
    , in vec3 normal
    , in vec4 material
    , in vec3 albedo
    , in bool twoSided
)
{
    vec3 lightDir = (li.position.xyz - pos);
    float attenuation = FalloffWindow(li.range, lightDir);
    if (attenuation < 0.0001f)
        return vec3(0);

     // Calculate LTC LUT uv
    float NV = saturate(dot(normal, viewVec));
    float ltcRoughness = sqr(material[MAT_ROUGHNESS]);
    vec2 uv = vec2(clamp(0.01f, 0.98f, ltcRoughness), sqrt(1.0f - NV));
    uv = uv * LUT_SCALE + LUT_BIAS;

    // Sample LTC LUTs
    vec4 t1 = sample2DLod(ltcLUT0, LinearSampler, uv, 0);

    // Transform 4 rect points to light
    vec3 points[3];
    
    // Because of some numerical instability, we have to slightly increase the size in Y for the disk
    vec3 dx = li.xAxis * li.width;
    vec3 dy = li.yAxis * li.height;
    points[0] = li.position + dx - dy;
    points[1] = li.position - dx - dy;
    points[2] = li.position - dx + dy;

    // Construct linear cosine transform
    mat3 minv = mat3
    (
        vec3(t1.x, 0, t1.y)
        , vec3(0, 1, 0)
        , vec3(t1.z, 0, t1.w)
    );

    // Integrate specular
    vec3 spec = vec3(LtcDiskIntegrate(normal, viewVec, pos, minv, points, true, twoSided));

    // Integrate diffuse
    vec3 diff = vec3(LtcDiskIntegrate(normal, viewVec, pos, mat3(1), points, false, twoSided)) * albedo;

    return li.color * (diff + spec) * attenuation;
}

//------------------------------------------------------------------------------
/**
*/
vec3 
CalculateTubeLight(
    in AreaLight li
    , in vec3 pos
    , in vec3 viewVec
    , in vec3 normal
    , in vec4 material
    , in vec3 albedo
    , in bool twoSided
)
{
    vec3 lightDir = (li.position.xyz - pos);
    float attenuation = FalloffWindow(li.range, lightDir);
    if (attenuation < 0.0001f)
        return vec3(0);

    // Calculate LTC LUT uv
    float NV = saturate(dot(normal, viewVec));
    float ltcRoughness = sqr(material[MAT_ROUGHNESS]);
    vec2 uv = vec2(clamp(0.01f, 0.98f, ltcRoughness), sqrt(1.0f - NV));
    uv = uv * LUT_SCALE + LUT_BIAS;

    // Sample LTC LUTs
    vec4 t1 = sample2DLod(ltcLUT0, LinearSampler, uv, 0);

    vec3 points[2];
    vec3 dx = li.xAxis * li.width;
    vec3 dy = li.yAxis * li.height;
    points[0] = li.position - dx - dy;
    points[1] = li.position + dx + dy;

    // Construct linear cosine transform
    mat3 minv = mat3
    (
        vec3(t1.x, 0, t1.y)
        , vec3(0, 1, 0)
        , vec3(t1.z, 0, t1.w)
    );

    // Integrate specular
    vec3 spec = vec3(LtcLineIntegrate(normal, viewVec, pos, li.radius, minv, points));

    // Integrate diffuse
    vec3 diff = vec3(LtcLineIntegrate(normal, viewVec, pos, li.radius, mat3(1), points)) * albedo;

    return li.color * (spec + diff) * (1.0f / 2 * PI) * attenuation;
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
CalculateGlobalLight(vec3 diffuseColor, vec4 material, vec3 F0, vec3 viewVec, vec3 worldSpaceNormal, vec3 worldSpacePosition)
{
    float NL = saturate(dot(GlobalLightDirWorldspace.xyz, worldSpaceNormal));
    if (NL <= 0) { return vec3(0); }

#ifdef CSM_DEBUG
    vec4 csmDebug;
#endif

    float shadowFactor = 1.0f;
    if (FlagSet(GlobalLightFlags, USE_SHADOW_BITFLAG))
    {
        vec4 shadowPos = CSMShadowMatrix * vec4(worldSpacePosition, 1); // csm contains inversed view + csm transform
        shadowFactor = CSMPS(shadowPos,	GlobalLightShadowBuffer
#ifdef CSM_DEBUG
        , csmDebug  
#endif
        );

        if (EnableTerrainShadows == 1)
        {
            vec2 terrainUv = mad(worldSpacePosition.xz, InvTerrainSize, vec2(0.5f));
            //shadowFactor *= sample2DLod(TerrainShadowBuffer, CSMTextureSampler, terrainUv, 0).r;
            vec2 terrainShadow = TerrainShadows(TerrainShadowBuffer, terrainUv, TerrainShadowMapPixelSize); 
            float blend = abs(worldSpacePosition.y - terrainShadow.y * 0.8f) / (terrainShadow.y - terrainShadow.y * 0.8f);
            shadowFactor *= terrainShadow.x * blend;
        }
        
        //shadowFactor *= terrainShadow.x < 1.0f ?  * terrainShadow.x : 1.0f;
        //shadowFactor *= lerp(1.0f, terrainShadow.x, smoothstep(terrainShadow.y * 0.8f, terrainShadow.y, worldSpacePosition.y));

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
    @param normal       	The fragments normal
    @param depth			The fragments depth (gl_FragCoord.z)
*/
vec3
LocalLights(uint clusterIndex, vec3 viewVec, vec3 diffuseColor, vec4 material, vec3 F0, vec3 pos, vec3 normal, float depth)
{
    vec3 light = vec3(0, 0, 0);
    uint flag = AABBs[clusterIndex].featureFlags;
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
                pos,
                viewVec,
                normal,
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
                shadow = CalculateSpotLightShadow(li, SpotLightShadow[li.shadowExtension], pos);
            if (li.projectionExtension != -1)
                projection = CalculateSpotLightProjection(li, SpotLightProjection[li.projectionExtension], pos);

            light += CalculateSpotLight(
                li,
                projection,
                shadow,
                pos,
                viewVec,
                normal,
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
                    , pos
                    , viewVec
                    , normal
                    , material
                    , diffuseColor
                    , CHECK_FLAG(li.flags, AREA_LIGHT_TWOSIDED)
                );
            }
            else if (CHECK_FLAG(li.flags, AREA_LIGHT_SHAPE_DISK))
            {
                light += CalculateDiskLight(
                    li
                    , pos
                    , viewVec
                    , normal
                    , material
                    , diffuseColor
                    , CHECK_FLAG(li.flags, AREA_LIGHT_TWOSIDED)
                );
            }
            else if (CHECK_FLAG(li.flags, AREA_LIGHT_SHAPE_TUBE))
            {
                light += CalculateTubeLight(
                    li
                    , pos
                    , viewVec
                    , normal
                    , material
                    , diffuseColor
                    , CHECK_FLAG(li.flags, AREA_LIGHT_TWOSIDED)
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
GI(uint clusterIndex, vec3 viewVec, vec3 pos, vec3 normal, vec3 albedo)
{
    vec3 accumulatedGi = vec3(0, 0, 0);
    uint flag = AABBs[clusterIndex].featureFlags;
    if (CHECK_FLAG(flag, CLUSTER_GI_VOLUME_BIT))
    {
        uint count = GIVolumeCountList[clusterIndex];
        for (int i = 0; i < count; i++)
        {
            uint lidx = GIVolumeIndexLists[clusterIndex * MAX_GI_VOLUMES_PER_CLUSTER + i];
            GIVolume gi = GIVolumes[lidx];
            vec3 relativePosition = pos - gi.Offset;
            /// TODO: Rotate
            if (relativePosition.x > gi.Size.x || relativePosition.y > gi.Size.y || relativePosition.z > gi.Size.z)
                continue;
                
                
            vec3 edgeDistance = gi.Size - abs(relativePosition);
            float edgeMinDistance = min(min(edgeDistance.x, edgeDistance.y), edgeDistance.z);
            float weight = 0.0f;
            if (gi.Blend == 0.0f)
                weight = (edgeMinDistance < gi.BlendCutoff) ? 0.0f : 1.0f;
            else
                weight = clamp((edgeMinDistance - gi.BlendCutoff) / gi.Blend, 0.0f, 1.0f);
            
            vec3 surfaceBias = DDGISurfaceBias(normal, viewVec, gi.NormalBias, gi.ViewBias);
            //light += vec3(1,0,0);
            vec3 volumeGI = max(vec3(0), EvaluateDDGIIrradiance(pos, surfaceBias, normal, gi, gi.Options) * (albedo / PI) / gi.IrradianceScale);
            accumulatedGi = mix(accumulatedGi, volumeGI, vec3(weight));
        }
    }
    return accumulatedGi;
}

//------------------------------------------------------------------------------
/**
*/
vec3
CalculatePointLightAmbientTransmission(
    in PointLight light,
    in PointLightShadowExtension ext,
    in vec3 pos,
    in vec3 viewVec,
    in vec3 normal,
    in float depth,
    in vec4 material,
    in vec4 albedo,
    in float transmission)
{
    vec3 lightDir = (light.position.xyz - pos);
    float lightDirLen = length(lightDir);

    float d2 = lightDirLen * lightDirLen;
    float factor = d2 / (light.range * light.range);
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
    float att = InvSquareFalloff(light.range, lightDirLen, lightDir);
    lightDir = lightDir / lightDirLen;

    float theta = dot(light.forward.xyz, lightDir);
    float intensity = saturate((theta - light.angleSinCos.y) * light.angleFade);

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
CalculateGlobalLightAmbientTransmission(vec3 pos, vec3 viewVec, vec3 normal, float depth, vec4 material, vec4 albedo, in float transmission)
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
        vec4 shadowPos = CSMShadowMatrix * vec4(pos, 1); // csm contains inversed view + csm transform
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

#define USE_SCALARIZATION_LOOP 0
//------------------------------------------------------------------------------
/**
*/
vec3
CalculateLight(vec3 worldSpacePos, vec3 clipXYZ, vec3 albedo, vec4 material, vec3 normal)
{
    float viewDepth = CalculateViewDepth(View, worldSpacePos);
    uint3 index3D = CalculateClusterIndex(clipXYZ.xy / BlockSize, viewDepth, InvZScale, InvZBias);
    uint idx = Pack3DTo1D(index3D, NumCells.x, NumCells.y);

    vec3 light = vec3(0, 0, 0);
    vec3 viewVec = normalize(EyePos.xyz - worldSpacePos.xyz);
    vec3 F0 = CalculateF0(albedo.rgb, material[MAT_METALLIC], vec3(0.04));
    light += CalculateGlobalLight(albedo, material, F0, viewVec, normal, worldSpacePos);

#if USE_SCALARIZATION_LOOP
    // Get the mask for the invocation
    uint4 laneMask = gl_SubgroupEqMask;

    // Get the execution mask for the wavefront
    uint4 execMask = subgroupBallot(true);
    uint firstWaveIndex = subgroupBroadcastFirst(idx);

    // Check if all waves use the same index and do this super cheaply
    if (subgroupBallot(firstWaveIndex == idx) == execMask)
    {
        light += LocalLights(firstWaveIndex, viewVec, albedo, material, F0, worldSpacePos, normal, clipXYZ.z);
        light += GI(firstWaveIndex, viewVec, worldSpacePos, normal, albedo);
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
                light += LocalLights(scalarIdx, viewVec, albedo, material, F0, worldSpacePos, normal, clipXYZ.z);
                light += GI(scalarIdx, viewVec, worldSpacePos, normal, albedo);
            }
        }
    }
#else
    light += LocalLights(idx, viewVec, albedo, material, F0, worldSpacePos, normal, clipXYZ.z);
    light += GI(idx, viewVec, worldSpacePos, normal, albedo);
#endif

    //light += IBL(albedo, F0, normal, viewVec, material);
    light += albedo.rgb * material[MAT_EMISSIVE];
    return light;
}