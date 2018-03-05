//------------------------------------------------------------------------------
//  simple.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/shared.fxh"
#include "lib/util.fxh"
#include "lib/skinning.fxh"
#include "lib/techniques.fxh"

vec4 MatDiffuse;
state SilhouetteState
{
	DepthEnabled = false;
	DepthWrite = false;
	DepthFunc = Less;
	BlendEnabled[0] = true;
	SrcBlend[0] = SrcAlpha;	
	DstBlend[0] = OneMinusSrcAlpha;
	FillMode = Line;
	MultisampleEnabled = true;

	StencilEnabled = true;
	StencilFunc = Nequal;
	StencilFrontRef = 1;
	StencilBackRef = 1;
	StencilWriteMask = 1;
	StencilReadMask = -1;
	StencilFailOp = Keep;
	StencilPassOp = Keep;
	StencilDepthFailOp = Keep;
};

state PrepassState
{
	DepthEnabled = false;
	DepthWrite = false;
	DepthFunc = Lequal;
	BlendEnabled[0] = true;
	SrcBlend[0] = SrcAlpha;	
	DstBlend[0] = OneMinusSrcAlpha;
	
	StencilEnabled = true;
	StencilFunc = Always;
	StencilFrontRef = 1;
	StencilBackRef = 1;
	StencilWriteMask = 1;
	StencilReadMask = -1;
	StencilFailOp = Keep;
	StencilPassOp = Replace;
	StencilDepthFailOp = Keep;
};

//------------------------------------------------------------------------------
/**
*/
shader
void
vsStatic(in vec3 position,
		 in vec3 normal) 
{
	gl_Position = ViewProjection * Model * vec4(position, 1);
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsSkinned(in vec3 position,
	in vec3 normal,
	in vec2 uv,
	in vec3 tangent,
	in vec3 binormal,
	in vec4 weights,
	in uvec4 indices) 
{
	vec4 skinnedPos = SkinnedPosition(position, weights, indices);
	gl_Position = ViewProjection * Model * skinnedPos;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsParticle(in vec3 position) 
{
	gl_Position = ViewProjection * Model * vec4(position, 1);
}
	
//------------------------------------------------------------------------------
/**
*/
shader
void
psColor([color0] out vec4 Color) 
{
	Color = MatDiffuse;	
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psEmpty([color0] out vec4 Color) 
{
	Color = vec4(0,0,0,0);	
}

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(SilhouettePrepass, "Alt0", vsStatic(), psEmpty(), PrepassState);
SimpleTechnique(SilhouetteSkinnedPrepass, "Alt0|Skinned", vsSkinned(), psEmpty(), PrepassState);
SimpleTechnique(SilhouetteParticlePrepass, "Alt0|Particle", vsParticle(), psEmpty(), PrepassState);

SimpleTechnique(Silhouette, "Alt1", vsStatic(), psColor(), SilhouetteState);
SimpleTechnique(SilhouetteSkinned, "Alt1|Skinned", vsSkinned(), psColor(), SilhouetteState);
SimpleTechnique(SilhouetteParticle, "Alt1|Particle", vsParticle(), psColor(), SilhouetteState);