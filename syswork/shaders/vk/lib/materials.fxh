//------------------------------------------------------------------------------
//  materials.fxh
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#pragma once
#include <lib/std.fxh>
// This header provides a few helpers to bind a material buffer
#define MATERIAL_BINDING group(BATCH_GROUP) binding(51)

ptr struct BRDFMaterial
{
    vec3 MatAlbedoIntensity;
    textureHandle AlbedoMap;
    float MatRoughnessIntensity;
    textureHandle ParameterMap;
    textureHandle NormalMap;
};

ptr struct GLTFMaterial
{
    vec4 baseColorFactor;
    vec4 emissiveFactor;
    float metallicFactor;
    float roughnessFactor;
    float normalScale;
    float alphaCutoff;
    textureHandle baseColorTexture;
    textureHandle normalTexture;
    textureHandle metallicRoughnessTexture;
    textureHandle emissiveTexture;
    textureHandle occlusionTexture;
};

ptr struct BSDFMaterial
{
    vec3 MatAlbedoIntensity;
    textureHandle AlbedoMap;
    float MatRoughnessIntensity;
    textureHandle ParameterMap;
    textureHandle NormalMap;
    textureHandle AbsorptionMap;
    textureHandle ScatterMap;
};

ptr struct TransparentMaterial
{
    vec3 MatAlbedoIntensity;
    textureHandle AlbedoMap;
    textureHandle ParameterMap;
    textureHandle NormalMap;
    float blendFactor;
};

ptr struct CutoutMaterial
{
    vec3 MatAlbedoIntensity;
    textureHandle AlbedoMap;
    textureHandle ParameterMap;
    textureHandle NormalMap;
    float cutoutFactor;
};

ptr alignment(4) struct UnlitMaterial
{
    textureHandle AlbedoMap;
};

ptr alignment(8) struct Unlit2Material
{
    textureHandle Layer1;
    textureHandle Layer2;
};

ptr struct Unlit3Material
{
    textureHandle Layer1;
    textureHandle Layer2;
    textureHandle Layer3;
};

ptr struct Unlit4Material
{
    textureHandle Layer1;
    textureHandle Layer2;
    textureHandle Layer3;
    textureHandle Layer4;
};

ptr struct SkyboxMaterial
{
    textureHandle SkyLayer1;
    textureHandle SkyLayer2;
    float Contrast;
    float Brightness;
};

ptr struct LegacyMaterial
{
    vec4 MatAlbedoIntensity;
    vec4 MatSpecularIntensity;

    float AlphaSensitivity;
    float AlphaBlendFactor;
    float MatRoughnessIntensity;
    float MatMetallicIntensity;

    textureHandle AlbedoMap;
    textureHandle ParameterMap;
    textureHandle NormalMap;
};

ptr struct TerrainMaterial
{
    textureHandle LowresAlbedoFallback;
    textureHandle LowresNormalFallback;
    textureHandle LowresMaterialFallback;
    float WorldSize;
};

MATERIAL_BINDING rw_buffer MaterialBindings
{
    BRDFMaterial BRDFMaterials;
    BSDFMaterial BSDFMaterials;
    GLTFMaterial GLTFMaterials;
    UnlitMaterial UnlitMaterials;
    Unlit2Material Unlit2Materials;
    Unlit3Material Unlit3Materials;
    Unlit4Material Unlit4Materials;
    SkyboxMaterial SkyboxMaterials;
    LegacyMaterial LegacyMaterials;
    TerrainMaterial TerrainMaterials;
};