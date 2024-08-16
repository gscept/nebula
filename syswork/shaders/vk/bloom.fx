//------------------------------------------------------------------------------
//  bloom.fx
//
//  (C) 2023 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "lib/shared.fxh"
#include "lib/techniques.fxh"

#define KERNEL_SIZE 16

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
write rgba16f image2D BloomOutput;
texture2D Intermediate;
readwrite rgba16f image2D BloomIntermediate[14];

groupshared vec3 SampleLookup[KERNEL_SIZE][KERNEL_SIZE];

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
[local_size_z] = 1
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

    const uvec2 tileEndClamped = min(tileEnd, uvec2(Resolutions[gl_WorkGroupID.z].xy));

    // Only run waves on pixels within the target resolution
    bool pixelOutputMask = all(lessThan(outputPixel, tileEndClamped));
    vec2 mippedDimensions = Resolutions[gl_WorkGroupID.z].xy;
    vec2 sampleUV = (vec2(sampleCoord) + vec2(0.5f)) / mippedDimensions;

    const float weights[14] = { 0.3465, 0.138, 0.1176, 0.066, 0.066, 0.061, 0.061, 0.055, 0.055, 0.050, 0.050, 0.045, 0.045, 0.040 };

    // All waves load into LDS
    SampleLookup[gl_LocalInvocationID.x][gl_LocalInvocationID.y] = SampleCubic(Input, InputSampler, Resolutions[gl_WorkGroupID.z], sampleUV, gl_WorkGroupID.z);
    barrier();

    // Only the 14x14 kernel inside the padded one runs the tent filter
    if (pixelOutputMask)
    {
        vec3 sum = SampleTentUpscale(ivec2(gl_LocalInvocationID.xy) + ivec2(1)) * weights[gl_WorkGroupID.z];
        imageStore(BloomIntermediate[gl_WorkGroupID.z], ivec2(outputPixel.x, outputPixel.y), vec4(sum, 0));
    }
}


//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 256
shader
void
csMerge()
{
    // Running a 16x16 kernel with a 3x3 kernel means we need a 1x1 extra radius
    const uvec2 tileStart = uvec2(gl_WorkGroupID.x * 256, gl_WorkGroupID.y);
    const uvec2 tileEnd = tileStart + uvec2(256, 1);

    uvec2 outputPixel = tileStart + gl_LocalInvocationID.xy;

    const uvec2 tileEndClamped = min(tileEnd, uvec2(Resolutions[0].xy));

    // Only run waves on pixels within the target resolution
    vec3 sum = vec3(0);
    bool pixelOutputMask = all(lessThan(outputPixel, tileEndClamped));
    vec2 uv = (vec2(outputPixel) + vec2(0.5f)) / Resolutions[0].xy;
    for (int i = 0; i < Mips; i++)
    {
        ivec2 inputPixel = ivec2(outputPixel) >> i;
        
        sum += textureLod(sampler2D(Intermediate, InputSampler), uv, i).rgb;
        //sum += imageLoad(BloomIntermediate[i], inputPixel).rgb;
    }
    
    if (pixelOutputMask)
    {
        vec3 weight = sum / Mips;
        imageStore(BloomOutput, ivec2(outputPixel.x, outputPixel.y), vec4(sum, saturate(dot(weight, weight)) * BloomIntensity));
    }
        
}

//------------------------------------------------------------------------------
/**
*/
program Bloom[ string Mask = "Intermediate"; ]
{
    ComputeShader = csUpscale();
};

program BloomMerge[ string Mask = "Merge"; ]
{
    ComputeShader = csMerge();
};

