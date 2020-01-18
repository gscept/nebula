//------------------------------------------------------------------------------
//  volumefog.fx
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/shared.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"

float DepthDensity = 1.0f;
float AlphaDensity = 0.0f;
float LayerDistance = 0.0f;
vec2 Velocity = vec2(0,0);
vec2 VolumeFogDistances = vec2(0.0f, 50.0f);
vec4 VolumeFogColor = vec4(0.5f, 0.5f, 0.63f, 0.0f);

sampler2D AlbedoMap;
sampler2D DepthMap;

samplerstate VolumeFogSampler
{
	Samplers = { AlbedoMap, DepthMap };
};

state VolumeFogState
{
	CullMode = None;
	DepthWrite = false;
};

const vec2 layerVelocities[4] = { 
									vec2(1.0, 1.0),
                                    vec2(-0.9, -0.8),
                                    vec2( 0.8, -0.9),
                                    vec2(-0.4,  0.5)
									};

//------------------------------------------------------------------------------
/**
*/
void 
SampleTexture(in vec2 uv, in vec4 vertexColor, inout vec4 dstColor)
{
    vec4 srcColor = texture(AlbedoMap, uv) * vertexColor;
    dstColor.rgb = lerp(dstColor.rgb, srcColor.rgb, srcColor.a);
    dstColor.a += srcColor.a * 0.25;
}

//------------------------------------------------------------------------------
/**
    Compute fogging given a sampled fog intensity value from the depth
    pass and a fog color.
*/
vec4 
psFog(float fogDepth, vec4 color)
{
    float fogIntensity = clamp((VolumeFogDistances.y - fogDepth) / (VolumeFogDistances.y - VolumeFogDistances.x), VolumeFogColor.a, 1.0);
    return vec4(lerp(VolumeFogColor.rgb, color.rgb, fogIntensity), color.a);
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsMain(
	[slot=0] in vec3 position,
	[slot=1] in vec3 normal,
	[slot=2] in vec2 uv,
	[slot=3] in vec3 tangent,
	[slot=4] in vec3 binormal,
	[slot=5] in vec4 color,
	out vec3 ViewSpacePos,
	out vec4 UVSet1,
	out vec4 UVSet2,
	out vec4 Color) 
{
	gl_Position = Projection * View * Model * vec4(position, 1);
	ViewSpacePos = mat3(View * Model) * position;
	
	vec4 worldPos = Model * vec4(position, 1);
	vec3 worldEyeVec = normalize(EyePos.xyz - worldPos.xyz);
	vec3 worldNormal = normalize(mat3(Model) * normal);
	
	// compute animated uvs
	vec2 uvs[4];
	vec2 uvDiff = worldEyeVec.xy * LayerDistance;
	vec2 uvOffset = -4 * uvDiff;
	
	for (int i = 0; i < 4; i++)
	{
		uvOffset += uvDiff;
		uvs[i] = uv + uvOffset + Velocity.xy * TimeAndRandom.x * layerVelocities[i];
	}
	
	UVSet1.xy = uvs[0];
	UVSet1.zw = uvs[1];
	UVSet2.xy = uvs[2];
	UVSet2.zw = uvs[3];
	
	Color = color;
	float dotP = dot(worldNormal, worldEyeVec);
	Color.a = dotP * dotP;
}


//------------------------------------------------------------------------------
/**
*/
shader
void
psMain(in vec3 ViewSpacePos,
	in vec4 UVSet1,
	in vec4 UVSet2,
	in vec4 Color,
	[color0] out vec4 Result,
	[color1] out vec4 Unshaded)
{
	vec2 normalMapPixelSize = GetPixelSize(AlbedoMap);
	vec2 screenUv = PixelToNormalized(gl_FragCoord.xy, normalMapPixelSize.xy);
	
	vec4 dstColor = vec4(0,0,0,0);
	SampleTexture(UVSet1.xy, Color, dstColor);
	SampleTexture(UVSet1.zw, Color, dstColor);
	SampleTexture(UVSet2.xy, Color, dstColor);
	SampleTexture(UVSet2.zw, Color, dstColor);
	
	vec3 viewVec = normalize(ViewSpacePos);
	float depth = texture(DepthMap, screenUv).r;
	vec3 surfacePos = viewVec * depth;
	
	float depthDiff = ViewSpacePos.z - surfacePos.z;
	
	// modulate alpha by depth
	float modAlpha = saturate(depthDiff * 5.0f) * DepthDensity * AlphaDensity * 2;
	Result.rgb = dstColor.rgb;
	Result.a = dstColor.a * modAlpha;
	Result = psFog(gl_FragCoord.z, Result);
}

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(Default, "Static", vsMain(), psMain(), VolumeFogState);
