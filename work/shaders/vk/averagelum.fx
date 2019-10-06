//------------------------------------------------------------------------------
//  averagelum.fx
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/techniques.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"

texture2D ColorSource;
texture2D PreviousLum;

varblock AverageLumBlock
{
	float TimeDiff;
};


samplerstate LuminanceSampler
{
	//Samplers = { ColorSource, PreviousLum };
	Filter = Point;
};



state AverageLumState
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
	Performs a 2x2 kernel downscale, will only render 1 pixel
*/
shader
void
psMain(in vec2 UV,
	[color0] out float result)
{
	//vec2 pixelSize = GetPixelSize(ColorSource);
	
	// source should be a 512x512 texture, so we sample the 8'th mip of the texture
	vec4 sample1 = texelFetch(sampler2D(ColorSource, LuminanceSampler), ivec2(1, 0), 0);
	vec4 sample2 = texelFetch(sampler2D(ColorSource, LuminanceSampler), ivec2(0, 1), 0);
	vec4 sample3 = texelFetch(sampler2D(ColorSource, LuminanceSampler), ivec2(1, 1), 0);
	vec4 sample4 = texelFetch(sampler2D(ColorSource, LuminanceSampler), ivec2(0, 0), 0);
	float currentLum = dot((sample1 + sample2 + sample3 + sample4) * 0.25f, vec4(0.2126f, 0.7152f, 0.0722f, 0));
	float lastLum = texelFetch(sampler2D(PreviousLum, LuminanceSampler), ivec2(0, 0), 0).r;
	
/*	float	sigma = 0.04/(0.04 + Clum.x);
	float	tau = sigma*0.4 + (1.0 - sigma)*0.1;
	vec3	col = Alum + (Clum - Alum) * (1.0 - exp(-dtime/tau));
*/

	//vec3	col = pow( pow( Alum, 0.25 ) + ( pow( Clum, 0.25 ) - pow( Alum, 0.25 ) ) * ( 1.0 - pow( 0.98, 30 * dtime ) ), 4.0);
	float lum = lastLum + (currentLum - lastLum) * (1.0 - pow(0.98, 30.0 * TimeDiff));
	lum = clamp(lum, 0.25f, 5.0f);
	//Color.y = clamp(Color.y, 0.5f, 2);
	//gl_FragColor = vec4(col.x,col.y, 0.0, 1.0);
	result = lum;
}

//------------------------------------------------------------------------------
/**
*/
PostEffect(vsMain(), psMain(), AverageLumState);
