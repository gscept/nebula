//------------------------------------------------------------------------------
//  downsample_cs.fx
//  Parallel reduction downsample shader, based on https://github.com/GPUOpen-Effects/FidelityFX-SPD/blob/master/ffx-spd/ffx_spd.h
//  (C) 2021 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"

#define KERNEL_SIZE 256
#define SHARED_MEMORY_SIZE 16
#if !defined FORMAT || !defined IMAGE_DATA_TYPE
#define FORMAT rgba16f
#define IMAGE_DATA_TYPE vec4
#endif

#if ARRAY_TEXTURE
readwrite FORMAT image2DArray Output[13];
atomic FORMAT image2DArray Output6;
#else
readwrite FORMAT image2D Output[13];
atomic FORMAT image2D Output6;
#endif

group_shared IMAGE_DATA_TYPE SharedMemory[SHARED_MEMORY_SIZE][SHARED_MEMORY_SIZE];
group_shared uint Counter;

constant DownsampleUniforms
{
    uint Mips;
    uint NumGroups;
};

atomic rw_buffer AtomicCounter [string Visibility = "CS";]
{
    uint Counters[];     // one counter per slice
};


//------------------------------------------------------------------------------
/**
    Custom reduction function, set a define to change if we should use max, min or average value
*/
IMAGE_DATA_TYPE
Reduce(IMAGE_DATA_TYPE p0, IMAGE_DATA_TYPE p1, IMAGE_DATA_TYPE p2, IMAGE_DATA_TYPE p3)
{
    IMAGE_DATA_TYPE ret;
#if defined KERNEL_MAX
     ret = max(max(max(p0, p1), p2), p3);
#elif defined KERNEL_MIN
    ret = min(min(min(p0, p1), p2), p3);
#else // average value
    ret = (p0 + p1 + p2 + p3) * 0.25f;
#endif
    return ret;
}

//------------------------------------------------------------------------------
/**
    Sample a 2x2 grid and run the reduction function
*/
IMAGE_DATA_TYPE
Sample2x2(ivec2 texel, uint slice)
{
    IMAGE_DATA_TYPE samples[4];
#if ARRAY_TEXTURE    
    samples[0] = imageLoad(Output[0], ivec3(texel, slice));
    samples[1] = imageLoad(Output[0], ivec3(texel, slice) + ivec3(1, 0, 0));
    samples[2] = imageLoad(Output[0], ivec3(texel, slice) + ivec3(0, 1, 0));
    samples[3] = imageLoad(Output[0], ivec3(texel, slice) + ivec3(1, 1, 0));
#else
    samples[0] = imageLoad(Output[0], texel);
    samples[1] = imageLoad(Output[0], texel + ivec2(1, 0));
    samples[2] = imageLoad(Output[0], texel + ivec2(0, 1));
    samples[3] = imageLoad(Output[0], texel + ivec2(1, 1));
#endif
    return Reduce(samples[0], samples[1], samples[2], samples[3]);   
}

//------------------------------------------------------------------------------
/**
    Sample a 2x2 grid and run the reduction function
*/
IMAGE_DATA_TYPE
Sample2x2Output(ivec2 texel, uint slice)
{
    IMAGE_DATA_TYPE samples[4];
#if ARRAY_TEXTURE    
    samples[0] = imageLoad(Output6, ivec3(texel, slice));
    samples[1] = imageLoad(Output6, ivec3(texel, slice) + ivec3(1, 0, 0));
    samples[2] = imageLoad(Output6, ivec3(texel, slice) + ivec3(0, 1, 0));
    samples[3] = imageLoad(Output6, ivec3(texel, slice) + ivec3(1, 1, 0));
#else
    samples[0] = imageLoad(Output6, texel);
    samples[1] = imageLoad(Output6, texel + ivec2(1, 0));
    samples[2] = imageLoad(Output6, texel + ivec2(0, 1));
    samples[3] = imageLoad(Output6, texel + ivec2(1, 1));
#endif
    return Reduce(samples[0], samples[1], samples[2], samples[3]);
}

