//------------------------------------------------------------------------------
//  terrain.fx
//  (C) 2020 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"
#include "lib/techniques.fxh"

// this is used to keep track of how many lights we have active
group(BATCH_GROUP) constant TerrainUniforms [ string Visibility = "VS|HS|DS"; ]
{
	mat4 Transform;
	vec2 TerrainDimensions;
	float MinLODDistance;
	float MaxLODDistance;
	float MinTessellation;
	float MaxTessellation;
	float MinHeight;
	float MaxHeight;
	textureHandle HeightMap;
	textureHandle NormalMap;
	textureHandle DecisionMap;
};

sampler_state TextureSampler
{
	//Samplers = { Texture };
};

sampler_state DecisionSampler
{
	Filter = Point;
	//Samplers = { Texture };
};

//------------------------------------------------------------------------------
/**
	Tessellation terrain vertex shader
*/
shader
void
vsTerrain(
	[slot=0] in vec3 position,
	[slot=2] in vec2 uv,
	out vec4 Position,
	out vec2 UV,
	out float Tessellation) 
{
	vec4 modelSpace = Transform * vec4(position, 1);
	Position = modelSpace;

	float vertexDistance = distance( Position.xyz, EyePos.xyz );
	float factor = 1.0f - saturate((MinLODDistance - vertexDistance) / (MinLODDistance - MaxLODDistance));
	float decision = 1.0f - sample2DLod(DecisionMap, DecisionSampler, uv, 0).r;

	Tessellation = MinTessellation + factor * (MaxTessellation - MinTessellation) * decision;

	gl_Position = modelSpace;
	UV = uv;
}

//------------------------------------------------------------------------------
/**
	Tessellation terrain hull shader
*/
[inputvertices] = 3
[outputvertices] = 3
shader
void
hsTerrain(
	in vec4 position[],
	in vec2 uv[],
	in float tessellation[],
	out vec2 UV[],
	out vec4 Position[]) 
{
	Position[gl_InvocationID]	= position[gl_InvocationID];
	UV[gl_InvocationID]			= uv[gl_InvocationID];

	// provoking vertex gets to decide tessellation factors
	if (gl_InvocationID == 0)
	{
		vec4 EdgeTessFactors;
		EdgeTessFactors.x = 0.5 * (tessellation[1] + tessellation[2]);
		EdgeTessFactors.y = 0.5 * (tessellation[2] + tessellation[0]);
		EdgeTessFactors.z = 0.5 * (tessellation[0] + tessellation[1]);

		gl_TessLevelOuter[0] = EdgeTessFactors.x;
		gl_TessLevelOuter[1] = EdgeTessFactors.y;
		gl_TessLevelOuter[2] = EdgeTessFactors.z;
		gl_TessLevelInner[0] = gl_TessLevelOuter[0];
	}
}

//------------------------------------------------------------------------------
/**
	Tessellation terrain shader
*/
[inputvertices] = 3
[winding] = cw
[topology] = triangle
[partition] = odd
shader
void
dsTerrain(
	in vec2 uv[],
	in vec4 position[],
	out vec3 Normal,
	out vec2 UV) 
{
	vec3 Position = 
		gl_TessCoord.x * position[0].xyz + 
		gl_TessCoord.y * position[1].xyz + 
		gl_TessCoord.z * position[2].xyz;

	UV = 
		gl_TessCoord.x * uv[0] + 
		gl_TessCoord.y * uv[1] + 
		gl_TessCoord.z * uv[2];

	// sample height map
	float heightValue = sample2D(HeightMap, TextureSampler, UV).r;
	float height = MinHeight + heightValue * (MaxHeight - MinHeight);
	Position.y += height;
	vec2 normalSample = sample2D(NormalMap, TextureSampler, UV).ag;
	Normal.xy = (normalSample.xy * 2.0f) - 1.0f;
	Normal.z = saturate(sqrt(1.0f - dot(Normal.xy, Normal.xy)));
	Normal = Normal.xzy;

	gl_Position = ViewProjection * vec4(Position, 1);
}

//------------------------------------------------------------------------------
/**
	Simple terrain vertex shader
*/
shader
void
vsTerrainZ(
	[slot=0] in vec3 position,
	[slot=2] in vec2 uv,
	out vec2 UV) 
{
	vec4 modelSpace = Transform * vec4(position, 1);
	gl_Position = ViewProjection * modelSpace;
	UV = uv;
}

