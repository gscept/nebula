//------------------------------------------------------------------------------
//  reflectionprojector.fx
//  (C) 2015 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"
#include "lib/shared.fxh"
#include "lib/pbr.fxh"

sampler2D NormalMap;
sampler2D ParameterMap;
sampler2D AlbedoMap;
sampler2D DepthMap;
readwrite r16f image2D DistanceFieldWeightMap;

shared constant ReflectionProjectorBlock
{
    mat4 Transform;
    mat4 InvTransform;
    vec4 BBoxMin;
    vec4 BBoxMax;
    vec4 BBoxCenter;
    float FalloffDistance = 0.2f;
    float FalloffPower = 16.0f;
    int LocalNumEnvMips = 9;
};

samplerCube LocalEnvironmentMap;
samplerCube LocalIrradianceMap;

sampler_state GeometrySampler
{
    Samplers = { NormalMap, DepthMap, ParameterMap, AlbedoMap };
    //Filter = Point;
    AddressU = Wrap;
    AddressV = Wrap;
    AddressW = Wrap;
    Filter = MinMagMipLinear;
};

sampler_state EnvironmentSampler
{
    Samplers = { LocalEnvironmentMap, LocalIrradianceMap };
    Filter = MinMagMipLinear;
    AddressU = Wrap;
    AddressV = Wrap;
    AddressW = Wrap;
};

render_state ProjectorState
{
    BlendEnabled[0] = true;
    //SrcBlend[0] = One;
    //DstBlend[0] = One;
    SrcBlend[0] = SrcAlpha;
    DstBlend[0] = OneMinusSrcAlpha;
    //SrcBlendAlpha[0] = One;
    //DstBlendAlpha[0] = One;
    //SeparateBlend = true;
    CullMode = Front;
    DepthEnabled = true;
    DepthFunc = Greater;
    //DepthClamp = false;
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

CalculateCubemapCorrection calcCubemapCorrection;
//------------------------------------------------------------------------------
/**
*/
shader
void
vsMain(
    [slot=0] in vec3 position,
    [slot=1] in vec3 normal,
    [slot=2] in vec2 uv,
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
    vec2 screenUV = PixelToNormalized(gl_FragCoord.xy, pixelSize.xy);
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
        
        // load biggest distance from texture, basically solving the distance field blending
        float weight = imageLoad(DistanceFieldWeightMap, ivec2(gl_FragCoord.xy)).r;     
        memoryBarrierImage();
        float diff = saturate(distanceFalloff - weight);
        imageStore(DistanceFieldWeightMap, ivec2(gl_FragCoord.xy), vec4(max(weight, distanceFalloff)));
    
        // sample normal and specular, do some pseudo-PBR energy balance between albedo and spec
        vec3 viewSpaceNormal = UnpackViewSpaceNormal(textureLod(NormalMap, screenUV, 0));
        vec4 spec = textureLod(ParameterMap, screenUV, 0);
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
        vec3 refl = textureLod(LocalEnvironmentMap, reflectVec, oneMinusSpec.a * LocalNumEnvMips).rgb * rim;
        vec3 irr = textureLod(LocalIrradianceMap, worldNormal.xyz, 0).rgb;
        Color = vec4(irr * albedo + refl, diff);
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
