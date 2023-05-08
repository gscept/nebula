//------------------------------------------------------------------------------
//  particle.fx
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/particles.fxh"
#include "lib/shared.fxh"
#include "lib/objects_shared.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"
#include "lib/defaultsamplers.fxh"
#include "lib/clustering.fxh"
#include "lib/lighting_functions.fxh"

group(BATCH_GROUP) shared constant Particle[string Visibility = "PS"; ]
{
    textureHandle Layer1;
    textureHandle Layer2;
    textureHandle Layer3;
    textureHandle Layer4;

    vec2 UVAnim1;
    vec2 UVAnim2;
    vec2 UVAnim3;
    vec2 UVAnim4;
    float LightMapIntensity;
    float Transmission;
};

// samplers
sampler_state ParticleSampler
{
    Filter = MinMagMipLinear;
    AddressU = Wrap;
    AddressV = Wrap;
};

sampler_state LayerSampler
{
    AddressU = Mirror;
    AddressV = Mirror;
};

render_state LitParticleState
{
    BlendEnabled[0] = true;
    SrcBlend[0] = SrcAlpha;
    DstBlend[0] = OneMinusSrcAlpha;
    CullMode = None;
    DepthEnabled = true;
    DepthWrite = false;
    DepthFunc = Less;
};

render_state UnlitParticleState
{
    BlendEnabled[0] = true;
    SrcBlend[0] = SrcAlpha;
    DstBlend[0] = OneMinusSrcAlpha;
    
    CullMode = None;
    DepthEnabled = true;
    DepthWrite = false;
    DepthFunc = Less;
};

render_state UnlitAdditiveParticleState
{
    BlendEnabled[0] = true;
    SrcBlend[0] = SrcAlpha;
    DstBlend[0] = One;
    
    CullMode = None;
    DepthEnabled = true;
    DepthWrite = false;
    DepthFunc = Less;
};

