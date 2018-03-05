#ifndef UTIL_FXH
#define UTIL_FXH

static const float depthScale = 100.0f;


//------------------------------------------------------------------------------
/**
    Sample a texel with manual box blur
*/
float4 
SampleSimpleBoxBlur(sampler texSampler, float2 uv, float2 pixelSize, float2 boxSize)
{
    float4 sum = 0;
    for(float x = -boxSize.x; x <= boxSize.x; x++)
    {
        for(float y = -boxSize.y; y <= boxSize.y; y++)
        {
            float2 boxUv = uv + float2(x * pixelSize.x, y * pixelSize.y);
            sum += tex2D(texSampler, boxUv);
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
float4
SampleBilinear(sampler texSampler, float2 uv, float2 pixelSize)
{
#if USE_HARDWARE_SAMPLING
    return tex2D(texSampler, uv);
#else    
    float2 uv01 = float2(uv.x + pixelSize.x, uv.y);
    float2 uv10 = float2(uv.x, uv.y + pixelSize.y);
    float2 uv11 = uv + pixelSize;
    
    float4 c00 = tex2D(texSampler, uv);
    float4 c01 = tex2D(texSampler, uv01);
    float4 c10 = tex2D(texSampler, uv10);
    float4 c11 = tex2D(texSampler, uv11);

    float2 ratios = frac(uv / pixelSize);    
    float4 c = lerp(lerp(c00, c01, ratios.x), lerp(c10, c11, ratios.x), ratios.y);
    return c;    
#endif    
}

//------------------------------------------------------------------------------
/**
    Encode 2 values in the range 0..1 into a 4-channel vector. Used
    for encoding PSSM-depth values into a 32-bit-rgba value.
*/
float4
Encode2(float2 inVals)
{
    return float4(inVals.x, frac(inVals.x * 256.0), inVals.y, frac(inVals.y * 256.0));
}

//------------------------------------------------------------------------------
/**
    Decode 2 values encoded by Encode2().
*/
float2
Decode2(float4 inVals)
{
    return float2(inVals.x + (inVals.y / 256.0), inVals.z + (inVals.w / 256.0));
}

//------------------------------------------------------------------------------
/**
    Encode PSSM depth values from the projection space depth (0..1)
*/
float4
EncodePSSMDepth(float depth)
{
    return float4(depth, depth*depth, 0.0, 0.0);
    
    // partial derivates of depth
    float dx = ddx(depth);
    float dy = ddy(depth);
    
    // compute second moment over the pixel extents
    //float momentY = depth * depth + 0.25 * (dx * dx + dy * dy);
    //return float4(depth, momentY, 0.0, 0.0);
}

//------------------------------------------------------------------------------
/**
    Decode PSSM depth values from a shadow map texture.
*/
float2
DecodePSSMDepth(sampler texSampler, float2 uv, float2 pixelSize)
{
    return SampleBilinear(texSampler, uv, pixelSize).xy;
    //float2 boxSize = (1.0, 1.0);
    //return SampleSimpleBoxBlur(texSampler, uv, texSize, boxSize);
}

//------------------------------------------------------------------------------
/**
    Sample a surface normal from a DXT5nm-compressed texture, return 
    TangentSurfaceNormal.
*/
float3
SampleNormal(sampler bumpSampler, float2 uv)
{
    float3 n;
    #ifdef __XBOX360__
        // DXN compression
        n.xy = (tex2D(bumpSampler, uv).xy * 2.0) - 1.0;
        n.z = saturate(1.0 - dot(n.xy, n.xy));
    #else
        // DX5N compression
        n.xy = (tex2D(bumpSampler, uv).ag * 2.0) - 1.0;    
        n.z = saturate(1.0 - dot(n.xy, n.xy));
    #endif
    return n;
}

//------------------------------------------------------------------------------
/**
    Sample tangent surface normal from bump map and transform into 
    world space.
*/
float3
psWorldSpaceNormalFromBumpMap(sampler bumpMapSampler, float2 uv, float3 worldNormal, float3 worldTangent, float3 worldBinormal)
{
    float3x3 tangentToWorld = float3x3(worldTangent, worldBinormal, worldNormal);
    float3 tangentNormal = SampleNormal(bumpMapSampler, uv);
    return mul(tangentNormal, tangentToWorld);
}

//------------------------------------------------------------------------------
/**
    Unpack a UB4N packed normal.
*/
float3
UnpackNormal(float3 packedNormal)
{
    return (packedNormal * 2.0) - 1.0;
}

//------------------------------------------------------------------------------
/**
    Unpack a packed vertex normal.
*/
float4
UnpackNormal4(float4 packedNormal)
{
    return float4((packedNormal.xyz * 2.0) - 1.0, 1.0f);
}

//------------------------------------------------------------------------------
/**
    Unpack a 4.12 packed texture coord.
*/
float2
UnpackUv(float2 packedUv)
{
    return (packedUv / 8192.0);
}   

//------------------------------------------------------------------------------
/**
    Unpack a skin weight vertex component. Since the packing looses some
    precision we need to re-normalize the weights.
*/
float4
UnpackWeights(float4 weights)
{
    return (weights / dot(weights, float4(1.0, 1.0, 1.0, 1.0)));
}

//------------------------------------------------------------------------------
/**
    Scale down pseudo-HDR-value into RGB8.
*/
float4
EncodeHDR(in float4 rgba)
{
    return rgba * float4(0.5, 0.5, 0.5, 1.0);
}

//------------------------------------------------------------------------------
/**
    Scale down pseudo-HDR-value into RGB8.
*/
float4
EncodeHDR4(in float4 rgba)
{
    return rgba * float4(0.5, 0.5, 0.5, 0.5);
}

/*
float4 EncodeHDR(in float4 rgba)
{
    const float colorSpace = 0.8;
    const float maxHDR = 8;
    const float hdrSpace = 1 - colorSpace;
    const float hdrPow = 10;
    const float hdrRt = 0.3;
    
    float3 col = clamp(rgba.rgb,0,1) * colorSpace;
    float3 hdr = pow(clamp(rgba.rgb,1,10),hdrRt)-1;
    float4 result;
    hdr = clamp(hdr, 0, hdrSpace);
    result.rgb = col + hdr;
    result.a = rgba.a;
    return result;
}
*/


//------------------------------------------------------------------------------
/**
    Scale up pseudo-HDR-value encoded by EncodeHDR() in shaders.fx.
*/   
float4 
DecodeHDR(in float4 rgba)
{
    return rgba * float4(2.0f, 2.0f, 2.0f, 1.0f);
}  


//------------------------------------------------------------------------------
/**
    Scale up pseudo-HDR-value encoded by EncodeHDR() in shaders.fx.
*/   
float4 
DecodeHDR4(in float4 rgba)
{
    return rgba * float4(2.0f, 2.0f, 2.0f, 2.0f);
}  

/*
float4 DecodeHDR(in float4 rgba)
{
    const float colorSpace = 0.8;
    const float maxHDR = 8;
    const float hdrSpace = 1 - colorSpace;
    //const float hdrPow = log(maxHDR)/log(hdrSpace);
    const float hdrPow = 10;
    //const float hdrRt = 1/hdrPow;
    const float hdrRt = 0.3;
        
    float3 col = clamp(rgba.rgb,0,colorSpace) * (1/colorSpace);
    float3 hdr = pow(clamp(rgba.rgb - colorSpace, 0, hdrSpace)+1,hdrPow)-1;
    float4 result;
    result.rgb = col + hdr;
    result.a = rgba.a;
    return result;
}
*/

static const float MiddleGrey = 0.6f; 
static const float MaxLuminance = 16.0f; 
static const float MinLuminance = 0.3f;

//------------------------------------------------------------------------------
/**
	Calculates HDR tone mapping
*/
float4
ToneMap(float4 vColor, float lumAvg, float4 luminance)
{	
	// Calculate the luminance of the current pixel 
	float fLumPixel = dot(vColor.rgb, luminance.rgb);     

	// Apply the modified operator (Eq. 4) 
	float fLumScaled = (fLumPixel * MiddleGrey) / lumAvg;     
	float fLumCompressed = (fLumScaled * (1 + (fLumScaled / (MaxLuminance * MaxLuminance))) / (1 + fLumScaled)); 
	return float4(fLumCompressed * vColor.rgb, vColor.a); 
}


// From GPUGems3: "Don't know why this isn't in the standard library..."
float linstep(float min, float max, float v)
{
    return clamp((v - min) / (max - min), 0, 1);
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
    byte_b = frac(tmp);
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
float4 pack_normalxy_into_rgba8(in float normal_x, in float normal_y)
{
    float4 ret;
    pack_16bit_normal_component(normal_x, ret.x, ret.y);
    pack_16bit_normal_component(normal_y, ret.z, ret.w);
    return ret;
}

//-------------------------------------------------------------------------------------------------------------
/**
    loot at pack_16bit_normal_component for details
*/
float4 unpack_normalxy_from_rgba8(in float4 packed)
{
    return float4(  unpack_16bit_normal_component(packed.x, packed.y),
                    unpack_16bit_normal_component(packed.z, packed.w),
                    0.0f,
                    0.0f);
}

//------------------------------------------------------------------------------
/**    
    Pack
    
    out float4 normal -> a view space normal into 16 bit's for x (normal.x & normal.y) and y-component (normal.z & normal.w), preserve sign of z-component.
*/
float4 PackViewSpaceNormal(float3 viewSpaceNormal)
{
    // make sure normal is actually normalized
    viewSpaceNormal = normalize(viewSpaceNormal);

    // use Stereographic Projection, to avoid saveing a sign bit
    // see http://aras-p.info/texts/CompactNormalStorage.html for further info
    float scale = 1.7777;
    float2 enc = viewSpaceNormal.xy / (viewSpaceNormal.z+1);
    enc /= scale;
    enc = enc * 0.5 + 0.5;
    
    // pack normal x and y
    float4 normal = pack_normalxy_into_rgba8(enc.x, enc.y);
    return normal;
}

//------------------------------------------------------------------------------
/**
    Unpack a view space normal packed with PackViewSpaceNormalPosition().
*/
float3 
UnpackViewSpaceNormal(in float4 packedValue)
{
    // unpack x and y of the normal
    float4 unpacked = unpack_normalxy_from_rgba8(packedValue);
    
    // packedValue is float4, with .rg containing encoded normal
    float scale = 1.7777;
    float3 nn = unpacked.xyz * float3(2 * scale, 2 * scale, 0) + float3(-scale, -scale, 1);
    float g = 2.0 / dot(nn.xyz, nn.xyz);
    float3 outViewSpaceNormal;
    outViewSpaceNormal.xy = g * nn.xy;
    outViewSpaceNormal.z = g - 1;    
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
float4 PackObjectDepth(in float ObjectId, in float NormalGroupId, in float depth)
{    
    float4 packed;
    packed.x = ObjectId;
    packed.y = NormalGroupId;
    // we need to multiply the depth by a factor, otherwise we would have a raster on small distances
    // normal minimal raster is 1 / 255 (0.00392)
    depth = depth * depthScale;
    pack_u16(depth, packed.z, packed.w);
    return packed;
}

//------------------------------------------------------------------------------
/**    
    Unpack Depth

    x: objectId
    y: NormalGroupId
    z: depth upper 8 bit
    w: depth lower 8 bit
*/
float UnpackDepth(in float4 packed)
{    
    return unpack_u16(packed.z, packed.w) / depthScale;
}

//------------------------------------------------------------------------------
/**
    Unpack only the view space normal packed with PackViewSpaceNormalPosition().
*/
void 
UnpackViewSpaceNormal(in float4 packedValue, out float3 outViewSpaceNormal)
{
    // restore view-space normal with correct z-direction   
    float scale = 1.7777;
    outViewSpaceNormal.xyz = packedValue.xyz * float3(2*scale, 2*scale, 0) + float3(-scale, -scale, 1);
    float g = 2.0 / dot(outViewSpaceNormal.xyz, outViewSpaceNormal.xyz);
    outViewSpaceNormal.xy = g * outViewSpaceNormal.xy;
    outViewSpaceNormal.z = g - 1;
}

//------------------------------------------------------------------------------
/**
    Compute a rim light intensity value.
*/
static const float RimIntensity = 0.9;//3.0;//
static const float RimPower = 2.0;
float RimLightIntensity(float3 worldNormal,     // surface normal in world space
                        float3 worldEyeVec)     // eye vector in world space
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
float4 
psLightMaterial(in float4 lightValues,                
               in float4 diffColor, 
               in float3 emsvColor, 
               in float emsvIntensity, 
               in float3 specColor,                    
               in float specIntensity)
{
    lightValues = DecodeHDR(lightValues);    
    float4 color = diffColor;
    // exagerate optional Rim
    color.xyz *= lightValues.xyz;    
    color.xyz += emsvColor * emsvIntensity;
    // color with diff color
    float3 normedColor = normalize(lightValues.xyz);
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
psComputeDepthFadeOut(in float myViewSpaceDepth, in float4 projPos, in sampler2D normDepthBuffer, in float fadeOutRange)
{   
    return 0.0f;
/*
    TODO  UnpackViewSpaceDepth & UnpackViewSpaceNormal changed 
    
    float backgroundDepth;
    float2 posDivW = projPos.xy / projPos.ww;
    float2 screenUv = posDivW * float2(0.5, -0.5) + 0.5;
    float4 packedValue = tex2D(normDepthBuffer, screenUv);
    UnpackViewSpaceDepth(packedValue, backgroundDepth);
    float3 backgroundNormal;
    UnpackViewSpaceNormal(packedValue, backgroundNormal);
    float diffDistance = backgroundDepth - myViewSpaceDepth;
    float VdotN = abs(backgroundNormal.z); // normal is in viewspace
    float normalConsideredFadeOut = lerp(fadeOutRange * fadeOutRange, fadeOutRange, VdotN); 
    float modAlpha = saturate(diffDistance * diffDistance / normalConsideredFadeOut); 
    return modAlpha;
*/
}

//-------------------------------------------------------------------------------------------------------------
/**
    Compute screen ccordinates 
*/
float2 
psComputeScreenCoord(in float2 screenPixel, in float2 pixelSize)
{
    float2 screenUv = (screenPixel.xy) * pixelSize.xy;
    return screenUv;
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
    Compute pixel size
*/
float2
GetPixelSize(in Texture2D tex)
{
	float2 pixelSize;
	tex.GetDimensions(pixelSize.x, pixelSize.y);
	pixelSize.xy = 1 / pixelSize.xy;
	return pixelSize;
}

//-------------------------------------------------------------------------------------------------------------
/**
    Compute texture ratio to current pixel size
*/
float2
GetTextureRatio(in Texture2D tex, float2 pixelSize)
{
	float2 textureSize;
	tex.GetDimensions(textureSize.x, textureSize.y);
	float2 currentTextureSize = 1 / pixelSize;
	return textureSize / currentTextureSize;
}



//------------------------------------------------------------------------------
#endif
