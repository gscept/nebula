//------------------------------------------------------------------------------
//  bloom.fx
//
//  (C) 2023 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "lib/shared.fxh"
#include "lib/techniques.fxh"

#define KERNEL_SIZE 8

sampler_state InputSampler
{
    Filter = Linear;
    AddressU = Clamp;
    AddressV = Clamp;
};

constant BloomUniforms
{
    int Mips;
    vec4 Resolutions[14];
};

render_state PostEffectState
{
    CullMode = Back;
    DepthEnabled = false;
    DepthWrite = false;
};

texture2D Input;
write r11g11b10f image2D BloomOutput;

groupshared vec3 SampleLookup[KERNEL_SIZE][KERNEL_SIZE];

//------------------------------------------------------------------------------
/**
*/
vec4 
CubicWeights(float v)
{
    vec4 n = vec4(1, 2, 3, 4) - v;
    vec4 s = n * n * n;
    float x = s.x;
    float y = s.y - 4 * s.x;
    float z = s.z - 4 * s.y + 6 * s.x;
    float w = 6 - x - y - z;
    return vec4(x, y, z, w) / 6.0f;
}

//------------------------------------------------------------------------------
/**
*/
vec3
Sample(vec2 pixel, int mip)
{
    vec2 coords = pixel * Resolutions[mip].xy - 0.5f;
    vec2 fxy = fract(coords);
    coords -= fxy;

    vec4 xcubic = CubicWeights(fxy.x);
    vec4 ycubic = CubicWeights(fxy.y);

    vec4 c = coords.xxyy + vec2(-0.5f, 1.5f).xyxy;
    vec4 s = vec4(xcubic.xz + xcubic.yw, ycubic.xz + ycubic.yw);
    vec4 offset = c + vec4(xcubic.yw, ycubic.yw) / s;

    offset *= Resolutions[mip].zzww;
    vec3 sample0 = textureLod(sampler2D(Input, InputSampler), offset.xz, mip).rgb;
    vec3 sample1 = textureLod(sampler2D(Input, InputSampler), offset.yz, mip).rgb;
    vec3 sample2 = textureLod(sampler2D(Input, InputSampler), offset.xw, mip).rgb;
    vec3 sample3 = textureLod(sampler2D(Input, InputSampler), offset.yw, mip).rgb;
    float sx = s.x / (s.x + s.y);
    float sy = s.z / (s.z + s.w);
    return lerp(lerp(sample3, sample2, sx), lerp(sample1, sample0, sx), sy);
}

//------------------------------------------------------------------------------
/**
*/
vec3
LoadLDS(ivec2 pixel)
{
    return SampleLookup[pixel.x][pixel.y];
}

//------------------------------------------------------------------------------
/**
*/
vec3 
SampleTentUpscale(ivec2 center)
{
    vec3 a = LoadLDS(center + ivec2(-1, 1));
    vec3 b = LoadLDS(center + ivec2(0, 1));
    vec3 c = LoadLDS(center + ivec2(1, 1));

    vec3 d = LoadLDS(center + ivec2(-1, 0));
    vec3 e = LoadLDS(center + ivec2(0, 0));
    vec3 f = LoadLDS(center + ivec2(1, 0));

    vec3 g = LoadLDS(center + ivec2(-1, -1));
    vec3 h = LoadLDS(center + ivec2(0, -1));
    vec3 i = LoadLDS(center + ivec2(1, -1));

    vec3 res = e * 4.0f;
    res += (b + d + f + h) * 2.0f;
    res += (a + c + g + i);
    return res / 16.0f;
}

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = KERNEL_SIZE
[local_size_y] = KERNEL_SIZE
shader
void
csUpscale()
{
    // Running a 16x16 kernel with a 3x3 kernel means we need a 1x1 extra radius
    const uvec2 tileStart = gl_WorkGroupID.xy * (KERNEL_SIZE - 2);
    const uvec2 tileEnd = tileStart + uvec2(KERNEL_SIZE - 2);

    // The apron defines which pixels we will sample and load into LDS
    const uvec2 apronStart = tileStart - uvec2(1);
    const uvec2 apronEnd = tileEnd + uvec2(1);
    const uvec2 sampleCoord = apronStart + gl_LocalInvocationID.xy;

    uvec2 outputPixel = tileStart + gl_LocalInvocationID.xy;

    const uvec2 tileEndClamped = min(tileEnd, uvec2(Resolutions[0].xy));

    // Only run waves on pixels within the target resolution
    vec3 sum = vec3(0);
    bool pixelOutputMask = all(lessThan(outputPixel, tileEndClamped));
    vec2 mippedDimensions = Resolutions[0].xy;
    vec2 sampleUV = vec2(sampleCoord) / mippedDimensions;

    const float weights[14] = { 0.3465, 0.138, 0.1176, 0.066, 0.066, 0.061, 0.061, 0.055, 0.055, 0.050, 0.050, 0.045, 0.045, 0.040 };

    for (int i = 0; i < Mips; i++)
    {
        // All waves load into LDS
        SampleLookup[gl_LocalInvocationID.x][gl_LocalInvocationID.y] = Sample(sampleUV, i);

        // Only the 14x14 kernel inside the padded one runs the tent filter
        if (pixelOutputMask)
        {
            memoryBarrierShared();
            sum += SampleTentUpscale(ivec2(gl_LocalInvocationID.xy) + ivec2(1)) * weights[i];
        }
    }

    if (pixelOutputMask)
    {
        imageStore(BloomOutput, ivec2(outputPixel.x, outputPixel.y), vec4(sum, 1));
    }
}


//------------------------------------------------------------------------------
/**
*/
program Bloom[ string Mask = "Bloom"; ]
{
    ComputeShader = csUpscale();
};

