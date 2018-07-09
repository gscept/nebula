//------------------------------------------------------------------------------
//  glslheader.fxh
//
//	Base header for AnyFX based shaders
//
//  (C) 2014 Gustav Sterbrant
//------------------------------------------------------------------------------

#define float2 vec2
#define float3 vec3
#define float4 vec4
#define int2 ivec2
#define int3 ivec3
#define int4 ivec4
#define matrix3x2 mat3x2
#define matrix3x3 mat3x3
#define matrix3x4 mat3x4
#define matrix4x2 mat4x2
#define matrix4x3 mat4x3
#define matrix4x4 mat4x4

struct PixelShaderParameters
{
	vec3 viewSpacePos;
	vec3 tangent;
	vec3 normal;
	vec3 binormal;
	vec2 uv;
	vec3 worldViewVec;
	
#if USE_SECONDARY_UV
	vec2 uv2;
#endif
	
#if USE_VERTEX_COLOR
	vec4 color;
#endif
};

#if USE_PBR_REFLECTIONS
mat2x3
PBR(
	in vec4 specularColor, 
	in vec3 viewSpaceNormal, 
	in vec3 viewSpacePos, 
	in vec3 worldViewVec,
	in mat4 invView,
	in float roughness)
{
	mat2x3 ret;
	vec4 worldNorm = (invView * vec4(viewSpaceNormal, 0));
	vec3 reflectVec = reflect(worldViewVec, worldNorm.xyz);
	float x = dot(-viewSpaceNormal, normalize(viewSpacePos));
	vec3 rim = FresnelSchlickGloss(specularColor.rgb, x, roughness);
	ret[1] = textureLod(EnvironmentMap, reflectVec, (1.0f - roughness) * NumEnvMips).rgb * rim;
	ret[0] = textureLod(IrradianceMap, worldNorm.xyz, 0).rgb;
	return ret;
}
#endif

