//------------------------------------------------------------------------------
//  sprite.fx
//  (C) 2015 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/shared.fxh"
#include "lib/objects_shared.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"
#include "lib/materialparams.fxh"

float Brightness = 0.0f;
int NumTilesX = 10;
int NumTilesY = 10;
int NumTilesPerSec = 1;

/// Declaring used textures
sampler2D AlbedoMap;

/// Declaring used samplers
sampler_state DefaultSampler
{
    Samplers = { AlbedoMap };
};

render_state SpriteOpaqueState
{
    CullMode = None;
};

render_state SpriteAlphaState
{
    BlendEnabled[0] = true;
    SrcBlend[0] = SrcAlpha;
    DstBlend[0] = OneMinusSrcAlpha;
    CullMode = None;
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
    int tileDiff = int(floor(TimeAndRandom.x * NumTilesPerSec));
    int xTile = tileDiff % NumTilesX;
    int yTile = tileDiff / NumTilesX;
    
    vec2 tiledUv = (uv + vec2(xTile, yTile)) * 1 / vec2(NumTilesX, NumTilesY);
    UV = tiledUv;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psMain(in vec2 UV,
    [color0] out vec4 Albedo) 
{
    vec4 diffColor = texture(AlbedoMap, UV.xy);
    float alpha = diffColor.a;
    if (alpha < AlphaSensitivity) discard;
    Albedo = EncodeHDR(diffColor * Brightness);
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psMainAlpha(in vec2 UV,
    [color0] out vec4 Albedo) 
{
    vec4 diffColor = texture(AlbedoMap, UV.xy);
    float alpha = diffColor.a;
    if (alpha < AlphaSensitivity) discard;
    Albedo = EncodeHDR(diffColor * AlphaBlendFactor * Brightness);
}


//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(Static, "Static", vsMain(), psMain(), SpriteOpaqueState);
SimpleTechnique(Alpha, "Alpha", vsMain(), psMainAlpha(), SpriteAlphaState);