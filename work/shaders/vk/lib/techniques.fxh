//------------------------------------------------------------------------------
//  techniques.fxh
//  (C) 2013 Gustav Sterbbrant
//------------------------------------------------------------------------------

#ifndef TECHNIQUES_FXH
#define TECHNIQUES_FXH

//------------------------------------------------------------------------------
// Define some macros which helps with declaring techniques
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
	Most common, simple vertex-pixel shader technique
*/

#define PostEffect(vertexShader, pixelShader, renderState) \
program Main [ string Mask = "Alt0"; ] \
{ \
	VertexShader = vertexShader; \
	PixelShader = pixelShader; \
	Renderrender_state = renderState; \
}

#define Compute(computeShader) \
program Main [ string Mask = "Alt0"; ] \
{ \
	ComputeShader = computeShader; \
}

#define SimpleTechnique(name, features, vertexShader, pixelShader, renderState) \
program name [ string Mask = features; ] \
{ \
	VertexShader = vertexShader; \
	PixelShader = pixelShader; \
	Renderrender_state = renderState; \
} 

#define GeometryTechnique(name, features, vertexShader, pixelShader, geometryShader, renderState) \
program name [ string Mask = features; ] \
{ \
	VertexShader = vertexShader; \
	PixelShader = pixelShader; \
	GeometryShader = geometryShader; \
	Renderrender_state = renderState; \
} 


#define TessellationTechnique(name, features, vertexShader, pixelShader, hullShader, domainShader, renderState) \
program name [ string Mask = features; ] \
{ \
	VertexShader = vertexShader; \
	PixelShader = pixelShader; \
	HullShader = hullShader; \
	DomainShader = domainShader; \
	Renderrender_state = renderState; \
} 


#define FullTechnique(name, features, vertexShader, pixelShader, hullShader, domainShader, geometryShader, renderState) \
program name [ string Mask = features; ] \
{ \
	VertexShader = vertexShader; \
	PixelShader = pixelShader; \
	GeometryShader = geometryShader; \
	HullShader = hullShader; \
	DomainShader = domainShader; \
	Renderrender_state = renderState; \
} 

#define TransformFeedbackTechnique(name, features, vertexShader) \
program name [ string Mask = features; ] \
{ \
	VertexShader = vertexShader; \
}

#define StateLessTechnique(name, features, vertexShader, pixelShader) \
program name [ string Mask = features; ] \
{ \
	VertexShader = vertexShader; \
	PixelShader = pixelShader; \
}

#endif