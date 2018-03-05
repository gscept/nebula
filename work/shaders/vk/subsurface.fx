//------------------------------------------------------------------------------
//  subsurface.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/geometrybase.fxh"
#include "lib/techniques.fxh"
#include "lib/shared.fxh"

textureHandle AbsorptionMap;
textureHandle ScatterMap;

samplerstate SubsurfaceSampler
{
	//Samplers = { AbsorptionMap, ScatterMap };
	Filter = MinMagMipLinear;
};

float SubsurfaceStrength = 0.0f;
float SubsurfaceWidth = 0.0f;
float SubsurfaceCorrection = 0.0f;

state SubsurfaceState
{
//	DepthEnabled = true;
//	DepthWrite = true;
	DepthFunc = Lequal;
};

//------------------------------------------------------------------------------
/**
	Renders Subsurface geometry to buffers.
	Writes absorption, scattering and mask
*/
shader
void
psSkin(
	in vec3 ViewSpacePos,
	in vec3 Tangent,
	in vec3 Normal,
	in vec3 Binormal,
	in vec2 UV,
	[color0] out vec4 absorption,
	[color1] out vec4 scatter,
	[color2] out vec4 mask)
{
	// sample textures
	absorption = sample2D(AbsorptionMap, SubsurfaceSampler, UV);
	scatter = sample2D(ScatterMap, SubsurfaceSampler, UV);
	mask.a = 255;
	mask.r = SubsurfaceStrength;
	mask.g = SubsurfaceWidth;
	mask.b = SubsurfaceCorrection;
}

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(Skinned, "Skinned", vsSkinned(), psSkin(), SubsurfaceState);
SimpleTechnique(Static, "Static", vsStatic(), psSkin(), SubsurfaceState);