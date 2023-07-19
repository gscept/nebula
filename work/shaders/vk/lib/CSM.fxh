//------------------------------------------------------------------------------
//  CSM.fxh
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#ifndef CSM_FXH
#define CSM_FXH

//#define CSM_DEBUG

#include "shadowbase.fxh"
#include "std.fxh"

const vec4 DebugColors[8] =
{
    vec4 ( 1.5f, 0.0f, 0.0f, 1.0f ),
    vec4 ( 0.0f, 1.5f, 0.0f, 1.0f ),
    vec4 ( 0.0f, 0.0f, 5.5f, 1.0f ),
    vec4 ( 1.5f, 0.0f, 5.5f, 1.0f ),
    vec4 ( 1.5f, 1.5f, 0.0f, 1.0f ),
    vec4 ( 1.0f, 1.0f, 1.0f, 1.0f ),
    vec4 ( 0.0f, 1.0f, 5.5f, 1.0f ),
    vec4 ( 0.5f, 3.5f, 0.75f, 1.0f )
};

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

//------------------------------------------------------------------------------
/**
	CSM shadow sampling entry point
*/
float
CSMPS(
	  in vec4 TexShadow
	, in uint Texture
    , in float noise
#ifdef CSM_DEBUG
	, out vec4 Debug
#endif
)
{
	vec4 texCoordShadow = vec4(0.0f);
	bool cascadeFound = false;

    TexShadow = TexShadow / TexShadow.wwww;
    vec2 uvDdx = dFdx(TexShadow.xy);
    vec2 uvDdy = dFdy(TexShadow.xy);

	int cascadeIndex;
	for( cascadeIndex = 0; cascadeIndex < NUM_CASCADES; ++cascadeIndex)
	{
		texCoordShadow = mad(TexShadow, CascadeScale[cascadeIndex], CascadeOffset[cascadeIndex]);

		if ( min( texCoordShadow.x, texCoordShadow.y ) >= 0.0f
		  && max( texCoordShadow.x, texCoordShadow.y ) <= 1.0f )
		{
			cascadeFound = true;
			break;
		}
	}

#ifdef CSM_DEBUG
	Debug = DebugColors[cascadeIndex];
#endif

	// if we have no matching cascade, return with a fully lit pixel
	if (!cascadeFound)
	{
		return 1.0f;
	}

	// calculate texture coordinate in shadow space
	float occlusion = PCFShadowArray(Texture, texCoordShadow.z, texCoordShadow.xy, cascadeIndex, GlobalLightShadowMapSize.xy, noise);
	return occlusion;
}

#endif
