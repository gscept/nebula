//------------------------------------------------------------------------------
//  compute.fxh
//  (C) 2016 Gustav Sterbrant
//
//	Contains shared functions used by compute shaders
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
	Generate texture coordinates for a X-wise kernel.
*/
void 
ComputePixelX(const uint tileWidth, const uint kernelSize, const uvec2 group, const uvec2 local, out uint x, out uint y, out uint min, out uint max)
{
	const uint         tileStart = group.x * tileWidth;
	const uint           tileEnd = tileStart + tileWidth;
	const uint        apronStart = tileStart - kernelSize;
	const uint          apronEnd = tileEnd   + kernelSize;
	
	x = apronStart + local.x;
	y = group.y;
	min = tileStart;
	max = tileEnd;
}

//------------------------------------------------------------------------------
/**
	Generate texture coordinates for a Y-wise kernel.
*/
void 
ComputePixelY(const uint tileWidth, const uint kernelSize, const uvec2 group, const uvec2 local, out uint x, out uint y, out uint min, out uint max)
{
	const uint         tileStart = group.x * tileWidth;
	const uint           tileEnd = tileStart + tileWidth;
	const uint        apronStart = tileStart - kernelSize;
	const uint          apronEnd = tileEnd   + kernelSize;
	
	x = group.y;
	y = apronStart + local.x;
	min = tileStart;
	max = tileEnd;
}