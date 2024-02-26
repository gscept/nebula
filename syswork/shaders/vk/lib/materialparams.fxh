//------------------------------------------------------------------------------
//  materialparams.fxh
//  (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#ifndef MATERIALPARAMS
#define MATERIALPARAMS
#include "std.fxh"

// material properties
group(BATCH_GROUP) shared constant MaterialParams [ string Visibility = "VS|PS"; ]
{
	vec4 MatAlbedoIntensity;
	vec4 MatSpecularIntensity;

	float AlphaSensitivity;
	float AlphaBlendFactor;
	float MatRoughnessIntensity;
	float MatMetallicIntensity;

    textureHandle AlbedoMap;
    textureHandle ParameterMap;
    textureHandle NormalMap;
};

#endif // MATERIALPARAMS
