
readwrite rgba32f image2D tex;
[localsizex] = 1
shader
void
ShaderMain()
{
	imageStore(tex, ivec2(gl_GlobalInvocationID.xy), vec4(min(gl_LocalInvocationID.x, 255)));
}


program Test
{
	ComputeShader = ShaderMain();
};