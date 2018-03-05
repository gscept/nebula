//------------------------------------------------------------------------------
//  brightpass.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"
#include "lib/techniques.fxh"


/// Declaring used textures
textureHandle ColorSource;
textureHandle LuminanceTexture;

samplerstate BrightPassSampler
{
	//Samplers = { ColorSource, LuminanceTexture };
	Filter = Point;
};

state BrightPassState
{
	CullMode = Back;
	DepthEnabled = false;
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
psMain(in vec2 uv,
	[color0] out vec4 Color) 
{	
	vec4 sampleColor = sample2DLod(ColorSource, BrightPassSampler, uv, 0);
	
	// Get the calculated average luminance 
	float lumavg = fetch2D(LuminanceTexture, BrightPassSampler, ivec2(0, 0), 0).r;
	//float lumavg = 1.0f;
	
	vec4 tonedColor = ToneMap(sampleColor, vec4(lumavg), MaxLuminance);
	vec3 brightColor = max(tonedColor.rgb - HDRBrightPassThreshold, vec3(0.0f));
	Color = HDRBloomColor * vec4(brightColor, sampleColor.a);
}

//------------------------------------------------------------------------------
/**
*/
PostEffect(vsMain(), psMain(), BrightPassState);
