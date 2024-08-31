//------------------------------------------------------------------------------
//  @file bccompression.fxh
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

// Code from https://github.com/microsoft/Xbox-ATG-Samples/blob/main/XDKSamples/Graphics/FastBlockCompress/Shaders/BlockCompress.hlsli

//--------------------------------------------------------------------------------------
// Name: GetMinMaxRGB
// Desc: Get the min and max of three channels (RGB)
//--------------------------------------------------------------------------------------
void GetMinMaxRGB(vec3 colorBlock[16], out vec3 minColor, out vec3 maxColor)
{
    minColor = colorBlock[0];
    maxColor = colorBlock[0];

    for (int i = 1; i < 16; ++i)
    {
        minColor = min(minColor, colorBlock[i]);
        maxColor = max(maxColor, colorBlock[i]);
    }
}

//--------------------------------------------------------------------------------------
// Name: GetMinMaxChannel
// Desc: Get the min and max of a single channel
//--------------------------------------------------------------------------------------
void GetMinMaxChannel(float block[16], out float minC, out float maxC)
{
    minC = block[0];
    maxC = block[0];

    for (int i = 1; i < 16; ++i)
    {
        minC = min(minC, block[i]);
        maxC = max(maxC, block[i]);
    }
}

//--------------------------------------------------------------------------------------
// Name: GetMinMaxUV
// Desc: Get the min and max of two channels (UV)
//--------------------------------------------------------------------------------------
void GetMinMaxUV(float blockU[16], float blockV[16], out float minU, out float maxU, out float minV, out float maxV)
{
    minU = blockU[0];
    maxU = blockU[0];
    minV = blockV[0];
    maxV = blockV[0];

    for (int i = 1; i < 16; ++i)
    {
        minU = min(minU, blockU[i]);
        maxU = max(maxU, blockU[i]);
        minV = min(minV, blockV[i]);
        maxV = max(maxV, blockV[i]);
    }
}

//--------------------------------------------------------------------------------------
// Name: InsetMinMaxRGB
// Desc: Slightly inset the min and max color values to reduce RMS error.
//      This is recommended by van Waveren & Castano, "Real-Time YCoCg-DXT Compression"
//      http://www.nvidia.com/object/real-time-ycocg-dxt-compression.html
//--------------------------------------------------------------------------------------
void InsetMinMaxRGB(inout vec3 minColor, inout vec3 maxColor, float colorScale)
{
    // Since we have four points, (1/16) * (max-min) will give us half the distance between
    //  two points on the line in color space
    vec3 offset = (1.0f / 16.0f) * (maxColor - minColor);

    // After applying the offset, we want to round up or down to the next integral color value (0 to 255)
    colorScale *= 255.0f;
    maxColor = ceil((maxColor - offset) * colorScale) / colorScale;
    minColor = floor((minColor + offset) * colorScale) / colorScale;
}

//--------------------------------------------------------------------------------------
// Name: GetIndicesRGB
// Desc: Calculate the BC block indices for each color in the block
//--------------------------------------------------------------------------------------
uint GetIndicesRGB(vec3 block[16], vec3 minColor, vec3 maxColor)
{
    uint indices = 0;

    // For each input color, we need to select between one of the following output colors:
    //  0: maxColor
    //  1: (2/3)*maxColor + (1/3)*minColor
    //  2: (1/3)*maxColor + (2/3)*minColor
    //  3: minColor  
    //
    // We essentially just project (block[i] - maxColor) onto (minColor - maxColor), but we pull out
    //  a few constant terms.
    vec3 diag = minColor - maxColor;
    float stepInc = 3.0f / dot(diag, diag); // Scale up by 3, because our indices are between 0 and 3
    diag *= stepInc;
    float c = stepInc * (dot(maxColor, maxColor) - dot(maxColor, minColor));

    for (int i = 15; i >= 0; --i)
    {
        // Compute the index for this block element
        uint index = uint(round(dot(block[i], diag) + c));

        // Now we need to convert our index into the somewhat unintuivive BC1 indexing scheme:
        //  0: maxColor
        //  1: minColor
        //  2: (2/3)*maxColor + (1/3)*minColor
        //  3: (1/3)*maxColor + (2/3)*minColor
        //
        // The mapping is:
        //  0 -> 0
        //  1 -> 2
        //  2 -> 3
        //  3 -> 1
        //
        // We can perform this mapping using bitwise operations, which is faster
        //  than predication or branching as long as it doesn't increase our register
        //  count too much. The mapping in binary looks like:
        //  00 -> 00
        //  01 -> 10
        //  10 -> 11
        //  11 -> 01
        //
        // Splitting it up by bit, the output looks like:
        //  bit1_out = bit0_in XOR bit1_in
        //  bit0_out = bit1_in 
        uint bit0_in = index & 1;
        uint bit1_in = index >> 1;
        indices |= ((bit0_in ^ bit1_in) << 1) | bit1_in;

        if (i != 0)
        {
            indices <<= 2;
        }
    }

    return indices;
}

