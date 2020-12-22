//------------------------------------------------------------------------------
//  lightscatter.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"
    
float Density = 0.93f;
float Decay = 0.74f;
float Weight = 0.20f;
float Exposure = 0.21f;
vec2 LightPos = vec2(0.5f, 0.5f);

#define NUM_GLOBAL_SAMPLES 32
#define NUM_LOCAL_SAMPLES 16

/// Declaring used textures
sampler2D ColorSource;

sampler_state ColorSampler
{
    Samplers = { ColorSource };
    Filter = Point;
    AddressU = Clamp;
    AddressV = Clamp;
};

render_state LightScatterState
{
    CullMode = Back;
    DepthWrite = false;
};

//------------------------------------------------------------------------------
/**
*/
shader
void
vsMain(
    [slot=0] in vec3 position,
    [slot=2] in vec2 uv,
    out vec2 UV) 
{
    gl_Position = vec4(position, 1);
    UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psMainLocal(in vec2 UV,
    [color0] out vec4 Color) 
{   
    vec2 screenUV = UV;
    vec2 lightScreenPos = LightPos;
    lightScreenPos.y = 1 - lightScreenPos.y;
    vec2 deltaTexCoord = vec2(screenUV - lightScreenPos);
    deltaTexCoord *= 1.0f / NUM_LOCAL_SAMPLES * Density;
    vec4 color = textureLod(ColorSource, screenUV, 0);
    float alpha = color.a;
    float illuminationDecay = 1.0f;
    
    for (int i = 0; i < NUM_LOCAL_SAMPLES; i++)
    {
        screenUV -= deltaTexCoord;
        vec4 sampleColor = textureLod(ColorSource, screenUV, 0);
        sampleColor *= illuminationDecay * Weight;
        color += sampleColor * alpha;
        illuminationDecay *= Decay;
    }
    color *= Exposure;
    Color = vec4(color.rgb, alpha);
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psMainGlobal(in vec2 UV,
    [color0] out vec4 Color) 
{
    vec2 screenUV = UV; 
    vec2 lightScreenPos = LightPos;
    lightScreenPos.y = 1 - lightScreenPos.y;
    vec2 deltaTexCoord = vec2(screenUV - lightScreenPos);
    deltaTexCoord *= 1.0f / NUM_GLOBAL_SAMPLES * Density;
    vec4 color = textureLod(ColorSource, screenUV, 0);
    float alpha = color.a;
    float illuminationDecay = 1.0f;
    
    for (int i = 0; i < NUM_GLOBAL_SAMPLES; i++)
    {
        screenUV -= deltaTexCoord;
        vec3 sampleColor = textureLod(ColorSource, screenUV, 0).rgb;
        sampleColor *= illuminationDecay * Weight;
        color += vec4(sampleColor, 0);
        illuminationDecay *= Decay;
    }
    color *= Exposure;
    Color = vec4(color.rgb, alpha);
}

//------------------------------------------------------------------------------
/**
*/
//SimpleTechnique(LocalScatter, "Light|Local", vsMain(), psMainLocal(), LightScatterState);
SimpleTechnique(GlobalScatter, "Global", vsMain(), psMainGlobal(), LightScatterState);
