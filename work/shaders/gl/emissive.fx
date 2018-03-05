//------------------------------------------------------------------------------
//  emissive.fx
//  (C) 2015 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/shared.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"

float EmissiveIntensity = 0.0f;

/// Declaring used textures
sampler2D EmissiveMap;

/// Declaring used samplers
samplerstate DefaultSampler
{
	Samplers = { EmissiveMap };
};

state EmissiveState
{
	CullMode = Back;
	DepthEnabled = true;
	DepthFunc = Equal;
	BlendEnabled[0] = true;
	SrcBlend[0] = SrcAlpha;
	DstBlend[0] = OneMinusSrcAlpha;
};


//------------------------------------------------------------------------------
/**
*/
shader
void
vsMain(in vec3 position,
	in vec3 normal,
	in vec2 uv,
	in vec3 tangent,
	in vec3 binormal,
	out vec2 UV) 
{
    gl_Position = ViewProjection * Model * vec4(position, 1);
    UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psMain(in vec2 UV,
	[color0] out vec4 Albedo) 
{
	vec4 diffColor = texture(EmissiveMap, UV.xy) * EmissiveIntensity;	
	Albedo = EncodeHDR(diffColor);
}


//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(Static, "Static", vsMain(), psMain(), EmissiveState);