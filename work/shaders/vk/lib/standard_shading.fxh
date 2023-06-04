//------------------------------------------------------------------------------
//  standard_shading.fxh
//  (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#ifndef STANDARD_SHADING_FXH
#define STANDARD_SHADING_FXH 

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"
#include "lib/defaultsamplers.fxh"
#include "lib/pbr.fxh"
#include "lib/geometrybase.fxh"
#include "lib/materialparams.fxh"
#include "lib/clustering.fxh"
#include "lib/lighting_functions.fxh"

//---------------------------------------------------------------------------------------------------------------------------
//											FINALIZE COLOR
//---------------------------------------------------------------------------------------------------------------------------
prototype vec4 FinalizeColor(in vec3 color, in float alpha);

subroutine (FinalizeColor) vec4 FinalizeOpaque(
    in vec3 color,
    in float alpha)
{
    return vec4(color, 1.0f);
}

subroutine (FinalizeColor) vec4 FinalizeAlpha(
    in vec3 color,
    in float alpha)
{
    return vec4(color, alpha);
}

FinalizeColor finalizeColor;

//------------------------------------------------------------------------------
/**
    Standard shader for conductors (metals) and dielectic materials (non-metal)
*/
[early_depth]
shader
void
psStandard(
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

    vec4 albedo   = calcColor(sample2D(AlbedoMap, MaterialSampler, UV)) * MatAlbedoIntensity;
    vec4 material = calcMaterial(sample2D(ParameterMap, MaterialSampler, UV));
    vec3 N        = normalize(calcBump(Tangent, Normal, Sign, sample2D(NormalMap, NormalSampler, UV)));
    
    //ApplyDecals(idx, ViewSpacePos, vec4(WorldSpacePos, 1), gl_FragCoord.z, albedo, N, material);
    vec3 viewVec = normalize(EyePos.xyz - WorldSpacePos.xyz);
    vec3 F0 = CalculateF0(albedo.rgb, material[MAT_METALLIC], vec3(0.04));

    vec3 light = vec3(0, 0, 0);
    light += CalculateLight(WorldSpacePos, gl_FragCoord.xyz, albedo.rgb, material, N);
    light += calcEnv(albedo, F0, N, viewVec, material);
    
    OutColor = finalizeColor(light.rgb, albedo.a);
    //OutNormal = vec4(N, 0);
    //OutSpecular = vec4(F0, material[MAT_ROUGHNESS]);
}

#endif
