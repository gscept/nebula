//------------------------------------------------------------------------------
//  gltf.fx
//  (C) 2020 Individual contributors, See LICENSE file
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/geometrybase.fxh"
#include "lib/techniques.fxh"
#include "lib/clustering.fxh"
#include "lib/lights_clustered.fxh"

render_state DoubleSidedState
{
	CullMode = None;
};

render_state AlphaDoubleSidedState
{
	BlendEnabled[0] = true;
	SrcBlend[0] = SrcAlpha;
	DstBlend[0] = OneMinusSrcAlpha;
	DepthWrite = false;
	DepthEnabled = true;
	CullMode = None;
};

group(BATCH_GROUP) shared constant GLTFBlock
{
	// lower camel case names by design, just to keep it consistent with the GLTF standard.
	textureHandle baseColorTexture;
	textureHandle normalTexture;
	textureHandle metallicRoughnessTexture;
	textureHandle emissiveTexture;
	textureHandle occlusionTexture;
	vec4 baseColorFactor;
	vec4 emissiveFactor;
	float metallicFactor;
	float roughnessFactor;
	float normalScale;
	float alphaCutoff;
};

float Greyscale(in vec4 color)
{
	return dot(color.rgb, vec3(0.299, 0.587, 0.114));
}

subroutine (CalculateBump) vec3 GLTFNormalMapFunctor(
	in vec3 tangent,
	in vec3 binormal,
	in vec3 normal,
	in vec4 bumpData)
{
	mat3 tangentViewMatrix = mat3(normalize(tangent.xyz), normalize(binormal.xyz), normalize(normal.xyz));
	vec3 tNormal = vec3(0,0,0);
	tNormal.xy = (bumpData.xy * 2.0f) - 1.0f;
	tNormal.z = saturate(sqrt(1.0f - dot(tNormal.xy, tNormal.xy)));
	return tangentViewMatrix * normalize((tNormal * vec3(normalScale, normalScale, 1.0f)));
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsGLTFStatic(
	[slot=0] in vec3 position,
	[slot=1] in vec3 normal,
	[slot=2] in vec2 uv,
	[slot=3] in vec3 tangent,
	[slot=4] in vec3 binormal,
	out vec3 Tangent,
	out vec3 Normal,
	out vec3 Binormal,
	out vec2 UV,
	out vec3 WorldViewVec,
	out vec3 WorldPos,
	out vec4 ViewPos)
{
	vec4 modelSpace = Model * vec4(position, 1);
    gl_Position = ViewProjection * modelSpace;
    UV = uv;

	Tangent 	= (Model * vec4(tangent, 0)).xyz;
	Normal 		= (Model * vec4(normal, 0)).xyz;
	Binormal 	= (Model * vec4(binormal, 0)).xyz;
	WorldViewVec = EyePos.xyz - modelSpace.xyz;
	WorldPos = modelSpace.xyz;
	ViewPos = View * modelSpace;
}

//------------------------------------------------------------------------------
/**
*/
[earlydepth]
shader
void
psGLTF(
	in vec3 Tangent,
	in vec3 Normal,
	in vec3 Binormal,
	in vec2 UV,
	in vec3 WorldViewVec,
	[color0] out vec4 Albedo,
	[color1] out vec3 Normals,
	[color2] out vec4 Material,
	[color3] out vec4 Emissive)
{
	vec4 baseColor = sample2D(baseColorTexture, MaterialSampler, UV) * vec4(baseColorFactor.rgb, 1);
	vec4 material = sample2D(metallicRoughnessTexture, MaterialSampler, UV) * vec4(1.0f, roughnessFactor, metallicFactor, 1.0f);
	vec4 emissive = sample2D(emissiveTexture, MaterialSampler, UV) * emissiveFactor;
	vec4 occlusion = sample2D(occlusionTexture, MaterialSampler, UV);
	
	vec4 normals = sample2D(normalTexture, NormalSampler, UV);
	vec3 bumpNormal = normalize(calcBump(Tangent, Binormal, Normal, normals));

	Albedo = baseColor;
	Normals = bumpNormal;
	Material[MAT_METALLIC] = material.b;
	Material[MAT_ROUGHNESS] = material.g;
	Material[MAT_CAVITY] = Greyscale(occlusion);
	Material[MAT_EMISSIVE] = 0;
	Emissive = emissive;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psGLTFAlphaMask(
	in vec3 Tangent,
	in vec3 Normal,
	in vec3 Binormal,
	in vec2 UV,
	in vec3 WorldViewVec,
	[color0] out vec4 Albedo,
	[color1] out vec3 Normals,
	[color2] out vec4 Material)
{
	vec4 baseColor = sample2D(baseColorTexture, MaterialSampler, UV) * vec4(baseColorFactor.rgb, 1);

	if(baseColor.a <= alphaCutoff)
		discard;

	vec4 material = sample2D(metallicRoughnessTexture, MaterialSampler, UV) * vec4(1.0f, roughnessFactor, metallicFactor, 1.0f);
	vec4 emissive = sample2D(emissiveTexture, MaterialSampler, UV) * emissiveFactor;
	vec4 occlusion = sample2D(occlusionTexture, MaterialSampler, UV);
	
	vec4 normals = sample2D(normalTexture, NormalSampler, UV);
	vec3 bumpNormal = normalize(calcBump(Tangent, Binormal, Normal, normals));

	Albedo = baseColor;
	Normals = bumpNormal;
	Material[MAT_METALLIC] = material.b;
	Material[MAT_ROUGHNESS] = material.g;
	Material[MAT_CAVITY] = Greyscale(occlusion);
	Material[MAT_EMISSIVE] = Greyscale(emissive);
}

//------------------------------------------------------------------------------
/**
*/
vec3 CalculateLighting(
	vec4 viewPos,
	vec4 worldPos,
	vec3 worldViewVec,
	vec3 viewVec,
	vec3 viewNormal,
	vec3 normal,
	vec4 albedo,
	vec4 material,
	vec4 emissive,
	vec3 fragCoords
)
{
	vec3 light = vec3(0,0,0);
	uint3 index3D = CalculateClusterIndex(fragCoords.xy / BlockSize, viewPos.z, InvZScale, InvZBias); 
	uint idx = Pack3DTo1D(index3D, NumCells.x, NumCells.y);

	// render global light
	light += CalculateGlobalLight(viewPos, worldViewVec, normal.xyz, fragCoords.z, material, albedo);

	// render local lights
	// TODO: new material model for local lights
	light += LocalLights(idx, viewPos, viewVec, viewNormal, fragCoords.z, material, albedo);

	// reflections and irradiance
	vec3 F0 = vec3(0.04);
	CalculateF0(albedo.rgb, material[MAT_METALLIC], F0);
	vec3 reflectVec = reflect(-worldViewVec, normal.xyz);
	float cosTheta = dot(normal.xyz, worldViewVec);
	vec3 F = FresnelSchlickGloss(F0, cosTheta, material[MAT_ROUGHNESS]);
	vec3 reflection = sampleCubeLod(EnvironmentMap, CubeSampler, reflectVec, material[MAT_ROUGHNESS] * NumEnvMips).rgb * GlobalLightColor.xyz;
	vec3 irradiance = sampleCubeLod(IrradianceMap, CubeSampler, normal.xyz, 0).rgb * GlobalLightColor.xyz;
	float cavity = material[MAT_CAVITY];
	
	vec3 kD = vec3(1.0f) - F;
	kD *= 1.0f - material[MAT_METALLIC];
	
	const vec3 ambientTerm = (irradiance * kD * albedo.rgb);
	light += (ambientTerm + reflection * F) * cavity; 
	light += emissive.rgb;
	return light;
}

//------------------------------------------------------------------------------
/**
	Forward shaded alpha
*/
shader
void
psGLTFAlpha(
	in vec3 Tangent,
	in vec3 Normal,
	in vec3 Binormal,
	in vec2 UV,
	in vec3 WorldViewVec,
	in vec3 WorldPos,
	in vec4 ViewPos,
	[color0] out vec4 Color)
{
	vec4 baseColor = sample2D(baseColorTexture, MaterialSampler, UV) * vec4(baseColorFactor.rgb, 1);

	if(baseColor.a <= alphaCutoff)
		discard;

	vec4 metallicRoughness = sample2D(metallicRoughnessTexture, MaterialSampler, UV) * vec4(1.0f, roughnessFactor, metallicFactor, 1.0f);
	vec4 emissive = sample2D(emissiveTexture, MaterialSampler, UV) * emissiveFactor;
	vec4 occlusion = sample2D(occlusionTexture, MaterialSampler, UV);
	
	vec4 normals = sample2D(normalTexture, NormalSampler, UV);
	vec3 N = normalize(calcBump(Tangent, Binormal, Normal, normals));

	vec4 material;
	material[MAT_METALLIC] = metallicRoughness.b;
	material[MAT_ROUGHNESS] = metallicRoughness.g;
	material[MAT_CAVITY] = Greyscale(occlusion);
	material[MAT_EMISSIVE] = 1.0f;
	
	vec3 light = CalculateLighting(
		ViewPos,
		vec4(WorldPos, 1.0f),
		WorldViewVec,
		-normalize(ViewPos.xyz),
		(View * vec4(N.xyz, 0)).xyz,
		N,
		baseColor,
		material,
		emissive,
		gl_FragCoord.xyz
	);

	Color = vec4(light, baseColor.a);
}

//------------------------------------------------------------------------------
//	Techniques
//------------------------------------------------------------------------------
SimpleTechnique(
	GLTFStatic,
	"Static",
	vsStatic(), 
	psGLTF(
		calcColor = SimpleColor,
		calcBump = GLTFNormalMapFunctor
	),
	StandardState);

SimpleTechnique(
	GLTFStaticDoubleSided,
	"Static|DoubleSided",
	vsStatic(), 
	psGLTF(
		calcColor = SimpleColor,
		calcBump = GLTFNormalMapFunctor
	),
	DoubleSidedState);

SimpleTechnique(
	GLTFStaticAlphaMask, 
	"Static|AlphaMask", 
	vsStatic(),
	psGLTFAlphaMask(
		calcColor = SimpleColor,
		calcBump = GLTFNormalMapFunctor
	),
	StandardState);
	
SimpleTechnique(
	GLTFStaticAlphaMaskDoubleSided, 
	"Static|AlphaMask|DoubleSided", 
	vsStatic(),
	psGLTFAlphaMask(
		calcColor = SimpleColor,
		calcBump = GLTFNormalMapFunctor
	),
	DoubleSidedState);
	
SimpleTechnique(
	GLTFStaticAlphaBlend, 
	"Static|AlphaBlend", 
	vsGLTFStatic(),
	psGLTFAlpha(
		calcColor = SimpleColor,
		calcBump = GLTFNormalMapFunctor
	),
	AlphaState);

SimpleTechnique(
	GLTFStaticAlphaBlendDoubleSided, 
	"Static|AlphaBlend|DoubleSided", 
	vsGLTFStatic(),
	psGLTFAlpha(
		calcColor = SimpleColor,
		calcBump = GLTFNormalMapFunctor
	),
	AlphaDoubleSidedState);