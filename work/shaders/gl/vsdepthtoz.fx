//------------------------------------------------------------------------------
//  vsdepthtoz.fx
//  (C) 2016 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"
#include "lib/techniques.fxh"

/// Declaring used textures
sampler2D DepthBuffer;


samplerstate VsDepthToZSampler
{
	Samplers = { DepthBuffer };
	Filter = Point;
};

state VsDepthToZState
{
	CullMode = Back;
	DepthEnabled = false;
	DepthWrite = false;
};
#define FLT_MAX (1E+37 / 2)

//------------------------------------------------------------------------------
/**
*/
shader
void
vsMain(in vec3 position,
	[slot=2] in vec2 uv,
	out vec2 UV,
	out vec3 ViewSpacePosition) 
{
	gl_Position = vec4(position, 1);
	ViewSpacePosition = vec3(position.xy * FocalLength.xy, -1);
	UV = FlipY(uv);
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psMain(in vec2 uv,
	in vec3 ViewSpacePosition,
	[color0] out float Color) 
{	
	vec2 pixelSize = GetPixelSize(DepthBuffer);
	float Depth = textureLod(DepthBuffer, uv, 0).r;
	if (Depth < 0) { Color = 1000; return; }
	
	vec3 viewVec = normalize(ViewSpacePosition);
	vec3 surfacePos = viewVec * Depth;
	vec4 worldPos = (InvView * vec4(surfacePos, 1));
	vec4 projPos = (Projection * vec4(surfacePos, 1));
	//Color = distance(EyePos.xyz, worldPos.xyz);
	Color = length(surfacePos);
	//Color = (projPos.z / projPos.w);
	//Color = projPos.z;
}

//------------------------------------------------------------------------------
/**
*/
PostEffect(vsMain(), psMain(), VsDepthToZState);
