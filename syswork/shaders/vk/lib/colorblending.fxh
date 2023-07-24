//------------------------------------------------------------------------------
//  colorblending.fxh
//
//	Contains the photoshop blending modes at: 
//	http://www.deepskycolors.com/archive/2010/04/21/formulas-for-Photoshop-blending-modes.html
//
//  (C) 2014 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#ifndef COLORBLENDING_FXH
#define COLORBLENDING_FXH


//------------------------------------------------------------------------------
/**
*/
vec3
Darken(vec3 target, vec3 blend)
{
	return min(target, blend);
}

//------------------------------------------------------------------------------
/**
*/
vec3
Multiply(vec3 target, vec3 blend)
{
	return target * blend;
}

//------------------------------------------------------------------------------
/**
*/
vec3
ColorBurn(vec3 target, vec3 blend)
{
	return 1 - (1 - target) / blend;
}

//------------------------------------------------------------------------------
/**
*/
vec3
LinearBurn(vec3 target, vec3 blend)
{
	return target + blend - 1;
}

//------------------------------------------------------------------------------
/**
*/
vec3
Lighten(vec3 target, vec3 blend)
{
	return max(target, blend);
}

//------------------------------------------------------------------------------
/**
*/
vec3
Screen(vec3 target, vec3 blend)
{
	return 1 - (1 - target) * (1 - blend);
}

//------------------------------------------------------------------------------
/**
*/
vec3
ColorDodge(vec3 target, vec3 blend)
{
	return target / (1 - blend);
}

//------------------------------------------------------------------------------
/**
*/
vec3
LinearDodge(vec3 target, vec3 blend)
{
	return target + blend;
}

//------------------------------------------------------------------------------
/**
*/
vec3
Overlay(vec3 target, vec3 blend)
{
	float red = float(target.r > 0.5f) * (1 - (1 - 2 * (target.r - 0.5f)) * (1 - blend.r)) + float(target.r <= 0.5f) * ((2 * target.r) * blend.r);
	float green = float(target.g > 0.5f) * (1 - (1 - 2 * (target.g - 0.5f)) * (1 - blend.g)) + float(target.g <= 0.5f) * ((2 * target.g) * blend.g);
	float blue = float(target.b > 0.5f) * (1 - (1 - 2 * (target.b - 0.5f)) * (1 - blend.b)) + float(target.b <= 0.5f) * ((2 * target.b) * blend.b);
	return vec3(red, green, blue);
}

//------------------------------------------------------------------------------
/**
*/
vec3
SoftLight(vec3 target, vec3 blend)
{
	float red = float(blend.r > 0.5f) * (1 - (1 - target.r)) * (1 - (blend.r - 0.5f)) + float(blend.r <= 0.5f) * (target.r * (blend.r + 0.5f));
	float green = float(blend.g > 0.5f) * (1 - (1 - target.g)) * (1 - (blend.g - 0.5f)) + float(blend.g <= 0.5f) * (target.g * (blend.g + 0.5f));
	float blue = float(blend.b > 0.5f) * (1 - (1 - target.b)) * (1 - (blend.b - 0.5f)) + float(blend.b <= 0.5f) * (target.b * (blend.b + 0.5f));
	return vec3(red, green, blue);
}

//------------------------------------------------------------------------------
/**
*/
vec3
HardLight(vec3 target, vec3 blend)
{
	float red = float(blend.r > 0.5f) * (1 - (1 - target.r)) * (1 - 2 * (blend.r - 0.5f)) + float(blend.r <= 0.5f) * (target.r * (2 * blend.r));
	float green = float(blend.g > 0.5f) * (1 - (1 - target.g)) * (1 - 2 * (blend.g - 0.5f)) + float(blend.g <= 0.5f) * (target.g * (2 * blend.g));
	float blue = float(blend.b > 0.5f) * (1 - (1 - target.b)) * (1 - 2 * (blend.b - 0.5f)) + float(blend.b <= 0.5f) * (target.b * (2 * blend.b));
	return vec3(red, green, blue);
}

//------------------------------------------------------------------------------
/**
*/
vec3
VividLight(vec3 target, vec3 blend)
{
	float red = float(blend.r > 0.5f) * (1 - (1 - target.r)) * (2 * (blend.r - 0.5f)) + float(blend.r <= 0.5f) * (target.r / (1 - 2 * blend.r));
	float green = float(blend.g > 0.5f) * (1 - (1 - target.g)) * (2 * (blend.g - 0.5f)) + float(blend.g <= 0.5f) * (target.g / (1 - 2 * blend.g));
	float blue = float(blend.b > 0.5f) * (1 - (1 - target.b)) * (2 * (blend.b - 0.5f)) + float(blend.b <= 0.5f) * (target.b / (1 - 2 * blend.b));
	return vec3(red, green, blue);
}

//------------------------------------------------------------------------------
/**
*/
vec3
LinearLight(vec3 target, vec3 blend)
{
	float red = float(blend.r > 0.5f) * (target.r + 2 * (blend.r - 0.5f)) + float(blend.r <= 0.5f) * (target.r + 2 * blend.r - 1);
	float green = float(blend.g > 0.5f) * (target.g + 2 * (blend.g - 0.5f)) + float(blend.g <= 0.5f) * (target.g + 2 * blend.g - 1);
	float blue = float(blend.b > 0.5f) * (target.b + 2 * (blend.b - 0.5f)) + float(blend.b <= 0.5f) * (target.b + 2 * blend.b - 1);
	return vec3(red, green, blue);
}

//------------------------------------------------------------------------------
/**
*/
vec3
PinLight(vec3 target, vec3 blend)
{
	float red = float(blend.r > 0.5f) * max(target.r, 2 * (blend.r - 0.5f)) + float(blend.r <= 0.5f) * (target.r + 2 * blend.r - 1);
	float green = float(blend.g > 0.5f) * max(target.g, 2 * (blend.g - 0.5f)) + float(blend.g <= 0.5f) * (target.g + 2 * blend.g - 1);
	float blue = float(blend.b > 0.5f) * max(target.b, 2 * (blend.b - 0.5f)) + float(blend.b <= 0.5f) * (target.b + 2 * blend.b - 1);
	return vec3(red, green, blue);
}

//------------------------------------------------------------------------------
/**
*/
vec3
Difference(vec3 target, vec3 blend)
{
	return vec3(length(target - blend));
}

//------------------------------------------------------------------------------
/**
*/
vec3
Exclusion(vec3 target, vec3 blend)
{
	return 0.5f - 2 * (target - 0.5f) * (blend - 0.5f);
}

#endif