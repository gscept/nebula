//------------------------------------------------------------------------------
//  particle.fx
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/particles.fxh"
#include "lib/shared.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"
#include "lib/defaultsamplers.fxh"
#include "lib/shadowbase.fxh"

sampler2D DepthBuffer;
float LightMapIntensity;

sampler2D Layer1;
sampler2D Layer2;
sampler2D Layer3;
sampler2D Layer4;

float2 UVAnim1;
float2 UVAnim2;
float2 UVAnim3;
float2 UVAnim4;

// samplers
samplerstate ParticleSampler
{
	Samplers = { SpecularMap, EmissiveMap, NormalMap, AlbedoMap, DisplacementMap, RoughnessMap, Layer1, Layer2, Layer3, Layer4 };
	Filter = MinMagMipLinear;
	AddressU = Wrap;
	AddressV = Wrap;
};

samplerstate LayerSampler
{
	Samplers = {Layer1, Layer2, Layer3, Layer4 };
	AddressU = Mirror;
	AddressV = Mirror;
};

state LitParticleState
{
	BlendEnabled[0] = true;
	SrcBlend[0] = SrcAlpha;
	DstBlend[0] = OneMinusSrcAlpha;
	CullMode = None;
	DepthEnabled = true;
	DepthWrite = false;
	DepthFunc = Less;
};

state ParticleShadowState
{
	DepthClamp = false;
	BlendEnabled[0] = true;
	SrcBlend[0] = One;
	DstBlend[0] = One;
	CullMode = None;
	DepthEnabled = true;
	DepthWrite = false;
	DepthFunc = Less;
};

state UnlitParticleState
{
	BlendEnabled[0] = true;
	SrcBlend[0] = SrcAlpha;
	DstBlend[0] = OneMinusSrcAlpha;
	
	CullMode = None;
	DepthEnabled = true;
	DepthWrite = false;
	DepthFunc = Less;
};

state UnlitAdditiveParticleState
{
	BlendEnabled[0] = true;
	SrcBlend[0] = SrcAlpha;
	DstBlend[0] = One;
	
	CullMode = None;
	DepthEnabled = true;
	DepthWrite = false;
	DepthFunc = Less;
};

state UnlitParticleStateBlendAdd
{
	BlendEnabled[0] = true;
	SrcBlend[0] = One;
	DstBlend[0] = OneMinusSrcAlpha;
	
	CullMode = None;
	DepthEnabled = true;
	DepthWrite = false;
	DepthFunc = Less;
};

//------------------------------------------------------------------------------
/**
*/
#define numAlphaLayers 4
const vec2 stippleMasks[numAlphaLayers] = {
		vec2(0,0), 
		vec2(1,0), 
		vec2(0,1),
		vec2(1,1)
		};


