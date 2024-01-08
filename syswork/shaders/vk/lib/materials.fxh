//------------------------------------------------------------------------------
//  materials.fxh
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

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

ptr struct BSDFMaterial
{
    vec3 MatAlbedoIntensity;
    textureHandle AlbedoMap;
    float MatRoughnessIntensity;
    textureHandle ParameterMap;
    textureHandle NormalMap;
    textureHandle AbsorptionTexture;
    textureHandle ScatterTexture;
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

ptr struct UnlitMaterial
{
    textureHandle AlbedoMap;
};

ptr struct Unlit2Material
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