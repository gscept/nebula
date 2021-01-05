//------------------------------------------------------------------------------
//  simple.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/shared.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"

group(BATCH_GROUP) push constant Simple
{
    mat4 ShapeModel;
    vec4 MatDiffuse;
};

render_state WireframeState
{
    CullMode = None;    
    BlendEnabled[0] = true; 
    SrcBlend[0] = SrcAlpha;
    DstBlend[0] = OneMinusSrcAlpha;
    FillMode = Line;
    //MultisampleEnabled = true;
};

render_state DepthEnabledState
{
    CullMode = None;    
    BlendEnabled[0] = true;
    SrcBlend[0] = SrcAlpha; 
    DstBlend[0] = OneMinusSrcAlpha;
    DepthEnabled = true;
    DepthWrite = false;
    //MultisampleEnabled = true;
};

render_state DepthDisabledState
{
    CullMode = None;    
    BlendEnabled[0] = true;
    SrcBlend[0] = SrcAlpha;
    DstBlend[0] = OneMinusSrcAlpha;
    DepthEnabled = false;
    DepthWrite = false;
    //MultisampleEnabled = true;
};

//------------------------------------------------------------------------------
/**
*/
shader
void
vsMainPrimitives(
    [slot=0] in vec3 position, 
    [slot=5] in vec4 color, 
    out vec4 Color)  
{
    gl_Position = ViewProjection * Simple.ShapeModel * vec4(position, 1);
    Color = color;
}
    
//------------------------------------------------------------------------------
/**
*/
shader
void
psMainPrimitives(in vec4 color, [color0] out vec4 Color) 
{
    Color = color * Simple.MatDiffuse;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsMainShape(
    [slot=0] in vec3 position)  
{
    gl_Position = ViewProjection * Simple.ShapeModel * vec4(position, 1);
}
    
//------------------------------------------------------------------------------
/**
*/
shader
void
psMainShape([color0] out vec4 Color) 
{
    Color = Simple.MatDiffuse;
}

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(DefaultPrim, "Colored", vsMainPrimitives(), psMainPrimitives(), DepthDisabledState);
SimpleTechnique(DepthPrim, "Colored|Alt0", vsMainPrimitives(), psMainPrimitives(), DepthEnabledState);
SimpleTechnique(WireframePrim, "Colored|Alt1", vsMainPrimitives(), psMainPrimitives(), WireframeState);

SimpleTechnique(DefaultShape, "Static", vsMainShape(), psMainShape(), DepthDisabledState);
SimpleTechnique(DepthShape, "Static|Alt0", vsMainShape(), psMainShape(), DepthEnabledState);
SimpleTechnique(WireframeShape, "Static|Alt1", vsMainShape(), psMainShape(), WireframeState);