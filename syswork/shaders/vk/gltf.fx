//------------------------------------------------------------------------------
//  gltf.fx
//  (C) 2020 Individual contributors, See LICENSE file
//------------------------------------------------------------------------------
#include "lib/std.fxh"
#include "lib/geometrybase.fxh"
#include "lib/techniques.fxh"
#include "lib/standard_shading.fxh"
#include "lib/decals.fxh"

subroutine(CalculateBump) vec3 GLTFNormalMapFunctor(in vec3 tangent, in vec3 normal, in float sign, in vec4 bumpData)
{
    vec3 tan = normalize(tangent);
    vec3 norm = normalize(normal);
    vec3 bin = cross(norm, tan) * sign;
    mat3 tangentViewMatrix = mat3(tan, bin, norm);
    vec3 tNormal = vec3(0, 0, 0);
    tNormal.xy = (bumpData.xy * 2.0f) - 1.0f;
    tNormal.z = saturate(sqrt(1.0f - dot(tNormal.xy, tNormal.xy)));
    return tangentViewMatrix * normalize((tNormal * vec3(_GLTF.normalScale, _GLTF.normalScale, 1.0f)));
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psGLTFDepthOnlyAlphaMask(in vec2 UV)
{
    const vec4 baseColor = sample2D(_GLTF.baseColorTexture, MaterialSampler, UV);
    if (baseColor.a <= _GLTF.alphaCutoff)
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
        [slot = 2] in ivec2 uv,
        [slot = 3] in vec4 tangent,
        out vec4 Tangent,
        out vec3 Normal,
        out vec2 UV,
        out vec3 WorldSpacePos
)
{
    vec4 modelSpace = Model * vec4(position, 1);
    gl_Position = ViewProjection * modelSpace;
    UV = UnpackUV(uv);

    Tangent     = vec4((Model * vec4(tangent.xyz, 0)).xyz, tangent.w);
    Normal      = (Model * vec4(normal, 0)).xyz;
    WorldSpacePos = modelSpace.xyz;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsGLTFSkinned(
    [slot=0] in vec3 position,
    [slot=2] in ivec2 uv,
    [slot=1] in vec3 normal,
    [slot=3] in vec4 tangent,
    [slot=7] in vec4 weights,
    [slot=8] in uvec4 indices,
    out vec4 Tangent,
    out vec3 Normal,    
    out vec2 UV,
    out vec3 WorldSpacePos
)
{
    vec4 skinnedPos      = SkinnedPosition(position, weights, indices);
    vec4 skinnedNormal   = SkinnedNormal(normal, weights, indices);
    vec4 skinnedTangent  = SkinnedNormal(tangent.xyz, weights, indices);

    vec4 modelSpace = Model * skinnedPos;
    gl_Position = ViewProjection * modelSpace;
    
    UV            = UnpackUV(uv);
    Tangent 	  = vec4(normalize((Model * skinnedTangent).xyz), tangent.w);
    Normal 		  = normalize((Model * skinnedNormal).xyz);
    WorldSpacePos = modelSpace.xyz;
}

//------------------------------------------------------------------------------
/**
*/
[early_depth]
shader
void
psGLTF(
    in vec4 Tangent,
    in vec3 Normal,
    in vec2 UV,
    in vec3 WorldSpacePos,
    [color0] out vec4 OutColor,
    [color1] out vec4 OutNormal,
    [color2] out vec4 OutSpecular)
{
    vec4 baseColor = calcColor(sample2D(_GLTF.baseColorTexture, MaterialSampler, UV) * _GLTF.baseColorFactor);
    vec4 metallicRoughness = sample2D(_GLTF.metallicRoughnessTexture, MaterialSampler, UV) * vec4(1.0f, _GLTF.roughnessFactor, _GLTF.metallicFactor, 1.0f);
    vec4 emissive = sample2D(_GLTF.emissiveTexture, MaterialSampler, UV) * _GLTF.emissiveFactor;
    vec4 occlusion = sample2D(_GLTF.occlusionTexture, MaterialSampler, UV);
    vec4 normals = sample2D(_GLTF.normalTexture, NormalSampler, UV);
    vec4 material;
    material[MAT_METALLIC] = metallicRoughness.b;
    material[MAT_ROUGHNESS] = metallicRoughness.g;
    material[MAT_CAVITY] = occlusion.r;
    material[MAT_EMISSIVE] = 0.0f; // TODO: Add support for emissive
    vec3 N = normalize(calcBump(Tangent.xyz, Normal, Tangent.w, normals));
    
    vec3 viewVec = normalize(EyePos.xyz - WorldSpacePos.xyz);
    vec3 F0 = CalculateF0(baseColor.rgb, material[MAT_METALLIC], vec3(0.04));
    
    float viewDepth = CalculateViewDepth(View, WorldSpacePos);
    uint3 index3D = CalculateClusterIndex(gl_FragCoord.xy / BlockSize, viewDepth, InvZScale, InvZBias);
    uint idx = Pack3DTo1D(index3D, NumCells.x, NumCells.y);
    
    vec3 light = vec3(0, 0, 0);
    light += CalculateLight(WorldSpacePos, gl_FragCoord.xyz, baseColor.rgb, material, N);
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
    GLTFSkinned,
    "Skinned",
    vsGLTFSkinned(),
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
