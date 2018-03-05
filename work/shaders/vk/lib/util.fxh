//------------------------------------------------------------------------------
//  util.fxh
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#ifndef UTIL_FXH
#define UTIL_FXH
#include "lib/std.fxh"
#include "lib/shared.fxh" 

const float depthScale = 100.0f;

//------------------------------------------------------------------------------
/**
    Sample a texel with manual box blur
*/
vec4 
SampleSimpleBoxBlur(sampler2D texSampler, vec2 uv, vec2 pixelSize, vec2 boxSize)
{
    vec4 sum = vec4(0.0f);
    for(float x = -boxSize.x; x <= boxSize.x; x++)
    {
        for(float y = -boxSize.y; y <= boxSize.y; y++)
        {
            vec2 boxUv = uv + vec2(x * pixelSize.x, y * pixelSize.y);
            sum += texture(texSampler, boxUv);
        }
    }
    sum /= (boxSize.x * 2 + 1) * (boxSize.y * 2 + 1);
    return sum;
}

//------------------------------------------------------------------------------
/**
    Sample a texel with manual bilinear filtering (for texture formats which
    don't support filtering).
    
    FIXME: on 360 we can actually do the same without knowing texSize, saves
    the coordinate computations!
*/
vec4
SampleBilinear(sampler2D texSampler, vec2 uv, vec2 pixelSize)
{
#if USE_HARDWARE_SAMPLING
    return tex2D(texSampler, uv);
#else    
    vec2 uv01 = vec2(uv.x + pixelSize.x, uv.y);
    vec2 uv10 = vec2(uv.x, uv.y + pixelSize.y);
    vec2 uv11 = uv + pixelSize;
    
    vec4 c00 = texture(texSampler, uv);
    vec4 c01 = texture(texSampler, uv01);
    vec4 c10 = texture(texSampler, uv10);
    vec4 c11 = texture(texSampler, uv11);

    vec2 ratios = fract(uv / pixelSize);    
    vec4 c = mix(mix(c00, c01, ratios.x), mix(c10, c11, ratios.x), ratios.y);
    return c;    
#endif    
}

//------------------------------------------------------------------------------
/**
    Encode 2 values in the range 0..1 into a 4-channel vector. Used
    for encoding PSSM-depth values into a 32-bit-rgba value.
*/
vec4
Encode2(vec2 inVals)
{
    return vec4(inVals.x, fract(inVals.x * 256.0), inVals.y, fract(inVals.y * 256.0));
}

//------------------------------------------------------------------------------
/**
    Decode 2 values encoded by Encode2().
*/
vec2
Decode2(vec4 inVals)
{
    return vec2(inVals.x + (inVals.y / 256.0), inVals.z + (inVals.w / 256.0));
}

//------------------------------------------------------------------------------
/**
    Decode PSSM depth values from a shadow map texture.
*/
vec2
DecodePSSMDepth(sampler2D texSampler, vec2 uv, vec2 pixelSize)
{
    return SampleBilinear(texSampler, uv, pixelSize).xy;
    //vec2 boxSize = (1.0, 1.0);
    //return SampleSimpleBoxBlur(texSampler, uv, texSize, boxSize);
}

//------------------------------------------------------------------------------
/**
    Sample a surface normal from a DXT5nm-compressed texture, return 
    TangentSurfaceNormal.
*/
vec3
SampleNormal(sampler2D bumpSampler, vec2 uv)
{
    vec3 n;
    #ifdef __XBOX360__
        // DXN compression
        n.xy = (texture(bumpSampler, uv).xy * 2.0) - 1.0;
        n.z = clamp(1.0 - dot(n.xy, n.xy), 0.0f, 1.0f);
    #else
        // DX5N compression
        n.xy = (texture(bumpSampler, uv).ag * 2.0) - 1.0;    
        n.z = clamp(1.0 - dot(n.xy, n.xy), 0.0f, 1.0f);
    #endif
    return n;
}

//------------------------------------------------------------------------------
/**
    Sample tangent surface normal from bump map and transform into 
    world space.
*/
vec3
psWorldSpaceNormalFromBumpMap(sampler2D bumpMapSampler, vec2 uv, vec3 worldNormal, vec3 worldTangent, vec3 worldBinormal)
{
    mat3 tangentToWorld = mat3(worldTangent, worldBinormal, worldNormal);
    vec3 tangentNormal = SampleNormal(bumpMapSampler, uv);
    return tangentToWorld * tangentNormal;
}

