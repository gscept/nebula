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
#include "lib/lights_clustered.fxh"

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
shader
void
psStandard(
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

    vec4 albedo   = calcColor(sample2D(AlbedoMap, MaterialSampler, UV)) * MatAlbedoIntensity;
    vec4 material = calcMaterial(sample2D(ParameterMap, MaterialSampler, UV));
    vec3 N        = normalize(calcBump(Tangent, Binormal, Normal, sample2D(NormalMap, NormalSampler, UV)));
    
    //ApplyDecals(idx, ViewSpacePos, vec4(WorldSpacePos, 1), gl_FragCoord.z, albedo, N, material);
    
    vec3 viewVec = normalize(EyePos.xyz - WorldSpacePos.xyz);
    vec3 F0 = CalculateF0(albedo.rgb, material[MAT_METALLIC], vec3(0.04));
    vec3 viewNormal = (View * vec4(N.xyz, 0)).xyz;
    
    vec3 light = vec3(0, 0, 0);
    light += CalculateGlobalLight(albedo.rgb, material, F0, viewVec, N.xyz, ViewSpacePos);
    light += LocalLights(idx, albedo.rgb, material, F0, ViewSpacePos, viewNormal, gl_FragCoord.z);
    light += calcEnv(albedo, F0, N, viewVec, material);
    light += albedo.rgb * material[MAT_EMISSIVE];
    
    OutColor = finalizeColor(light.rgb, albedo.a);
    OutNormal = vec4(N, 0);
    OutSpecular = vec4(F0, material[MAT_ROUGHNESS]);
}

#endif
