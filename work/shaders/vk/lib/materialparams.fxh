//------------------------------------------------------------------------------
//  materialparams.fxh
//  (C) 2016 Gustav Sterbrant
//------------------------------------------------------------------------------

#ifndef MATERIALPARAMS
#define MATERIALPARAMS
#include "std.fxh"

// material properties
group(BATCH_GROUP) shared varblock MaterialParams [ string Visibility = "VS|PS"; ]
{
	vec4 MatAlbedoIntensity = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	vec4 MatSpecularIntensity = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	vec4 MatEmissiveIntensity = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	vec4 MatFresnelIntensity = vec4(1.0f, 1.0f, 1.0f, 1.0f);

	float AlphaSensitivity = 1.0f;
	float AlphaBlendFactor = 0.0f;
	float MatRoughnessIntensity = 0.0f;
	float MatMetallicIntensity = 0.0f;
};

#endif // MATERIALPARAMS
