//------------------------------------------------------------------------------
//  geometrybase.fxh
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"
#include "lib/geometrybase.fxh"
#include "lib/shadowbase.fxh"
#include "lib/techniques.fxh"
#include "lib/lightmapbase.fxh"

state FoliageState
{
	CullMode = None;
	//AlphaToCoverageEnabled = true;
};

//------------------------------------------------------------------------------
/**
*/
shader
void
vsTree(in vec3 position,
	in vec3 normal,
	in vec2 uv,
	in vec3 tangent,
	in vec3 binormal,
	[slot=5] in vec4 color,
	out vec3 ViewSpacePos,
	out vec3 Tangent,
	out vec3 Normal,
	out vec3 Binormal,
	out vec2 UV,
	out vec3 WorldViewVec)	
{
	UV = uv;
	
	vec4 dir = InvModel * vec4(WindDirection.xyz, 0);
	vec4 windDir = WindForce * normalize(dir);
	float len = length(position);
	
	float windSpeed = WindSpeed * (TimeAndRandom.x + ObjectId);
	float windAmplitude = len / WindWaveSize;
	float windStrength = sin(windSpeed + windAmplitude);
	
	vec4 finalOffset = windDir * windStrength * color.r;
	vec4 finalPos = vec4(position + finalOffset.xyz, 1);
	vec4 modelSpace = Model * finalPos;
	gl_Position = ViewProjection * modelSpace;

	mat4 modelView = View * Model;
	ViewSpacePos = (modelView * finalPos).xyz;
    
	Tangent  = (modelView * vec4(tangent, 0)).xyz;
	Normal   = (modelView * vec4(normal, 0)).xyz;
	Binormal = (modelView * vec4(binormal, 0)).xyz;
	WorldViewVec = modelSpace.xyz - EyePos.xyz;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsTreeInstanced(in vec3 position,
	in vec3 normal,
	in vec2 uv,
	in vec3 tangent,
	in vec3 binormal,
	[slot=5] in vec4 color,
	out vec3 ViewSpacePos,
	out vec3 Tangent,
	out vec3 Normal,
	out vec3 Binormal,
	out vec2 UV,
	out vec3 WorldViewVec)	
{
	UV = uv;
	
	vec4 dir = InvModel * vec4(WindDirection.xyz, 0);
	vec4 windDir = WindForce * normalize(dir);
	
	float windSpeed = WindSpeed * (TimeAndRandom.x + gl_InstanceID);
	float windAmplitude = length(position) / WindWaveSize;
	float windStrength = sin(windSpeed + windAmplitude);
	
	vec4 finalOffset = windDir * windStrength * color.r;
	vec4 finalPos = vec4(position + finalOffset.xyz, 1);
	vec4 modelSpace = ModelArray[gl_InstanceID] * finalPos;
	gl_Position = ViewProjection * modelSpace;

	mat4 modelView = View * ModelArray[gl_InstanceID];
	ViewSpacePos = (modelView * finalPos).xyz;
	    
	Tangent  = (modelView * vec4(tangent, 0)).xyz;
	Normal   = (modelView * vec4(normal, 0)).xyz;
	Binormal = (modelView * vec4(binormal, 0)).xyz;
	WorldViewVec = modelSpace.xyz - EyePos.xyz;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsGrass(in vec3 position,
	in vec3 normal,
	in vec2 uv,
	in vec3 tangent,
	in vec3 binormal,
	[slot=5] in vec4 color,
	out vec3 ViewSpacePos,
	out vec3 Tangent,
	out vec3 Normal,
	out vec3 Binormal,
	out vec2 UV,
	out vec3 WorldViewVec)	
{
	UV = uv;
	
	vec4 dir = InvModel * vec4(WindDirection.xyz, 0);
	vec4 windDir = WindForce * normalize(dir);
	
	float windSpeed = WindSpeed * (TimeAndRandom.x + ObjectId);
	float windAmplitude = length(position) / WindWaveSize;
	float windStrength = sin(windSpeed + windAmplitude);
	
	vec4 finalOffset = windDir * windStrength * color.r;
	vec4 finalPos = vec4(position + finalOffset.xyz, 1);
	vec4 modelSpace = Model * finalPos;
	gl_Position = ViewProjection * modelSpace;
	
	mat4 modelView = View * Model;
	ViewSpacePos = (modelView * finalPos).xyz;
    
	Tangent  = (modelView * vec4(tangent, 0)).xyz;
	Normal   = (modelView * vec4(normal, 0)).xyz;
	Binormal = (modelView * vec4(binormal, 0)).xyz;
	WorldViewVec = modelSpace.xyz - EyePos.xyz;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsGrassInstanced(in vec3 position,
	in vec3 normal,
	in vec2 uv,
	in vec3 tangent,
	in vec3 binormal,
	[slot=5] in vec4 color,
	out vec3 ViewSpacePos,
	out vec3 Tangent,
	out vec3 Normal,
	out vec3 Binormal,
	out vec2 UV,
	out vec3 WorldViewVec)	
{
	UV = uv;
	
	vec4 dir = InvModel * vec4(WindDirection.xyz, 0);
	vec4 windDir = WindForce * normalize(dir);
	
	float windSpeed = WindSpeed * (TimeAndRandom.x + gl_InstanceID);
	float windAmplitude = length(position) / WindWaveSize;
	float windStrength = sin(windSpeed + windAmplitude);
	
	vec4 finalOffset = windDir * windStrength * color.r;
	vec4 finalPos = vec4(position + finalOffset.xyz, 1);
	vec4 modelSpace = ModelArray[gl_InstanceID] * finalPos;
	gl_Position = ViewProjection * modelSpace;
	
	mat4 modelView = View * ModelArray[gl_InstanceID];
	ViewSpacePos = (modelView * finalPos).xyz;
    
	Tangent  = (modelView * vec4(tangent, 0)).xyz;
	Normal   = (modelView * vec4(normal, 0)).xyz;
	Binormal = (modelView * vec4(binormal, 0)).xyz;
	WorldViewVec = modelSpace.xyz - EyePos.xyz;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsTreeShadow(in vec4 position,
	in vec3 normal,
	in vec2 uv,
	in vec3 tangent,
	in vec3 binormal,
	out vec2 UV,
	out vec4 ProjPos)
{
	vec4 dir = InvModel * vec4(WindDirection.xyz, 0);
	vec4 windDir = WindForce * normalize(dir);
	
	float windSpeed = WindSpeed * (TimeAndRandom.x + ObjectId);
	float windAmplitude = length(position) / WindWaveSize;
	float windStrength = sin(windSpeed + windAmplitude);
	
	vec4 finalOffset = windDir * windStrength;
	gl_Position = position + finalOffset;
	gl_Position = ViewMatrixArray[0] * Model * gl_Position;
	ProjPos = gl_Position;
	UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsGrassShadow(in vec4 position,
	in vec3 normal,
	in vec2 uv,
	in vec3 tangent,
	in vec3 binormal,
	[slot=5] in vec4 color,
	out vec2 UV,
	out vec4 ProjPos) 
{
	vec4 dir = InvModel * vec4(WindDirection.xyz, 0);
	vec4 windDir = WindForce * normalize(dir);
	
	float windSpeed = WindSpeed * (TimeAndRandom.x + ObjectId);
	float windAmplitude = length(position) / WindWaveSize;
	float windStrength = sin(windSpeed + windAmplitude);
	
	vec4 finalOffset = windDir * windStrength;
	gl_Position = position + finalOffset * color;
	gl_Position = ViewMatrixArray[0] * Model * gl_Position;
	ProjPos = gl_Position;
	UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsTreeShadowCSM(in vec4 position,
	in vec3 normal,
	in vec2 uv,
	in vec3 tangent,
	in vec3 binormal,
	out vec2 UV,
	out vec4 ProjPos,
	out int Instance) 
{
	vec4 dir = vec4(WindDirection.xyz, 0);
	vec4 windDir = WindForce * normalize(dir);
	
	float windSpeed = WindSpeed * (TimeAndRandom.x + ObjectId);
	float windAmplitude = length(position) / WindWaveSize;
	float windStrength = sin(windSpeed + windAmplitude);
		
	vec4 finalOffset = windDir * windStrength;
	finalOffset = position + finalOffset;
	ProjPos = ViewMatrixArray[gl_InstanceID] * Model * finalOffset;
	UV = uv;
	Instance = gl_InstanceID;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsGrassShadowCSM(in vec4 position,
	in vec3 normal,
	in vec2 uv,
	in vec3 tangent,
	in vec3 binormal,
	[slot=5] in vec4 color,
	out vec2 UV,
	out vec4 ProjPos,
	out int Instance) 
{
	vec4 dir = vec4(WindDirection.xyz, 0);
	vec4 windDir = WindForce * normalize(dir);
	
	float windSpeed = WindSpeed * (TimeAndRandom.x + ObjectId);
	float windAmplitude = length(position) / WindWaveSize;
	float windStrength = sin(windSpeed + windAmplitude);
	
	vec4 finalOffset = windDir * windStrength;
	finalOffset = position + finalOffset;
	ProjPos = ViewMatrixArray[gl_InstanceID] * Model * finalOffset;
	UV = uv;
	Instance = gl_InstanceID;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsTreeShadowPoint(in vec4 position,
	in vec3 normal,
	in vec2 uv,
	in vec3 tangent,
	in vec3 binormal,
	out vec2 UV,
	out vec4 ProjPos,
	out int Instance) 
{
	vec4 dir = vec4(WindDirection.xyz, 0);
	vec4 windDir = WindForce * normalize(dir);
	
	float windSpeed = WindSpeed * (TimeAndRandom.x + ObjectId);
	float windAmplitude = length(position) / WindWaveSize;
	float windStrength = sin(windSpeed + windAmplitude);
		
	vec4 finalOffset = windDir * windStrength;
	finalOffset = position + finalOffset;
	ProjPos = ViewMatrixArray[gl_InstanceID] * Model * finalOffset;
	UV = uv;
	Instance = gl_InstanceID;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsGrassShadowPoint(in vec4 position,
	in vec3 normal,
	in vec2 uv,
	in vec3 tangent,
	in vec3 binormal,
	[slot=5] in vec4 color,
	out vec2 UV,
	out vec4 ProjPos,
	out int Instance) 
{
	vec4 dir = vec4(WindDirection.xyz, 0);
	vec4 windDir = WindForce * normalize(dir);
	
	float windSpeed = WindSpeed * (TimeAndRandom.x + ObjectId);
	float windAmplitude = length(position) / WindWaveSize;
	float windStrength = sin(windSpeed + windAmplitude);
	
	vec4 finalOffset = windDir * windStrength;
	finalOffset = position + finalOffset;
	ProjPos = ViewMatrixArray[gl_InstanceID] * Model * finalOffset;
	UV = uv;
	Instance = gl_InstanceID;
}


//------------------------------------------------------------------------------
/**
	Used for lightmapped 
*/
shader
void
vsTreeLightmapped(in vec3 position,
	in vec3 normal,
	in vec2 uv1,
	in vec3 tangent,
	in vec3 binormal,
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
psPicking(in vec2 UV,
		in vec4 ProjPos,		
		[color0] out float Id) 
{
	Id = float(ObjectId);
}

//------------------------------------------------------------------------------
//	Coloring methods
//------------------------------------------------------------------------------
SimpleTechnique(
	Tree, 
	"Static", 
	vsTree(), 
	psUberAlphaTest(
		calcColor = SimpleColor,
		calcBump = NormalMapFunctor,
		calcSpec = NonReflectiveSpecularFunctor,
		calcDepth = ViewSpaceDepthFunctor,
		calcEnv = IrradianceOnly
	),
	FoliageState);
	
SimpleTechnique(
	TreeInstanced, 
	"Static|Instanced", 
	vsTreeInstanced(), 
	psUberAlphaTest(
		calcColor = SimpleColor,
		calcBump = NormalMapFunctor,
		calcSpec = NonReflectiveSpecularFunctor,
		calcDepth = ViewSpaceDepthFunctor,
		calcEnv = IrradianceOnly
	),
	FoliageState);
	
SimpleTechnique(
	Grass, 
	"Static|Colored", 
	vsGrass(), 
	psUberAlphaTest(
		calcColor = SimpleColor,
		calcBump = NormalMapFunctor,
		calcSpec = NonReflectiveSpecularFunctor,
		calcDepth = ViewSpaceDepthFunctor,
		calcEnv = IrradianceOnly
	),
	FoliageState);
	
SimpleTechnique(
	GrassInstanced, 
	"Static|Colored|Instanced", 
	vsGrassInstanced(), 
	psUberAlphaTest(
		calcColor = SimpleColor,
		calcBump = NormalMapFunctor,
		calcSpec = NonReflectiveSpecularFunctor,
		calcDepth = ViewSpaceDepthFunctor,
		calcEnv = IrradianceOnly
	),
	FoliageState);

//------------------------------------------------------------------------------
//	Shadowing methods
//------------------------------------------------------------------------------
SPOTLIGHT_SHADOW_ALPHATEST(vsTreeShadow(), FoliageState);
GLOBALLIGHT_SHADOW_ALPHATEST(vsTreeShadowCSM(), FoliageState);
POINTLIGHT_SHADOW_ALPHATEST(vsTreeShadowCSM(), FoliageState);
SimpleTechnique(Picking, "Static|Picking", vsTreeShadow(), psPicking(), FoliageState);

//------------------------------------------------------------------------------
//	Lightmapped methods
//------------------------------------------------------------------------------
SimpleTechnique(LitFoliage, "Static|Lightmapped", vsTreeLightmapped(), psLightmappedLit(), FoliageState);
SimpleTechnique(UnlitFoliage, "Static|Unlit|Lightmapped", vsTreeLightmapped(), psLightmappedUnlit(), FoliageState);
