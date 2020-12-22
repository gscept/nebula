//------------------------------------------------------------------------------
//  finalize.fx
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"
#include "lib/techniques.fxh"
#include "lib/shared.fxh"
#include "lib/preetham.fxh"

group(BATCH_GROUP) shared constant FinalizeBlock
{
	textureHandle DepthTexture;
	textureHandle ColorTexture;
	textureHandle NormalTexture;
	textureHandle LuminanceTexture;
	textureHandle BloomTexture;
};


sampler_state UpscaleSampler
{
	//Samplers = { BloomTexture, GodrayTexture, LuminanceTexture };
	AddressU = Border;
	AddressV = Border;
	BorderColor = Transparent;
};

sampler_state DefaultSampler
{
	//Samplers = { ColorTexture, DepthTexture, ShapeTexture};
	Filter = Point;
	AddressU = Border;
	AddressV = Border;
	BorderColor = Transparent;
};

render_state FinalizeState
{
	CullMode = Back;
	DepthEnabled = false;
	DepthWrite = false;
};

// depth of field samples
#define MAXDOFSAMPLES 23

const vec2 DofSamples[MAXDOFSAMPLES] = {
        vec2( 0.0, 0.0 ),
        vec2( -0.326212, -0.40581  ),
        vec2( -0.840144, -0.07358  ),
        vec2( -0.695914,  0.457137 ),
        vec2( -0.203345,  0.620716 ),
        vec2(  0.96234,  -0.194983 ),
        vec2(  0.473434, -0.480026 ),
        vec2(  0.519456,  0.767022 ),
        vec2(  0.185461, -0.893124 ),
        vec2(  0.507431,  0.064425 ),
        vec2(  0.89642,   0.412458 ),
        vec2( -0.32194,   0.93261f ),
        vec2(  0.326212,  0.40581  ),
        vec2(  0.840144,  0.07358  ),
        vec2(  0.695914, -0.457137 ),
        vec2(  0.203345, -0.620716 ),
        vec2( -0.96234,   0.194983 ),
        vec2( -0.473434,  0.480026 ),
        vec2( -0.519456, -0.767022 ),
        vec2( -0.185461,  0.893124 ),
        vec2( -0.507431, -0.064425 ),
        vec2( -0.89642,  -0.412458 ),
        vec2(  0.32194,  -0.93261f )
};   

//------------------------------------------------------------------------------
/**
*/
shader
void
vsMain(
	[slot=0] in vec3 position,
	[slot=2] in vec2 uv,
	out vec2 UV) 
{
	gl_Position = vec4(position, 1);
	UV = uv;
}

//------------------------------------------------------------------------------
/**
	Compute fogging given a sampled fog intensity value from the depth
	pass and a fog color.
*/
float
Fog(float fogDepth)
{
	return clamp((FogDistances.y - fogDepth) / (FogDistances.y - FogDistances.x), FogColor.a, 1.0);
}


//------------------------------------------------------------------------------
/**
	Get a depth-of-field blurred sample. Set all values to 0 in order to disable DoF
*/
vec4 
DepthOfField(float depth, vec2 uv)
{
    // compute focus-blur param (0 -> no blur, 1 -> max blur)
    float focusDist = DoFDistances.x;
    float focusLength = DoFDistances.y;
    float filterRadius = DoFDistances.z;    
    float focus = saturate(abs(depth - focusDist) / focusLength);
    
    // perform a gaussian blur around uv
    vec3 sampleColor = vec3(0.0f);
    float dofWeight = 1.0f / MAXDOFSAMPLES;
	vec2 pixelSize = RenderTargetDimensions[0].zw;
    vec2 uvMul = focus * filterRadius * pixelSize.xy;
    int i;
    for (i = 0; i < MAXDOFSAMPLES; i++)
    {
        sampleColor += sample2DLod(ColorTexture, DefaultSampler, uv + (DofSamples[i] * uvMul), 0).rgb;
    }
    sampleColor *= dofWeight;
    return vec4(sampleColor, 1);
} 

//------------------------------------------------------------------------------
/**
*/
shader
void
psMain(in vec2 UV,
	[color0] out vec4 color) 
{
    // get an averaged depth value        
    float depth = sample2DLod(DepthBuffer, DefaultSampler, UV, 0).r;
	vec4 viewPos = PixelToView(UV, depth);
	vec3 normal = sample2DLod(NormalBuffer, DefaultSampler, UV, 0).xyz;

	vec4 worldPos = ViewToWorld(viewPos);
	vec3 viewVec = EyePos.xyz - worldPos.xyz;
	vec3 viewNormal = (View * vec4(normal, 0)).xyz;

	vec3 fogColor = FogColor.rgb;
	fogColor *= Preetham(-normalize(viewVec), GlobalLightDirWorldspace.xyz, A, B, C, D, E, Z) * GlobalLightColor.xyz;

	float fogIntensity = Fog(length(viewVec)); 
	
	vec4 c = DepthOfField(depth, UV);
	c = vec4(lerp(fogColor, c.rgb, fogIntensity), c.a);
	
	// Get the calculated average luminance 
	float fLumAvg = sample2DLod(LuminanceTexture, UpscaleSampler, vec2(0.5f, 0.5f), 0).r;
	
	vec4 bloom = sample2DLod(BloomTexture, UpscaleSampler, UV, 0);
	//vec4 godray = subpassLoad(InputAttachment1);
	c += bloom;   
	//c.rgb += godray.rgb;
	//c.rgb += godray.rgb;
	vec4 grey = vec4(dot(c.xyz, Luminance.xyz));
	c = Balance * lerp(grey, c, Saturation);
	c.rgb *= FadeValue;

	// tonemap before presenting to screen
	c = ToneMap(c, vec4(fLumAvg), MaxLuminance);
	color = c;	
}

//------------------------------------------------------------------------------
/**
*/
PostEffect(vsMain(), psMain(), FinalizeState);