//------------------------------------------------------------------------------
/**
	Tessellation terrain hull shader
*/
[inputvertices] = 3
[outputvertices] = 3
shader
void
hsTerrainZ(
	in vec4 position[],
	in vec2 uv[],
	in float tessellation[],
	out vec2 UV[],
	out vec4 Position[]) 
{
	Position[gl_InvocationID]	= position[gl_InvocationID];
	UV[gl_InvocationID]			= uv[gl_InvocationID];

	// provoking vertex gets to decide tessellation factors
	if (gl_InvocationID == 0)
	{
		vec4 EdgeTessFactors;
		EdgeTessFactors.x = 0.5 * (tessellation[1] + tessellation[2]);
		EdgeTessFactors.y = 0.5 * (tessellation[2] + tessellation[0]);
		EdgeTessFactors.z = 0.5 * (tessellation[0] + tessellation[1]);

		gl_TessLevelOuter[0] = EdgeTessFactors.x;
		gl_TessLevelOuter[1] = EdgeTessFactors.y;
		gl_TessLevelOuter[2] = EdgeTessFactors.z;
		gl_TessLevelInner[0] = gl_TessLevelOuter[0];
	}
}

//------------------------------------------------------------------------------
/**
	Tessellation terrain shader
*/
[inputvertices] = 3
[winding] = cw
[topology] = triangle
[partition] = odd
shader
void
dsTerrainZ(
	in vec2 uv[],
	in vec4 position[],
	out vec2 UV) 
{
	vec3 Position = 
		gl_TessCoord.x * position[0].xyz + 
		gl_TessCoord.y * position[1].xyz + 
		gl_TessCoord.z * position[2].xyz;

	UV = 
		gl_TessCoord.x * uv[0] + 
		gl_TessCoord.y * uv[1] + 
		gl_TessCoord.z * uv[2];

	// sample height map
	float heightValue = sample2D(HeightMap, TextureSampler, UV).r;
	float height = MinHeight + heightValue * (MaxHeight - MinHeight);
	Position.y += height;

	gl_Position = ViewProjection * vec4(Position, 1);
}


//------------------------------------------------------------------------------
/**
	Pixel shader for multilayered painting
*/
[early_depth]
shader
void
psTerrain(
	in vec3 Normal,
	in vec2 UV,
	[color0] out vec4 Albedo,
	[color1] out vec3 Normals,
	[color2] out vec4 Material) 
{	
	Material = vec4(0.0f, 1.0f, 0.5f, 0.0f);
	Albedo = vec4(1, 1, 1, 1);
	Normals = Normal;
}

//------------------------------------------------------------------------------
/**
	Pixel shader for Z pass
*/
[early_depth]
shader
void
psTerrainZ(in vec2 UV) 
{	
	// do nothing, we use the built-in depth generated for this pass
}

//------------------------------------------------------------------------------
/**
	Pixel shader for multilayered painting
*/
shader
void
psTerrainShadow(
	in vec3 Normal,
	in vec2 UV,
	[color0] out vec2 Shadow) 
{	
	float depth = gl_FragCoord.z / gl_FragCoord.w;
	float moment1 = depth;
	float moment2 = depth * depth;

	// Adjusting moments (this is sort of bias per pixel) using derivative
	float dx = dFdx(depth);
	float dy = dFdy(depth);
	moment2 += 0.25f*(dx*dx+dy*dy);

	Shadow = vec2(moment1, moment2);
}

//------------------------------------------------------------------------------
/**
*/
render_state TerrainState
{
	CullMode = Back;
	//FillMode = Line;
};

render_state TerrainShadowState
{
	CullMode = Back;
	DepthClamp = false;
	DepthEnabled = false;
	DepthWrite = false;
	BlendEnabled[0] = true;
	SrcBlend[0] = One;
	DstBlend[0] = One;
	BlendOp[0] = Min;
};

TessellationTechnique(Terrain, "Terrain", vsTerrain(), psTerrain(), hsTerrain(), dsTerrain(), TerrainState);
TessellationTechnique(TerrainZ, "TerrainZ", vsTerrainZ(), psTerrainZ(), hsTerrainZ(), dsTerrainZ(), TerrainState);
