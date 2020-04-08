//------------------------------------------------------------------------------
//  CSM.fxh
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#ifndef CSM_FXH
#define CSM_FXH

#define PCF 0
#define BLURSAMPLES 1
#define NO_COMPARISON 1

#include "shadowbase.fxh"
#include "std.fxh"


const int SplitsPerRow = 1;
const int SplitsPerColumn = 1;

const vec4 DebugColors[8] =
{
    float4 ( 1.5f, 0.0f, 0.0f, 1.0f ),
    float4 ( 0.0f, 1.5f, 0.0f, 1.0f ),
    float4 ( 0.0f, 0.0f, 5.5f, 1.0f ),
    float4 ( 1.5f, 0.0f, 5.5f, 1.0f ),
    float4 ( 1.5f, 1.5f, 0.0f, 1.0f ),
    float4 ( 1.0f, 1.0f, 1.0f, 1.0f ),
    float4 ( 0.0f, 1.0f, 5.5f, 1.0f ),
    float4 ( 0.5f, 3.5f, 0.75f, 1.0f )
};

sampler_state CSMTextureSampler
{
	//Samplers = { ShadowProjMap };
	//Filter = MinMagLinearMipPoint;
	AddressU = Border;
	AddressV = Border;
	//MaxAnisotropic = 16;
	//BorderColor = White;
};

const float CascadeBlendArea = 0.2f;
//------------------------------------------------------------------------------
/**
*/
vec4 ConvertViewRayToWorldPos(vec3 viewRay, float length, vec3 cameraPosition)
{
	return vec4(cameraPosition + viewRay * length, 1);
}

//------------------------------------------------------------------------------
/**
	Converts World Position into shadow texture lookup vector, modelviewprojection position and interpolated position, as well as depth
*/
void CSMConvert(in vec4 worldPosition,
				out vec4 texShadow)
{
    // Transform the shadow texture coordinates for all the cascades.
    texShadow = CSMShadowMatrix * worldPosition;
}

const vec2 sampleOffsets[] = {
	vec2(-.326,-.406),
	vec2(-.840,-.074),
	vec2(-.696, .457),
	vec2(-.203, .621),
	vec2( .962,-.195),
	vec2( .473,-.480),
	vec2( .519, .767),
	vec2( .185,-.893),
	vec2( .507, .064),
	vec2( .896, .412),
	vec2(-.322,-.933),
	vec2(-.792,-.598)
};

//------------------------------------------------------------------------------
/**
*/
void
CalculateBlendAmountForMap(in vec4 texCoord,
                           out float blendBandLocation,
                           out float blendAmount)
{
    // calculate the blend band for the map based selection.
    vec2 distanceToOne = vec2(1.0f - texCoord.x, 1.0f - texCoord.y);
    blendBandLocation = min(texCoord.x, texCoord.y);
    float blendBandLocation2 = min(distanceToOne.x, distanceToOne.y);
    blendBandLocation = min(blendBandLocation, blendBandLocation2);
    blendAmount = blendBandLocation / CascadeBlendArea;
}

