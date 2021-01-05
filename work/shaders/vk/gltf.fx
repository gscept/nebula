//------------------------------------------------------------------------------
//  gltf.fx
//  (C) 2020 Individual contributors, See LICENSE file
//------------------------------------------------------------------------------
#include "lib/std.fxh"
#include "lib/geometrybase.fxh"
#include "lib/techniques.fxh"
#include "lib/standard_shading.fxh"
#include "lib/decals.fxh"

group(BATCH_GROUP) shared constant GLTFBlock
{
    // lower camel case names by design, just to keep it consistent with the GLTF standard.
    textureHandle baseColorTexture;
    textureHandle normalTexture;
    textureHandle metallicRoughnessTexture;
    textureHandle emissiveTexture;
    textureHandle occlusionTexture;
    vec4 baseColorFactor;
    vec4 emissiveFactor;
    float metallicFactor;
    float roughnessFactor;
    float normalScale;
    float alphaCutoff;
};

float Greyscale(in vec4 color)
{
    return dot(color.rgb, vec3(0.299, 0.587, 0.114));
}

subroutine(CalculateBump) vec3 GLTFNormalMapFunctor(in vec3 tangent, in vec3 binormal, in vec3 normal, in vec4 bumpData)
{
    mat3 tangentViewMatrix = mat3(normalize(tangent.xyz), normalize(binormal.xyz), normalize(normal.xyz));
    vec3 tNormal = vec3(0, 0, 0);
    tNormal.xy = (bumpData.xy * 2.0f) - 1.0f;
    tNormal.z = saturate(sqrt(1.0f - dot(tNormal.xy, tNormal.xy)));
    return tangentViewMatrix * normalize((tNormal * vec3(normalScale, normalScale, 1.0f)));
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psGLTFDepthOnlyAlphaMask(in vec2 UV)
{
    const vec4 baseColor = sample2D(baseColorTexture, MaterialSampler, UV);
    if (baseColor.a <= alphaCutoff)
        discard;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsGLTFStatic(
        [slot = 0] in vec3 position,
        [slot = 1] in vec3 normal,
        [slot = 2] in vec2 uv,
        [slot = 3] in vec3 tangent,
        [slot = 4] in vec3 binormal,
        out vec3 Tangent,
        out vec3 Normal,
        out vec3 Binormal,
        out vec2 UV,
        out vec3 WorldSpacePos,
        out vec4 ViewSpacePos)
{
    vec4 modelSpace = Model * vec4(position, 1);
    gl_Position = ViewProjection * modelSpace;
    UV = uv;

    Tangent = (Model * vec4(tangent, 0)).xyz;
    Normal = (Model * vec4(normal, 0)).xyz;
    Binormal = (Model * vec4(binormal, 0)).xyz;
    WorldSpacePos = modelSpace.xyz;
    ViewSpacePos = View * modelSpace;
}

//------------------------------------------------------------------------------
/**
*/
[earlydepth]
shader
void
psGLTF(
    in vec3 Tangent,
    in vec3 Normal,
    in vec3 Binormal,
    in vec2 UV,
    in vec3 WorldSpacePos,
    in vec4 ViewSpacePos,
    [color0] out vec4 OutColor,
    [color1] out vec4 OutNormal,
    [color2] out vec4 OutSpecular)
{
    vec2 seed = gl_FragCoord.xy * RenderTargetDimensions[0].zw;
	float dither = hash12(seed);
	if (dither < DitherFactor)
		discard;

    uint3 index3D = CalculateClusterIndex(gl_FragCoord.xy / BlockSize, ViewSpacePos.z, InvZScale, InvZBias);
    uint idx = Pack3DTo1D(index3D, NumCells.x, NumCells.y);

    vec4 baseColor = calcColor(sample2D(baseColorTexture, MaterialSampler, UV) * baseColorFactor);
    vec4 metallicRoughness = sample2D(metallicRoughnessTexture, MaterialSampler, UV) * vec4(1.0f, roughnessFactor, metallicFactor, 1.0f);
    vec4 emissive = sample2D(emissiveTexture, MaterialSampler, UV) * emissiveFactor;
    vec4 occlusion = sample2D(occlusionTexture, MaterialSampler, UV);
    vec4 normals = sample2D(normalTexture, NormalSampler, UV);
    vec4 material;
    material[MAT_METALLIC] = metallicRoughness.b;
    material[MAT_ROUGHNESS] = metallicRoughness.g;
    material[MAT_CAVITY] = Greyscale(occlusion);
    material[MAT_EMISSIVE] = 0.0f;
    vec3 N = normalize(calcBump(Tangent, Binormal, Normal, normals));
    
    //ApplyDecals(idx, ViewSpacePos, vec4(WorldSpacePos, 1), gl_FragCoord.z, baseColor, N, material);
    
    vec3 viewVec = normalize(EyePos.xyz - WorldSpacePos.xyz);
    float NdotV = saturate(dot(N, viewVec));
    vec3 F0 = CalculateF0(baseColor.rgb, material[MAT_METALLIC], vec3(0.04));
    
    vec3 viewNormal = (View * vec4(N.xyz, 0)).xyz;
    
    vec3 light = vec3(0, 0, 0);
    light += CalculateGlobalLight(baseColor.rgb, material, F0, viewVec, N.xyz, ViewSpacePos);
    light += LocalLights(idx, baseColor.rgb, material, F0, ViewSpacePos, viewNormal, gl_FragCoord.z);
    light += calcEnv(baseColor, F0, N, viewVec, material);
    light += emissive.rgb;
    
    OutColor = finalizeColor(light.rgb, baseColor.a);
    OutNormal = vec4(N, 0);
    OutSpecular = vec4(F0, material[MAT_ROUGHNESS]);
}

//------------------------------------------------------------------------------
//  Techniques
//------------------------------------------------------------------------------
SimpleTechnique(GLTFStaticDepthAlphaMask, "Static|Depth|AlphaMask", vsDepthStaticAlphaMask(), psGLTFDepthOnlyAlphaMask(), DepthState);
SimpleTechnique(GLTFStaticDepthAlphaMaskDoubleSided, "Static|Depth|AlphaMask|DoubleSided", vsDepthStaticAlphaMask(), psGLTFDepthOnlyAlphaMask(), DepthStateDoubleSided);

SimpleTechnique(
    GLTFStatic,
    "Static",
    vsGLTFStatic(),
    psGLTF(
        calcColor = SimpleColor,
        calcBump = GLTFNormalMapFunctor,
        calcEnv = IBL,
        finalizeColor = FinalizeOpaque
    ),
    DefaultState);

SimpleTechnique(
    GLTFStaticDoubleSided,
    "Static|DoubleSided",
    vsGLTFStatic(),
    psGLTF(
        calcColor = SimpleColor,
        calcBump = GLTFNormalMapFunctor,
        calcEnv = IBL,
        finalizeColor = FinalizeOpaque
    ),
    DoubleSidedState);

SimpleTechnique(
    GLTFStaticAlphaMask,
    "Static|AlphaMask",
    vsGLTFStatic(),
    psGLTF(
        calcColor = AlphaMaskSimpleColor,
        calcBump = GLTFNormalMapFunctor,
        calcEnv = IBL,
        finalizeColor = FinalizeOpaque
    ),
    DefaultState);

SimpleTechnique(
    GLTFStaticAlphaMaskDoubleSided,
    "Static|AlphaMask|DoubleSided",
    vsGLTFStatic(),
    psGLTF(
        calcColor = AlphaMaskSimpleColor,
        calcBump = GLTFNormalMapFunctor,
        calcEnv = IBL,
        finalizeColor = FinalizeOpaque
    ),
    DoubleSidedState);

SimpleTechnique(
    GLTFStaticAlphaBlend,
    "Static|AlphaBlend",
    vsGLTFStatic(),
    psGLTF(
        calcColor = AlphaMaskAlphaColor,
        calcBump = GLTFNormalMapFunctor,
        calcEnv = IBL,
        finalizeColor = FinalizeAlpha
    ),
    AlphaState);

SimpleTechnique(
    GLTFStaticAlphaBlendDoubleSided,
    "Static|AlphaBlend|DoubleSided",
    vsGLTFStatic(),
    psGLTF(
        calcColor = AlphaMaskAlphaColor,
        calcBump = GLTFNormalMapFunctor,
        calcEnv = IBL,
        finalizeColor = FinalizeAlpha
    ),
    AlphaDoubleSidedState);
