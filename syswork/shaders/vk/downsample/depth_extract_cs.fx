
sampler2D ZBufferInput;
write r32f image2D DepthOutput;

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 64
shader
void
csMain()
{
    float depth = texelFetch(ZBufferInput, ivec2(gl_GlobalInvocationID.xy), 0).r;
    imageStore(DepthOutput, ivec2(gl_GlobalInvocationID.xy), vec4(depth));
}

//------------------------------------------------------------------------------
/**
*/
program Extract [ string Mask = "Extract"; ]
{
    ComputeShader = csMain();
};
