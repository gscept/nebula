//------------------------------------------------------------------------------
//  boxtap.fxh
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#ifndef BOXTAP_FXH
#define BOXTAP_FXH

#include "lib/shared.fxh"

/// Declaring used textures
sampler2D SourceBuffer;

samplerstate BoxTapSampler
{
	Samplers = { SourceBuffer };
	AddressU = Clamp;
	AddressV = Clamp;
	Filter = Point;
};

state BoxtapState
{
	CullMode = Back;
	DepthEnabled = false;
	DepthWrite = false;
};

vec4 ShadowConstants = vec4(100.0f, 100.0f, 0.003f, 1024.0f);

#ifdef Hori
    #define DIRECTION   vec2(1.0, 0.0)
#endif
#ifdef Vert
    #define DIRECTION   vec2(0.0, 1.0)
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
    vec4 position;
    vec4 taps1_2;
#ifdef Taps3 
    vec2 taps3_4;
#endif
#ifdef Taps5 
    vec4 taps3_4;
    vec2 taps5_6;
#endif
#ifdef Taps7
    vec4 taps3_4;
    vec4 taps5_6;
    vec2 taps7;
#endif
};

//------------------------------------------------------------------------------
/**
*/
void
GenerateFilterSteps(vec2 uv, out vec2 result[STEP_TAPS])
{
    // calulate some uv's   
    vec2 texelSize = 1.0f / vec2(ShadowConstants.w);
    vec2 step = DIRECTION * texelSize;
    vec2 base = uv - (((float(STEP_TAPS) - 1)*0.5f) * step);
    for (int i = 0; i < STEP_TAPS; i++)
    {
        result[i] = base;
        base += step;
    }
}


//------------------------------------------------------------------------------
/**
*/
shader
void
vsMain(
	[slot=0] in vec3 position,
	[slot=2] in vec2 uv,
    out vsInOut vsOut) 
{
	vec2 taps[STEP_TAPS];
	vec2 invUv = uv;
	invUv.y = 1 - invUv.y;
	GenerateFilterSteps(invUv, taps);

	gl_Position = vec4(position, 1);
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
vec2 
FilterTaps(in vec2 UVs[STEP_TAPS])
{
	const float c = (1.f/float(STEP_TAPS));
	vec2 filteredValue = vec2(0);
	int index;
	for (index = 0; index < STEP_TAPS; index++)
	{
		vec4 color = textureLod(SourceBuffer, UVs[index], 0);
		filteredValue += color.rg;
	}
	return filteredValue * c;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psMain(in vsInOut psIn,
	[color0] out vec2 ShadowColor) 
{

    vec2 uvs[STEP_TAPS] = vec2[]( 
							vec2(psIn.taps1_2.xy), 
							vec2(psIn.taps1_2.zw), 
							vec2(psIn.taps3_4.xy) 
                            #if (STEP_TAPS > 3)
                              ,
							  vec2(psIn.taps3_4.zw), 
							  vec2(psIn.taps5_6.xy) 
                            #if (STEP_TAPS > 5)
                              ,
							  vec2(psIn.taps5_6.zw), 
							  vec2(psIn.taps7)
                            #endif
                            #endif
                            );
	ShadowColor = FilterTaps(uvs);
}

#endif