//--------------------------------------------------------------------------------------
// Name: GetIndicesAlpha
// Desc: Calculate the BC block indices for an alpha channel
//--------------------------------------------------------------------------------------
void GetIndicesAlpha(float block[16], float minA, float maxA, inout uvec2 packed)
{
    float d = minA - maxA;
    float stepInc = 7.0f / d;

    // Both packed.x and packed.y contain index values, so we need two loops

    uint index = 0;
    uint shift = 16;
    for (int i = 0; i < 6; ++i)
    {
        // For each input alpha value, we need to select between one of eight output values
        //  0: maxA
        //  1: (6/7)*maxA + (1/7)*minA
        //  ...
        //  6: (1/7)*maxA + (6/3)*minA
        //  7: minA  
        index = uint(round(stepInc * (block[i] - maxA)));

        // Now we need to convert our index into the BC indexing scheme:
        //  0: maxA
        //  1: minA
        //  2: (6/7)*maxA + (1/7)*minA
        //  ...
        //  7: (1/7)*maxA + (6/3)*minA
        index += uint(index > 0u) - (7u * uint(index == 7u));

        packed.x |= (index << shift);
        shift += 3;
    }

    // The 6th index straddles the two uints
    packed.y |= (index >> 1);

    shift = 2;
    for (int i = 6; i < 16; ++i)
    {
        index = uint(round((block[i] - maxA) * stepInc));
        index += uint(index > 0) - (7 * uint(index == 7));

        packed.y |= (index << shift);
        shift += 3;
    }
}

//--------------------------------------------------------------------------------------
// Name: ColorTo565
// Desc: Pack a 3-component color into a uint
//--------------------------------------------------------------------------------------
uint ColorTo565(vec3 color)
{
    uvec3 rgb = uvec3(round(color * vec3(31.0f, 63.0f, 31.0f)));
    return (rgb.r << 11) | (rgb.g << 5) | rgb.b;
}

//--------------------------------------------------------------------------------------
// Name: CompressBC3Block
// Desc: Compress a BC3 block. valueScale is a scale value to be applied to the input 
//          values; this used as an optimization when compressing two mips at a time.
//          When compressing only a single mip, valueScale is always 1.0
//--------------------------------------------------------------------------------------
uvec4 CompressBC3Block(vec3 blockRGB[16], float blockA[16], float valueScale)
{
    vec3 minColor, maxColor;
    float minA, maxA;
    GetMinMaxRGB(blockRGB, minColor, maxColor);
    GetMinMaxChannel(blockA, minA, maxA);

    // Inset the min and max color values. We don't inset the alpha values
    //  because, while it may reduce the RMS error, it has a tendency to turn
    //  fully opaque texels partially transparent, which is probably not desirable.
    InsetMinMaxRGB(minColor, maxColor, valueScale);

    // Pack our colors and alpha values into uints
    uint minColor565 = ColorTo565(valueScale * minColor);
    uint maxColor565 = ColorTo565(valueScale * maxColor);
    uint minAPacked = uint(round(minA * valueScale * 255.0f));
    uint maxAPacked = uint(round(maxA * valueScale * 255.0f));

    uint indices = 0;
    if (minColor565 < maxColor565)
    {
        indices = GetIndicesRGB(blockRGB, minColor, maxColor);
    }

    uvec2 outA = uvec2((minAPacked << 8) | maxAPacked, 0);
    if (minAPacked < maxAPacked)
    {
        GetIndicesAlpha(blockA, minA, maxA, outA);
    }

    return uvec4(outA.x, outA.y, (minColor565 << 16) | maxColor565, indices);
}


