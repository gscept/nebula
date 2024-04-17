//------------------------------------------------------------------------------
//  unit.fx
//  (C) 2012 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#define USE_CUSTOM_MATERIAL
#include "lib/techniques.fxh"
#include "lib/colorblending.fxh"
#include "lib/standard_shading.fxh"

group(INSTANCE_GROUP) shared constant UnitInstanceBlock
{
    vec4 TeamColor;
};

sampler_state TeamSampler
{
    // Samplers = { TeamColorMask };
    Filter = MinMagMipLinear;
    AddressU = Wrap;
    AddressV = Wrap;
};

//------------------------------------------------------------------------------
/**
    Not a ubershader
*/
[early_depth]
shader
void
psNotaUnit(
    in vec3 Tangent,
    in vec3 Normal,
    in flat float Sign,
    in vec2 UV,
    in vec3 WorldSpacePos,
    [color0] out vec4 OutColor)
{
    float viewDepth = CalculateViewDepth(View, WorldSpacePos);
    uint3 index3D = CalculateClusterIndex(gl_FragCoord.xy / BlockSize, viewDepth, InvZScale, InvZBias);
    uint idx = Pack3DTo1D(index3D, NumCells.x, NumCells.y);

    float teamMask = sample2D(_Nota.TeamColorMask, TeamSampler, UV).r;
    vec4 maskColor = TeamColor * teamMask;
    vec4 albedo = vec4(sample2D(_Nota.AlbedoMap, MaterialSampler, UV).rgb, 1.0f) * _Nota.MatAlbedoIntensity;
    vec4 material = calcMaterial(sample2D(_Nota.ParameterMap, MaterialSampler, UV));
    vec3 N = normalize(calcBump(Tangent, Normal, Sign, sample2D(_Nota.NormalMap, NormalSampler, UV)));

    albedo.rgb += Overlay(albedo.rgb, maskColor.rgb);

    //ApplyDecals(idx, ViewSpacePos, vec4(WorldSpacePos, 1), gl_FragCoord.z, albedo, N, material);

    vec3 viewVec = normalize(EyePos.xyz - WorldSpacePos.xyz);
    vec3 F0 = CalculateF0(albedo.rgb, material[MAT_METALLIC], vec3(0.04));

    vec3 light = vec3(0, 0, 0);
    light += CalculateLight(WorldSpacePos, gl_FragCoord.xyz, albedo.rgb, material, N);
    light += calcEnv(albedo, F0, N, viewVec, material);

    OutColor = finalizeColor(light.rgb, albedo.a);
}

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(
    Static,
    "Static",
    vsStatic(),
    psNotaUnit(
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcEnv = IBL,
        finalizeColor = FinalizeOpaque
    ),
    DefaultState);

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(
    Skinned,
    "Skinned",
    vsSkinned(),
    psNotaUnit(
        calcBump = NormalMapFunctor,
        calcMaterial = DefaultMaterialFunctor,
        calcEnv = IBL,
        finalizeColor = FinalizeOpaque
    ),
    DefaultState);