//------------------------------------------------------------------------------
/**
*/
void
Save(ivec2 texel, IMAGE_DATA_TYPE value, uint mip, uint slice)
{
#if ARRAY_TEXTURE
    if (mip == 5)
        imageStore(Output6, ivec3(texel, slice), value);
    else
        imageStore(Output[mip + 1], ivec3(texel, slice), value);
#else
    if (mip == 5)
        imageStore(Output6, texel, value);
    else
        imageStore(Output[mip + 1], texel, value);
#endif
}

//------------------------------------------------------------------------------
/**
*/
IMAGE_DATA_TYPE
LDSLoad(uint x, uint y)
{
    return SharedMemory[x][y];
}

//------------------------------------------------------------------------------
/**
*/
void
LDSStore(uint x, uint y, IMAGE_DATA_TYPE value)
{
    SharedMemory[x][y] = value;
}

//------------------------------------------------------------------------------
/**
    Use quad wave intrinsics to reduce by reading from neighbouring threads
*/
IMAGE_DATA_TYPE
ReduceQuad(IMAGE_DATA_TYPE pixel)
{
    IMAGE_DATA_TYPE p0 = pixel;
    IMAGE_DATA_TYPE p1 = subgroupQuadSwapHorizontal(pixel);
    IMAGE_DATA_TYPE p2 = subgroupQuadSwapVertical(pixel);
    IMAGE_DATA_TYPE p3 = subgroupQuadSwapDiagonal(pixel);
    return Reduce(p0, p1, p2, p3);
}

