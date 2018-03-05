//--------------------------------------------------------------------------------------
// Cascading Shadow Maps tools
//--------------------------------------------------------------------------------------



#define VARIANCE 0
#define PCF 1
#define BLURSAMPLES 1
#define NO_COMPARISON 1

cbuffer CSMVariables
{
	matrix          CSMShadowMatrix;

	float4          CascadeOffset[CASCADE_COUNT_FLAG];
	float4          CascadeScale[CASCADE_COUNT_FLAG];
	float		CascadeBlendArea = 0.3f;

	float           MinBorderPadding;     
	float           MaxBorderPadding;
	float           ShadowPartitionSize; 
	float 		GlobalLightShadowBias;
}

static const int SplitsPerRow = 2;
static const int SplitsPerColumn = 2;

Texture2D    	ShadowProjMap;            

SamplerState 	ShadowProjMapSampler
{
	Filter = MIN_MAG_LINEAR_MIP_POINT;
	AddressU = 4;
	AddressV = 4;
	BorderColor = float4(1,1,1,1);
};


float4 colors[] = {
	float4(1, 0, 0, 0),
	float4(0, 1, 0, 0),
	float4(0, 0, 1, 0),
	float4(1, 1, 1, 0)
};

float2 kernel[] = {
	float2(0.5142, 0.6165),
	float2(-0.03512, -0.02231),
	float2(0.04147, 0.01242),
	float2(-0.02526, 0.01662),
	float2(0.03134, -0.02526),
	float2(-0.03725, 0.01724),
	float2(0.01252, 0.038314),
	float2(0.02127, -0.02667),
	float2(-0.01231, 0.04142),
	float2(0.03124, 0.02253),
	float2(-0.05661, -0.07112),
	float2(0.09937, 0.02521),
	float2(0.04132, 0.04566),
	float2(-0.07264, -0.01778),
	float2(0.03831, 0.08636),
	float2(-0.03573, -0.07284),
};

//------------------------------------------------------------------------------
/**
	Calculates Chebyshevs upper bound for use with VSM shadow mapping
*/
float 
ChebyshevUpperBound(float2 Moments, float t)  
{  
	// One-tailed inequality valid if t > Moments.x  
	if (t <= Moments.x) return 1.0f;
	
	// Compute variance.  
	float Variance = Moments.y - (Moments.x*Moments.x);  
	Variance = max(Variance, 0);  
	
	// Compute probabilistic upper bound.  
	float d = t - Moments.x;  
	float p_max = Variance / (Variance + d*d);  
	
	return p_max;  
} 

//------------------------------------------------------------------------------
/**
*/
float4 ConvertViewRayToWorldPos(float3 viewRay, float length, float3 cameraPosition)
{
	return float4(cameraPosition + viewRay * length, 1);
}

//------------------------------------------------------------------------------
/**
	Converts World Position into shadow texture lookup vector, modelviewprojection position and interpolated position, as well as depth
*/
void CSMConvert(in float4 worldPosition,	
				out float4 texShadow)
{       
    // Transform the shadow texture coordinates for all the cascades.
    texShadow = mul( worldPosition, CSMShadowMatrix );
}

//---------------------------------------------------------------------------------------------------------------------------
/**
*/
float 
GetInvertedOcclusionCSM(float2 lightSpaceUv,
					 float lightSpaceDepth,
					 float bias)
{
	float2 shadowSample = ShadowProjMap.SampleLevel(ShadowProjMapSampler, lightSpaceUv, 0).rg;
	return ChebyshevUpperBound(shadowSample, lightSpaceDepth);
}

//------------------------------------------------------------------------------
/**
*/
void 
CalculateBlendAmountForMap ( in float4 vShadowMapTextureCoord, 
                             in out float fCurrentPixelsBlendBandLocation,
                             out float fBlendBetweenCascadesAmount ) 
{
    // Calcaulte the blend band for the map based selection.
    float2 distanceToOne = float2 ( 1.0f - vShadowMapTextureCoord.x, 1.0f - vShadowMapTextureCoord.y );
    fCurrentPixelsBlendBandLocation = min( vShadowMapTextureCoord.x, vShadowMapTextureCoord.y );
    float fCurrentPixelsBlendBandLocation2 = min( distanceToOne.x, distanceToOne.y );
    fCurrentPixelsBlendBandLocation = 
        min( fCurrentPixelsBlendBandLocation, fCurrentPixelsBlendBandLocation2 );
    fBlendBetweenCascadesAmount = fCurrentPixelsBlendBandLocation / CascadeBlendArea;
}

