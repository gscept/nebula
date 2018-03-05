#ifndef PARTICLES_H
#define PARTICLES_H

#include "lib/shared.fxh"

cbuffer ParticleVars
{
	float4   BBoxCenter;
	float4   BBoxSize;
	int 	 NumAnimPhases;
	float	 AnimFramesPerSecond;
	bool 	 IsBillBoard;
}

struct CornerVertex
{
	float4 worldPos		: TEXCOORD0;
	float3 worldNormal	: TEXCOORD1;
	float3 worldTangent	: TEXCOORD2;
	float3 worldBinormal: TEXCOORD3;
	float2 UV		: TEXCOORD4;
};

/**
    ComputeCornerVertex
    Computes the expanded vertex position for the current corner. Takes
    rotation into account.
*/   
CornerVertex
ComputeCornerVertex(uniform bool hemisphereNormal,
                    in float2 corner,
                    in float4 position,
                    in float4 stretchPos,
                    in float4 uvMinMax,
                    in float rotate,
                    in float size)
{

	CornerVertex output;
	
    // build a 3x3 rotation matrix from the rotation
    float rotSin, rotCos;
    sincos(rotate, rotSin, rotCos);
	
    
    // compute 2d extruded corner position
    float2 extrude = ((corner * 2.0) - 1.0) * size;

    // rotate particle sprite
    float3x3 rotMatrix = {
        rotCos, -rotSin, 0.0,
        rotSin, rotCos,  0.0,
        0.0,    0.0,     1.0
    };
    extrude = mul(extrude, rotMatrix);
    
    // check if billboard 
    float4x4 transform;
    if (IsBillBoard)
    {
        transform = InvView;
    }
    else
    {
        transform = EmitterTransform;
    }
    
    // transform to world space
    float4 worldExtrude = mul(float4(extrude, 0.0, 0.0), transform);
    
    // depending on corner, use position or stretchPos as center point   
    // also compute uv coordinates (v: texture tiling, x: anim phases)
    float du = frac(floor(Time * AnimFramesPerSecond) / NumAnimPhases);
    if (corner.x != 0)
    {
		output.UV.x = (uvMinMax.z / NumAnimPhases) + du;
    }
    else
    {
		output.UV.x = (uvMinMax.x / NumAnimPhases) + du;
    }
    if (corner.y != 0)
    {    
		output.worldPos = position + worldExtrude;
		output.UV.y = uvMinMax.w;
    }
    else
    {
		output.worldPos = stretchPos + worldExtrude;
		output.UV.y = uvMinMax.y;
    }
    
    // setup normal, tangent, binormal in world space
    if (hemisphereNormal)
    { 
	output.worldNormal = normalize(output.worldPos - BBoxCenter.xyz);
	output.worldTangent = cross(output.worldNormal, float3(0,1,0));
	output.worldBinormal = cross(output.worldNormal, output.worldTangent);
	output.worldTangent = cross(output.worldNormal, output.worldBinormal);
	
	output.worldNormal = output.worldNormal.xyz;
	output.worldTangent = output.worldTangent.xyz;
	output.worldBinormal = output.worldBinormal.xyz;
    }
    else
    {
	float2 viewTangent  = mul(rotMatrix, float2(-1.0, 0.0));
	float2 viewBinormal = mul(rotMatrix, float2(0.0, 1.0));
		
	output.worldNormal   = transform[2].xyz;
	output.worldTangent  = mul(viewTangent, transform).xyz;
	output.worldBinormal = mul(viewBinormal, transform).xyz;
    }
	return output;
}

#endif