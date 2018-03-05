//------------------------------------------------------------------------------
//  geometrybase.fxh
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "lib/util.fxh"
#include "lib/shared.fxh"
#include "lib/geometrybase.fxh"

RasterizerState FoliageRasterizer
{
	CullMode = 1;
};


//------------------------------------------------------------------------------
/**
*/
void
vsTree(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	out float3 ViewSpacePos : TEXCOORD0,
	out float3 Tangent : TANGENT0,
	out float3 Normal : NORMAL0,
	out float4 Position : SV_POSITION,
	out float3 Binormal : BINORMAL0,
	out float2 UV : TEXCOORD1)	
{
	UV = uv;
	
	float4 dir = mul(float4(WindDirection.xyz, 0), InvModel);
	float4 windDir = WindForce * normalize(dir);
	
	float windSpeed = WindSpeed * Time;
	float windAmplitude = length(position) / WindWaveSize;
	float windStrength = sin(windSpeed + windAmplitude);
	
	float inverseWindStrength = (1-windStrength) * WindInverseStrength;
	
	windSpeed = WindAlphaSpeed * Time;
	windAmplitude = length(position) / WindAlphaWaveSize;
	float windAlphaStrength = clamp(pow(sin(windSpeed + windAmplitude), WindAlphaPower), 0, 1);
	
	float4 finalOffset = windDir * lerp(windStrength, inverseWindStrength, windAlphaStrength);
	Position = position + finalOffset;
	Position = mul(Position, mul(Model, ViewProjection));

	float4x4 modelView = mul(Model, View);	
	ViewSpacePos = mul(position, modelView).xyz;
    
	Tangent  = mul(tangent, modelView).xyz;
	Normal   = mul(normal, modelView).xyz;
	Binormal = mul(binormal, modelView).xyz;
}

//------------------------------------------------------------------------------
/**
*/
void
vsTreeInstanced(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	uint ID : SV_InstanceID,
	out float3 ViewSpacePos : TEXCOORD0,
	out float3 Tangent : TANGENT0,
	out float3 Normal : NORMAL0,
	out float4 Position : SV_POSITION,
	out float3 Binormal : BINORMAL0,
	out float2 UV : TEXCOORD1)	
{
	UV = uv;
	
	float4 dir = mul(float4(WindDirection.xyz, 0), InvModel);
	float4 windDir = WindForce * normalize(dir);
	
	float windSpeed = WindSpeed * Time;
	float windAmplitude = length(position) / WindWaveSize;
	float windStrength = sin(windSpeed + windAmplitude);
	
	float inverseWindStrength = (1-windStrength) * WindInverseStrength;
	
	windSpeed = WindAlphaSpeed * Time;
	windAmplitude = length(position) / WindAlphaWaveSize;
	float windAlphaStrength = clamp(pow(sin(windSpeed + windAmplitude), WindAlphaPower), 0, 1);
	
	float4 finalOffset = windDir * lerp(windStrength, inverseWindStrength, windAlphaStrength);
	Position = position + finalOffset;
	Position = mul(Position, mul(ModelArray[ID], ViewProjection));

	float4x4 modelView = mul(ModelArray[ID], View);	
	ViewSpacePos = mul(position, modelView).xyz;
	    
	Tangent  = mul(tangent, modelView).xyz;
	Normal   = mul(normal, modelView).xyz;
	Binormal = mul(binormal, modelView).xyz;
}

//------------------------------------------------------------------------------
/**
*/
void
vsGrass(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD,
	float4 color : COLOR,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	out float3 ViewSpacePos : TEXCOORD0,
	out float3 Tangent : TANGENT0,
	out float3 Normal : NORMAL0,
	out float4 Position : SV_POSITION,
	out float3 Binormal : BINORMAL0,
	out float2 UV : TEXCOORD1)	
{
	UV = uv;
	
	float4 dir = mul(float4(WindDirection.xyz, 0), InvModel);
	float4 windDir = WindForce * normalize(dir);
	
	float windSpeed = WindSpeed * Time;
	float windAmplitude = length(position) / WindWaveSize;
	float windStrength = sin(windSpeed + windAmplitude);
	
	float inverseWindStrength = (1-windStrength) * WindInverseStrength;
	
	windSpeed = WindAlphaSpeed * Time;
	windAmplitude = length(position) / WindAlphaWaveSize;
	float windAlphaStrength = clamp(pow(sin(windSpeed + windAmplitude), WindAlphaPower), 0, 1);
	
	float4 finalOffset = windDir * lerp(windStrength, inverseWindStrength, windAlphaStrength);
	Position = position + finalOffset * color;
	Position = mul(Position, mul(Model, ViewProjection));
	
	float4x4 modelView = mul(Model, View);
	ViewSpacePos = mul(position, modelView).xyz;
    
	Tangent  = mul(tangent, modelView).xyz;
	Normal   = mul(normal, modelView).xyz;
	Binormal = mul(binormal, modelView).xyz;
}