//------------------------------------------------------------------------------
/**
	CSM shadow sampling entry point
*/
float CSMPS(in float4 TexShadow,
			in float2 FSUV)
{
	float4 vShadowMapTextureCoord = 0.0f;
    
    float fPercentLit = 1.0f;
    int iCurrentCascadeIndex = 0;
	bool iCascadeFound = false;
	float bias = GlobalLightShadowBias;
    
    // This for loop is not necessary when the frustum is uniformaly divided and interval based selection is used.
    // In this case fCurrentPixelDepth could be used as an array lookup into the correct frustum. 
    float4 vShadowMapTextureCoordViewSpace = TexShadow;
	
	
	int iCascadeIndex;
	int iNextCascadeIndex;
	[unroll(CASCADE_COUNT_FLAG)]
	for( iCascadeIndex = 0; iCascadeIndex < CASCADE_COUNT_FLAG; ++iCascadeIndex) 
	{
		vShadowMapTextureCoord = vShadowMapTextureCoordViewSpace * CascadeScale[iCascadeIndex];
		vShadowMapTextureCoord += CascadeOffset[iCascadeIndex];

		if ( min( vShadowMapTextureCoord.x, vShadowMapTextureCoord.y ) > MinBorderPadding
		  && max( vShadowMapTextureCoord.x, vShadowMapTextureCoord.y ) < MaxBorderPadding )
		{ 
			iCurrentCascadeIndex = iCascadeIndex;   
			iCascadeFound = true;
			break;
		}
	}
	
	float fBlendBetweenCascadesAmount = 1.0f;
	float fCurrentPixelsBlendBandLocation = 1.0f;
	CalculateBlendAmountForMap ( vShadowMapTextureCoord, fCurrentPixelsBlendBandLocation, fBlendBetweenCascadesAmount );
				
	// if we have no matching cascade, return with a fully lit pixel
	if (!iCascadeFound)
	{
		return 1.0f;
	}		
	
	// calculate texture coordinate in shadow space
	float2 texCoord = vShadowMapTextureCoord.xy / vShadowMapTextureCoord.ww;
	float depth = vShadowMapTextureCoord.z / vShadowMapTextureCoord.w;

	float2 sampleCoord = texCoord;
	sampleCoord.xy *= ShadowPartitionSize;
	sampleCoord.xy += float2((iCurrentCascadeIndex % SplitsPerRow) * ShadowPartitionSize, (iCurrentCascadeIndex / SplitsPerColumn) * ShadowPartitionSize);
	
	float occlusion = GetInvertedOcclusionCSM(sampleCoord, depth, bias);
	
	iNextCascadeIndex = iCurrentCascadeIndex + 1; 
	float occlusionBlend = 1.0f;
	if (fCurrentPixelsBlendBandLocation < CascadeBlendArea)
	{
		if (iNextCascadeIndex < CASCADE_COUNT_FLAG)
		{
			vShadowMapTextureCoord = vShadowMapTextureCoordViewSpace * CascadeScale[iNextCascadeIndex];
			vShadowMapTextureCoord += CascadeOffset[iNextCascadeIndex];
			
			sampleCoord = vShadowMapTextureCoord.xy / vShadowMapTextureCoord.ww;
			depth = vShadowMapTextureCoord.z / vShadowMapTextureCoord.w;
			
			sampleCoord.xy *= ShadowPartitionSize;
			sampleCoord.xy += float2((iNextCascadeIndex % SplitsPerRow) * ShadowPartitionSize, (iNextCascadeIndex / SplitsPerColumn) * ShadowPartitionSize);
			
			occlusionBlend = GetInvertedOcclusionCSM(sampleCoord, depth, bias);
		}
		
		occlusion = lerp(occlusionBlend, occlusion, fBlendBetweenCascadesAmount);
	}
	return occlusion; //linstep(0.75f, 1, occlusion);
}
