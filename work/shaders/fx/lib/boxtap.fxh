#include "lib/defaultsampler.fxh"

/// Declaring used textures
Texture2D SourceBuffer;

float4 ShadowConstants : ShadowConstants = {100.0f, 100.0f, 0.003f, 1024.0f};

#ifdef Hori
    #define DIRECTION   float2(1.0, 0.0)
#endif
#ifdef Vert
    #define DIRECTION   float2(0.0, 1.0)
#endif

#ifdef Taps3
    #define STEP_TAPS   3
#endif
#ifdef Taps5
    #define STEP_TAPS   5
#endif
#ifdef Taps7
    #define STEP_TAPS   7
#endif

//---------------------------------------------------------------------------------------------------------------------
struct vsInOut
{
    float4 position : SV_POSITION0;
    float4 taps1_2  : TEXCOORD0;
#ifdef Taps3 
    float2 taps3_4  : TEXCOORD1;
#endif
#ifdef Taps5 
    float4 taps3_4  : TEXCOORD1;
    float2 taps5    : TEXCOORD2;
#endif
#ifdef Taps7
    float4 taps3_4  : TEXCOORD1;
    float4 taps5_6  : TEXCOORD2;
    float2 taps7    : TEXCOORD3;
#endif
};

//------------------------------------------------------------------------------
/**
*/
void
GenerateFilterSteps(float2 uv, out float2 result[STEP_TAPS])
{
    // calulate some uv's   
    float2 texelSize = 1.0f / float2(ShadowConstants.w, ShadowConstants.w);
    float2 step = DIRECTION * texelSize;
    float2 base = uv - ((((float)STEP_TAPS - 1)*0.5f) * step);
    for (int i = 0; i < STEP_TAPS; i++)
    {
        result[i] = base;
        base += step;
    }
}


//------------------------------------------------------------------------------
/**
*/
void
vsMain(in float4 position : POSITION,
	in float4 uv : TEXCOORD,
    out vsInOut vsOut) 
{
	float2 taps[STEP_TAPS];
	GenerateFilterSteps(uv, taps);

	vsOut.position = position;
    vsOut.taps1_2.xy = taps[0];
    vsOut.taps1_2.zw = taps[1];
    vsOut.taps3_4.xy = taps[2];
 #if (STEP_TAPS > 3)    
    vsOut.taps3_4.zw = taps[3];   
    vsOut.taps5_6.xy = taps[4];
 #if (STEP_TAPS > 5)    
    vsOut.taps5_6.zw = taps[5];
    vsOut.taps7      = taps[6];
 #endif
 #endif
}

//------------------------------------------------------------------------------
/**
*/
float FilterTaps(in float2 UVs[STEP_TAPS])
{
	const float c = (1.f/float(STEP_TAPS));
	float filteredValue = 0;
	int index;
	for (index = 0; index < STEP_TAPS; index++)
	{
		filteredValue += SourceBuffer.Sample( sourceSampler, UVs[index] );
	}
	return filteredValue * c;
}

//------------------------------------------------------------------------------
/**
*/
void
psMain(in vsInOut psIn,
	out float ShadowColor : SV_TARGET0) 
{

    float2 uvs[STEP_TAPS] = { psIn.taps1_2.xy, psIn.taps1_2.zw, psIn.taps3_4.xy, 
                            #if STEP_TAPS > 3
                              psIn.taps3_4.zw, psIn.taps5_6.xy, 
                            #if STEP_TAPS > 5
                              psIn.taps5_6.zw, psIn.taps7
                            #endif
                            #endif
                            };
	ShadowColor = FilterTaps(uvs);
}