//------------------------------------------------------------------------------
/**
	CSM shadow sampling entry point
*/
float
CSMPS(in vec4 TexShadow,
	  in uint Texture,
	  out vec4 Debug)
{
	vec4 texCoordShadow = vec4(0.0f);
	bool cascadeFound = false;
	float bias = GlobalLightShadowBias;

    TexShadow = TexShadow / TexShadow.wwww;

	vec3 shadowPosDDX = dFdx(TexShadow.xyz);
	vec3 shadowPosDDY = dFdy(TexShadow.xyz);

	int cascadeIndex;
	for( cascadeIndex = 0; cascadeIndex < CASCADE_COUNT_FLAG; ++cascadeIndex)
	{
		texCoordShadow = TexShadow * CascadeScale[cascadeIndex];
		texCoordShadow += CascadeOffset[cascadeIndex];

		if ( min( texCoordShadow.x, texCoordShadow.y ) > MinBorderPadding
		  && max( texCoordShadow.x, texCoordShadow.y ) < MaxBorderPadding )
		{
			cascadeFound = true;
			break;
		}
	}

	shadowPosDDX *= CascadeScale[cascadeIndex].xyz;
	shadowPosDDX += CascadeOffset[cascadeIndex].xyz;
	shadowPosDDY *= CascadeScale[cascadeIndex].xyz;
	shadowPosDDY += CascadeOffset[cascadeIndex].xyz;

	float blendAmount = 0;
	float blendBandLocation = 0;
	CalculateBlendAmountForMap ( texCoordShadow, blendBandLocation, blendAmount );

	Debug = DebugColors[cascadeIndex];

	// if we have no matching cascade, return with a fully lit pixel
	if (!cascadeFound)
	{
		return 1.0f;
	}

	// calculate texture coordinate in shadow space
	vec2 texCoord = texCoordShadow.xy;
	float depth = texCoordShadow.z;

	vec2 mapDepth = sample2DArrayGrad(Texture, CSMTextureSampler, vec3(texCoord, cascadeIndex), shadowPosDDX.xy, shadowPosDDY.xy).rg;
	float occlusion = ChebyshevUpperBound(mapDepth, depth, 0.0000001f);
	//float occlusion = ExponentialShadowSample(mapDepth, depth, 0.0f);

	int nextCascade = cascadeIndex + 1;
	float occlusionBlend = 1.0f;
	if (blendBandLocation < CascadeBlendArea)
	{
		if (nextCascade < CASCADE_COUNT_FLAG)
		{
			texCoordShadow = TexShadow * CascadeScale[nextCascade];
			texCoordShadow += CascadeOffset[nextCascade];

			texCoord = texCoordShadow.xy;
			depth = texCoordShadow.z;

			mapDepth = sample2DArrayGrad(Texture, CSMTextureSampler, vec3(texCoord, nextCascade), shadowPosDDX.xy, shadowPosDDY.xy).rg;
			occlusionBlend = ChebyshevUpperBound(mapDepth, depth, 0.0000001f);
		}

		// blend next cascade onto previous
		occlusion = lerp(occlusionBlend, occlusion, blendAmount);
	}
	//occlusion += smoothstep(0.98f, 1.0f, increment);
	//occlusion += increment;


	// finally clamp all shadow values 0.5, this avoids any weird color differences when blending between cascades

	//return smoothstep(0.5f, 1.0f, occlusion);
	return occlusion;
}

//------------------------------------------------------------------------------
/**
*/
float
SampleShadowCascade(
	vec4 position, vec3 ddx, vec3 ddy,
	uint cascadeIndex)
{
	vec3 shadowPosition = position.xyz;
	shadowPosition += CascadeOffset[cascadeIndex].xyz;
	shadowPosition *= CascadeScale[cascadeIndex].xyz;
	vec3 shadowPosDX = ddx * CascadeScale[cascadeIndex].xyz;
	vec3 shadowPosDY = ddy * CascadeScale[cascadeIndex].xyz;

	float lightDepth = shadowPosition.z;
	const float bias = GlobalLightShadowBias;
	lightDepth -= bias;
	vec2 samp = sample2DArrayLod(GlobalLightShadowBuffer, CSMTextureSampler, vec3(shadowPosition.xy, cascadeIndex), 0).rg;

	return ChebyshevUpperBound(samp, lightDepth, 0.0001f);
}

//------------------------------------------------------------------------------
/**
*/
float
CSMPS2(
	in float Depth,
	in vec4 PositionShadowSpace,
	in float NL,
	in vec3 Normal,
	out vec4 Debug)
{
	int cascadeIndex = CASCADE_COUNT_FLAG;
	for (int i = CASCADE_COUNT_FLAG - 1; i >= 0; --i)
	{
		if (Depth < CascadeDistances[i])
			cascadeIndex = i;
	}

	// if we have no matching cascade, return with a fully lit pixel
	if (cascadeIndex == CASCADE_COUNT_FLAG)
	{
		Debug = vec4(0,0,0,0);
		return 1.0f;
	}
	else
	{
		Debug = DebugColors[cascadeIndex];
	}

	vec4 shadowPosition = PositionShadowSpace;
	//vec4 shadowPosition = CSMShadowMatrix * samplePos;
	vec3 shadowPosDDX = dFdx(shadowPosition.xyz);
	vec3 shadowPosDDY = dFdy(shadowPosition.xyz);

	float shadowVisibility = SampleShadowCascade(shadowPosition, shadowPosDDX, shadowPosDDY, cascadeIndex);

	return shadowVisibility;
}

#endif
