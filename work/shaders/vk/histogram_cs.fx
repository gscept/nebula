//------------------------------------------------------------------------------
//  histogram_cs.fx
//  (C) 2021 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"

sampler2D ColorSource;

rw_buffer ColorValueCounters [ string Visibility = "CS"; ]
{
    // 
    uint Counters[255];
};

constant HistogramConstants [ string Visibility = "CS"; ]
{
    ivec2 WindowOffset;
    ivec2 TextureSize;
    int Mip;
};

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 64
shader
void
csMain()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    pixel += WindowOffset;

    // don't sample from outside the texture
    if (all(lessThan(pixel, TextureSize)))
    {
        vec4 color = texelFetch(ColorSource, pixel, Mip);
        float luminance = dot(color, Luminance);

        // floor the value of luminance scaled to ubyte to find which bucket it belongs in
        int bucket = int(floor(luminance * 255.0f));
        atomicAdd(Counters[bucket], 1);
    }
}

//------------------------------------------------------------------------------
/**
*/
program HistogramCategorize [ string Mask = "HistogramCategorize"; ]
{
    ComputeShader = csMain();
};