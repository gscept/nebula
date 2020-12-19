//------------------------------------------------------------------------------
//  emissive.fx
//  (C) 2015 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/shared.fxh"
#include "lib/objects_shared.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"

float EmissiveIntensity = 0.0f;

/// Declaring used textures
textureHandle EmissiveMap;

/// Declaring used samplers
sampler_state DefaultSampler
{
    //Samplers = { EmissiveMap };
};

render_state EmissiveState
{
    CullMode = Back;
    DepthEnabled = true;
    DepthFunc = Equal;
    BlendEnabled[0] = true;
    SrcBlend[0] = SrcAlpha;
    DstBlend[0] = OneMinusSrcAlpha;
};


//------------------------------------------------------------------------------
/**
*/
shader
void
vsMain(
    [slot=0] in vec3 position,
    [slot=1] in vec3 normal,
    [slot=2] in vec2 uv,
    [slot=3] in vec3 tangent,
    [slot=4] in vec3 binormal,
    out vec2 UV) 
{
    gl_Position = ViewProjection * Model * vec4(position, 1);
    UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psMain(in vec2 UV,
    [color0] out vec4 Albedo) 
{
    vec4 diffColor = texture(sampler2D(Textures2D[EmissiveMap], DefaultSampler), UV.xy) * EmissiveIntensity;    
    Albedo = EncodeHDR(diffColor);
}


//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(Static, "Static", vsMain(), psMain(), EmissiveState);