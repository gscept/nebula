//------------------------------------------------------------------------------
//  brightpass_cs.fx
//  (C) 2016 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"
#include "lib/techniques.fxh"

float HDRBrightPassThreshold = float(1.0f);
vec4 HDRBloomColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);

/// Declaring used textures
read rgba16f image2D ColorSource;
read rg16f image2D LuminanceTexture;
write rgba8 image2D BrightBuffer;


//------------------------------------------------------------------------------
/**
*/
[localsizex] = 320
//[localsizey] = 16
shader
void
csMain() 
{	
	ivec2 uv = ivec2(gl_GlobalInvocationID.xy);
	vec4 sampleColor = DecodeHDR(imageLoad(ColorSource, uv * 2));
	
	// Get the calculated average luminance 
	vec4 fLumAvg = imageLoad(LuminanceTexture, ivec2(0, 0));
	
	vec4 tonedColor = ToneMap(sampleColor, fLumAvg);
	vec3 brightColor = max(tonedColor.rgb - HDRBrightPassThreshold, vec3(0.0f));
	imageStore(BrightBuffer, uv, HDRBloomColor * vec4(brightColor, sampleColor.a));
	memoryBarrierImage();
}

//------------------------------------------------------------------------------
/**
*/
program PostEffect [ string Mask = "Alt0"; ]
{
	ComputeShader = csMain();
};
