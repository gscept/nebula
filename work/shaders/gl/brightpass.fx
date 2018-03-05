//------------------------------------------------------------------------------
//  brightpass.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"
#include "lib/techniques.fxh"


float HDRBrightPassThreshold = float(1.0f);
vec4 HDRBloomColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);

/// Declaring used textures
sampler2D ColorSource;
sampler2D LuminanceTexture;

samplerstate BrightPassSampler
{
	Samplers = { ColorSource, LuminanceTexture };
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
vsMain(in vec3 position,
	[slot=2] in vec2 uv,
	out vec2 UV) 
{
	gl_Position = vec4(position, 1);
	UV = FlipY(uv);
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psMain(in vec2 uv,
	[color0] out vec4 Color) 
{	
	vec4 sampleColor = DecodeHDR(textureLod(ColorSource, uv, 0));
	
	// Get the calculated average luminance 
	vec4 fLumAvg = textureLod(LuminanceTexture, vec2(0.5f, 0.5f), 0);
	
	vec4 tonedColor = ToneMap(sampleColor, fLumAvg);
	vec3 brightColor = max(tonedColor.rgb - HDRBrightPassThreshold, vec3(0.0f));
	Color = HDRBloomColor * vec4(brightColor, sampleColor.a);
}

//------------------------------------------------------------------------------
/**
*/
PostEffect(vsMain(), psMain(), BrightPassState);
