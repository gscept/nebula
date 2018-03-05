//------------------------------------------------------------------------------
//  shared.fxh
//  (C) 2011 LTU Skellefteå
//------------------------------------------------------------------------------

#ifndef SHARED_FXH
#define SHARED_FXH

#include "lib/std.fxh"
#include "lib/util.fxh"

// define how many objects we can render simultaneously 
#define MAX_BATCH_SIZE 256
#define INFINITE_DEPTH -1000

// instancing transforms
shared varblock InstanceBlock [bool System = true; bool Instancing = true;]
{
	mat4 ModelArray[MAX_BATCH_SIZE];
	mat4 ModelViewArray[MAX_BATCH_SIZE];
	mat4 ModelViewProjectionArray[MAX_BATCH_SIZE];
	uint IdArray[MAX_BATCH_SIZE];
};

// contains the state of the camera (and time)
group(1) shared varblock CameraBlock [bool System = true;]
{
	mat4 View;
	mat4 InvView;
	mat4 ViewProjection;
	mat4 Projection;
	mat4 InvProjection;
	mat4 InvViewProjection;
	vec4 EyePos;	
	vec4 FocalLength;
	vec4 TimeAndRandom;
};

// constains the state of a global light
shared varblock GlobalLightBlock [bool System = true;]
{
	vec4 GlobalLightDirWorldspace;
	vec4 GlobalLightDir;
	vec4 GlobalLightColor;
	vec4 GlobalBackLightColor;
	vec4 GlobalAmbientLightColor;
	float GlobalBackLightOffset;
	mat4 CSMShadowMatrix;
};

// contains the environmental state
shared varblock EnvironmentParamBlock [bool System = true;]
{
	vec4 GlobalWindDirection;
	
	// these params are for the Preetham sky model
	vec4 A;
	vec4 B;
	vec4 C;
	vec4 D;
	vec4 E;
	vec4 Z;
};

// contains the state of either a point light shadow caster (6 view matrices) or the 4 CSM projection matrices
shared varblock ShadowCameraBlock [bool System = true;]
{
	mat4 ViewMatrixArray[6];
};

#define FLT_MAX     3.40282347E+38F
#define FLT_MIN     -3.40282347E+38F

#define MAX_NUM_LIGHTS 16

// contains the state of all the lights used for forward shading
shared varblock LightBlock [bool System = true;]
{
	int			NumActiveLights = 0;
	vec4		LightPositionsArray[MAX_NUM_LIGHTS];
	mat4		LightProjTransformArray[MAX_NUM_LIGHTS];
	vec4		LightColorArray[MAX_NUM_LIGHTS];
	vec4		LightProjMapOffsetArray[MAX_NUM_LIGHTS];
	vec4		LightShadowMapOffsetArray[MAX_NUM_LIGHTS];
	vec4		LightShadowSizeArray[MAX_NUM_LIGHTS];
	float		LightInvRangeArray[MAX_NUM_LIGHTS];
	int			LightTypeArray[MAX_NUM_LIGHTS];
	bool		LightCastsShadowsArray[MAX_NUM_LIGHTS];
};

// shadow map for spotlights is a single atlas, point lights have separate cubes
sampler2D	SpotlightShadowAtlas;
samplerCubeArray PointLightCubeArray;

// contains variables which are guaranteed to be unique per object.
shared varblock ObjectBlock [bool System = true;]
{
	mat4 Model;
	mat4 InvModel;
	mat4 ModelViewProjection;
	mat4 ModelView;
	int ObjectId;
};

float FresnelPower = 0.0f;
float FresnelStrength = 0.0f;

// material properties
float AlphaSensitivity = 1.0f;
float AlphaBlendFactor = 0.0f;
vec4 MatAlbedoIntensity = vec4(1.0f, 1.0f, 1.0f, 1.0f);
vec4 MatSpecularIntensity = vec4(1.0f, 1.0f, 1.0f, 1.0f);	
vec4 MatEmissiveIntensity = vec4(1.0f, 1.0f, 1.0f, 1.0f);	
vec4 MatFresnelIntensity = vec4(1.0f, 1.0f, 1.0f, 1.0f);
float MatRoughnessIntensity = 0.0f;

float TessellationFactor = 1.0f;
float MaxDistance = 250.0f;
float MinDistance = 20.0f;
float HeightScale = 0.0f;
float SceneScale = 1.0f;

vec2 AnimationDirection;
float AnimationAngle;
float AnimationLinearSpeed;
float AnimationAngularSpeed;
int NumXTiles = 1;
int NumYTiles = 1;

vec4 WindDirection = vec4(0.0f,0.0f,0.0f,1.0f);
float WindWaveSize = 1.0f;
float WindSpeed = 0.0f;
float WindIntensity = 0.0f;
float WindForce = 0.0f;

// The number of CSM cascades 
#ifndef CASCADE_COUNT_FLAG
#define CASCADE_COUNT_FLAG 4
#endif

#endif // SHARED_H