render_state UnlitParticleStateBlendAdd
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
vsUnlit(
    [slot=0] in vec2 corner,
    [slot=1] in vec4 position,
    [slot=2] in vec4 stretchPos,
    [slot=3] in vec4 color,
    [slot=4] in vec4 uvMinMax,
    [slot=5] in vec4 rotSize,
    out vec4 ViewSpacePos,
    out vec4 Color,
    out vec2 UV)
{
    CornerVertex cornerVert = ComputeCornerVertex(
                                        corner,
                                        position,
                                        stretchPos,
                                        uvMinMax,
                                        vec2(rotSize.x, rotSize.y),
                                        rotSize.z);
                                        
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
vsLit(
    [slot=0] in vec2 corner,
    [slot=1] in vec4 position,
    [slot=2] in vec4 stretchPos,
    [slot=3] in vec4 color,
    [slot=4] in vec4 uvMinMax,
    [slot=5] in vec4 rotSize,
    out vec4 ViewSpacePos, 
    out vec3 Normal,
    out vec3 Tangent,
    out vec3 WorldEyeVec,
    out vec4 Color,
    out vec2 UV) 
{
    CornerVertex cornerVert = ComputeCornerVertex(
                                        corner,
                                        position,
                                        stretchPos,
                                        uvMinMax,
                                        vec2(rotSize.x, rotSize.y),
                                        rotSize.z);
                                        
    Normal = cornerVert.worldNormal;
    Tangent = cornerVert.worldTangent;
    UV = cornerVert.UV;
    WorldEyeVec = normalize(EyePos - cornerVert.worldPos).xyz;
    ViewSpacePos = View * cornerVert.worldPos;
    gl_Position = ViewProjection * cornerVert.worldPos;
    Color = color;
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
    vec2 pixelSize = RenderTargetDimensions[0].zw;
    vec2 screenUV = PixelToNormalized(gl_FragCoord.xy, pixelSize.xy);
    vec4 diffColor = sample2D(AlbedoMap, ParticleSampler, UV);
    
    vec4 color = diffColor * vec4(Color.rgb, 0);
    float depth = sample2DLod(DepthBufferCopy, ParticleSampler, screenUV, 0).r;
    float AlphaMod = saturate(abs(depth - gl_FragCoord.z) * (FocalLengthNearFar.w - FocalLengthNearFar.z));
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
    vec2 pixelSize = RenderTargetDimensions[0].zw;
    vec2 screenUV = PixelToNormalized(gl_FragCoord.xy, pixelSize.xy);
    vec4 layer1 = sample2D(Layer1, LayerSampler, UV + UVAnim1 * Time_Random_Luminance_X.x);
    vec4 layer2 = sample2D(Layer2, LayerSampler, UV + UVAnim2 * Time_Random_Luminance_X.x);
    
    vec4 color = layer1 * layer2 * 2;
    float depth = sample2DLod(DepthBufferCopy, ParticleSampler, screenUV, 0).r;
    float AlphaMod = saturate(abs(depth - gl_FragCoord.z) * (FocalLengthNearFar.w - FocalLengthNearFar.z));
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
    vec2 pixelSize = RenderTargetDimensions[0].zw;
    vec2 screenUV = PixelToNormalized(gl_FragCoord.xy, pixelSize.xy);
    vec4 layer1 = sample2D(Layer1, LayerSampler, UV + UVAnim1 * Time_Random_Luminance_X.x);
    vec4 layer2 = sample2D(Layer2, LayerSampler, UV + UVAnim2 * Time_Random_Luminance_X.x);
    vec4 layer3 = sample2D(Layer3, LayerSampler, UV + UVAnim3 * Time_Random_Luminance_X.x);
    
    vec4 color = ((layer1 * layer2 * 2) * layer3 * 2);
    float depth = sample2DLod(DepthBufferCopy, ParticleSampler, screenUV, 0).r;
    float AlphaMod = saturate(abs(depth - gl_FragCoord.z) * (FocalLengthNearFar.w - FocalLengthNearFar.z));
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
    vec2 pixelSize = RenderTargetDimensions[0].zw;
    vec2 screenUV = PixelToNormalized(gl_FragCoord.xy, pixelSize.xy);
    vec4 layer1 = sample2D(Layer1, LayerSampler, UV + UVAnim1 * Time_Random_Luminance_X.x);
    vec4 layer2 = sample2D(Layer2, LayerSampler, UV + UVAnim2 * Time_Random_Luminance_X.x);
    vec4 layer3 = sample2D(Layer3, LayerSampler, UV + UVAnim3 * Time_Random_Luminance_X.x);
    vec4 layer4 = sample2D(Layer4, LayerSampler, UV + UVAnim4 * Time_Random_Luminance_X.x);
    
    vec4 color = ((layer1 * layer2 * 2) * layer3 * 2) * layer4;
    float depth = sample2DLod(DepthBufferCopy, ParticleSampler, screenUV, 0).r;
    float AlphaMod = saturate(abs(depth - gl_FragCoord.z) * (FocalLengthNearFar.w - FocalLengthNearFar.z));
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
    in vec3 Normal,
    in vec3 Tangent,
    in vec3 WorldEyeVec,
    in vec4 Color,
    in vec2 UV,
    [color0] out vec4 Light) 
{   
    vec4 albedo =       sample2D(AlbedoMap, ParticleSampler, UV);
    vec4 material =     sample2D(ParameterMap, ParticleSampler, UV);
    
    const float depth = fetch2D(DepthBufferCopy, ParticleSampler, ivec2(gl_FragCoord.xy), 0).r;
    const float particleDepth = gl_FragCoord.z;
    float AlphaMod = saturate(abs(depth - particleDepth) * (FocalLengthNearFar.w - FocalLengthNearFar.z));
    const float finalAlpha = albedo.a * Color.a * AlphaMod;
    if (finalAlpha < 0.001f)
        discard;

    vec3 tNormal = vec3(0,0,0);

    tNormal.xy = (sample2D(NormalMap, ParticleSampler, UV).ag * 2.0) - 1.0;
    tNormal.z = saturate(sqrt(1.0 - dot(tNormal.xy, tNormal.xy)));
    vec3 binormal = cross(Normal.xyz, Tangent.xyz);

    if (!gl_FrontFacing)
    {
        // flip tangent space if backface, and transform normal
        tNormal = mat3(Tangent.xyz, binormal.xyz, -Normal.xyz) * tNormal;
    }
    else
    {
        // transform normal to tangent space
        tNormal = mat3(Tangent.xyz, binormal.xyz, Normal.xyz) * tNormal;
    }

    // calculate cluster index
    uint3 index3D = CalculateClusterIndex(gl_FragCoord.xy / BlockSize, ViewSpacePosition.z, InvZScale, InvZBias);
    uint idx = Pack3DTo1D(index3D, NumCells.x, NumCells.y);

    vec3 light = vec3(0, 0, 0);

    // add light
    light += CalculateGlobalLightAmbientTransmission(ViewSpacePosition, WorldEyeVec, tNormal.xyz, particleDepth, material, albedo, Transmission);
    vec3 viewVec = -normalize(ViewSpacePosition.xyz);
    vec3 viewNormal = (View * vec4(tNormal.xyz, 0)).xyz;
    light += LocalLightsAmbientTransmission(idx, ViewSpacePosition, viewVec, viewNormal, particleDepth, material, albedo, Transmission);
    
    Light = vec4(light, finalAlpha);
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