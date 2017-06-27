//------------------------------------------------------------------------------
//  vertexbufferbase.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/base/feedbackbufferbase.h"
#include "coregraphics/vertexlayoutserver.h"

namespace Base
{
__ImplementClass(Base::FeedbackBufferBase, 'FDBB', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
FeedbackBufferBase::FeedbackBufferBase() :
	numPrimitives(0),
	numElements(0),
	size(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FeedbackBufferBase::~FeedbackBufferBase()
{
    // empty
}

//------------------------------------------------------------------------------
/**
	Initiate the feedback buffer using the primitive type and number of primitives provided,
	allocate a buffer with the given size and bind it properly depending on the implementation.	
	This function calculates the size per element in the feedback buffer.
*/
void
FeedbackBufferBase::Setup()
{
	n_assert(this->numPrimitives > 0);
	n_assert(this->size == 0);
	n_assert(this->prim != CoreGraphics::PrimitiveTopology::InvalidPrimitiveTopology);
	this->size = CoreGraphics::VertexLayoutServer::Instance()->CalculateVertexSize(this->components);
	this->numElements = CoreGraphics::PrimitiveTopology::NumberOfVertices(this->prim, this->numPrimitives);
	this->primGroup.SetBaseVertex(0);
	this->primGroup.SetNumVertices(this->numElements);

	// create layout
	this->layout = CoreGraphics::VertexLayout::Create();
}

//------------------------------------------------------------------------------
/**
	Discard the buffer created in the setup procedure
*/
void
FeedbackBufferBase::Discard()
{
	this->layout->Discard();
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
FeedbackBufferBase::Map(GpuResourceBase::MapType mapType)
{
    n_error("VertexBufferBase::Map() called!");
    return 0;
}

//------------------------------------------------------------------------------
/**
    Give up CPU access on the vertex buffer content.
*/
void
FeedbackBufferBase::Unmap()
{
    n_error("VertexBufferBase::Unmap() called!");
}


} // namespace Base
