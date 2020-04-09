//------------------------------------------------------------------------------
//  simple.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/shared.fxh"
#include "lib/objects_shared.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"


vec4 MatDiffuse;
render_state WireframeState
{
	CullMode = None;
	DepthWrite = false;
	DepthEnabled = false;
	FillMode = Line;
	MultisampleEnabled = true;
};

render_state SolidState
{
	CullMode = None;
	DepthWrite = false;
	DepthEnabled = false;
	FillMode = Fill;
	MultisampleEnabled = true;
};


//------------------------------------------------------------------------------
/**
*/
shader
void
vsMain([slot=0] in vec4 position)
{
	gl_Position = ViewProjection * Model * position;
}

//------------------------------------------------------------------------------
/**
*/
[earlydepth]
shader
void
psPicking([color0] out float Id) 
{
	Id = float(ObjectId);
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
SimpleTechnique(Color, "Alt0", vsMain(), psColor(), WireframeState);
SimpleTechnique(Picking, "Alt1", vsMain(), psPicking(), WireframeState);

SimpleTechnique(ColorSolid, "Alt0|Alt2", vsMain(), psColor(), SolidState);
SimpleTechnique(PickingSolid, "Alt1|Alt2", vsMain(), psPicking(), SolidState);