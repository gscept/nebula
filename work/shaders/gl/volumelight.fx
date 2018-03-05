//------------------------------------------------------------------------------
//  volumelight.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"
#include "lib/techniques.fxh"
#include "lib/preetham.fxh"

mat4 Transform;
vec4 LightColor = vec4(1,1,1,1);
float VolumetricScale = 1.0f;
float VolumetricIntensity = 0.14f;
vec2 LightCenter;

/// Declaring used textures

sampler2D UnshadedTexture;
samplerCube LightProjCube;
sampler2D LightProjMap;


state LocalLightState
{
	BlendEnabled[0] = true;
	SrcBlend[0] = One;
	DstBlend[0] = OneMinusSrcAlpha;
	CullMode = None;
	DepthWrite = false;
};

state GlobalLightState
{
	//BlendEnabled[0] = true;
	
	CullMode = Back;
	DepthEnabled = true;
	DepthWrite = false;
	DepthFunc = Equal;
};

state ScatterState
{
	//BlendEnabled[0] = true;
	DepthWrite = false;
	DepthWrite = false;
	DepthFunc = Equal;
};

//------------------------------------------------------------------------------
/**
*/
shader
void
vsGeometry(in vec3 position,
	in vec3 normal,
	in vec2 uv,
	in vec3 tangent,
	in vec3 binormal,
	out vec2 UV,
	out vec3 Direction) 
{
	vec4 modelSpace = Transform * vec4(position, 1);
	gl_Position = ViewProjection * modelSpace;
	Direction = (View * modelSpace).xyz;
	UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsVolumetric(in vec3 position,
	in vec3 normal,
	in vec2 uv,
	in vec3 tangent,
	in vec3 binormal,
	out vec3 ViewSpacePos,
	out vec3 WorldPos,
	out vec3 Tangent,
	out vec3 Normal,
	out vec3 Binormal,
	out vec2 UV,
	out vec3 Direction) 
{
	mat4 modelView = View * Transform;
	Tangent 	= (modelView * vec4(tangent, 0)).xyz;
	Normal 		= (modelView * vec4(normal, 0)).xyz;
	Binormal 	= (modelView * vec4(binormal, 0)).xyz;
	ViewSpacePos = (modelView * vec4(position, 1)).xyz;
	WorldPos = vec4(position * VolumetricScale, 1).xyz;
	vec4 scaledPos = vec4(position * VolumetricScale, 1);
	gl_Position = ViewProjection * Transform * scaledPos;
	Direction = (modelView * scaledPos).xyz;
	UV = uv;
}

#define PI 3.14159265
#define ONE_OVER_PI 1/(3*PI)

//------------------------------------------------------------------------------
/**
	Render spotlight using LightProjMap and use 
*/
shader
void
psVolumetricSpot(in vec3 ViewSpacePos,
		in vec3 WorldPos,
		in vec3 Tangent,
		in vec3 Normal,
		in vec3 Binormal,
		in vec2 UV,
		[color0] out vec4 Color) 
{	
	// calculate freznel effect to give a smooth linear falloff of the godray effect
	mat3 tangentViewMatrix = mat3(normalize(Tangent.xyz), normalize(Binormal.xyz), normalize(Normal.xyz));   
	vec3 tNormal = vec3(0,0,0);
	tNormal.xy = vec2(0);
	tNormal.z = 1;
	vec3 worldSpaceNormal = (tangentViewMatrix * tNormal).xyz;
	float rim = pow(abs(abs(dot(normalize(ViewSpacePos.xyz), worldSpaceNormal))), 3);
	
	vec4 colorSample = textureLod(LightProjMap, UV, 0);
	Color = colorSample * (LightColor * ONE_OVER_PI) * rim * VolumetricIntensity;
	Color.a = 1.0f;
}

//------------------------------------------------------------------------------
/**
	Render the actual godray stuff, we do this by simply rendering the color and texture
*/
shader
void
psVolumetricPoint(in vec3 ViewSpacePos,
		in vec3 WorldPos,
		in vec3 Tangent,
		in vec3 Normal,
		in vec3 Binormal,
		in vec2 UV,
		[color0] out vec4 Color) 
{	
	// calculate freznel effect to give a smooth linear falloff of the godray effect
	mat3 tangentViewMatrix = mat3(normalize(Tangent.xyz), normalize(Binormal.xyz), normalize(Normal.xyz));   
	vec3 tNormal = vec3(0,0,0);
	tNormal.xy = vec2(0);
	tNormal.z = 1;
	vec3 worldSpaceNormal = (tangentViewMatrix * tNormal).xyz;
	float rim = pow(abs(abs(dot(normalize(ViewSpacePos.xyz), worldSpaceNormal))), 3);
	
	vec4 colorSample = textureLod(LightProjCube, normalize(WorldPos), 0);
	Color = colorSample * (LightColor * ONE_OVER_PI) * rim * VolumetricIntensity;
	Color.a = 1.0f;
}

//------------------------------------------------------------------------------
/**
	Render the actual godray stuff, we do this by simply rendering the color and texture
*/
shader
void
psVolumetricGlobal(in vec3 ViewSpacePos,
		in vec3 WorldPos,
		in vec3 Tangent,
		in vec3 Normal,
		in vec3 Binormal,
		in vec2 UV,
		in vec3 Direction,
		[color0] out vec4 Color) 
{
	vec2 pixelSize = RenderTargetDimensions.xy;
	vec2 screenUV = psComputeScreenCoord(gl_FragCoord.xy, pixelSize.xy);
	
	float unshaded = texture(UnshadedTexture, screenUV).a;
	
	// calculate sky contribution
	vec3 lightDir = normalize(GlobalLightDir.xyz);
	vec3 dir = normalize(Direction);
	vec3 atmo = Preetham(dir, lightDir, A, B, C, D, E) * GlobalLightColor.rgb;
	
	// calculate freznel effect to give a smooth linear falloff of the godray effect
	mat3 tangentViewMatrix = mat3(normalize(Tangent.xyz), normalize(Binormal.xyz), normalize(Normal.xyz));   
	vec3 tNormal = vec3(0,0,0);
	tNormal.xy = vec2(0);
	tNormal.z = 1;
	vec3 worldSpaceNormal = (tangentViewMatrix * tNormal).xyz;
	float rim = pow(abs(abs(dot(normalize(ViewSpacePos.xyz), worldSpaceNormal))), 3);
	
	vec4 colorSample = textureLod(LightProjMap, UV, 0);
	Color.rgb = saturate(atmo) * colorSample.rgb * ONE_OVER_PI * rim * VolumetricIntensity;
	Color.a = 1.0f;
	
	gl_FragDepth = 1.0f;
}

//------------------------------------------------------------------------------
/**
	Render the sun geometry, this needs to be here because we don't want to see the skybox
	We intentionally render it completely white which gives a nice bloom effect
*/
shader
void
psLocalLight(in vec2 UV,
	[color0] out vec4 Color) 
{
	vec4 sunSample = textureLod(LightProjMap, UV, 0);
	Color = LightColor * sunSample;
	Color.a = 1.0f;
}

//------------------------------------------------------------------------------
/**
	Render the sun geometry, this needs to be here because we don't want to see the skybox
	We intentionally render it completely white which gives a nice bloom effect
*/
shader
void
psGlobalLight(in vec2 UV,
	in vec3 Direction,
	[color0] out vec4 Color) 
{
	// calculate sky contribution
	vec3 lightDir = normalize(GlobalLightDir.xyz);
	vec3 dir = normalize(Direction);
	vec3 atmo = Preetham(dir, lightDir, A, B, C, D, E) * GlobalLightColor.rgb;
	
	vec4 sunSample = texture(LightProjMap, UV);
	vec2 texSize = textureSize(LightProjMap, 0);
	float falloff = saturate(distance(gl_FragCoord.xy / texSize, LightCenter));
	Color.rgb = saturate(atmo) * sunSample.rgb * falloff * VolumetricIntensity;
	Color.a = 1.0f;
	
	gl_FragDepth = 1.0f;
}

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(Volumetric, "Alt0", vsGeometry(), psGlobalLight(), ScatterState);
SimpleTechnique(PointLight, "Point", vsVolumetric(), psVolumetricPoint(), LocalLightState);
SimpleTechnique(SpotLight, "Spot", vsVolumetric(), psVolumetricSpot(), LocalLightState);
SimpleTechnique(GlobalLight, "Global", vsVolumetric(), psVolumetricGlobal(), GlobalLightState);
