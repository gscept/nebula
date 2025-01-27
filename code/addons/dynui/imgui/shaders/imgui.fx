//------------------------------------------------------------------------------
//  imgui.fx
//
//  Shader for ImGUI rendering
//
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/techniques.fxh" 
#include "lib/shared.fxh"
#include "lib/defaultsamplers.fxh"


// put variables in push-constant block
push constant ImGUI [ string Visibility = "PS|VS"; ]
{
    mat4 TextProjectionModel;
    uint ColorMask;
    uint PackedTextureInfo;
    float RangeMin;
    float RangeMax;
};

group(BATCH_GROUP) sampler_state TextureSampler
{
    Filter = Linear;
};

render_state TextState
{
    BlendEnabled[0] = true;
    SrcBlend[0] = SrcAlpha;
    DstBlend[0] = OneMinusSrcAlpha;
    DepthWrite = false;
    DepthEnabled = false;
    CullMode = None;
    ScissorEnabled = true;
};

//------------------------------------------------------------------------------
/**
*/
void 
UnpackTexture(uint val, out uint id, out uint type, out uint mip, out uint layer, out uint useRange, out uint useAlpha)
{
    const uint TEXTURE_TYPE_MASK = 0xF;
    const uint TEXTURE_LAYER_MASK = 0xFF;
    const uint TEXTURE_MIP_MASK = 0xF;
    const uint TEXTURE_USE_RANGE_MASK = 0x1;
    const uint TEXTURE_USE_ALPHA_MASK = 0x1;
    const uint TEXTURE_ID_MASK = 0xFFF;

    type = val & TEXTURE_TYPE_MASK;
    layer = (val >> 4) & TEXTURE_LAYER_MASK;
    mip = (val >> 12) & TEXTURE_MIP_MASK;
    useRange = (val >> 16) & TEXTURE_USE_RANGE_MASK;
    useAlpha = (val >> 17) & TEXTURE_USE_ALPHA_MASK;
    id = (val >> 18) & TEXTURE_ID_MASK;
}

//------------------------------------------------------------------------------
/**
*/
vec4
UnpackMask(uint val)
{
    return vec4(val & 0x1, (val >> 1) & 0x1, (val >> 2) & 0x1, (val >> 3) & 0x1);
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsMain(
    [slot=0] in vec2 position,
    [slot=1] in vec2 uv,
    [slot=2] in vec4 color, 
    out vec2 UV,
    out vec4 Color) 
{
    vec4 pos = vec4(position, 0, 1);    
    gl_Position = ImGUI.TextProjectionModel * pos;
    Color = color;
    UV = uv;
}

//------------------------------------------------------------------------------
/** 
*/
shader
void
psMain(
    in vec2 UV,
    in vec4 Color,
    [color0] out vec4 FinalColor) 
{
    vec4 texColor;
    uint id, type, layer, mip, useAlpha, useRange;
    UnpackTexture(ImGUI.PackedTextureInfo, id, type, mip, layer, useRange, useAlpha);
    if (type == 0)
        texColor = sample2DLod(id, TextureSampler, UV, mip);
    else if (type == 1)
        texColor = sample2DArrayLod(id, TextureSampler, vec3(UV, layer), mip);
    else if (type == 2)
    {
        ivec3 size = textureSize(make_sampler3D(id, TextureSampler), int(mip));
        texColor = sample3DLod(id, TextureSampler, vec3(UV, layer / float(size.z)), mip);
    }
    
    if (useRange != 0)
    {
        texColor.rgb = (texColor.rgb - ImGUI.RangeMin) / (ImGUI.RangeMax - ImGUI.RangeMin);
    }

    if (useAlpha == 0)
        texColor.a = 1;
        
    vec4 mask = UnpackMask(ImGUI.ColorMask);

    // Since we are using sRGB output, remember to degamma
    FinalColor = Color * texColor * mask;
}


//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(Default, "Static", vsMain(), psMain(), TextState);
