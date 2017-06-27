//------------------------------------------------------------------------------
//  vertexbufferbase.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/base/vertexbufferbase.h"

namespace Base
{
__ImplementClass(Base::VertexBufferBase, 'VXBB', Base::GpuResourceBase);

//------------------------------------------------------------------------------
/**
*/
VertexBufferBase::VertexBufferBase() :
    numVertices(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
VertexBufferBase::~VertexBufferBase()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
VertexBufferBase::Unload()
{
	if (this->vertexLayout.isvalid())
	{
		this->vertexLayout->Discard();
		this->vertexLayout = 0;
	}
    GpuResourceBase::Unload();
}

//------------------------------------------------------------------------------
/**
    Make the vertex buffer content accessible by the CPU. The vertex buffer
    must have been initialized with the right Access and Usage flags 
    (see parent class for details). There are several reasons why a mapping
    the resource may fail, this depends on the platform (for instance, the
    resource may currently be busy, or selected for rendering).
*/
void*
VertexBufferBase::Map(MapType mapType)
{
    n_error("VertexBufferBase::Map() called!");
    return 0;
}

//------------------------------------------------------------------------------
/**
    Give up CPU access on the vertex buffer content.
*/
void
VertexBufferBase::Unmap()
{
    n_error("VertexBufferBase::Unmap() called!");
}

} // namespace Base
