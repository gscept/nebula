//------------------------------------------------------------------------------
//  lights_tiled.fx
//  (C) 2016 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"

#define TILE_SQUARE_SIZE 16
#define MAX_LIGHTS_PER_TILE 32

#define LIGHT_TYPE__SPOTLIGHT 0
#define LIGHT_TYPE__POINTLIGHT 1
#define LIGHT_TYPE__AREALIGHT 2

// note, this is just the information we require from the light to perform the tiling, and doesn't contain the lights themselves
struct Light
{
	int type;		// type of light, look at above definitions for a mapping
	vec4 position;	// world space position of light
	vec4 forward;	// forward vector of light (spotlight and arealights)
	float radius;	// radius of sphere (pointlight) or cone angle (spotlight)
};

// do not modify this one, keep it the same, its being fed through the lightserver
varbuffer Input
{
	Light lights[];
};

// this is the output list, pointing to an index i in the Input.lights buffer and next to the next element in Output.list. 
struct LightTileList
{
	uint lightIndex[MAX_LIGHTS_PER_TILE];
};

// this is the buffer we want to modify!
varbuffer Output
{
	LightTileList list[];
};

varblock Uniforms
{
	int NumInputLights = 0;
	uvec2 FramebufferDimensions;
};

//------------------------------------------------------------------------------
/**
*/
bool
RaySphereIntersection(in vec4 start, in vec4 line, in vec4 sphere, in float radius, out float dt)
{
	// create ray from start to sphere, if dot with line is bigger than 0, great!
	vec4 dd = sphere - start;
	dt = dot(dd, line);
	float radsq = radius*radius;
	float dtsq = dt * dt;
	
	// if dt is bigger than 0, then sphere is ahead of start in line direction
	// if the squared radius is smaller than the squared projection, then the sphere must intersect with line
	return dt > 0 && radsq <= dtsq;
}

//------------------------------------------------------------------------------
/**
*/
bool
RaySphereIntersection(in vec4 start, in vec4 stop, in vec4 sphere, in float radius)
{
	// create line between start and stop
	vec4 line = stop - start;
	
	// create vector between sphere center and start/stop individually
	vec4 dd = sphere - start;
	vec4 ds = sphere - stop;
	
	// project vectors to line
	float dt = dot(dd, line);
	float dt2 = dot(ds, line);
	
	float radsq = radius*radius;
	float dtsq = dt * dt;
	float dt2sq = dt2 * dt2;
	
	// if dt is bigger than 0, then sphere is ahead of start in line direction
	// if the squared radius is smaller than the squared projection, then the sphere must intersect with line
	return dt > 0 && 				// sphere center is ahead of start
		   dt2 < 0 && 				// sphere center is behind stop
		   radsq <= dtsq && 		// distance between start and center is less than radius
		   radsq <= dt2sq;			// distance between stop and center is less than radius
}

//------------------------------------------------------------------------------
/**
*/
bool
RayConeIntersection(in vec4 start, in vec4 stop, in vec4 centerline, in float radius)
{
	// create line between start and stop
	vec4 line = stop - start;
	
	// create vector between sphere center and start/stop individually
	//vec4 dd = sphere - start;
	//vec4 ds = sphere - stop;
	
	// project vectors to line
	//float dt = dot(dd, line);
	//float dt2 = dot(ds, line);
	return false;
}

//------------------------------------------------------------------------------
/**
	Calculate an exponential depth based on the slice and number of slices.
	The range of the value generated will be between 0..1, then multiplied by Z-far
*/
float
CalculateLogDepth(uint slice, uint numslices, float zfar)
{
	const float expo = 100.0f;
	float slicerange = slice / float(numslices-1);
	return ((pow(expo, slicerange) - 1) / (slicerange - 1)) * zfar;
}

groupshared uint mindepth;
groupshared uint maxdepth;
groupshared uint tilelights;
groupshared LightTileList tilelightindices[MAX_LIGHTS_PER_TILE];

//------------------------------------------------------------------------------
/**
*/
[localsizex] = TILE_SQUARE_SIZE
[localsizey] = TILE_SQUARE_SIZE
shader 
void csTileCulling()
{
	// reset min/max depth to outer limits
	bool first = false;
	if (gl_LocalInvocationID.x == 0 && gl_LocalInvocationID.y == 0)
	{
		atomicExchange(mindepth, 0xFFFFFFFF);
		atomicExchange(maxdepth, 0);
		atomicExchange(tilelights, 0);
		first = true;
	}
	groupMemoryBarrier();
	
	// calculate pixel coordinate and tile
	ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
	vec2 tile = vec2(gl_WorkGroupID.xy * gl_WorkGroupSize.xy) / vec2(FramebufferDimensions);
	uint tileIndex = gl_WorkGroupID.x % FramebufferDimensions.x + gl_WorkGroupID.y / FramebufferDimensions.y;
	
	// fetch depth and perform min-max calculation, convert depth to uint first
	float d = fetch2D(DepthBuffer, Basic2DSampler, pixel, 0).r;
	uint depth = uint(d * 0xFFFFFFFF);
	
	// calculate atomic min-max
	atomicMin(mindepth, depth);
	atomicMax(maxdepth, depth);
	
	// block other threads in group to finish first
	groupMemoryBarrier();
	
	// reconstruct min-max Z
	float near = float(mindepth / float(0xFFFFFFFF));
	float far = float(maxdepth / float(0xFFFFFFFF));
	
	// calculate rays on each edge of the frustum
	vec4 raybegin = vec4(EyePos.x, EyePos.y, mindepth, 0);
	uvec2 offset = gl_LocalInvocationID.xy;
	vec4 rays[4];
	rays[0] = (vec4((pixel.x + offset.x) / 0.5f, 	pixel.y, 						maxdepth, 0));	// top
	rays[1] = (vec4(pixel.x, 						(pixel.y - offset.y) / 0.5f, 	maxdepth, 0));	// left
	rays[2] = (vec4((pixel.x + offset.x) / 0.5f, 	pixel.y - offset.y, 			maxdepth, 0));	// bottom
	rays[3] = (vec4(pixel.x + offset.x, 			(pixel.y - offset.y) / 0.5f, 	maxdepth, 0));	// right
	
	// make into world space
	rays[0] = (InvViewProjection * rays[0]) - raybegin;
	rays[1] = (InvViewProjection * rays[1]) - raybegin;
	rays[2] = (InvViewProjection * rays[2]) - raybegin;
	rays[3] = (InvViewProjection * rays[3]) - raybegin;
	
	// for all lights
	const int NumThreadsPerTile = TILE_SQUARE_SIZE * TILE_SQUARE_SIZE;
	for (uint i = 0; i < NumInputLights && i < MAX_LIGHTS_PER_TILE; i += NumThreadsPerTile)
	{
		int il = int(gl_LocalInvocationIndex + i);
		if (il < NumInputLights)
		{
			const Light light = lights[il]; 
			
			// for all rays, might end early
			for (uint j = 0; j < 4; j++)
			{
				float dt;
				if (light.type == LIGHT_TYPE__POINTLIGHT)
				{
					if (RaySphereIntersection(raybegin, rays[j], light.position, light.radius, dt))
					{
						// increment index atomically, this ensures 
						uint id = atomicAdd(tilelights, 1);
						tilelightindices[tileIndex].lightIndex[id] = il;
						//tilelightindices[id].lightIndex = il;
						break;
					}
				}
				else if (light.type == LIGHT_TYPE__SPOTLIGHT)
				{
					// implement me!
				}
			}
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
program Tile [ string Mask = "Alt0"; ]
{
	ComputeShader = csTileCulling();
};
