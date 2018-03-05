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

	
const int SplitsPerRow = 2;
const int SplitsPerColumn = 2;

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

samplerstate CSMTextureSampler
{
	//Samplers = { ShadowProjMap };
	//Filter = MinMagLinearMipPoint;
	AddressU = Border;
	AddressV = Border;
	//MaxAnisotropic = 16;
	//BorderColor = { 1,1,1,1 };
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
	  in vec2 FSUV,
	  in uint Texture,
	  out vec4 Debug)
{
	vec4 texCoordShadow = vec4(0.0f);
    
    float fPercentLit = 1.0f;
	bool cascadeFound = false;
	float bias = GlobalLightShadowBias;
    
    // This for loop is not necessary when the frustum is uniformly divided and interval based selection is used.
    // In this case fCurrentPixelDepth could be used as an array lookup into the correct frustum. 
    vec4 texCoordViewspace = TexShadow / TexShadow.wwww;
	
	int cascadeIndex;
	for( cascadeIndex = 0; cascadeIndex < CASCADE_COUNT_FLAG; ++cascadeIndex) 
	{
		texCoordShadow = texCoordViewspace * CascadeScale[cascadeIndex];
		texCoordShadow += CascadeOffset[cascadeIndex];

		if ( min( texCoordShadow.x, texCoordShadow.y ) > MinBorderPadding
		  && max( texCoordShadow.x, texCoordShadow.y ) < MaxBorderPadding )
		{ 
			cascadeFound = true;
			break;
		}
	}
	
	float blendAmount = 0;
	float blendBandLocation = 0;
	CalculateBlendAmountForMap ( texCoordShadow, blendBandLocation, blendAmount );
				
	// if we have no matching cascade, return with a fully lit pixel
	if (!cascadeFound)
	{
		return 1.0f;
	}		
	
	// calculate texture coordinate in shadow space
	vec2 texCoord = texCoordShadow.xy;
	float depth = texCoordShadow.z;

	vec2 sampleCoord = texCoord;
	sampleCoord.xy *= ShadowPartitionSize;
	sampleCoord.xy += vec2((cascadeIndex % SplitsPerRow) * ShadowPartitionSize, (cascadeIndex / SplitsPerColumn) * ShadowPartitionSize);

	// do an ugly poisson sample disk
	// this only causes errors when samples are taken outside 
	vec2 pixelSize = GetPixelSize(ShadowProjMap);
	vec2 uvSample;
	//vec2 currentSample = vec2(0,0);
	/*
	vec2 mapDepth = vec2(0.0f);
	float occlusion = 0.0f;
	int i;
    for (i = 0; i < 13; i++)
    {
		vec2 uvSample = sampleCoord.xy + sampleOffsets[i] * pixelSize.xy;
		mapDepth += textureLod(ShadowProjMap, sampleCoord, 0).rg;
	}
	mapDepth /= 13.0f;
	*/
	
	vec2 mapDepth = sample2DLod(Texture, CSMTextureSampler, sampleCoord, 0).rg;
	float occlusion = ChebyshevUpperBound(mapDepth, depth, 0.0000001f);
	//float occlusion = ExponentialShadowSample(mapDepth, depth, 0.0f);
		
	int nextCascade = cascadeIndex + 1; 
	float occlusionBlend = 1.0f;
	if (blendBandLocation < CascadeBlendArea)
	{
		if (nextCascade < CASCADE_COUNT_FLAG)
		{
			texCoordShadow = texCoordViewspace * CascadeScale[nextCascade];
			texCoordShadow += CascadeOffset[nextCascade];
			
			texCoord = texCoordShadow.xy;
			depth = texCoordShadow.z;
			
			sampleCoord = texCoord;			
			sampleCoord.xy *= ShadowPartitionSize;
			sampleCoord.xy += vec2((nextCascade % SplitsPerRow) * ShadowPartitionSize, (nextCascade / SplitsPerColumn) * ShadowPartitionSize);
			uvSample = sampleCoord.xy;
					
			mapDepth = sample2DLod(Texture, CSMTextureSampler, uvSample, 0).rg;
			occlusionBlend = ChebyshevUpperBound(mapDepth, depth, 0.0000001f);		
		}
		
		// blend next cascade onto previous
		occlusion = lerp(occlusionBlend, occlusion, blendAmount);
	}
	//occlusion += smoothstep(0.98f, 1.0f, increment);
	//occlusion += increment;
	
	
	// finally clamp all shadow values 0.5, this avoids any weird color differences when blending between cascades
	Debug = DebugColors[cascadeIndex];
	//return smoothstep(0.5f, 1.0f, occlusion);
	return occlusion;
}

#endif