//------------------------------------------------------------------------------
/**
    Unpack a UB4N packedData normal.
*/
vec3
UnpackNormal(vec3 packedDataNormal)
{
    return (packedDataNormal * 2.0) - 1.0;
}

//------------------------------------------------------------------------------
/**
    Unpack a packedData vertex normal.
*/
vec4
UnpackNormal4(vec4 packedDataNormal)
{
    return vec4((packedDataNormal.xyz * 2.0) - 1.0, 1.0f);
}

//------------------------------------------------------------------------------
/**
    Unpack a 4.12 packedData texture coord.
*/
vec2
UnpackUv(vec2 packedDataUv)
{
    return (packedDataUv / 8192.0);
}   

//------------------------------------------------------------------------------
/**
    Unpack a skin weight vertex component. Since the packing looses some
    precision we need to re-normalize the weights.
*/
vec4
UnpackWeights(vec4 weights)
{
    return (weights / dot(weights, vec4(1.0, 1.0, 1.0, 1.0)));
}

#define USE_SRGB 1
#if USE_SRGB
#define EncodeHDR(x) x
#define EncodeHDR4(x) x
#define DecodeHDR(x) x
#define DecodeHDR4(x) x
#else
//------------------------------------------------------------------------------
/**
    Scale down pseudo-HDR-value into RGB8.
*/
vec4
EncodeHDR(in vec4 rgba)
{
    return rgba * vec4(0.5, 0.5, 0.5, 1.0);
}

//------------------------------------------------------------------------------
/**
    Scale down pseudo-HDR-value into RGB8.
*/
vec4
EncodeHDR4(in vec4 rgba)
{
    return rgba * vec4(0.5, 0.5, 0.5, 0.5);
}

//------------------------------------------------------------------------------
/**
    Scale up pseudo-HDR-value encoded by EncodeHDR() in shaders.fx.
*/   
vec4 
DecodeHDR(in vec4 rgba)
{
    return rgba * vec4(2.0f, 2.0f, 2.0f, 1.0f);
}  


//------------------------------------------------------------------------------
/**
    Scale up pseudo-HDR-value encoded by EncodeHDR() in shaders.fx.
*/   
vec4 
DecodeHDR4(in vec4 rgba)
{
    return rgba * vec4(2.0f, 2.0f, 2.0f, 2.0f);
}  
#endif

const float MiddleGrey = 0.5f; 
const float Key = 0.3f;
const vec4 Luminance = vec4(0.2126f, 0.7152f, 0.0722f, 0.0f);
//const vec4 Luminance = vec4(0.299f, 0.587f, 0.114f, 0.0f);


//------------------------------------------------------------------------------
/**
	Calculates HDR tone mapping
*/
vec4
ToneMap(vec4 vColor, vec4 lumAvg, float maxLum)
{	
	// Calculate the luminance of the current pixel 
	//float fLumPixel = dot(vColor.rgb, Luminance.rgb);     
	//vec4 lum = lumAvg;
	

	// Apply the modified operator (Eq. 4) 
	//float fLumScaled = (fLumPixel * MiddleGrey) / lumAvg;     
	//float fLumCompressed = (fLumScaled * (1 + (fLumScaled / (MaxLuminance * MaxLuminance))) / (1 + fLumScaled));
	
	float L = dot(vColor, Luminance);
	float Lp = L * Key / lumAvg.x;
	float nL = (Lp * (1.0f + Lp / (MiddleGrey))) / (1.0f + Lp);
	//float lp = (MaxLuminance / lumAvg.x) * (vColor.x + vColor.y + vColor.z) * 0.33f;
    //float luminanceSquared = (lumAvg.y + MiddleGrey * lumAvg.y) * (lumAvg.y + MiddleGrey * lumAvg.y);
    //float scalar = (lp * (1.0f + (lp / (luminanceSquared)))) / (1.0f + lp);
	
	vec3 color = vColor.rgb * (nL / L) * maxLum;
	//color = color / (1 + color);
	//color = pow(color, vec3(1/2.2f));

	return vec4(color, 1.0f); 	
	
}

