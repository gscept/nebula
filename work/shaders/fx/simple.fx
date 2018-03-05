//------------------------------------------------------------------------------
//  simple.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/shared.fxh"
#include "lib/util.fxh"


BlendState Blend 
{
	BlendEnable[0] = TRUE;
	SrcBlend[0] = 5;
	DestBlend[0] = 6;
};

// we use the same rasterizer for both states
RasterizerState DefaultRasterizer
{
	CullMode = 1;
	//AntialiasedLineEnable = true;
};

RasterizerState WireframeRasterizer
{
	CullMode = 1;
	FillMode = 2;
};

DepthStencilState NoDepth
{
	DepthEnable = false;
	DepthWriteMask = 0;
};

DepthStencilState DoDepth
{
	DepthEnable = true;
	DepthWriteMask = 1;
};

//------------------------------------------------------------------------------
/**
*/
void
vsMain(float4 position : POSITION,
	out float4 Position : SV_POSITION) 
{
	Position = mul(position, mul(Model, ViewProjection));
}
	
//------------------------------------------------------------------------------
/**
*/
void
psMain(in float4 Position : SV_POSITION,
	out float4 Color : SV_TARGET0) 
{
	Color = MatDiffuse;
}

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(Default, "Static", vsMain(), psMain(), Blend, NoDepth, DefaultRasterizer)
SimpleTechnique(Depth, "Static|Depth", vsMain(), psMain(), Blend, DoDepth, DefaultRasterizer)
SimpleTechnique(Wireframe, "Static|Wireframe", vsMain(), psMain(), Blend, NoDepth, WireframeRasterizer)
