//------------------------------------------------------------------------------
//  skybox.fx
//  (C) 2012-2021 Individual contributors, See LICENSE file
//------------------------------------------------------------------------------
#include <lib/std.fxh>
#include <lib/shared.fxh>
#include <lib/objects_shared.fxh>
#include <lib/util.fxh>
#include <lib/techniques.fxh>
#include <lib/preetham.fxh>
#include <lib/mie-rayleigh.fxh>

#include <material_interfaces.fx>

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
    [slot=2] in ivec2 uv,
    [slot=1] in vec3 normal,
    [slot=3] in vec3 tangent,
    out vec3 UV,
    out vec3 Direction)
{
    vec3 tempPos = normalize(position);
    gl_Position = Projection * vec4(tempPos, 1);
    float animationSpeed = Time_Random_Luminance_X.x * SkyRotationFactor;
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
    vec3 dir = normalize(Direction);
    vec3 atmo = CalculateAtmosphericScattering(dir, GlobalLightDirWorldspace.xyz) * GlobalLightColor.rgb;
    
    // rotate uvs around center with constant speed
    /*
    vec3 baseColor = sampleCubeLod(EnvironmentMap, SkySampler, UV, 0).rgb;
    vec3 blendColor = sampleCubeLod(SkyLayer2, SkySampler, UV, 0).rgb;
    vec3 color = mix(baseColor, blendColor, SkyBlendFactor);
    color = ((color - 0.5f) * Contrast) + 0.5f;
    color *= Brightness;
    */

    Color = vec4(atmo, 1);
    gl_FragDepth = 1.0f;
}

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(Default, "Static", vsMain(), psMain(), SkyboxState);