//------------------------------------------------------------------------------
/**
*/
// From GPUGems3: "Don't know why this isn't in the standard library..."
float linstep(float min, float max, float v)
{
    return clamp((v - min) / (max - min), 0.0f, 1.0f);
}

//-------------------------------------------------------------------------------------------------------------
/**
    pack an unsigned 16 bit value into 8 bit values, to store it into an RGBA8 texture
    input: 
        0 <= input <= 65535.0
    output: 
        0 <= byte_a < 1.0f
        0 <= byte_b < 1.0f
*/
void pack_u16(in float depth, out float byte_a, out float byte_b)
{        
    float tmp = depth / 256.0f;
    byte_a = floor(tmp) / 256.0f;
    byte_b = fract(tmp);
    
}

//-------------------------------------------------------------------------------------------------------------
/**
    loot at pack_u16 for details
*/
float unpack_u16(in float byte_a, in float byte_b)
{
    return ((byte_a * 256.0f) * 256.0f) + (byte_b * 256.0f);
}

//-------------------------------------------------------------------------------------------------------------
/**
    Encode a float value between -1.0 .. 1.0 into 2 seperate 8 bits. Used to store a normal component
    into 2 channels of an 8-Bit RGBA texture, so the normal component is stored in 16 bits
    Normal.x -> AR 16 bit
    Normal.y -> GB 16 bit
*/
void pack_16bit_normal_component(in float n, out float byte_a, out float byte_b)
{
    n = ((n * 0.5f) + 0.5f) * 65535.0f;

    pack_u16(n, byte_a, byte_b);
}

//-------------------------------------------------------------------------------------------------------------
/**
    loot at pack_16bit_normal_component for details
*/
float unpack_16bit_normal_component(in float byte_a, in float byte_b)
{
    return ((unpack_u16(byte_a, byte_b) / 65535.0f) - 0.5f) * 2.0f;
}

//-------------------------------------------------------------------------------------------------------------
/**
    loot at pack_16bit_normal_component for details
*/
vec4 pack_normalxy_into_rgba8(in float normal_x, in float normal_y)
{
    vec4 ret;
    pack_16bit_normal_component(normal_x, ret.x, ret.y);
    pack_16bit_normal_component(normal_y, ret.z, ret.w);
    return ret;
}

//-------------------------------------------------------------------------------------------------------------
/**
    loot at pack_16bit_normal_component for details
*/
vec4 unpack_normalxy_from_rgba8(in vec4 packedData)
{
    return vec4(  unpack_16bit_normal_component(packedData.x, packedData.y),
                    unpack_16bit_normal_component(packedData.z, packedData.w),
                    0.0f,
                    0.0f);
}

//------------------------------------------------------------------------------
/**    
    Pack
    
    out vec4 normal -> a view space normal into 16 bit's for x (normal.x & normal.y) and y-component (normal.z & normal.w), preserve sign of z-component.
*/
vec4 PackViewSpaceNormal(in vec3 viewSpaceNormal)
{
    // make sure normal is actually normalized
    viewSpaceNormal = normalize(viewSpaceNormal);

    // use Stereographic Projection, to avoid saveing a sign bit
    // see http://aras-p.info/texts/CompactNormalStorage.html for further info
    const float scale = 1.7777f;
    vec2 enc = viewSpaceNormal.xy / (viewSpaceNormal.z+1.0f);
    enc /= scale;
    enc = enc * 0.5f + 0.5f;
    
    // pack normal x and y
    vec4 normal = pack_normalxy_into_rgba8(enc.x, enc.y);
    return normal;
}

//------------------------------------------------------------------------------
/**
    Unpack a view space normal packedData with PackViewSpaceNormalPosition().
*/
vec3 
UnpackViewSpaceNormal(in vec4 packedDataValue)
{
    // unpack x and y of the normal
    vec4 unpackedData = unpack_normalxy_from_rgba8(packedDataValue);
    
    // packedDataValue is vec4, with .rg containing encoded normal
    const float scale = 1.7777f;
    vec3 nn = unpackedData.xyz * vec3(2.0f * scale, 2.0f * scale, 0.0f) + vec3(-scale, -scale, 1.0f);
    float g = 2.0f / dot(nn.xyz, nn.xyz);
    vec3 outViewSpaceNormal;
    outViewSpaceNormal.xy = g * nn.xy;
    outViewSpaceNormal.z = g - 1.0f;
    return outViewSpaceNormal;
}

