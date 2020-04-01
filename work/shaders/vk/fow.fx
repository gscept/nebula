//------------------------------------------------------------------------------
//  fow.fx
//  (C) 2014 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/geometrybase.fxh"
#include "lib/techniques.fxh"

sampler2D FowMap;
vec2 FowSize;
vec2 FocalLengthFow;

render_state FowState
{
	CullMode = Back;
	DepthEnabled = false;
	DepthWrite = false;
	BlendEnabled[0] = true;
	SrcBlend[0] = Zero;
	DstBlend[0] = SrcColor;
};

// samplers
sampler_state FowMapSampler
{
	Samplers = { FowMap };
};


//------------------------------------------------------------------------------
/**
*/
shader
void
vsFow(
	[slot=0] in vec3 position,
	[slot=2] in vec2 uv,
	out vec2 UV,
	out vec3 ViewSpacePos) 
{
    gl_Position = vec4(position, 1);
	ViewSpacePos = vec3(position.xy * FocalLengthFow.xy, -1);
    UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psFow(in vec2 UV,
	in vec3 ViewSpacePosition,
	[color0] out vec4 Albedo) 
{
	vec2 pixelSize = RenderTargetDimensions[0].zw;
	vec2 screenUV = gl_FragCoord.xy * pixelSize;
	float Depth = sample2DLod(DepthBuffer, PosteffectSampler, screenUV, 0).r;
	
	vec3 viewVec = normalize(ViewSpacePosition);
	vec3 viewSpacePos = viewVec * Depth;    
	vec4 worldPos = InvView * vec4(viewSpacePos, 1);
	
	vec2 samplePos = (worldPos.zx + FowSize * 0.5f) / FowSize;
	vec2 fowPixelSize = GetPixelSize(FowMap);
	float sample1 = textureLod(FowMap, samplePos + vec2(0.5f, 0.5f) * fowPixelSize, 0).r;
	float sample2 = textureLod(FowMap, samplePos + vec2(0.5f, -0.5f) * fowPixelSize, 0).r;
	float sample3 = textureLod(FowMap, samplePos + vec2(-0.5f, 0.5f) * fowPixelSize, 0).r;
	float sample4 = textureLod(FowMap, samplePos + vec2(-0.5f, -0.5f) * fowPixelSize, 0).r;
	
	// the FowMap is R8, so duplicate every value into a float4, apply it and blend.
	vec4 fowColor = vec4((sample1 + sample2 + sample3 + sample4) * 0.25f);
	Albedo = EncodeHDR(fowColor);
}

//------------------------------------------------------------------------------
/**
*/
PostEffect(vsFow(), psFow(), FowState);