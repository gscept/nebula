//------------------------------------------------------------------------------
//  shadow.fx
//
//	Defines shadows for standard geometry
//
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "lib/shared.fxh"
#include "lib/skinning.fxh"
#include "lib/util.fxh"

/// Declaring used textures
Texture2D DiffuseMap;
Texture2D DisplacementMap;

SamplerState DefaultSampler;
BlendState Blend 
{
};

// we use the same rasterizer for both states
RasterizerState Rasterizer
{
	CullMode = 2;
};

DepthStencilState DepthStencil
{
};


//------------------------------------------------------------------------------
/**
*/
void
vsStatic(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	out float4 Position : SV_POSITION,
	out float2 UV : TEXCOORD,
	out float4 ProjPos : PROJPOS) 
{
	Position = mul(position, mul(Model, ViewProjection));
	ProjPos = Position;
	UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
void
vsSkinned(float4 position : POSITION,
	float4 normal : NORMAL,
	float2 uv : TEXCOORD,
	float4 tangent : TANGENT,
	float4 binormal : BINORMAL,
	float4 weights : BLENDWEIGHT,
	uint4 indices : BLENDINDICES,
	out float4 Position : SV_POSITION,
	out float2 UV : TEXCOORD,
	out float4 ProjPos : PROJPOS)	
{
	float4 skinnedPos = SkinnedPosition(position, weights, indices);
	Position = mul(skinnedPos, mul(Model, ViewProjection));
	ProjPos = Position;
	UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
void
vsStaticInst(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	uint ID : SV_InstanceID,
	out float4 Position : SV_POSITION,
	out float2 UV : TEXCOORD,
	out float4 ProjPos : PROJPOS) 
{
	Position = mul(position, mul(ModelArray[ID], ViewProjection));
	ProjPos = Position;
	UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
void
vsStaticCSM(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	out float4 Position : SV_POSITION,
	out float2 UV : TEXCOORD) 
{
	Position = mul(position, Model);
	UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
void
vsSkinnedCSM(float4 position : POSITION,
	float4 normal : NORMAL,
	float2 uv : TEXCOORD,
	float4 tangent : TANGENT,
	float4 binormal : BINORMAL,
	float4 weights : BLENDWEIGHT,
	uint4 indices : BLENDINDICES,
	out float4 Position : SV_POSITION,
	out float2 UV : TEXCOORD) 
{
	float4 skinnedPos = SkinnedPosition(position, weights, indices);
	Position = mul(skinnedPos, Model);
	UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
void
vsStaticInstCSM(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	uint ID : SV_InstanceID,
	out float4 Position : SV_POSITION,
	out float2 UV : TEXCOORD) 
{
	Position = mul(position, ModelArray[ID]);
	UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
struct VS_OUT
{
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD;	
};

struct GS_OUT
{
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD;	
	float4 ProjPos : PROJPOS;
	uint ViewportIndex : SV_ViewportArrayIndex;
};

//------------------------------------------------------------------------------
/**
	Geometry shader for CSM shadow instancing.
	We copy the geometry and project into each frustum.
*/
[maxvertexcount(CASCADE_COUNT_FLAG * 3)]
void 
gsMain(triangle VS_OUT In[3], inout TriangleStream<GS_OUT> triStream)
{
	[unroll]
	for (int split = 0; split < CASCADE_COUNT_FLAG; split++)
	{
		GS_OUT output;
		output.ViewportIndex = split;
		[unroll]
		for (int vertex = 0; vertex < 3; vertex++)
		{
			output.Position = mul(In[vertex].Position, CSMSplitMatrices[split]);
			output.UV = In[vertex].UV;
			output.ProjPos = output.Position;
			triStream.Append(output);
		}
		
		triStream.RestartStrip();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
vsTess(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	out float4 Position : POSITION,
	out float4 Normal : NORMAL,
	out float2 UV : TEXCOORD,
	out float Distance : CAMERADIST) 
{
    Position = mul(position, Model);
	Normal = mul(normal, Model);
	UV = uv;
	
	float vertexDistance = distance( Position.xyz, EyePos.xyz );
	Distance = 1.0 - clamp( ( (vertexDistance - MinDistance) / (MaxDistance - MinDistance) ), 0.0, 1.0 - 1.0/TessellationFactor);
}

//------------------------------------------------------------------------------
/**
*/
void
vsTessCSM(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	out float4 Position : POSITION,
	out float3 Normal : NORMAL,
	out float2 UV : TEXCOORD,
	out float Distance : CAMERADIST) 
{
    Position = mul(position, Model);
	Normal = mul(normal, Model);
	UV = uv;
	
	float vertexDistance = distance( Position.xyz, EyePos.xyz );
	Distance = 1.0 - clamp( ( (vertexDistance - MinDistance) / (MaxDistance - MinDistance) ), 0.0, 1.0 - 1.0/TessellationFactor);
}

//------------------------------------------------------------------------------
/**
*/
#define POINTS 3
struct HS_INPUT
{
	float4 Position : POSITION;
	float3 Normal : NORMAL;
	float2 UV : TEXCOORD;	
	float Distance : CAMERADIST;
};

struct HS_OUTPUT
{
	float4 Position : POSITION;
	float3 Normal : NORMAL;
	float2 UV : TEXCOORD0;
};

struct HS_CONSTANT_OUTPUT
{
	float Edges[3] : SV_TessFactor;
	float Inside : SV_InsideTessFactor;

#ifdef PN_TRIANGLES
	// Geometry cubic generated control points
    float3 f3B210   : POSITION3;
    float3 f3B120   : POSITION4;
    float3 f3B021   : POSITION5;
    float3 f3B012   : POSITION6;
    float3 f3B102   : POSITION7;
    float3 f3B201   : POSITION8;
    float3 f3B111   : CENTER;
#endif
};

//------------------------------------------------------------------------------
/**
*/
HS_CONSTANT_OUTPUT 
HSPatchFunc(InputPatch<HS_INPUT, POINTS> patch, uint patchID : SV_PrimitiveID)
{
	HS_CONSTANT_OUTPUT output;
	float4 EdgeTessFactors;
	EdgeTessFactors.x = 0.5 * (patch[1].Distance + patch[2].Distance);
	EdgeTessFactors.y = 0.5 * (patch[2].Distance + patch[0].Distance);
	EdgeTessFactors.z = 0.5 * (patch[0].Distance + patch[1].Distance);
	EdgeTessFactors *= TessellationFactor;
	
#ifdef PN_TRIANGLES
	// compute the cubic geometry control points
    // edge control points
    output.f3B210 = ( ( 2.0f * patch[0].Position ) + patch[1].Position - ( dot( ( patch[1].Position - patch[0].Position ), patch[0].Normal ) * patch[0].Normal ) ) / 3.0f;
    output.f3B120 = ( ( 2.0f * patch[1].Position ) + patch[0].Position - ( dot( ( patch[0].Position - patch[1].Position ), patch[1].Normal ) * patch[1].Normal ) ) / 3.0f;
    output.f3B021 = ( ( 2.0f * patch[1].Position ) + patch[2].Position - ( dot( ( patch[2].Position - patch[1].Position ), patch[1].Normal ) * patch[1].Normal ) ) / 3.0f;
    output.f3B012 = ( ( 2.0f * patch[2].Position ) + patch[1].Position - ( dot( ( patch[1].Position - patch[2].Position ), patch[2].Normal ) * patch[2].Normal ) ) / 3.0f;
    output.f3B102 = ( ( 2.0f * patch[2].Position ) + patch[0].Position - ( dot( ( patch[0].Position - patch[2].Position ), patch[2].Normal ) * patch[2].Normal ) ) / 3.0f;
    output.f3B201 = ( ( 2.0f * patch[0].Position ) + patch[2].Position - ( dot( ( patch[2].Position - patch[0].Position ), patch[0].Normal ) * patch[0].Normal ) ) / 3.0f;
    // center control point
    float3 f3E = ( output.f3B210 + output.f3B120 + output.f3B021 + output.f3B012 + output.f3B102 + output.f3B201 ) / 6.0f;
    float3 f3V = ( patch[0].Position + patch[1].Position + patch[2].Position ) / 3.0f;
    output.f3B111 = f3E + ( ( f3E - f3V ) / 2.0f );
#endif

	output.Edges[0] = EdgeTessFactors.x;
	output.Edges[1] = EdgeTessFactors.y;
	output.Edges[2] = EdgeTessFactors.z;
	output.Inside = (output.Edges[0] + output.Edges[1] + output.Edges[2]) / 3;

	return output;
}

//------------------------------------------------------------------------------
/**
*/
[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[maxtessfactor(32.0)]
[patchconstantfunc("HSPatchFunc")]
HS_OUTPUT 
hsDefault(InputPatch<HS_INPUT, POINTS> patch, uint i : SV_OutputControlPointID)
{
	HS_OUTPUT output;
	output.Position = patch[i].Position;
	output.UV = patch[i].UV;	
	output.Normal = patch[i].Normal;
	return output;
}

//------------------------------------------------------------------------------
/**
*/
struct DS_OUTPUT
{
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD;
	float4 ProjPos : PROJPOS;
};

//------------------------------------------------------------------------------
/**
*/
[domain("tri")]
DS_OUTPUT
dsDefault( HS_CONSTANT_OUTPUT input, float3 Coordinate : SV_DomainLocation, const OutputPatch<HS_OUTPUT, POINTS> Patch)
{
	DS_OUTPUT output;
	
	// The barycentric coordinates
    float fU = Coordinate.z;
    float fV = Coordinate.x;
    float fW = Coordinate.y;

    // Precompute squares and squares * 3 
    float fUU = fU * fU;
    float fVV = fV * fV;
    float fWW = fW * fW;
    float fUU3 = fUU * 3.0f;
    float fVV3 = fVV * 3.0f;
    float fWW3 = fWW * 3.0f;
	
#ifdef PN_TRIANGLES
	// Compute position from cubic control points and barycentric coords
    float3 Position = Patch[0].Position * fWW * fW + Patch[1].Position * fUU * fU + Patch[2].Position * fVV * fV +
					  input.f3B210 * fWW3 * fU + input.f3B120 * fW * fUU3 + input.f3B201 * fWW3 * fV + input.f3B021 * fUU3 * fV +
					  input.f3B102 * fW * fVV3 + input.f3B012 * fU * fVV3 + input.f3B111 * 6.0f * fW * fU * fV;
#else
	float3 Position = Coordinate.x * Patch[0].Position + Coordinate.y * Patch[1].Position + Coordinate.z * Patch[2].Position;
#endif
	output.UV = Coordinate.x * Patch[0].UV + Coordinate.y * Patch[1].UV + Coordinate.z * Patch[2].UV;
	float Height = 2.0f * DisplacementMap.SampleLevel(DefaultSampler, output.UV, 0).x - 1.0f;
	float3 normal = Coordinate.x * Patch[0].Normal + Coordinate.y * Patch[1].Normal + Coordinate.z * Patch[2].Normal;
	float3 VectorNormalized = normalize( normal );
	
	Position.xyz += VectorNormalized.xyz * HeightScale * SceneScale * Height;
	
	output.Position = float4(Position.xyz, 1);
	
	output.Position = mul(output.Position, ViewProjection);	
	output.ProjPos = output.Position;

	return output;
}

//------------------------------------------------------------------------------
/**
*/
[domain("tri")]
DS_OUTPUT
dsCSM( HS_CONSTANT_OUTPUT input, float3 Coordinate : SV_DomainLocation, const OutputPatch<HS_OUTPUT, POINTS> Patch)
{
	DS_OUTPUT output;
	
	// The barycentric coordinates
    float fU = Coordinate.z;
    float fV = Coordinate.x;
    float fW = Coordinate.y;

    // Precompute squares and squares * 3 
    float fUU = fU * fU;
    float fVV = fV * fV;
    float fWW = fW * fW;
    float fUU3 = fUU * 3.0f;
    float fVV3 = fVV * 3.0f;
    float fWW3 = fWW * 3.0f;
	
#ifdef PN_TRIANGLES
	// Compute position from cubic control points and barycentric coords
    float3 Position = Patch[0].Position * fWW * fW + Patch[1].Position * fUU * fU + Patch[2].Position * fVV * fV +
					  input.f3B210 * fWW3 * fU + input.f3B120 * fW * fUU3 + input.f3B201 * fWW3 * fV + input.f3B021 * fUU3 * fV +
					  input.f3B102 * fW * fVV3 + input.f3B012 * fU * fVV3 + input.f3B111 * 6.0f * fW * fU * fV;
#else
	float3 Position = Coordinate.x * Patch[0].Position + Coordinate.y * Patch[1].Position + Coordinate.z * Patch[2].Position;
#endif
	output.UV = Coordinate.x * Patch[0].UV + Coordinate.y * Patch[1].UV + Coordinate.z * Patch[2].UV;
	float Height = 2.0f * DisplacementMap.SampleLevel(DefaultSampler, output.UV, 0).x - 1.0f;
	float3 normal = Coordinate.x * Patch[0].Normal + Coordinate.y * Patch[1].Normal + Coordinate.z * Patch[2].Normal;
	float3 VectorNormalized = normalize( normal );
	
	Position.xyz += VectorNormalized.xyz * HeightScale * SceneScale * Height;
	
	output.Position = float4(Position.xyz, 1);
	output.ProjPos = output.Position;

	return output;
}


//------------------------------------------------------------------------------
/**
*/
[earlydepthstencil]
void
psSolid(in float4 Position : SV_POSITION,
	in float2 UV : TEXCOORD,
	in float4 ProjPos : PROJPOS,
	out float ShadowColor : SV_TARGET0) 
{
	ShadowColor = (ProjPos.z/ProjPos.w) * ShadowConstant;
}

//------------------------------------------------------------------------------
/**
*/
void
psAlpha(in float4 Position : SV_POSITION,
	in float2 UV : TEXCOORD,
	in float4 ProjPos : PROJPOS,
	out float ShadowColor : SV_TARGET0) 
{
	float alpha = DiffuseMap.Sample( DefaultSampler, UV).a;
	clip( alpha < AlphaSensitivity ? -1:1 );
	ShadowColor = (ProjPos.z/ProjPos.w) * ShadowConstant;
}

//------------------------------------------------------------------------------
/**
*/
[earlydepthstencil]
void
psVSM(in float4 Position : SV_POSITION,
	in float2 UV : TEXCOORD,
	in float4 ProjPos : PROJPOS,
	out float2 ShadowColor : SV_TARGET0) 
{
	float depth = (ProjPos.z / ProjPos.w);
	float moment1 = depth;
	float moment2 = depth * depth;

	// Adjusting moments (this is sort of bias per pixel) using derivative
	float dx = ddx(depth);
	float dy = ddy(depth);
	moment2 += 0.25*(dx*dx+dy*dy) ;
	
	ShadowColor = float2(moment1, moment2);
}

//------------------------------------------------------------------------------
/**
*/
void
psVSMAlpha(in float4 Position : SV_POSITION,
	in float2 UV : TEXCOORD,
	in float4 ProjPos : PROJPOS,
	out float2 ShadowColor : SV_TARGET0) 
{
	float alpha = DiffuseMap.Sample( DefaultSampler, UV).a;
	clip( alpha < AlphaSensitivity ? -1:1 );
	float depth = (ProjPos.z / ProjPos.w);
	float moment1 = depth;
	float moment2 = depth * depth;

	// Adjusting moments (this is sort of bias per pixel) using derivative
	float dx = ddx(depth);
	float dy = ddy(depth);
	moment2 += 0.25*(dx*dx+dy*dy) ;
	
	ShadowColor = float2(moment1, moment2);
}

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(Default, "Static|Shadow", vsStatic(), psSolid(), Blend, DepthStencil, Rasterizer)
SimpleTechnique(Alpha, "Alpha|Shadow", vsStatic(), psAlpha(), Blend, DepthStencil, Rasterizer)
SimpleTechnique(Skinned, "Skinned|Shadow", vsSkinned(), psSolid(), Blend, DepthStencil, Rasterizer)
SimpleTechnique(SkinnedAlpha, "Skinned|Alpha|Shadow", vsSkinned(), psAlpha(), Blend, DepthStencil, Rasterizer)
SimpleTechnique(Instanced, "Static|Instanced|Shadow", vsStaticInst(), psSolid(), Blend, DepthStencil, Rasterizer)
TessellationTechnique(Tessellated, "Static|Shadow|Tessellated", vsTess(), psSolid(), hsDefault(), dsDefault(), Blend, DepthStencil, Rasterizer)

// CSM methods
GeometryTechnique(CSM, "Static|CSM", vsStaticCSM(), psVSM(), gsMain(), Blend, DepthStencil, Rasterizer)
GeometryTechnique(AlphaCSM, "Alpha|CSM", vsStaticCSM(), psVSMAlpha(), gsMain(), Blend, DepthStencil, Rasterizer)
GeometryTechnique(InstancedCSM, "Static|Instanced|CSM", vsStaticInstCSM(), psVSM(), gsMain(), Blend, DepthStencil, Rasterizer)
GeometryTechnique(InstancedAlphaCSM, "Alpha|Instanced|CSM", vsStaticInstCSM(), psVSMAlpha(), gsMain(), Blend, DepthStencil, Rasterizer)
GeometryTechnique(SkinnedCSM, "Skinned|CSM", vsSkinnedCSM(), psVSM(), gsMain(), Blend, DepthStencil, Rasterizer)
GeometryTechnique(SkinnedAlphaCSM, "Skinned|Alpha|CSM", vsSkinnedCSM(), psVSMAlpha(), gsMain(), Blend, DepthStencil, Rasterizer)
FullTechnique(TessellatedCSM, "Static|CSM|Tessellated", vsTessCSM(), psVSM(), hsDefault(), dsCSM(), gsMain(), Blend, DepthStencil, Rasterizer)