//------------------------------------------------------------------------------
/**    
    Pack ObjectId NormalGroup Depth

    x: objectId
    y: NormalGroupId
    z: depth upper 8 bit
    w: depth lower 8 bit
*/
vec4 PackObjectDepth(in float ObjectId, in float NormalGroupId, in float depth)
{    
    vec4 packedData;
    packedData.x = ObjectId;
    packedData.y = NormalGroupId;
    // we need to multiply the depth by a factor, otherwise we would have a raster on small distances
    // normal minimal raster is 1 / 255 (0.00392)
    depth = depth * depthScale;
    pack_u16(depth, packedData.z, packedData.w);
    return packedData;
}

//------------------------------------------------------------------------------
/**    
    Unpack Depth

    x: objectId
    y: NormalGroupId
    z: depth upper 8 bit
    w: depth lower 8 bit
*/
float UnpackDepth(in vec4 packedData)
{    
    return unpack_u16(packedData.z, packedData.w) / depthScale;
}

//------------------------------------------------------------------------------
/**
    Compute a rim light intensity value.
*/
const float RimIntensity = 0.9;//3.0;//
const float RimPower = 2.0;
float RimLightIntensity(vec3 worldNormal,     // surface normal in world space
                        vec3 worldEyeVec)     // eye vector in world space
{
    float rimIntensity  = pow(abs(1.0f - abs(dot(worldNormal, worldEyeVec))), RimPower);    
    rimIntensity *= RimIntensity;
    return rimIntensity;
}

//-------------------------------------------------------------------------------------------------------------
/**
    Compute lighting with diffuse and specular from lightbuffer, 
    emissive and spec texture, optional add rim
*/
vec4 
psLightMaterial(in vec4 lightValues,                
               in vec4 diffColor, 
               in vec3 emsvColor, 
               in float emsvIntensity, 
               in vec3 specColor,                    
               in float specIntensity)
{
    lightValues = DecodeHDR(lightValues);    
    vec4 color = diffColor;
    // exagerate optional Rim
    color.xyz *= lightValues.xyz;    
    color.xyz += emsvColor * emsvIntensity;
    // color with diff color
    vec3 normedColor = normalize(lightValues.xyz);
    float maxColor = max(max(normedColor.x, normedColor.y), normedColor.z);
    normedColor /= maxColor;    
    float spec = lightValues.w;        
    color.xyz += specColor * specIntensity * spec * normedColor;
    
    return color;
}

//-------------------------------------------------------------------------------------------------------------
/**
    Compute depth fadout from 
*/
float 
psComputeDepthFadeOut(in float myViewSpaceDepth, in vec4 projPos, in sampler2D normDepthBuffer, in float fadeOutRange)
{   
    return 0.0f;
/*
    TODO  UnpackViewSpaceDepth & UnpackViewSpaceNormal changed 
    
    float backgroundDepth;
    vec2 posDivW = projPos.xy / projPos.ww;
    vec2 screenUv = posDivW * vec2(0.5, -0.5) + 0.5;
    vec4 packedDataValue = tex2D(normDepthBuffer, screenUv);
    UnpackViewSpaceDepth(packedDataValue, backgroundDepth);
    vec3 backgroundNormal;
    UnpackViewSpaceNormal(packedDataValue, backgroundNormal);
    float diffDistance = backgroundDepth - myViewSpaceDepth;
    float VdotN = abs(backgroundNormal.z); // normal is in viewspace
    float normalConsideredFadeOut = mix(fadeOutRange * fadeOutRange, fadeOutRange, VdotN); 
    float modAlpha = clamp(diffDistance * diffDistance / normalConsideredFadeOut, 0.0f, 1.0f); 
    return modAlpha;
*/
}