//------------------------------------------------------------------------------
/**
    Downsample mip 0 and 1
*/
void
Mips0_1(uint x, uint y, ivec2 workGroupId, uint localIndex, uint mips, uint slice)
{
    // we are taking 4 samples per thread
    IMAGE_DATA_TYPE pixels[4];

    // the indexing logic is as such:
    // we sample in a 8x8 grid, so the grid offset is the workGroupId * 8 * 8 = workGroupId * 64
    // the offset within the 8x8 grid is the x and y coordinate, multiplied by 2 since each sample is done in a 2x2 quad
    // each of the 4 samples are offset by half the grid size to account for upper left, upper right, lower left, lower right
    // the destination pixel then naturally is half of that (offset: 32, grid offset is 16 and index multiplier is 1)

    // every thread is offset by a 8x8 grid, so the offset is the group index * 64
    // then the offset within that grid is every other pixel, which is incremented by 32
    ivec2 sourceTexelIndex = workGroupId * 64 + ivec2(x * 2, y * 2);
    ivec2 targetTexelIndex = workGroupId * 32 + ivec2(x, y);
    pixels[0] = Sample2x2(sourceTexelIndex, slice);
    Save(targetTexelIndex, pixels[0], 0, slice);

    sourceTexelIndex = workGroupId * 64 + ivec2(x * 2 + 32, y * 2);
    targetTexelIndex = workGroupId * 32 + ivec2(x + 16, y);
    pixels[1] = Sample2x2(sourceTexelIndex, slice);
    Save(targetTexelIndex, pixels[1], 0, slice);

    sourceTexelIndex = workGroupId * 64 + ivec2(x * 2, y * 2 + 32);
    targetTexelIndex = workGroupId * 32 + ivec2(x, y + 16);
    pixels[2] = Sample2x2(sourceTexelIndex, slice);
    Save(targetTexelIndex, pixels[2], 0, slice);

    sourceTexelIndex = workGroupId * 64 + ivec2(x * 2 + 32, y * 2 + 32);
    targetTexelIndex = workGroupId * 32 + ivec2(x + 16, y + 16);
    pixels[3] = Sample2x2(sourceTexelIndex, slice);
    Save(targetTexelIndex, pixels[3], 0, slice);

    // in case we don't have more than 1 mip in the chain, abort early
    if (mips <= 1)
        return;

    // now all we have to do is to reduce the same pixels using subgroup intrinsics to get neighbouring thread values!
    pixels[0] = ReduceQuad(pixels[0]);
    pixels[1] = ReduceQuad(pixels[1]);
    pixels[2] = ReduceQuad(pixels[2]);
    pixels[3] = ReduceQuad(pixels[3]);

    // only store value for every fourth pixel
    if ((localIndex % 4) == 0)
    {
        // now the grid offset is going to be 16, the stride multiplier / 2 and stride offset 8
        // also store the value in shared memory for the next mips
        ivec2 gridOffset = ivec2(x / 2, y / 2);
        targetTexelIndex = workGroupId * 16 + gridOffset;
        Save(targetTexelIndex, pixels[0], 1, slice);
        LDSStore(gridOffset.x, gridOffset.y, pixels[0]);

        gridOffset = ivec2(x / 2 + 8, y / 2);
        targetTexelIndex = workGroupId * 16 + gridOffset;
        Save(targetTexelIndex, pixels[1], 1, slice);
        LDSStore(gridOffset.x, gridOffset.y, pixels[1]);

        gridOffset = ivec2(x / 2, y / 2 + 8);
        targetTexelIndex = workGroupId * 16 + gridOffset;
        Save(targetTexelIndex, pixels[2], 1, slice);
        LDSStore(gridOffset.x, gridOffset.y, pixels[2]);

        gridOffset = ivec2(x / 2 + 8, y / 2 + 8);
        targetTexelIndex = workGroupId * 16 + gridOffset;
        Save(targetTexelIndex, pixels[3], 1, slice);
        LDSStore(gridOffset.x, gridOffset.y, pixels[3]);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Mip2_8(uint x, uint y, ivec2 workGroupId, uint localIndex, uint mip, uint slice)
{
    IMAGE_DATA_TYPE pixel = LDSLoad(x, y);
    pixel = ReduceQuad(pixel);

    if ((localIndex % 4) == 0)
    {
        ivec2 targetTexelIndex = workGroupId * 8 + ivec2(x/2, y/2);
        Save(targetTexelIndex, pixel, mip, slice);
        LDSStore(x + (y/2) % 2, y, pixel);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Mip3_9(uint x, uint y, ivec2 workGroupId, uint localIndex, uint mip, uint slice)
{
    IMAGE_DATA_TYPE pixel = LDSLoad(x * 2 + y % 2, y * 2);
    pixel = ReduceQuad(pixel);

    if ((localIndex % 4) == 0)
    {
        ivec2 targetTexelIndex = workGroupId * 4 + ivec2(x/2, y/2);
        Save(targetTexelIndex, pixel, mip, slice);
        LDSStore(x * 2 + y/2, y * 2, pixel);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Mip4_10(uint x, uint y, ivec2 workGroupId, uint localIndex, uint mip, uint slice)
{
    IMAGE_DATA_TYPE pixel = LDSLoad(x * 4 + y, y * 4);
    pixel = ReduceQuad(pixel);

    if ((localIndex % 4) == 0)
    {
        ivec2 targetTexelIndex = workGroupId * 2 + ivec2(x/2, y/2);
        Save(targetTexelIndex, pixel, mip, slice);
        LDSStore(x / 2 + y, 0, pixel);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Mip5_11(ivec2 workGroupId, uint localIndex, uint mip, uint slice)
{
    IMAGE_DATA_TYPE pixel = LDSLoad(localIndex, 0);
    pixel = ReduceQuad(pixel);

    if ((localIndex % 4) == 0)
    {
        ivec2 targetTexelIndex = workGroupId;
        Save(targetTexelIndex, pixel, mip, slice);

        // for mip 5 and 11, don't save to LDS
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Mips2_5_and_8_11(uint x, uint y, ivec2 workGroupId, uint localIndex, uint mip, uint mips, uint slice)
{
    if (mips <= mip)
        return;
    groupMemoryBarrier();
    barrier();
    Mip2_8(x, y, workGroupId, localIndex, mip, slice);

    if (localIndex < 64)
    {
        if (mips <= mip + 1)
            return;
        groupMemoryBarrier();
        barrier();
        Mip3_9(x, y, workGroupId, localIndex, mip + 1, slice);

        if (localIndex < 16)
        {
            if (mips <= mip + 2)
                return;
            groupMemoryBarrier();
            barrier();
            Mip4_10(x, y, workGroupId, localIndex, mip + 2, slice);

            if (localIndex < 4)
            {
                if (mips <= mip + 3)
                    return;
                groupMemoryBarrier();
                barrier();
                Mip5_11(workGroupId, localIndex, mip + 3, slice);
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Mips6_7(uint x, uint y, uint mips, uint slice)
{
    // we are taking 4 samples per thread
    IMAGE_DATA_TYPE pixels[4];

    ivec2 sourceTexelIndex = ivec2(x * 4 + 0, y * 4 + 0);
    ivec2 targetTexelIndex = ivec2(x * 2 + 0, y * 2 + 0);
    pixels[0] = Sample2x2Output(sourceTexelIndex, slice);
    Save(targetTexelIndex, pixels[0], 6, slice);

    sourceTexelIndex = ivec2(x * 4 + 2, y * 4 + 0);
    targetTexelIndex = ivec2(x * 2 + 1, y * 2 + 0);
    pixels[1] = Sample2x2Output(sourceTexelIndex, slice);
    Save(targetTexelIndex, pixels[1], 6, slice);

    sourceTexelIndex = ivec2(x * 4 + 0, y * 4 + 2);
    targetTexelIndex = ivec2(x * 2 + 0, y * 2 + 1);
    pixels[2] = Sample2x2Output(sourceTexelIndex, slice);
    Save(targetTexelIndex, pixels[2], 6, slice);

    sourceTexelIndex = ivec2(x * 4 + 2, y * 4 + 2);
    targetTexelIndex = ivec2(x * 2 + 1, y * 2 + 1);
    pixels[3] = Sample2x2Output(sourceTexelIndex, slice);
    Save(targetTexelIndex, pixels[3], 6, slice);

    // skip mip 7 if we don't have one
    if (mips <= 7)
        return;

    IMAGE_DATA_TYPE pixel = Reduce(pixels[0], pixels[1], pixels[2], pixels[3]);
    Save(ivec2(x, y), pixel, 7, slice);
    LDSStore(x, y, pixel);
}

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = KERNEL_SIZE
shader
void
csMain()
{
    ivec2 groupId = ivec2(gl_WorkGroupID.xy);
    uint localIndex = gl_LocalInvocationIndex;
    int sliceIndex = int(gl_WorkGroupID.z);
    uvec2 gridIndex = MortonCurve8x8(localIndex % 64);
    uint x = gridIndex.x + 8 * ((localIndex >> 6) % 2);
    uint y = gridIndex.y + 8 * (localIndex >> 7);

    // reduce mips 0 and 1
    Mips0_1(x, y, groupId, localIndex, Mips, sliceIndex);

    // when that is done, we have the pixels in LDS and can sample from there efficiently
    Mips2_5_and_8_11(x, y, groupId, localIndex, 2, Mips, sliceIndex);

    if (Mips < 7)
        return;

    // filter out the last work group by incrementing an atomic counter 
    // and allowing only the last work group to proceed, we only want 
    // one work group to do mips 6-11
    if (localIndex == 0)
        Counter = atomicAdd(Counters[sliceIndex], 1);

    groupMemoryBarrier();
    barrier();
    if (Counter != (NumGroups - 1))
        return;
    Counters[sliceIndex] = 0;

    // reduce mips 6 and 7
    Mips6_7(x, y, Mips, sliceIndex);

    // same as with mips 0 and 1, we store pixels from 6_7 to LDS so we can reduce the rest
    // directly from LDS
    Mips2_5_and_8_11(x, y, ivec2(0, 0), localIndex, 8, Mips, sliceIndex);
}

//------------------------------------------------------------------------------
/**
*/
program Downsample [ string Mask = "Downsample"; ]
{
    ComputeShader = csMain();
};