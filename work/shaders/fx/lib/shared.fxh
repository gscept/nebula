//------------------------------------------------------------------------------
//  shared.fxh
//  (C) 2011 LTU Skellefteå
//------------------------------------------------------------------------------

#ifndef SHARED_H
#define SHARED_H

cbuffer PerFrame : register (c0)
{
	float4x4 View            		: View;
	float4x4 InvView         		: InvView;
	float4x4 ViewProjection        		: ViewProjection;
	float4x4 Projection            		: Projection;
	float4x4 InvProjection      		: InvProjection;
	float4x4 InvViewProjection		: InvViewProjection;
	float4   EyePos          		: EyePos;
	float4   FocalLength     		: FocalLength;
}
	
cbuffer PerObject : register (c1)
{
	//float4x4 ModelView       	: ModelView;
	//float4x4 ModelViewProjection  : ModelViewProjection;
	//float4x4 InvTransModelView	: InvTransModelView;
	//float4x4 InvModelView         : InvModelView;
	
	float4x4 Model           	: Model;
	float4x4 EmitterTransform       : EmitterTransform;	
	float4x4 InvModel		: InvModel;

	float4   OcclusionConstants     : OcclusionConstants = {0.05f, 0.03f, 0.1f, 0.05f};
	float4   TextureRatio		: TextureRatio = {1.0f, 1.0f, 0, 0};
}

cbuffer MaterialVars
{
	// pixel variables
	float4 MatDiffuse = {0.0f, 0.0f, 0.0f, 0.0f};
	float MatEmissiveIntensity = {0.0f};
	float MatSpecularIntensity = {0.0f};
	float AlphaSensitivity = {0.0f};
	float AlphaBlendFactor = {0.0f};
	float LightMapIntensity = {0.0f};
	
	float EnvironmentStrength = {0.0f};
	float FresnelPower = {0.0f};
	float FresnelStrength = {0.0f};
	int ObjectId;
}

cbuffer TessellationVars
{
	float TessellationFactor = {1.0f};
	float MaxDistance = {250.0f};
	float MinDistance = {20.0f};
	float HeightScale = {0.0f};
	float SceneScale = {1.0f};
}

cbuffer AnimVars
{
	// animation variables
	float2 AnimationDirection;
	float AnimationAngle;
	float Time;
	float Random;
	float AnimationLinearSpeed;
	float AnimationAngularSpeed;
	int NumXTiles = {1};
	int NumYTiles = {1};
}

cbuffer WindVars
{
	float WindWaveSize = {1.0f};
	float WindSpeed = {0.0f};
	float4 WindDirection = {0.0f,0.0f,0.0f,1.0f};
	float WindIntensity = {0.0f};
	float WindForce = {0.0f};
	float WindInverseStrength = {0.0f};
	float WindAlphaSpeed = {0.0f};
	float WindAlphaWaveSize = {1.0f};
	float WindAlphaPower = {0.0f};
}
	
tbuffer PerBatch : register (t0)
{
	// instancing transforms
	float4x4 ModelArray[256];
}

// The number of CSM cascades 
#ifndef CASCADE_COUNT_FLAG
#define CASCADE_COUNT_FLAG 4
#endif

cbuffer CSMTransforms
{
	float4x4 CSMSplitMatrices[CASCADE_COUNT_FLAG];
};

static const float ShadowConstant = 100.0f;

//------------------------------------------------------------------------------
// Define some macros which helps with declaring techniques
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
	Most common, simple vertex-pixel shader technique
*/
#define SimpleTechnique(name, features, vertexShader, pixelShader, blendState, depthStencilState, rasterizerState) \
technique11 name < string Mask = features; > \
{ \
	pass p \
	{ \
		SetBlendState(blendState, float4(0,0,0,0), 0xFFFFFFFF); \
		SetDepthStencilState(depthStencilState, 0); \
		SetRasterizerState(rasterizerState); \
		SetVertexShader(CompileShader(vs_5_0, vertexShader)); \
		SetPixelShader(CompileShader(ps_5_0, pixelShader)); \
		SetGeometryShader(NULL); \
		SetHullShader(NULL); \
		SetDomainShader(NULL); \
	} \
} 


#define GeometryTechnique(name, features, vertexShader, pixelShader, geometryShader, blendState, depthStencilState, rasterizerState) \
technique11 name < string Mask = features; > \
{ \
	pass p \
	{ \
		SetBlendState(blendState, float4(0,0,0,0), 0xFFFFFFFF); \
		SetDepthStencilState(depthStencilState, 0); \
		SetRasterizerState(rasterizerState); \
		SetVertexShader(CompileShader(vs_5_0, vertexShader)); \
		SetPixelShader(CompileShader(ps_5_0, pixelShader)); \
		SetGeometryShader(CompileShader(gs_5_0, geometryShader)); \
		SetHullShader(NULL); \
		SetDomainShader(NULL); \
	} \
} 


#define TessellationTechnique(name, features, vertexShader, pixelShader, hullShader, domainShader, blendState, depthStencilState, rasterizerState) \
technique11 name < string Mask = features; > \
{ \
	pass p \
	{ \
		SetBlendState(blendState, float4(0,0,0,0), 0xFFFFFFFF); \
		SetDepthStencilState(depthStencilState, 0); \
		SetRasterizerState(rasterizerState); \
		SetVertexShader(CompileShader(vs_5_0, vertexShader)); \
		SetPixelShader(CompileShader(ps_5_0, pixelShader)); \
		SetGeometryShader(NULL); \
		SetHullShader(CompileShader(hs_5_0, hullShader)); \
		SetDomainShader(CompileShader(ds_5_0, domainShader)); \
	} \
} 


#define FullTechnique(name, features, vertexShader, pixelShader, hullShader, domainShader, geometryShader, blendState, depthStencilState, rasterizerState) \
technique11 name < string Mask = features; > \
{ \
	pass p \
	{ \
		SetBlendState(blendState, float4(0,0,0,0), 0xFFFFFFFF); \
		SetDepthStencilState(depthStencilState, 0); \
		SetRasterizerState(rasterizerState); \
		SetVertexShader(CompileShader(vs_5_0, vertexShader)); \
		SetPixelShader(CompileShader(ps_5_0, pixelShader)); \
		SetGeometryShader(CompileShader(gs_5_0, geometryShader)); \
		SetHullShader(CompileShader(hs_5_0, hullShader)); \
		SetDomainShader(CompileShader(ds_5_0, domainShader)); \
	} \
} 


#endif // SHARED_H