//-------------------------------------------------------------------------------------------------------------
/**
    Compute screen ccordinates 
*/
vec2 
psComputeScreenCoord(in vec2 screenPixel, in vec2 pixelSize)
{
    vec2 screenUv = screenPixel.xy * pixelSize.xy;
    return screenUv;
}

#define PI 3.14159265
#define ONE_OVER_PI 1/PI
#define PI_OVER_FOUR PI/4.0f
#define PI_OVER_TWO PI/2.0f
//------------------------------------------------------------------------------
/**
	Calculate fresnel amount using Schlick's approximation
*/
vec3
FresnelSchlick(vec3 spec, float dotprod)
{
	float base = 1.0 - clamp(dotprod, 0.0f, 1.0f);
	float exponent = pow(base, 5);
	return spec + (1 - spec) * exponent;
}

//------------------------------------------------------------------------------
/**
	Calculate fresnel amount using Schlick's approximation but with roughness
*/
vec3
FresnelSchlickGloss(vec3 spec, float dotprod, float roughness)
{
	float base = 1.0 - clamp(dotprod, 0.0f, 1.0f);
	float exponent = pow(base, 5);
	return spec + (max(vec3(roughness), spec) - spec) * exponent;
}

//------------------------------------------------------------------------------
/**
	Calculates light diffuse and specular using physically based lighting
*/
void
BRDFLighting(
	 float NH,
	 float NL,
	 float NV,
	 float HL, 
	 float specPower,
	 vec3 specColor,
	 out vec3 spec)
{
	float normalizationTerm = (specPower + 2.0f) / 8.0f;
	float blinnPhong = pow(NH, specPower);
	float specularTerm = blinnPhong;
	float cosineTerm = NL;
	vec3 fresnelTerm = FresnelSchlick(specColor, HL);
	float alpha = 1.0f / ( sqrt ( PI_OVER_FOUR * specPower + PI_OVER_TWO) );
	float visibilityTerm = (NL * (1.0f - alpha) + alpha ) * (NV * ( 1.0f - alpha ) + alpha );
	visibilityTerm = 1.0f / visibilityTerm;
	spec = specularTerm * cosineTerm * fresnelTerm * visibilityTerm;	
	//spec = fresnelTerm * (roughness + 2) / 8.0f * pow(NH, roughness) * (NL);
}

//-------------------------------------------------------------------------------------------------------------
/**
    Logarithmic filtering
*/
float log_conv ( float x0, float x, float y0, float y )
{
    return (x + log(x0 + (y0 * exp(y - x))));
}

//-------------------------------------------------------------------------------------------------------------
/**
    Compute pixel size.
*/
vec2
GetPixelSize(in sampler2D tex)
{
	vec2 size = textureSize(tex, 0);	
	size = vec2(1.0f) / size;
	return size;
}

//-------------------------------------------------------------------------------------------------------------
/**
    Compute pixel size.
*/
vec2
GetScaledUVs(in vec2 uvs, in sampler2D tex, in vec2 dimensions)
{
	vec2 texSize = textureSize(tex, 0);
	uvs = uvs * (dimensions / texSize);
	return uvs;
}

//-------------------------------------------------------------------------------------------------------------
/**
    Compute UV from pixel coordinate and texture
*/
vec2
GetUV(in ivec2 pixel, in sampler2D tex)
{
	vec2 size = textureSize(tex, 0);
	size = pixel / size;
	return size;
}

//-------------------------------------------------------------------------------------------------------------
/**
    Compute pixel from UV
*/
ivec2
GetPixel(in vec2 uv, in sampler2D tex)
{
	ivec2 size = textureSize(tex, 0);
	size = ivec2(uv * size);
	return size;
}

//-------------------------------------------------------------------------------------------------------------
/**
    Compute texture ratio to current pixel size
*/
vec2
GetTextureRatio(in sampler2D tex, vec2 pixelSize)
{
	ivec2 size = textureSize(tex, 0);	 
	vec2 currentTextureSize = vec2(1.0f) / size;
	return size / currentTextureSize;
}

//-------------------------------------------------------------------------------------------------------------
/**
    Flips y in vec2, useful for opengl conversion of uv corods
*/
vec2
FlipY(vec2 uv)
{
	return vec2(uv.x, 1.0f - uv.y);
}

//------------------------------------------------------------------------------
#endif