//------------------------------------------------------------------------------
/**
*/
shader
void
vsUnlit(in vec2 corner,
	in vec4 position,
	in vec4 stretchPos,
	in vec4 color,
	in vec4 uvMinMax,
	in vec4 rotSize,
	out vec4 ViewSpacePos,
	out vec4 Color,
	out vec2 UV)
{
	CornerVertex cornerVert = ComputeCornerVertex(true,
										corner,
										position,
										stretchPos,
										uvMinMax,
										rotSize.x,
										rotSize.y);
										
	UV = cornerVert.UV;
	gl_Position = ViewProjection * cornerVert.worldPos;
	ViewSpacePos = View * cornerVert.worldPos;
	Color = color;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsLit(in vec2 corner,
	in vec4 position,
	in vec4 stretchPos,
	in vec4 color,
	in vec4 uvMinMax,
	in vec4 rotSize,
	out vec4 ViewSpacePos,
	out vec4 ProjPos,
	out vec4 WorldPos,
	out vec3 Normal,
	out vec3 Tangent,
	out vec3 Binormal,
	out vec3 WorldEyeVec,
	out vec4 Color,
	out vec2 UV) 
{
	CornerVertex cornerVert = ComputeCornerVertex(false,
										corner,
										position,
										stretchPos,
										uvMinMax,
										rotSize.x,
										rotSize.y);
										
	mat4 modelView = mul(Model, View);
	Normal = mat3(modelView) * cornerVert.worldNormal;
	Tangent = mat3(modelView) * cornerVert.worldTangent;
	Binormal = mat3(modelView) * cornerVert.worldBinormal;
	UV = cornerVert.UV;
	WorldEyeVec = normalize(EyePos - cornerVert.worldPos).xyz;
	WorldPos = cornerVert.worldPos;
	gl_Position = ViewProjection * cornerVert.worldPos;
	ViewSpacePos = View * cornerVert.worldPos;
	ProjPos = gl_Position;
	Color = color;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsShadow(in vec2 corner,
	in vec4 position,
	in vec4 stretchPos,
	in vec4 color,
	in vec4 uvMinMax,
	in vec4 rotSize,
	out vec2 UV,
	out vec4 ProjPos)
{
	CornerVertex cornerVert = ComputeCornerVertex(false,
										corner,
										position,
										stretchPos,
										uvMinMax,
										rotSize.x,
										rotSize.y);
										
	UV = cornerVert.UV;
	ProjPos = ViewMatrixArray[0] * cornerVert.worldPos;
	gl_Position = ProjPos; 
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsShadowCSM(in vec2 corner,
	in vec4 position,
	in vec4 stretchPos,
	in vec4 color,
	in vec4 uvMinMax,
	in vec4 rotSize,
	out vec2 UV,
	out vec4 ProjPos)
{
	CornerVertex cornerVert = ComputeCornerVertex(false,
										corner,
										position,
										stretchPos,
										uvMinMax,
										rotSize.x,
										rotSize.y);
										 
	UV = cornerVert.UV;
	ProjPos = cornerVert.worldPos;
	gl_Position = ProjPos; 
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psUnlit(in vec4 ViewSpacePosition,
	in vec4 Color,
	in vec2 UV,
	[color0] out vec4 FinalColor) 
{
	vec2 pixelSize = GetPixelSize(DepthBuffer);
	vec2 screenUV = psComputeScreenCoord(gl_FragCoord.xy, pixelSize.xy);
	vec4 diffColor = texture(AlbedoMap, UV);
	
	vec4 color = diffColor * vec4(Color.rgb, 0);
	float depth = textureLod(DepthBuffer, screenUV, 0).r;
	float particleDepth = length(ViewSpacePosition);
	float AlphaMod = saturate(abs(depth - particleDepth));
	color.a = diffColor.a * Color.a * AlphaMod;
	FinalColor = color;
}

//------------------------------------------------------------------------------
/**
*/ 
shader
void
psUnlit2Layers(in vec4 ViewSpacePosition,
	in vec4 Color,
	in vec2 UV,
	[color0] out vec4 FinalColor) 
{
	vec2 pixelSize = GetPixelSize(DepthBuffer);
	vec2 screenUV = psComputeScreenCoord(gl_FragCoord.xy, pixelSize.xy);
	vec4 layer1 = texture(Layer1, UV + UVAnim1 * TimeAndRandom.x);
	vec4 layer2 = texture(Layer2, UV + UVAnim2 * TimeAndRandom.x);
	
	vec4 color = layer1 * layer2 * 2;
	float depth = textureLod(DepthBuffer, screenUV, 0).r;
	float particleDepth = length(ViewSpacePosition);
	float AlphaMod = saturate(abs(depth - particleDepth));
	color.a = saturate(color.a);
	color.rgb += Color.rgb * color.a;
	color *= Color.a * AlphaMod;
	FinalColor = color;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psUnlit3Layers(in vec4 ViewSpacePosition,
	in vec4 Color,
	in vec2 UV,
	[color0] out vec4 FinalColor) 
{
	vec2 pixelSize = GetPixelSize(DepthBuffer);
	vec2 screenUV = psComputeScreenCoord(gl_FragCoord.xy, pixelSize.xy);
	vec4 layer1 = texture(Layer1, UV + UVAnim1 * TimeAndRandom.x);
	vec4 layer2 = texture(Layer2, UV + UVAnim2 * TimeAndRandom.x);
	vec4 layer3 = texture(Layer3, UV + UVAnim3 * TimeAndRandom.x);
	
	vec4 color = layer1 * layer2 * 2 * layer3 * 2;
	float depth = textureLod(DepthBuffer, screenUV, 0).r;
	float particleDepth = length(ViewSpacePosition);
	float AlphaMod = saturate(abs(depth - particleDepth));
	color.a = saturate(color.a);
	color.rgb += Color.rgb * color.a;
	color *= Color.a * AlphaMod;
	FinalColor = color;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psUnlit4Layers(in vec4 ViewSpacePosition,
	in vec4 Color,
	in vec2 UV,
	[color0] out vec4 FinalColor) 
{
	vec2 pixelSize = GetPixelSize(DepthBuffer);
	vec2 screenUV = psComputeScreenCoord(gl_FragCoord.xy, pixelSize.xy);
	vec4 layer1 = texture(Layer1, UV + UVAnim1 * TimeAndRandom.x);
	vec4 layer2 = texture(Layer2, UV + UVAnim2 * TimeAndRandom.x);
	vec4 layer3 = texture(Layer3, UV + UVAnim3 * TimeAndRandom.x);
	vec4 layer4 = texture(Layer4, UV + UVAnim4 * TimeAndRandom.x);
	
	vec4 color = layer1 * layer2 * 2 * layer3 * 2 * layer4;
	float depth = textureLod(DepthBuffer, screenUV, 0).r;
	float particleDepth = length(ViewSpacePosition);
	float AlphaMod = saturate(abs(depth - particleDepth));
	color.a = saturate(color.a);
	color.rgb += Color.rgb * color.a;
	color *= Color.a * AlphaMod;
	FinalColor = color;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psLit(in vec4 ViewSpacePosition,
	in vec4 ProjPos,
	in vec4 WorldPos,
	in vec3 Normal,
	in vec3 Tangent,
	in vec3 Binormal,
	in vec3 WorldEyeVec,
	in vec4 Color,
	in vec2 UV,
	[color0] out vec4 Albedo,
	[color1] out vec4 Normals,
	[color2] out float Depth,
	[color3] out vec4 Specular) 
{	
	vec4 diffColor = texture(AlbedoMap, UV.xy);
	vec4 emsvColor = texture(EmissiveMap, UV.xy);
	vec4 specColor = texture(SpecularMap, UV.xy);
	float roughness = texture(RoughnessMap, UV).r;
	
	Specular = vec4(specColor.rgb * MatSpecularIntensity.rgb, roughness);
	Albedo = diffColor + vec4(Color.rgb, 0);
	
	Depth = length(ViewSpacePosition.xyz);
	mat3 tangentViewMatrix = mat3(normalize(Tangent.xyz), normalize(Binormal.xyz), normalize(Normal.xyz));        
	vec3 tNormal = vec3(0,0,0);
	tNormal.xy = (texture(NormalMap, UV).ag * 2.0) - 1.0;
	tNormal.z = saturate(sqrt(1.0 - dot(tNormal.xy, tNormal.xy)));
	
	if (!gl_FrontFacing) tNormal = -tNormal;
	Normals = PackViewSpaceNormal((tangentViewMatrix * tNormal).xyz);
	
	float depth = Depth;
	float particleDepth = length(ViewSpacePosition);
	float alphaMod = saturate(abs(depth - particleDepth));
	Albedo.a = diffColor.a * Color.a;
} 

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(Unlit, "Unlit", vsUnlit(), psUnlit(), UnlitParticleState);
SimpleTechnique(UnlitAdditive, "Unlit|Alt0", vsUnlit(), psUnlit(), UnlitAdditiveParticleState);
SimpleTechnique(UnlitBlendAdd, "Unlit|Alt1", vsUnlit(), psUnlit(), UnlitParticleStateBlendAdd);
SimpleTechnique(UnlitBlendAdd2Layers, "Unlit|Alt2", vsUnlit(), psUnlit2Layers(), UnlitParticleStateBlendAdd);
SimpleTechnique(UnlitBlendAdd3Layers, "Unlit|Alt3", vsUnlit(), psUnlit3Layers(), UnlitParticleStateBlendAdd);
SimpleTechnique(UnlitBlendAdd4Layers, "Unlit|Alt4", vsUnlit(), psUnlit4Layers(), UnlitParticleStateBlendAdd);
SimpleTechnique(Lit, "Static", vsLit(), psLit(), LitParticleState);
GLOBALLIGHT_SHADOW_INST_ALPHA(vsShadowCSM(), ParticleShadowState);
//SimpleTechnique(LitShadow, "Global|Alpha", vsLit(), psLit(), LitParticleState);