//------------------------------------------------------------------------------
/**
*/
void
vsGrassInstanced(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD,
	float4 color : COLOR,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	uint ID : SV_InstanceID,
	out float3 ViewSpacePos : TEXCOORD0,
	out float3 Tangent : TANGENT0,
	out float3 Normal : NORMAL0,
	out float4 Position : SV_POSITION,
	out float3 Binormal : BINORMAL0,
	out float2 UV : TEXCOORD1)	
{
	UV = uv;
	
	float4 dir = mul(float4(WindDirection.xyz, 0), InvModel);
	float4 windDir = WindForce * normalize(dir);
	
	float windSpeed = WindSpeed * Time;
	float windAmplitude = length(position) / WindWaveSize;
	float windStrength = sin(windSpeed + windAmplitude);
	
	float inverseWindStrength = (1-windStrength) * WindInverseStrength;
	
	windSpeed = WindAlphaSpeed * Time;
	windAmplitude = length(position) / WindAlphaWaveSize;
	float windAlphaStrength = clamp(pow(sin(windSpeed + windAmplitude), WindAlphaPower), 0, 1);
	
	float4 finalOffset = windDir * lerp(windStrength, inverseWindStrength, windAlphaStrength);
	Position = position + finalOffset * color;
	Position = mul(Position, mul(ModelArray[ID], ViewProjection));
	
	float4x4 modelView = mul(ModelArray[ID], View);
	ViewSpacePos = mul(position, modelView).xyz;
    
	Tangent  = mul(tangent, modelView).xyz;
	Normal   = mul(normal, modelView).xyz;
	Binormal = mul(binormal, modelView).xyz;
}