//--------------------------------------------------------------------------------------
// Name: CompressBC5Block
// Desc: Compress a BC5 block. valueScale is a scale value to be applied to the input 
//          values; this used as an optimization when compressing two mips at a time.
//          When compressing only a single mip, valueScale is always 1.0
//--------------------------------------------------------------------------------------
uvec4 CompressBC5Block(float blockU[16], float blockV[16], float valueScale)
{
    float minU, maxU, minV, maxV;
    GetMinMaxUV(blockU, blockV, minU, maxU, minV, maxV);

    // Pack our min and max uv values
    uint minUPacked = uint(round(minU * valueScale * 255.0f));
    uint maxUPacked = uint(round(maxU * valueScale * 255.0f));
    uint minVPacked = uint(round(minV * valueScale * 255.0f));
    uint maxVPacked = uint(round(maxV * valueScale * 255.0f));

    uvec2 outU = uvec2((minUPacked << 8) | maxUPacked, 0);
    uvec2 outV = uvec2((minVPacked << 8) | maxVPacked, 0);

    if (minUPacked < maxUPacked)
    {
        GetIndicesAlpha(blockU, minU, maxU, outU);
    }

    if (minVPacked < maxVPacked)
    {
        GetIndicesAlpha(blockV, minV, maxV, outV);
    }

    return uvec4(outU.x, outU.y, outV.x, outV.y);
}

//--------------------------------------------------------------------------------------
// Name: TexelToUV
// Desc: Convert from a texel to the UV coordinates used in a Gather call
//--------------------------------------------------------------------------------------
vec2 TexelToUV(vec2 texel, float oneOverTextureWidth)
{
    // We Gather from the bottom-right corner of the texel
    return (texel + 1.0f) * oneOverTextureWidth;
}


//--------------------------------------------------------------------------------------
// Name: LoadTexelsRGB
// Desc: Load the 16 RGB texels that form a block
//--------------------------------------------------------------------------------------
void LoadTexelsRGB(sampler2D tex, float oneOverTextureWidth, uvec2 threadIDWithinDispatch, out vec3 block[16])
{
    vec2 uv = TexelToUV(vec2(threadIDWithinDispatch * 4), oneOverTextureWidth);

    vec4 red = textureGatherOffset(tex, uv, ivec2(0 ,0), 0);
    vec4 green = textureGatherOffset(tex, uv, ivec2(0, 0), 1);
    vec4 blue = textureGatherOffset(tex, uv, ivec2(0, 0), 2);
    block[0] = vec3(red[3], green[3], blue[3]);
    block[1] = vec3(red[2], green[2], blue[2]);
    block[4] = vec3(red[0], green[0], blue[0]);
    block[5] = vec3(red[1], green[1], blue[1]);

    red = textureGatherOffset(tex, uv, ivec2(2, 0), 0);
    green = textureGatherOffset(tex, uv, ivec2(2, 0), 1);
    blue = textureGatherOffset(tex, uv, ivec2(2, 0), 2);
    block[2] = vec3(red[3], green[3], blue[3]);
    block[3] = vec3(red[2], green[2], blue[2]);
    block[6] = vec3(red[0], green[0], blue[0]);
    block[7] = vec3(red[1], green[1], blue[1]);

    red = textureGatherOffset(tex, uv, ivec2(0, 2), 0);
    green = textureGatherOffset(tex, uv, ivec2(0, 2), 1);
    blue = textureGatherOffset(tex, uv, ivec2(0, 2), 2);
    block[8] = vec3(red[3], green[3], blue[3]);
    block[9] = vec3(red[2], green[2], blue[2]);
    block[12] = vec3(red[0], green[0], blue[0]);
    block[13] = vec3(red[1], green[1], blue[1]);

    red = textureGatherOffset(tex, uv, ivec2(2, 2), 0);
    green = textureGatherOffset(tex, uv, ivec2(2, 2), 1);
    blue = textureGatherOffset(tex, uv, ivec2(2, 2), 2);
    block[10] = vec3(red[3], green[3], blue[3]);
    block[11] = vec3(red[2], green[2], blue[2]);
    block[14] = vec3(red[0], green[0], blue[0]);
    block[15] = vec3(red[1], green[1], blue[1]);
}