//------------------------------------------------------------------------------
//  animated.fx
//
//	Provides a vertex shader which performs UV-animation over time
//
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "lib/std.fxh"
#include "lib/techniques.fxh"
#include "lib/geometrybase.fxh"
#include "lib/skinning.fxh"

#include "lib/animationparams.fxh"
//------------------------------------------------------------------------------
/**
*/
shader
void
vsAnimated(
	[slot=0] in vec3 position,
	[slot=1] in vec3 normal,
	[slot=2] in vec2 uv,
	[slot=3] in vec3 tangent,
	[slot=4] in vec3 binormal,
	out vec3 ViewSpacePos,
	out vec3 Tangent,
	out vec3 Normal,
	out vec3 Binormal,
	out vec2 UV,
	out vec3 WorldViewVec) 
{
	vec4 modelSpace = Model * vec4(position, 1);
    gl_Position = ViewProjection * modelSpace;

	// create rotation matrix
	float angle = AnimationAngle * TimeAndRandom.x * AnimationAngularSpeed;
	float rotSin, rotCos;
	rotSin = sin(angle);
	rotCos = cos(angle);
	mat2 rotationMat = mat2(rotCos, -rotSin, rotSin, rotCos);
							 
	// compute 2d extruded corner position
	vec2 uvCenter = ((uv * 2.0f) - 1.0f) / 2.0f;
	vec2 uvOffset = AnimationDirection * TimeAndRandom.x * AnimationLinearSpeed;
	UV = (rotationMat * uvCenter + vec2(0.5f, 0.5f)) * vec2(NumXTiles, NumYTiles) + uvOffset;
	
	mat4 modelView = View * Model;	
    ViewSpacePos = (modelView * vec4(position, 1)).xyz;
	Tangent = (modelView * vec4(tangent, 0)).xyz;
	Normal = (modelView * vec4(normal, 0)).xyz;
	Binormal = (modelView * vec4(binormal, 0)).xyz;
	WorldViewVec = modelSpace.xyz - EyePos.xyz;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsAnimatedColored(
	[slot=0] in vec3 position,
	[slot=1] in vec3 normal,
	[slot=2] in vec2 uv,
	[slot=3] in vec3 tangent,
	[slot=4] in vec3 binormal,
	[slot=5] in vec4 color,
	out vec3 ViewSpacePos,
	out vec3 Tangent,
	out vec3 Normal,
	out vec3 Binormal,
	out vec2 UV,
	out vec4 Color,
	out vec3 WorldViewVec) 
{
	vec4 modelSpace = Model * vec4(position, 1);
    gl_Position = ViewProjection * modelSpace;
	
	// create rotation matrix
	float angle = AnimationAngle * TimeAndRandom.x * AnimationAngularSpeed;
	float rotSin, rotCos;
	rotSin = sin(angle);
	rotCos = cos(angle);
	mat2 rotationMat = mat2(rotCos, -rotSin, rotSin, rotCos);
							 
	// compute 2d extruded corner position
	vec2 uvCenter = ((uv * 2.0f) - 1.0f) / 2.0f;
	vec2 uvOffset = AnimationDirection * TimeAndRandom.x * AnimationLinearSpeed;
	UV = (rotationMat * uvCenter + vec2(0.5f, 0.5f)) * vec2(NumXTiles, NumYTiles) + uvOffset;
	
	mat4 modelView = View * Model;	
	Color = color;
    ViewSpacePos = (modelView * vec4(position, 1)).xyz;
	Tangent = (modelView * vec4(tangent, 0)).xyz;
	Normal = (modelView * vec4(normal, 0)).xyz;
	Binormal = (modelView * vec4(binormal, 0)).xyz;
	WorldViewVec = modelSpace.xyz - EyePos.xyz;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsAnimatedSkinned(
	[slot=0] in vec3 position,
	[slot=1] in vec3 normal,
	[slot=2] in vec2 uv,
	[slot=3] in vec3 tangent,
	[slot=4] in vec3 binormal,
	[slot=7] in vec4 weights,
	[slot=8] in uvec4 indices,
	out vec3 ViewSpacePos,
	out vec3 Tangent,
	out vec3 Normal,
	out vec3 Binormal,
	out vec2 UV,
	out vec3 WorldViewVec) 
{
	vec4 skinnedPos      = SkinnedPosition(position, weights, indices);
	vec4 skinnedNormal   = SkinnedNormal(normal, weights, indices);
	vec4 skinnedTangent  = SkinnedNormal(tangent, weights, indices);
	vec4 skinnedBinormal = SkinnedNormal(binormal, weights, indices);
	
	vec4 modelSpace = Model * skinnedPos;
    gl_Position = ViewProjection * modelSpace;
	
	// create rotation matrix
	float angle = AnimationAngle * TimeAndRandom.x * AnimationAngularSpeed;
	float rotSin, rotCos;
	rotSin = sin(angle);
	rotCos = cos(angle);
	mat2 rotationMat = mat2(rotCos, -rotSin, rotSin, rotCos);
							 
	// compute 2d extruded corner position
	vec2 uvCenter = ((uv * 2.0f) - 1.0f) / 2.0f;
	vec2 uvOffset = AnimationDirection * TimeAndRandom.x * AnimationLinearSpeed;
	UV = (rotationMat * uvCenter + vec2(0.5f, 0.5f)) * vec2(NumXTiles, NumYTiles) + uvOffset;
	
	mat4 modelView = View * Model;	
    ViewSpacePos = (modelView * skinnedPos).xyz;
	Tangent = (modelView * skinnedTangent).xyz;
	Normal = (modelView * skinnedNormal).xyz;
	Binormal = (modelView * skinnedBinormal).xyz;
	WorldViewVec = modelSpace.xyz - EyePos.xyz;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsAnimatedSkinnedColored(
	[slot=0] in vec3 position,
	[slot=1] in vec3 normal,
	[slot=2] in vec2 uv,
	[slot=3] in vec3 tangent,
	[slot=4] in vec3 binormal,
	[slot=5] in vec4 color,
	[slot=7] in vec4 weights,
	[slot=8] in uvec4 indices,
	out vec3 ViewSpacePos,
	out vec3 Tangent,
	out vec3 Normal,
	out vec3 Binormal,
	out vec2 UV,
	out vec4 Color,
	out vec3 WorldViewVec) 
{
	vec4 skinnedPos      = SkinnedPosition(position, weights, indices);
	vec4 skinnedNormal   = SkinnedNormal(normal, weights, indices);
	vec4 skinnedTangent  = SkinnedNormal(tangent, weights, indices);
	vec4 skinnedBinormal = SkinnedNormal(binormal, weights, indices);
	
	vec4 modelSpace = Model * skinnedPos;
    gl_Position = ViewProjection * modelSpace;
	
	// create rotation matrix
	float angle = AnimationAngle * TimeAndRandom.x * AnimationAngularSpeed;
	float rotSin, rotCos;
	rotSin = sin(angle);
	rotCos = cos(angle);
	mat2 rotationMat = mat2(rotCos, -rotSin, rotSin, rotCos);
							 
	// compute 2d extruded corner position
	vec2 uvCenter = ((uv * 2.0f) - 1.0f) / 2.0f;
	vec2 uvOffset = AnimationDirection * TimeAndRandom.x * AnimationLinearSpeed;
	UV = (rotationMat * uvCenter + vec2(0.5f, 0.5f)) * vec2(NumXTiles, NumYTiles) + uvOffset;
	
	mat4 modelView = View * Model;	
	Color = color;
	ViewSpacePos = (modelView * skinnedPos).xyz;
	Tangent = (modelView * skinnedTangent).xyz;
	Normal = (modelView * skinnedNormal).xyz;
	Binormal = (modelView * skinnedBinormal).xyz;
	WorldViewVec = modelSpace.xyz - EyePos.xyz;
}

//------------------------------------------------------------------------------
/**
	Static versions
*/
SimpleTechnique(Static, "Static", vsAnimated(), psUber(
		calcColor = SimpleColor,
		calcBump = NormalMapFunctor,
		calcMaterial = DefaultMaterialFunctor,
		calcDepth = ViewSpaceDepthFunctor,
		calcEnv = PBR
	), StandardState);

SimpleTechnique(StaticColored, "Static|Colored", vsAnimatedColored(), psUberVertexColor(
	calcColor = SimpleColor,
	calcBump = NormalMapFunctor,
	calcMaterial = DefaultMaterialFunctor,
	calcDepth = ViewSpaceDepthFunctor,
	calcEnv = PBR
	), StandardState);
	
SimpleTechnique(Alpha, "Alpha", vsAnimated(), psUber(
		calcColor = AlphaColor,
		calcBump = NormalMapFunctor,
		calcMaterial = DefaultMaterialFunctor,
		calcDepth = ViewSpaceDepthFunctor,
		calcEnv = PBR
	), AlphaState);

SimpleTechnique(AlphaColored, "Alpha|Colored", vsAnimatedColored(), psUberVertexColor(
	calcColor = AlphaColor,
	calcBump = NormalMapFunctor,
	calcMaterial = DefaultMaterialFunctor,
	calcDepth = ViewSpaceDepthFunctor,
	calcEnv = PBR
	), AlphaState);
	
//------------------------------------------------------------------------------
/**
	Skinned versions
*/
SimpleTechnique(Skinned, "Skinned", vsAnimatedSkinned(), psUber(
		calcColor = SimpleColor,
		calcBump = NormalMapFunctor,
		calcMaterial = DefaultMaterialFunctor,
		calcDepth = ViewSpaceDepthFunctor,
		calcEnv = PBR
	), StandardState);

SimpleTechnique(SkinnedColored, "Skinned|Colored", vsAnimatedSkinnedColored(), psUberVertexColor(
	calcColor = SimpleColor,
	calcBump = NormalMapFunctor,
	calcMaterial = DefaultMaterialFunctor,
	calcDepth = ViewSpaceDepthFunctor,
	calcEnv = PBR
	), StandardState);
	
SimpleTechnique(SkinnedAlpha, "Skinned|Alpha", vsAnimatedSkinned(), psUber(
		calcColor = AlphaColor,
		calcBump = NormalMapFunctor,
		calcMaterial = DefaultMaterialFunctor,
		calcDepth = ViewSpaceDepthFunctor,
		calcEnv = PBR
	), AlphaState);

SimpleTechnique(SkinnedAlphaColored, "Skinned|Alpha|Colored", vsAnimatedSkinnedColored(), psUberVertexColor(
	calcColor = AlphaColor,
	calcBump = NormalMapFunctor,
	calcMaterial = DefaultMaterialFunctor,
	calcDepth = ViewSpaceDepthFunctor,
	calcEnv = PBR
	), AlphaState);