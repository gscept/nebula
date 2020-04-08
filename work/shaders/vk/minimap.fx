//------------------------------------------------------------------------------
//  minimap.fx
//  (C) 2016 Johannes Hirche
//------------------------------------------------------------------------------
#include "lib/std.fxh"
#include "lib/shared.fxh"
#include "lib/geometrybase.fxh"
#include "lib/techniques.fxh"

render_state MinimapState
{
	BlendEnabled[0] = true;
	SrcBlend[0] = SrcAlpha;
	DstBlend[0] = OneMinusSrcAlpha;
	CullMode = None;
	DepthEnabled = false;
	DepthWrite = false;
};

// samplers
sampler_state PortraitSampler
{
	//Samplers = { Portrait };
};

group(BATCH_GROUP) push constant MinimapBlock 
{	
	mat4 TransArray;
	vec4 ColorArray;	
	textureHandle Portrait;
};

//------------------------------------------------------------------------------
/**
*/
shader
void
vsMinimap(
	[slot=0] in vec2 uv,
	out vec2 UV) 
{	
    gl_Position = MinimapBlock.TransArray * vec4(uv,0,1);		
    UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psMinimap(in vec2 UV,	
	[color0] out vec4 FinalColor) 
{
	vec4 portraitColor = sample2D(MinimapBlock.Portrait, PortraitSampler,UV);
	FinalColor = portraitColor;//vec4(0.5,0.5, portraitColor.r, 1);
}

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(minimap, "Static", vsMinimap(), psMinimap(), MinimapState);