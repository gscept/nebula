//------------------------------------------------------------------------------
//  shadowbase.fxh
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------


#ifndef SHADOWBASE_FXH
#define SHADOWBASE_FXH

#include "lib/defaultsamplers.fxh"

const float DepthScaling = 5.0f;
const float DarkeningFactor = 1.0f;
const float ShadowConstant = 100.0f;

samplerstate ShadowSampler
{
	//Samplers = { AlbedoMap, DisplacementMap };
};

state ShadowState
{
	CullMode = Back;
	DepthClamp = false;
	DepthEnabled = false;
	DepthWrite = false;
	BlendEnabled[0] = true;
	SrcBlend[0] = One;
	DstBlend[0] = One;
	BlendOp[0] = Min;
};

state ShadowStateCSM
{
	CullMode = Back;
	DepthClamp = false;
	DepthEnabled = false;
	DepthWrite = false;
	BlendEnabled[0] = true;
	SrcBlend[0] = One;
	DstBlend[0] = One;
	BlendOp[0] = Min;
};

//------------------------------------------------------------------------------
/**
*/
shader
void
vsStatic(
	[slot=0] in vec3 position,
	[slot=2] in vec2 uv,
	out vec2 UV,
	out vec4 ProjPos)
{
	gl_Position = ViewMatrixArray[0] * Model * vec4(position, 1);
	ProjPos = gl_Position;
	UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsSkinned(
	[slot=0] in vec3 position,
	[slot=2] in vec2 uv,
	[slot=7] in vec4 weights,
	[slot=8] in uvec4 indices,
	out vec2 UV,
	out vec4 ProjPos)
{
	vec4 skinnedPos = SkinnedPosition(position, weights, indices);
	gl_Position = ViewMatrixArray[0] * Model * skinnedPos;
	ProjPos = gl_Position;
	UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsStaticInst(
	[slot=0] in vec3 position,
	[slot=2] in vec2 uv,
	out vec2 UV,
	out vec4 ProjPos)
{
	gl_Position = ModelArray[gl_InstanceID] * vec4(position, 1);
	ProjPos = gl_Position;
	UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsStaticCSM(
	[slot=0] in vec3 position,
	[slot=2] in vec2 uv,
	out vec2 UV,
	out vec4 ProjPos,
	out int Instance)
{
	ProjPos = Model * vec4(position, 1);
	Instance = gl_InstanceID;
	UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsSkinnedCSM(
	[slot=0] in vec3 position,
	[slot=2] in vec2 uv,
	[slot=7] in vec4 weights,
	[slot=8] in uvec4 indices,
	out vec2 UV,
	out vec4 ProjPos,
	out int Instance)
{
	vec4 skinnedPos = SkinnedPosition(position, weights, indices);
	ProjPos = Model * skinnedPos;
	Instance = gl_InstanceID;
	UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsStaticInstCSM(
	[slot=0] in vec3 position,
	[slot=2] in vec2 uv,
	out vec2 UV,
	out vec4 ProjPos,
	out int Instance)
{
	int csmStride = gl_InstanceID % 4;
	ProjPos = ModelArray[gl_InstanceID] * vec4(position, 1);
	Instance = csmStride;
	UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsStaticPoint(
	[slot=0] in vec3 position,
	[slot=2] in vec2 uv,
	out vec2 UV,
	out vec4 ProjPos,
	out int Instance)
{
	ProjPos = Model * vec4(position, 1);
	Instance = gl_InstanceID;
	UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsSkinnedPoint(
	[slot=0] in vec3 position,
	[slot=2] in vec2 uv,
	[slot=7] in vec4 weights,
	[slot=8] in uvec4 indices,
	out vec2 UV,
	out vec4 ProjPos,
	out int Instance)
{
	vec4 skinnedPos = SkinnedPosition(position, weights, indices);
	ProjPos = Model * skinnedPos;
	Instance = gl_InstanceID;
	UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsStaticInstPoint(
	[slot=0] in vec3 position,
	[slot=2] in vec2 uv,
	out vec2 UV,
	out vec4 ProjPos,
	out int Instance)
{
	int csmStride = gl_InstanceID % 4;
	ProjPos = ModelArray[gl_InstanceID] * vec4(position, 1);
	Instance = csmStride;
	UV = uv;
}

//------------------------------------------------------------------------------
/**
	Geometry shader for point light shadow instancing.
	We copy the geometry and project into each direction frustum.
	We then point to which render target we wish to write to using gl_Layer.

	Doesn't work though...
*/
[inputprimitive] = triangles
[outputprimitive] = triangle_strip
[maxvertexcount] = 3
[instances] = 6
shader
void
gsPoint(in vec2 uv[], in vec4 pos[], flat in int instance[], out vec2 UV, out vec4 ProjPos)
{
	UV = uv[0];
	ProjPos = ViewMatrixArray[gl_InvocationID] * pos[0];
	gl_Position = ProjPos;
	gl_Layer = instance[0];
	EmitVertex();

	UV = uv[1];
	ProjPos = ViewMatrixArray[gl_InvocationID] * pos[1];
	gl_Position = ProjPos;
	gl_Layer = instance[0];
	EmitVertex();

	UV = uv[2];
	ProjPos = ViewMatrixArray[gl_InvocationID] * pos[2];
	gl_Position = ProjPos;
	gl_Layer = instance[0];
	EmitVertex();
	EndPrimitive();
}

//------------------------------------------------------------------------------
/**
	Geometry shader for CSM shadow instancing.
	We copy the geometry and project into each frustum.
	We then point to which viewport we wish to use using gl_ViewportIndex
*/
[inputprimitive] = triangles
[outputprimitive] = triangle_strip
[maxvertexcount] = 3
shader
void
gsCSM(in vec2 uv[], in vec4 pos[], flat in int instance[], out vec2 UV, out vec4 ProjPos)
{
	gl_Layer = instance[0];

	// simply pass geometry straight through and set viewport
	UV = uv[0];
	ProjPos = ViewMatrixArray[instance[0]] * pos[0];
	gl_Position = ProjPos;
	EmitVertex();

	UV = uv[1];
	ProjPos = ViewMatrixArray[instance[0]] * pos[1];
	gl_Position = ProjPos;
	EmitVertex();

	UV = uv[2];
	ProjPos = ViewMatrixArray[instance[0]] * pos[2];
	gl_Position = ProjPos;
	EmitVertex();
	EndPrimitive();
	/*
	for (int instance = 0; instance < 4; instance++)
	{
		mat4 splitMatrix = ViewMatrixArray[instance];

		for (int vertex = 0; vertex < 3; vertex++)
		{
			vec4 pos = splitMatrix * gl_in[vertex].gl_Position;
			UV = uv[vertex];
			ProjPos = pos;
			gl_Position = pos;
			EmitVertex();
		}
		EndPrimitive();
	}
	*/
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsTess(
	[slot=0] in vec3 position,
	[slot=1] in vec3 normal,
	[slot=2] in vec2 uv,
	[slot=3] in vec3 tangent,
	[slot=4] in vec3 binormal,
	out vec4 Position,
	out vec3 Normal,
	out vec2 UV,
	out int Instance,
	out float Distance)
{
	Position = Model * vec4(position, 1);
	Normal = (Model * vec4(normal, 0)).xyz;
	UV = uv;
	Instance = gl_InstanceID;

	float vertexDistance = distance( Position.xyz, EyePos.xyz );
	Distance = 1.0 - clamp( ( (vertexDistance - MinDistance) / (MaxDistance - MinDistance) ), 0.0, 1.0 - 1.0/TessellationFactor);
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsTessCSM(
	[slot=0] in vec3 position,
	[slot=1] in vec3 normal,
	[slot=2] in vec2 uv,
	[slot=3] in vec3 tangent,
	[slot=4] in vec3 binormal,
	out vec4 Position,
	out vec3 Normal,
	out vec2 UV,
	out int Instance,
	out float Distance)
{
	Position = Model * vec4(position, 1);
	Normal = (Model * vec4(normal, 0)).xyz;
	UV = uv;
	Instance = gl_InstanceID;

	float vertexDistance = distance( Position.xyz, EyePos.xyz );
	Distance = 1.0 - clamp( ( (vertexDistance - MinDistance) / (MaxDistance - MinDistance) ), 0.0, 1.0 - 1.0/TessellationFactor);
}

//------------------------------------------------------------------------------
/**
*/
[inputvertices] = 3
[outputvertices] = 6

shader
void
hsShadow(
	in vec4 position[],
	in vec3 normal[],
	in vec2 uv[],
	flat in int instance[],
	in float distance[],
	out vec4 Position[],
	out vec3 Normal[],
	out vec2 UV[],
	flat out int Instance[]
#ifdef PN_TRIANGLES
	,
	patch out vec3 f3B210,
    patch out vec3 f3B120,
    patch out vec3 f3B021,
    patch out vec3 f3B012,
    patch out vec3 f3B102,
    patch out vec3 f3B201,
    patch out vec3 f3B111
#endif
)
{
	Position[gl_InvocationID] = position[gl_InvocationID];
	UV[gl_InvocationID] = uv[gl_InvocationID];
	Instance[gl_InvocationID] = instance[gl_InvocationID];
	Normal[gl_InvocationID] = normal[gl_InvocationID];

	// perform per-patch operation
	if (gl_InvocationID == 0)
	{
		vec4 EdgeTessFactors;
		EdgeTessFactors.x = 0.5 * (distance[1] + distance[2]);
		EdgeTessFactors.y = 0.5 * (distance[2] + distance[0]);
		EdgeTessFactors.z = 0.5 * (distance[0] + distance[1]);
		EdgeTessFactors *= TessellationFactor;

#ifdef PN_TRIANGLES
		// compute the cubic geometry control points
		// edge control points
		f3B210 = ( ( 2.0f * position[0] ) + position[1] - ( dot( ( position[1] - position[0] ), normal[0] ) * normal[0] ) ) / 3.0f;
		f3B120 = ( ( 2.0f * position[1] ) + position[0] - ( dot( ( position[0] - position[1] ), normal[1] ) * normal[1] ) ) / 3.0f;
		f3B021 = ( ( 2.0f * position[1] ) + position[2] - ( dot( ( position[2] - position[1] ), normal[1] ) * normal[1] ) ) / 3.0f;
		f3B012 = ( ( 2.0f * position[2] ) + position[1] - ( dot( ( position[1] - position[2] ), normal[2] ) * normal[2] ) ) / 3.0f;
		f3B102 = ( ( 2.0f * position[2] ) + position[0] - ( dot( ( position[0] - position[2] ), normal[2] ) * normal[2] ) ) / 3.0f;
		f3B201 = ( ( 2.0f * position[0] ) + position[2] - ( dot( ( position[2] - position[0] ), normal[0] ) * normal[0] ) ) / 3.0f;
		// center control point
		vec3 f3E = ( f3B210 + f3B120 + f3B021 + f3B012 + f3B102 + f3B201 ) / 6.0f;
		vec3 f3V = ( input[0].Position + input[1].Position + input[2].Position ) / 3.0f;
		f3B111 = f3E + ( ( f3E - f3V ) / 2.0f );
#endif

		gl_TessLevelOuter[0] = EdgeTessFactors.x;
		gl_TessLevelOuter[1] = EdgeTessFactors.y;
		gl_TessLevelOuter[2] = EdgeTessFactors.z;
		gl_TessLevelInner[0] = (gl_TessLevelOuter[0] + gl_TessLevelOuter[1] + gl_TessLevelOuter[2]) / 3;
	}
}

//------------------------------------------------------------------------------
/**
*/
[inputvertices] = 6
[winding] = ccw
[topology] = triangle
[partition] = odd

shader
void
dsShadow(
	in vec4 position[],
	in vec3 normal[],
	in vec2 uv[],
	out vec2 UV,
	out vec4 ProjPos
#ifdef PN_TRIANGLES
	,
	in vec3 f3B210,
    in vec3 f3B120,
    in vec3 f3B021,
    in vec3 f3B012,
    in vec3 f3B102,
    in vec3 f3B201,
    in vec3 f3B111
#endif
)
{
	// The barycentric coordinates
	float fU = gl_TessCoord.z;
	float fV = gl_TessCoord.x;
	float fW = gl_TessCoord.y;

	// Precompute squares and squares * 3
	float fUU = fU * fU;
	float fVV = fV * fV;
	float fWW = fW * fW;
	float fUU3 = fUU * 3.0f;
	float fVV3 = fVV * 3.0f;
	float fWW3 = fWW * 3.0f;

#ifdef PN_TRIANGLES
	// Compute position from cubic control points and barycentric coords
	vec3 Position = position[0] * fWW * fW + position[1] * fUU * fU + position[2] * fVV * fV +
					  f3B210 * fWW3 * fU + f3B120 * fW * fUU3 + f3B201 * fWW3 * fV + f3B021 * fUU3 * fV +
					  f3B102 * fW * fVV3 + f3B012 * fU * fVV3 + f3B111 * 6.0f * fW * fU * fV;
#else
	vec3 Position = gl_TessCoord.x * position[0].xyz + gl_TessCoord.y * position[1].xyz + gl_TessCoord.z * position[2].xyz;
#endif
	UV = gl_TessCoord.x * uv[0] + gl_TessCoord.y * uv[1] + gl_TessCoord.z * uv[2];
	vec3 Norm = gl_TessCoord.x * normal[0] + gl_TessCoord.y * normal[1] + gl_TessCoord.z * normal[2];
	float Height = 2.0f * sample2DLod(DisplacementMap, ShadowSampler, UV, 0).x - 1.0f;
	vec3 VectorNormalized = normalize( Norm );
	Position.xyz += VectorNormalized.xyz * HeightScale * SceneScale * Height;

	gl_Position = ViewMatrixArray[0] * vec4(Position.xyz, 1);
	ProjPos = gl_Position;
}

//------------------------------------------------------------------------------
/**
*/
[inputvertices] = 6
[winding] = ccw
[topology] = triangle
[partition] = odd
shader
void
dsCSM(
	in vec4 position[],
	in vec3 normal[],
	in vec2 uv[],
	flat in int instance[],
	out vec2 UV,
	out vec4 ProjPos,
	flat out int Instance
#ifdef PN_TRIANGLES
	,
	in vec3 f3B210,
    in vec3 f3B120,
    in vec3 f3B021,
    in vec3 f3B012,
    in vec3 f3B102,
    in vec3 f3B201,
    in vec3 f3B111
#endif
)
{

	// The barycentric coordinates
	float fU = gl_TessCoord.z;
	float fV = gl_TessCoord.x;
	float fW = gl_TessCoord.y;

    // Precompute squares and squares * 3
    float fUU = fU * fU;
    float fVV = fV * fV;
    float fWW = fW * fW;
    float fUU3 = fUU * 3.0f;
    float fVV3 = fVV * 3.0f;
    float fWW3 = fWW * 3.0f;

#ifdef PN_TRIANGLES
	// Compute position from cubic control points and barycentric coords
	vec3 Position = position[0] * fWW * fW + position[1] * fUU * fU + position[2] * fVV * fV +
					  f3B210 * fWW3 * fU + f3B120 * fW * fUU3 + f3B201 * fWW3 * fV + f3B021 * fUU3 * fV +
					  f3B102 * fW * fVV3 + f3B012 * fU * fVV3 + f3B111 * 6.0f * fW * fU * fV;
#else
	vec3 Position = gl_TessCoord.x * position[0].xyz + gl_TessCoord.y * position[1].xyz + gl_TessCoord.z * position[2].xyz;
#endif

	UV = gl_TessCoord.x * uv[0] + gl_TessCoord.y * uv[1] + gl_TessCoord.z * uv[2];
	Instance = instance[0];
	vec3 Norm = gl_TessCoord.x * normal[0] + gl_TessCoord.y * normal[1] + gl_TessCoord.z * normal[2];
	float Height = 2.0f * sample2DLod(DisplacementMap, ShadowSampler, UV, 0).x - 1.0f;
	vec3 VectorNormalized = normalize( Norm );
	Position.xyz += VectorNormalized.xyz * HeightScale * SceneScale * Height;

	gl_Position = vec4(Position.xyz, 1);
	ProjPos = gl_Position;
}

//------------------------------------------------------------------------------
/**
*/
[earlydepth]
shader
void
psShadow(in vec2 UV,
	in vec4 ProjPos,
	[color0] out vec2 ShadowColor)
{
	float depth = ProjPos.z / ProjPos.w;
	float moment1 = depth;
	float moment2 = depth * depth;

	// Adjusting moments (this is sort of bias per pixel) using derivative
	float dx = dFdx(depth);
	float dy = dFdy(depth);
	moment2 += 0.25f*(dx*dx+dy*dy);

	ShadowColor = vec2(moment1, moment2);
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psShadowAlpha(in vec2 UV,
	in vec4 ProjPos,
	[color0] out vec2 ShadowColor)
{
	float alpha = sample2D(AlbedoMap, ShadowSampler, UV).a;
	if (alpha < AlphaSensitivity) discard;

	float depth = ProjPos.z / ProjPos.w;
	float moment1 = depth;
	float moment2 = depth * depth;

	// Adjusting moments (this is sort of bias per pixel) using derivative
	float dx = dFdx(depth);
	float dy = dFdy(depth);
	moment2 += 0.25f*(dx*dx+dy*dy);

	ShadowColor = vec2(moment1, moment2);
}

//------------------------------------------------------------------------------
/**
*/
[earlydepth]
shader
void
psESM(in vec2 UV,
	  in vec4 ProjPos,
	  [color0] out float ShadowColor)
{
	ShadowColor = (ProjPos.z/ProjPos.w) * DepthScaling;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psESMAlpha(in vec2 UV,
	  in vec4 ProjPos,
	  [color0] out float ShadowColor)
{
	float alpha = sample2D(AlbedoMap, ShadowSampler, UV).a;
	if (alpha < AlphaSensitivity) discard;
	ShadowColor = (ProjPos.z/ProjPos.w) * DepthScaling;
}

//------------------------------------------------------------------------------
/**
*/
[earlydepth]
shader
void
psVSM(in vec2 UV,
	in vec4 ProjPos,
	[color0] out vec2 ShadowColor)
{
	float depth = ProjPos.z / ProjPos.w;
	float moment1 = depth;
	float moment2 = depth * depth;

	// Adjusting moments (this is sort of bias per pixel) using derivative
	//float dx = dFdx(depth);
	//float dy = dFdy(depth);
	//moment2 += 0.25f*(dx*dx+dy*dy);

	ShadowColor = vec2(moment1, moment2);
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psVSMAlpha(in vec2 UV,
	in vec4 ProjPos,
	[color0] out vec2 ShadowColor)
{
	float alpha = sample2D(AlbedoMap, ShadowSampler, UV).a;
	if (alpha < AlphaSensitivity) discard;

	float depth = ProjPos.z / ProjPos.w;
	float moment1 = depth;
	float moment2 = depth * depth;

	// Adjusting moments (this is sort of bias per pixel) using derivative
	//float dx = dFdx(depth);
	//float dy = dFdy(depth);
	//moment2 += 0.25f*(dx*dx+dy*dy);

	ShadowColor = vec2(moment1, moment2);
}

//------------------------------------------------------------------------------
/**
*/
[earlydepth]
shader
void
psVSMPoint(in vec2 UV,
	in vec4 ProjPos,
	[color0] out vec2 ShadowColor)
{
	float depth = ProjPos.z / ProjPos.w;
	float moment1 = depth;
	float moment2 = depth * depth;

	// Adjusting moments (this is sort of bias per pixel) using derivative
	//float dx = dFdx(depth);
	//float dy = dFdy(depth);
	//moment2 += 0.25f*(dx*dx+dy*dy);

	ShadowColor = vec2(moment1, moment2);
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psVSMAlphaPoint(in vec2 UV,
	in vec4 ProjPos,
	[color0] out vec2 ShadowColor)
{
	float alpha = sample2D(AlbedoMap, ShadowSampler, UV).a;
	if (alpha < AlphaSensitivity) discard;

	float depth = ProjPos.z / ProjPos.w;
	float moment1 = depth;
	float moment2 = depth * depth;

	// Adjusting moments (this is sort of bias per pixel) using derivative
	//float dx = dFdx(depth);
	//float dy = dFdy(depth);
	//moment2 += 0.25f*(dx*dx+dy*dy);

	ShadowColor = vec2(moment1, moment2);
}

//---------------------------------------------------------------------------------------------------------------------------
/**
*/
float
Variance(vec2 shadowSample,
		 float lightSpaceDepth,
		 float tolerance)
{
	// get average and average squared
	float avgZ = shadowSample.x;
	float avgZ2 = shadowSample.y;

	// assume that if the projected depth is less than the average in the pixel, the pixel must be lit
	if (lightSpaceDepth <= avgZ)
	{
		return 1.0f;
	}
	else
	{
		float variance 	= (avgZ2) - (avgZ * avgZ);
		variance 		= min(1.0f, max(0.0f, variance + tolerance));

		float mean 		= avgZ;
		float d			= lightSpaceDepth - mean;
		float p_max		= variance / (variance + d*d);

		// to avoid light bleeding, change this constant
		return max(p_max, float(lightSpaceDepth <= avgZ));
	}
}

//------------------------------------------------------------------------------
/**
	Calculates Chebyshevs upper bound for use with VSM shadow mapping with local lights
*/
float
ChebyshevUpperBound(vec2 Moments, float t, float tolerance)
{
	// One-tailed inequality valid if t > Moments.x
	if (t <= Moments.x) return 1.0f;

	// Compute variance.
	float Variance = Moments.y - (Moments.x*Moments.x);
	Variance = max(Variance, tolerance);

	// Compute probabilistic upper bound.
	float d = t - Moments.x;
	float p_max = Variance / (Variance + d*d);

	return p_max;
}

//------------------------------------------------------------------------------
/**
*/
float
ExponentialShadowSample(float mapDepth, float depth, float bias)
{
	float receiverDepth = DepthScaling * depth - bias;
    float occluderReceiverDistance = mapDepth - receiverDepth;
	float occlusion = saturate(exp(DarkeningFactor * occluderReceiverDistance));
    //float occlusion = saturate(exp(DarkeningFactor * occluderReceiverDistance));
    return occlusion;
}
#endif // SHADOWBASE_FXH
