//------------------------------------------------------------------------------
//  glsltemplate.fxh
//
//	Base template for AnyFX based shaders
//
//  (C) 2014 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "lib/shared.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"
#include "lib/defaultsamplers.fxh"

#if ALPHA_TEST
float AlphaSensitivity = 0.0f;
#endif

#ifndef USE_CUSTOM_WORLD_OFFSET
vec4 GetWorldOffset(in vec3 position)
{
	return vec4(position.xyz, 1);
}
#endif

#ifndef USE_CUSTOM_DIFFUSE
vec4 GetDiffuse(const PixelShaderParameters params)
{
	return vec3(0, 0, 0);
}
#endif

#ifndef USE_CUSTOM_NORMAL
vec3 GetNormal(const PixelShaderParameters params)
{
	vec2 normalSample 		= texture(NormalMap, params.uv).ag;
	vec3 tNormal 			= vec3(0);
	tNormal.xy 				= (normalSample * 2.0f) - 1.0f;
	tNormal.z 				= saturate(sqrt(1.0f - dot(tNormal.xy, tNormal.xy)));
	return tNormal;
}
#endif

#ifndef USE_CUSTOM_SPECULAR
vec4 GetSpecular(const PixelShaderParameters params)
{
	return vec4(0);
}
#endif

#ifndef USE_CUSTOM_EMISSIVE
vec4 GetEmissive(const PixelShaderParameters params)
{
	return vec4(0);
}
#endif

#ifndef USE_CUSTOM_ROUGHNESS
float GetRoughness(const PixelShaderParameters params)
{
	return 0.f;
}
#endif

//------------------------------------------------------------------------------
/**
*/
shader
void
vertexShader(
	in vec3 vertexshader_input_position,
	in vec3 vertexshader_input_normal,
	in vec2 vertexshader_input_uv,
	in vec3 vertexshader_input_tangent,
	in vec3 vertexshader_input_binormal,
	#if USE_VERTEX_COLOR
	[slot=5] in vec4 vertexshader_input_color,
	#endif
	#if USE_SECONDARY_UV
	[slot=6] in vec2 vertexshader_input_uv2,
	#endif
	#if USE_SKINNING
	[slot=7] in vec4 vertexshader_input_weights,
	[slot=8] in uvec4 vertexshader_input_indices,
	#endif
	out PixelShaderParameters pixel) 
{
	// write uv
	pixel.uv = vertexshader_input_uv;
	
	// write vertex colors if present
#if USE_VERTEX_COLOR
	pixel.color = vertexshader_input_color;
#endif
	
	// write secondary uv
#if USE_SECONDARY_UV
	pixel.uv2 = vertexshader_input_uv2;
#endif
	
    mat4 modelView = View * Model;
	
#if USE_SKINNING
	vec4 offsetted			= GetWorldOffset(vertexshader_input_position);
	offsetted				= SkinnedPosition(offsetted, vertexshader_input_weights, vertexshader_input_indices);
	vec4 skinnedNormal		= SkinnedNormal(vertexshader_input_normal, vertexshader_input_weights, vertexshader_input_indices);
	vec4 skinnedTangent		= SkinnedNormal(vertexshader_input_tangent, vertexshader_input_weights, vertexshader_input_indices);
	vec4 skinnedBinormal	= SkinnedNormal(vertexshader_input_binormal, vertexshader_input_weights, vertexshader_input_indices);
	
	// write outputs
	pixel.viewSpacePos 		= (modelView * vec4(offsetted)).xyz;
	pixel.normal 			= (modelView * vec4(skinnedNormal)).xyz;
	pixel.tangent 			= (modelView * vec4(skinnedTangent)).xyz;
	pixel.binormal 			= (modelView * vec4(skinnedBinormal)).xyz;
	
// no skin
#else
	vec4 offsetted 			= GetWorldOffset(vertexshader_input_position);
	
    pixel.viewSpacePos 		= (modelView * vec4(offsetted)).xyz;
	pixel.normal 			= (modelView * vec4(vertexshader_input_normal, 0)).xyz;
	pixel.tangent 			= (modelView * vec4(vertexshader_input_tangent, 0)).xyz;	
	pixel.binormal 			= (modelView * vec4(vertexshader_input_binormal, 0)).xyz;
#endif
	offsetted 				= Model * offsetted;
	pixel.worldViewVec		= offsetted.xyz - EyePos.xyz;
	gl_Position 			= ViewProjection * offsetted;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
pixelShader(PixelShaderParameters params,
			#ifdef UNLIT_SHADING
			[color0] out vec4 Albedo
			#else
			[color0] out vec4 Albedo,
			[color1] out vec4 Normals,
			[color2] out float Depth,	
			[color3] out vec4 Specular,
			[color4] out vec4 Emissive
			#endif
			)
{
// unlit material
#if UNLIT_SHADING
	vec4 diffuse 			= GetDiffuse(params);
	
	#if ALPHA_TEST
		if (diffuse.a < AlphaSensitivity) discard;
	#endif
	
	Albedo 					= EncodeHDR(diffuse);
#else
	// default to lit material
	vec4 diffuse 			= GetDiffuse(params);
	
	#if ALPHA_TEST
		if (diffuse.a < AlphaSensitivity) discard;
	#endif
			
	// normal map calculation
	mat3 tangentViewMatrix 	= mat3(normalize(params.tangent), normalize(params.binormal), normalize(params.normal));        
	vec3 normal 			= normalize(tangentViewMatrix * GetNormal(params));
	
		
	// specular
	vec4 specular 			= GetSpecular(params);
	float roughness 		= GetRoughness(params);
	
	// get emissive
	vec4 emissive 			= GetEmissive(params);
	
	// write outputs
	#if USE_PBR_REFLECTIONS
	mat2x3 env 				= PBR(specular, normal, params.viewSpacePos, params.worldViewVec, InvView, roughness);
	Emissive 				= vec4(diffuse.rgb * env[0] + env[1], 0) + emissive;
	#else
	Emissive 				= emissive;
	#endif
	
	// save outputs which isn't associated with the PBR method
	Specular 				= vec4(specular.rgb, roughness);
	Depth 					= length(params.viewSpacePos);	
	Normals 				= PackViewSpaceNormal(normal);
	
	#if USE_PBR_REFLECTIONS
	Albedo 					= vec4(diffuse.rgb * (1 - spec.rgb), diffuse.a);
	#else
	Albedo 					= diffuse;
	#endif
	
#endif
}

// define a standard state
// ideally, we want to generate the draw state from some settings, but this will work as a placeholder
#ifndef USE_CUSTOM_STATE
state StandardState
{
};
#endif

#if DEFAULT_PROGRAM
SimpleTechnique(Main, "Generated", vertexShader(), pixelShader(), StandardState);
#elif GEOMETRY_PROGRAM
GeometryTechnique(Main, "Generated", vertexShader(), pixelShader(), geometryShader(), StandardState);
#elif TESSELLATION_PROGRAM
TessellationTechnique(Main, "Generated", vertexShader(), pixelShader(), hullShader(), domainShader(), StandardState);
#elif GEOMETRY_TESSELLATION_PROGRAM
FullTechnique(Main, "Generated", vertexShader(), pixelShader(), hullShader(), domainShader(), geometryShader(), StandardState);
#endif