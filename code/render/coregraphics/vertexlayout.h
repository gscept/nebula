#pragma once
//------------------------------------------------------------------------------
/**
	Vertex layout declares the interface between application and vertex shader

	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "coregraphics/coregraphics.h"
#include "coregraphics/vertexcomponent.h"
#include "coregraphics/shader.h"
namespace CoreGraphics
{

ID_32_24_8_TYPE(VertexLayoutId);

/// max number of vertex streams
static const IndexT MaxNumVertexStreams = 2;

struct VertexLayoutCreateInfo
{
	Util::Array<VertexComponent> comps;
	ShaderProgramId shader;
};

struct VertexLayoutInfo
{
	Util::StringAtom signature;
	SizeT vertexByteSize;
	Util::Array<VertexComponent> comps;
	ShaderProgramId shader;
	bool usedStreams[MaxNumVertexStreams];
};

/// create new vertex layout
const VertexLayoutId CreateVertexLayout(VertexLayoutCreateInfo& info);
/// destroy vertex layout
void DestroyVertexLayout(const VertexLayoutId id);

class VertexSignaturePool;
extern VertexSignaturePool* layoutPool;

} // CoreGraphics
