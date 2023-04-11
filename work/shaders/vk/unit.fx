//------------------------------------------------------------------------------
//  unit.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "lib/techniques.fxh"
#include "lib/colorblending.fxh"
#include "lib/standard_shading.fxh"

group(BATCH_GROUP) shared constant UnitBlock
{
    textureHandle TeamColorMask;
    textureHandle SpecularMap;
};

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
[earlydepth]
shader
void
psNotaUnit(
    in vec3 Tangent,
    in vec3 Normal,
    in flat float Sign,
    in vec2 UV,
    in vec3 WorldSpacePos,
    in vec4 ViewSpacePos,
    [color0] out vec4 OutColor,
    [color1] out vec4 OutNormal,
    [color2] out vec4 OutSpecular)
{
    vec4 diffColor = sample2D(AlbedoMap, TeamSampler, UV) * vec4(MatAlbedoIntensity.rgb, 1);
    float roughness = sample2D(ParameterMap, TeamSampler, UV).r * MatRoughnessIntensity;
    vec4 specColor = sample2D(SpecularMap, TeamSampler, UV) * MatSpecularIntensity;
    float cavity = 1.0f;
    float teamMask = sample2D(TeamColorMask, TeamSampler, UV).r;
    vec4 maskColor = TeamColor * teamMask;

    vec4 normals = sample2D(NormalMap, NormalSampler, UV);
    vec3 N = normalize(calcBump(Tangent, Normal, Sign, normals));

    vec4 material = vec4(specColor.r, roughness, cavity, 0);
    vec4 albedo = diffColor + vec4(Overlay(diffColor.rgb, maskColor.rgb), 0);

    uint3 index3D = CalculateClusterIndex(gl_FragCoord.xy / BlockSize, ViewSpacePos.z, InvZScale, InvZBias);
    uint idx = Pack3DTo1D(index3D, NumCells.x, NumCells.y);

    //ApplyDecals(idx, ViewSpacePos, vec4(WorldSpacePos, 1), gl_FragCoord.z, albedo, N, material);
    
    vec3 viewVec = normalize(EyePos.xyz - WorldSpacePos.xyz);
    vec3 F0 = CalculateF0(albedo.rgb, material[MAT_METALLIC], vec3(0.04));
    vec3 viewNormal = (View * vec4(N.xyz, 0)).xyz;
    
    vec3 light = vec3(0, 0, 0);
    light += CalculateGlobalLight(albedo.rgb, material, F0, viewVec, N.xyz, ViewSpacePos, vec4(WorldSpacePos, 1));
    light += LocalLights(idx, albedo.rgb, material, F0, ViewSpacePos, viewNormal, gl_FragCoord.z);
    light += calcEnv(albedo, F0, N, viewVec, material);
    light += albedo.rgb * material[MAT_EMISSIVE];
    
    OutColor = finalizeColor(light.rgb, albedo.a);
    OutNormal = vec4(N, 0);
    OutSpecular = vec4(F0, material[MAT_ROUGHNESS]);
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
