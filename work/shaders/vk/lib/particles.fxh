//------------------------------------------------------------------------------
//  particles.fxh
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#ifndef PARTICLES_FXH
#define PARTICLES_FXH

#include "lib/std.fxh"
#include "lib/shared.fxh"

struct CornerVertex
{
	vec4 worldPos;
	vec3 worldNormal;
	vec3 worldTangent;
	vec3 worldBinormal;
	vec2 UV;
};

group(DYNAMIC_OFFSET_GROUP) shared varblock ParticleObjectBlock [ string Visibility = "VS"; ]
{
	mat4	EmitterTransform;
	vec4	BBoxCenter;
	vec4	BBoxSize;

	int		NumAnimPhases;
	float	AnimFramesPerSecond;
	bool 	Billboard;
};

/**
    ComputeCornerVertex
    Computes the expanded vertex position for the current corner. Takes
    rotation into account.
*/
CornerVertex
ComputeCornerVertex(in bool hemisphereNormal,
                    in vec2 corner,
                    in vec4 position,
                    in vec4 stretchPos,
                    in vec4 uvMinMax,
                    in float rotate,
                    in float size)
{

	CornerVertex outputVert;

    // build a 3x3 rotation matrix from the rotation
    float rotSin, rotCos;
	rotSin = sin(rotate);
	rotCos = cos(rotate);

    // compute 2d extruded corner position
    vec2 extrude = ((corner * 2.0) - 1.0) * size;

    // rotate particle sprite
    mat2 rotMatrix = mat2(
        rotCos, -rotSin,
        rotSin, rotCos
    );
    extrude = mul(extrude, rotMatrix);

    // check if billboard
    const mat4 transform = EmitterTransform;

    // transform to world space
    vec4 worldExtrude = EmitterTransform * vec4(extrude, 0.0, 0.0);

    // depending on corner, use position or stretchPos as center point
    // also compute uv coordinates (v: texture tiling, x: anim phases)
    float du = frac(floor(TimeAndRandom.x * AnimFramesPerSecond) / NumAnimPhases);
    if (corner.x != 0)
    {
		outputVert.UV.x = (uvMinMax.z / NumAnimPhases) + du;
    }
    else
    {
		outputVert.UV.x = (uvMinMax.x / NumAnimPhases) + du;
    }

    if (corner.y != 0)
    {
		outputVert.worldPos = position + worldExtrude;
		outputVert.UV.y = uvMinMax.w;
    }
    else
    {
		outputVert.worldPos = stretchPos + worldExtrude;
		outputVert.UV.y = uvMinMax.y;
    }

    // setup normal, tangent, binormal in world space
    if (hemisphereNormal)
    {
		outputVert.worldNormal = normalize(outputVert.worldPos - BBoxCenter).xyz;
		outputVert.worldTangent = cross(outputVert.worldNormal, vec3(0,1,0));
		outputVert.worldBinormal = cross(outputVert.worldNormal, outputVert.worldTangent);
		outputVert.worldTangent = cross(outputVert.worldNormal, outputVert.worldBinormal);

		outputVert.worldNormal = outputVert.worldNormal.xyz;
		outputVert.worldTangent = outputVert.worldTangent.xyz;
		outputVert.worldBinormal = outputVert.worldBinormal.xyz;
    }
    else
    {
		vec2 viewTangent  = mul(rotMatrix, vec2(-1.0, 0.0));
		vec2 viewBinormal = mul(rotMatrix, vec2(0.0, 1.0));

		outputVert.worldNormal   = EmitterTransform[2].xyz;
		outputVert.worldTangent  = mat3(EmitterTransform) * vec3(viewTangent, 0);
		outputVert.worldBinormal = mat3(EmitterTransform) * vec3(viewBinormal, 0);
    }
	return outputVert;
}

#endif
