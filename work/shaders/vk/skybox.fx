//------------------------------------------------------------------------------
//  skybox.fx
//  (C) 2012-2021 Individual contributors, See LICENSE file
//------------------------------------------------------------------------------
#include "lib/std.fxh"
#include "lib/shared.fxh"
#include "lib/objects_shared.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"
#include "lib/preetham.fxh"
#include "lib/mie-rayleigh.fxh"

group(BATCH_GROUP) shared constant SkyBlock
{
    // declare two textures, one main texture and one blend texture together with a wrapping sampler
    textureHandle SkyLayer1;
    textureHandle SkyLayer2;
    float Contrast;
    float Brightness;
    float SkyBlendFactor;
    float SkyRotationFactor;
};

sampler_state SkySampler
{
    AddressU = Wrap;
    AddressV = Wrap;
    AddressW = Wrap;
    Filter = MinMagLinearMipPoint;
};

render_state SkyboxState
{
    CullMode = Front;
    DepthEnabled = true;
    DepthWrite = false;
    DepthFunc = Equal;
};

//------------------------------------------------------------------------------
/**
    Sky box vertex shader
*/
shader
void
vsMain(
    [slot=0] in vec3 position,
    [slot=1] in vec3 normal,
    [slot=2] in vec2 uv,
    [slot=3] in vec3 tangent,
    [slot=4] in vec3 binormal,
    out vec3 UV,
    out vec3 Direction)
{
    vec3 tempPos = normalize(position);
    gl_Position = Projection * vec4(tempPos, 1);
    float animationSpeed = TimeAndRandom.x * SkyRotationFactor;
    mat3 rotMat = mat3( cos(animationSpeed), 0, sin(animationSpeed),
                        0, 1, 0,
                        -sin(animationSpeed), 0, cos(animationSpeed));

    float3 viewSample = (InvView * vec4(tempPos, 0)).xyz;
    Direction = viewSample;
    UV = viewSample * rotMat;
}

//------------------------------------------------------------------------------
/**
    Skybox pixel shader
*/
shader
void
psMain(in vec3 UV,
    in vec3 Direction,
    [color0] out vec4 Color)
{
    vec3 lightDir = normalize(GlobalLightDirWorldspace.xyz);
    vec3 dir = normalize(Direction);
    //vec3 atmo = Preetham(dir, lightDir, A, B, C, D, E, Z) * GlobalLightColor.rgb;
    vec3 atmo = CalculateAtmosphericScattering(dir, GlobalLightDirWorldspace.xyz) * GlobalLightColor.rgb;
    
    // rotate uvs around center with constant speed
    vec3 baseColor = sampleCubeLod(EnvironmentMap, SkySampler, UV, 0).rgb;
    vec3 blendColor = sampleCubeLod(SkyLayer2, SkySampler, UV, 0).rgb;
    vec3 color = mix(baseColor, blendColor, SkyBlendFactor);
    color = ((color - 0.5f) * Contrast) + 0.5f;
    color *= Brightness;
    color = atmo;

    Color = vec4(color, 1);
    gl_FragDepth = 1.0f;
}

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(Default, "Static", vsMain(), psMain(), SkyboxState);
