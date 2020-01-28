//------------------------------------------------------------------------------
//  lightmapped.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/shared.fxh"
#include "lib/util.fxh"
#include "lib/CSM.fxh"
#include "lib/shadowbase.fxh"
#include "lib/techniques.fxh"
#include "lib/defaultsamplers.fxh"
#include "lib/lightmapbase.fxh"
#include "lib/geometrybase.fxh"

state LightmapState
{
	CullMode = Back;
};

//------------------------------------------------------------------------------
/**
	Used for lightmapped geometry
*/
shader
void
vsLightmapped(
	[slot=0] in vec3 position,
	[slot=1] in vec3 normal,
	[slot=2] in vec2 uv1,
	[slot=3] in vec3 tangent,
	[slot=4] in vec3 binormal,
	[slot=6] in vec2 uv2,
	out vec3 ViewSpacePos,
	out vec3 Tangent,
	out vec3 Normal,
	out vec3 Binormal,
	out vec2 UV1,
	out vec2 UV2) 
{
	gl_Position = ViewProjection * Model * vec4(position, 1);
	UV1 = uv1;
	UV2 = uv2;
	mat4 modelView = View * Model;

	ViewSpacePos = (modelView * vec4(position, 1)).xyz;
	Tangent = (modelView * vec4(tangent, 0)).xyz;
	Normal = (modelView * vec4(normal, 0)).xyz;
	Binormal = (modelView * vec4(binormal, 0)).xyz;
}

//------------------------------------------------------------------------------
/**
	Used for lightmapped geometry with vertex colors
*/
shader
void
vsLightmappedVertexColors(
	[slot=0] in vec3 position,
	[slot=1] in vec3 normal,
	[slot=2] in vec2 uv1,	
	[slot=3] in vec3 tangent,
	[slot=4] in vec3 binormal,
	[slot=5] in vec4 color,
	[slot=6] in vec2 uv2,
	out vec3 ViewSpacePos,
	out vec3 Tangent,
	out vec3 Normal,
	out vec3 Binormal,
	out vec2 UV1,
	out vec2 UV2,
	out vec4 Color) 
{
	gl_Position = ViewProjection * Model * vec4(position, 1);
	UV1 = uv1;
	UV2 = uv2;
	mat4 modelView = View * Model;
	Color = color;

	ViewSpacePos = (modelView * vec4(position, 1)).xyz;
	Tangent = (modelView * vec4(tangent, 0)).xyz;
	Normal = (modelView * vec4(normal, 0)).xyz;
	Binormal = (modelView * vec4(binormal, 0)).xyz;
}

//------------------------------------------------------------------------------
/**
	Used for lightmapped 
*/
shader
void
vsLightmappedFoliage(
	[slot=0] in vec3 position,
	[slot=1] in vec3 normal,
	[slot=2] in vec2 uv1,
	[slot=3] in vec3 tangent,
	[slot=4] in vec3 binormal,
	[slot=5] in vec4 color,
	[slot=6] in vec2 uv2,
	out vec3 ViewSpacePos,
	out vec3 Tangent,
	out vec3 Normal,
	out vec3 Binormal,
	out vec2 UV1,
	out vec2 UV2)
{
	vec4 dir = InvModel * vec4(WindDirection.xyz, 0);
	vec4 windDir = WindForce * normalize(dir);
	float len = length(position);
	
	float windSpeed = WindSpeed * (TimeAndRandom.x + ObjectId);
	float windAmplitude = len / WindWaveSize;
	float windStrength = sin(windSpeed + windAmplitude);
	
	vec4 finalOffset = windDir * windStrength * color.a;
	vec4 finalPos = vec4(position + finalOffset.xyz, 1);
	
	gl_Position = ViewProjection * Model * finalPos;
	UV1 = uv1;
	UV2 = uv2;
	mat4 modelView = View * Model;

	ViewSpacePos = (modelView * finalPos).xyz;
	Tangent = (modelView * vec4(tangent, 0)).xyz;
	Normal = (modelView * vec4(normal, 0)).xyz;
	Binormal = (modelView * vec4(binormal, 0)).xyz;	
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsShadow(
	[slot=0] in vec3 position,
	[slot=1] in vec3 normal,
	[slot=2] in vec2 uv1,
	[slot=3] in vec3 tangent,
	[slot=4] in vec3 binormal,
	[slot=6] in vec2 uv2,
	out vec2 UV,
	out vec4 Position) 
{
	gl_Position = ViewMatrixArray[0] * Model * vec4(position, 1);
	Position = gl_Position;
	UV = uv1;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsShadowCSM(
	[slot=0] in vec3 position,
	[slot=1] in vec3 normal,
	[slot=2] in vec2 uv1,
	[slot=3] in vec3 tangent,
	[slot=4] in vec3 binormal,
	[slot=6] in vec2 uv2,
	out vec2 UV,
	out vec4 Position,
	out int Instance) 
{
	Position = ViewMatrixArray[gl_InstanceID] * Model * vec4(position, 1);
	Instance = gl_InstanceID;
	UV = uv1;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsShadowFoliage(
	[slot=0] in vec3 position,
	[slot=1] in vec3 normal,
	[slot=2] in vec2 uv1,
	[slot=3] in vec3 tangent,
	[slot=4] in vec3 binormal,
	[slot=6] in vec2 uv2,
	out vec2 UV,
	out vec4 Position) 
{
	vec4 dir = InvModel * vec4(WindDirection.xyz, 0);
	vec4 windDir = WindForce * normalize(dir);
	float len = length(position);
	
	float windSpeed = WindSpeed * (TimeAndRandom.x + ObjectId);
	float windAmplitude = len / WindWaveSize;
	float windStrength = sin(windSpeed + windAmplitude);
	
	vec4 finalOffset = windDir * windStrength;
	vec4 finalPos = vec4(position + finalOffset.xyz, 1);
	
	gl_Position = ViewMatrixArray[0] * Model * finalPos;
	Position = gl_Position;
	UV = uv1;
}

//------------------------------------------------------------------------------
/**
	Standard techniques
*/
SimpleTechnique(
	Lit,
	"Static",
	vsLightmapped(),
	psLightmappedLit(
		calcColor = SimpleColor,
		calcBump = NormalMapFunctor,
		calcMaterial = DefaultMaterialFunctor
	),
	LightmapState
);
SimpleTechnique(Unlit, "Static|Unlit", vsLightmapped(), psLightmappedUnlit(), LightmapState);
SimpleTechnique(
	LitVertexColors,
	"Static|Colored",
	vsLightmappedVertexColors(),
	psLightmappedLitVertexColors(
		calcColor = SimpleColor,
		calcBump = NormalMapFunctor,
		calcMaterial = DefaultMaterialFunctor
	),
	LightmapState
);
SimpleTechnique(UnlitVertexColors, "Static|Unlit|Colored", vsLightmappedVertexColors(), psLightmappedUnlitVertexColors(), LightmapState);

//------------------------------------------------------------------------------
/**
	Shadow techniques
*/
SimpleTechnique(SpotlightShadow, "Static|Spot", vsShadow(), psShadow(), LightmapState);
GeometryTechnique(CSMShadow, "Static|Global", vsShadowCSM(), psShadow(), gsCSM(), LightmapState);