//------------------------------------------------------------------------------
/**
*/
void
vsTreeShadow(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	out float4 Position : SV_POSITION,
	out float4 ProjPos : PROJPOS,
	out float2 UV : TEXCOORD0) 
{
	float4 dir = mul(float4(WindDirection.xyz, 0), InvModel);
	float4 windDir = WindForce * normalize(dir);
	
	float windSpeed = WindSpeed * Time;
	float windAmplitude = length(position) / WindWaveSize;
	float windStrength = sin(windSpeed + windAmplitude);
	
	float inverseWindStrength = (1-windStrength) * WindInverseStrength;
	
	windSpeed = WindAlphaSpeed * Time;
	windAmplitude = length(position) / WindAlphaWaveSize;
	float windAlphaStrength = clamp(pow(sin(windSpeed + windAmplitude), WindAlphaPower), 0, 1);
	
	float4 finalOffset = windDir * lerp(windStrength, inverseWindStrength, windAlphaStrength);
	Position = position + finalOffset;
	Position = mul(Position, mul(Model, ViewProjection));
	ProjPos = Position;
	UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
void
vsGrassShadow(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD,
	float4 color : COLOR,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	out float4 Position : SV_POSITION,
	out float4 ProjPos : PROJPOS,
	out float2 UV : TEXCOORD0) 
{
	float4 dir = mul(float4(WindDirection.xyz, 0), InvModel);
	float4 windDir = WindForce * normalize(dir);
	
	float windSpeed = WindSpeed * Time;
	float windAmplitude = length(position) / WindWaveSize;
	float windStrength = sin(windSpeed + windAmplitude);
	
	float inverseWindStrength = (1-windStrength) * WindInverseStrength;
	
	windSpeed = WindAlphaSpeed * Time;
	windAmplitude = length(position) / WindAlphaWaveSize;
	float windAlphaStrength = clamp(pow(sin(windSpeed + windAmplitude), WindAlphaPower), 0, 1);
	
	float4 finalOffset = windDir * lerp(windStrength, inverseWindStrength, windAlphaStrength);
	Position = position + finalOffset * color;
	Position = mul(Position, mul(Model, ViewProjection));
	ProjPos = Position;
	UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
void
vsTreeShadowCSM(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	out float4 Position : SV_POSITION,
	out float4 ProjPos : PROJPOS,
	out float2 UV : TEXCOORD0) 
{
	float4 dir = float4(WindDirection.xyz, 0);
	float4 windDir = WindForce * normalize(dir);
	
	float windSpeed = WindSpeed * Time;
	float windAmplitude = length(position) / WindWaveSize;
	float windStrength = sin(windSpeed + windAmplitude);
	
	float inverseWindStrength = (1-windStrength) * WindInverseStrength;
	
	windSpeed = WindAlphaSpeed * Time;
	windAmplitude = length(position) / WindAlphaWaveSize;
	float windAlphaStrength = clamp(pow(sin(windSpeed + windAmplitude), WindAlphaPower), 0, 1);
	
	float4 finalOffset = windDir * lerp(windStrength, inverseWindStrength, windAlphaStrength);
	Position = position + finalOffset;
	Position = mul(Position, Model);
	ProjPos = Position;
	UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
void
vsGrassShadowCSM(float4 position : POSITION,
	float3 normal : NORMAL,
	float2 uv : TEXCOORD,
	float4 color : COLOR,
	float3 tangent : TANGENT,
	float3 binormal : BINORMAL,
	out float4 Position : SV_POSITION,
	out float4 ProjPos : PROJPOS,
	out float2 UV : TEXCOORD0) 
{
	float4 dir = float4(WindDirection.xyz, 0);
	float4 windDir = WindForce * normalize(dir);
	
	float windSpeed = WindSpeed * Time;
	float windAmplitude = length(position) / WindWaveSize;
	float windStrength = sin(windSpeed + windAmplitude);
	
	float inverseWindStrength = (1-windStrength) * WindInverseStrength;
	
	windSpeed = WindAlphaSpeed * Time;
	windAmplitude = length(position) / WindAlphaWaveSize;
	float windAlphaStrength = clamp(pow(sin(windSpeed + windAmplitude), WindAlphaPower), 0, 1);
	
	
	float4 finalOffset = windDir * lerp(windStrength, inverseWindStrength, windAlphaStrength);
	Position = position + finalOffset;
	Position = mul(Position, Model);
	ProjPos = Position;
	UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
struct VS_OUT
{
	float4 Position : SV_POSITION;
	float4 ProjPos : PROJPOS;
	float2 UV : TEXCOORD;	
};

struct GS_OUT
{
	float4 Position : SV_POSITION;
	float4 ProjPos : PROJPOS;
	float2 UV : TEXCOORD;	
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
	for (int split = 0; split < CASCADE_COUNT_FLAG; split++)
	{
		GS_OUT output;
		
		output.ViewportIndex = split;
		
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
psShadow(in float4 Position : SV_POSITION,
	in float4 ProjPos : PROJPOS,
	in float2 UV : TEXCOORD0,
	out float ShadowColor : SV_TARGET0) 
{
	float alpha = DiffuseMap.Sample( DefaultSampler, UV).a;
	clip( alpha < AlphaSensitivity ? -1:1 );
	ShadowColor = (ProjPos.z/ProjPos.w) * ShadowConstant;
}

//------------------------------------------------------------------------------
/**
*/
void
psShadowCSM(in float4 Position : SV_POSITION,
	in float4 ProjPos : PROJPOS,
	in float2 UV : TEXCOORD0,
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
void
psPicking(in float4 Position : SV_POSITION,
		in float4 ProjPos : PROJPOS,
		in float2 UV : TEXCOORD0,
		out float Id : SV_TARGET0) 
{
	Id = (float)ObjectId;
}

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(Tree, "Static", vsTree(), psTransulcent(), AlphaTestBlend, SolidDepthStencil, FoliageRasterizer)
SimpleTechnique(TreeInstanced, "Static|Instanced", vsTreeInstanced(), psTransulcent(), AlphaTestBlend, SolidDepthStencil, FoliageRasterizer)
SimpleTechnique(Grass, "Static|Weighted", vsGrass(), psTransulcent(), AlphaTestBlend, SolidDepthStencil, FoliageRasterizer)
SimpleTechnique(GrassInstanced, "Static|Weighted|Instanced", vsGrassInstanced(), psTransulcent(), AlphaTestBlend, SolidDepthStencil, FoliageRasterizer)

SimpleTechnique(DefaultShadow, "Static|Shadow", vsTreeShadow(), psShadow(), AlphaTestBlend, SolidDepthStencil, FoliageRasterizer)
GeometryTechnique(ShadowCSM, "Static|CSM", vsTreeShadowCSM(), psShadowCSM(), gsMain(), AlphaTestBlend, SolidDepthStencil, FoliageRasterizer)
SimpleTechnique(Picking, "Static|Picking", vsTreeShadow(), psPicking(), AlphaTestBlend, SolidDepthStencil, FoliageRasterizer)

