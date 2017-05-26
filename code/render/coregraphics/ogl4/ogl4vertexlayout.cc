//------------------------------------------------------------------------------
//  ogl4vertexlayout.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/ogl4/ogl4vertexlayout.h"
#include "coregraphics/ogl4/ogl4types.h"
#include "coregraphics/vertexbuffer.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/shaderserver.h"
#include "resources/resourceid.h"

namespace OpenGL4
{
__ImplementClass(OpenGL4::OGL4VertexLayout, 'D1VL', Base::VertexLayoutBase);

using namespace Base;
using namespace CoreGraphics;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
OGL4VertexLayout::OGL4VertexLayout() :
	ogl4Vao(0)
{
	this->vertexStreams[0] = 0;
	this->vertexStreams[1] = 0;
}

//------------------------------------------------------------------------------
/**
*/
OGL4VertexLayout::~OGL4VertexLayout()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4VertexLayout::Setup(const Array<VertexComponent>& c)
{
    // call parent class
    Base::VertexLayoutBase::Setup(c);

	// create vertex attribute object
	glGenVertexArrays(1, &this->ogl4Vao);

    // create a OGL4 vertex declaration object
    n_assert(this->components.Size() < maxElements);
	IndexT curOffset[RenderDevice::MaxNumVertexStreams] = { 0 };
	SizeT vertexByteSizes[RenderDevice::MaxNumVertexStreams] = { 0 };
	IndexT attribIndex[RenderDevice::MaxNumVertexStreams] = { 0 };

	// bind VAO
	glBindVertexArray(this->ogl4Vao);

	IndexT compIndex;
	for (compIndex = 0; compIndex < this->components.Size(); compIndex++)
	{
		const VertexComponent& component = this->components[compIndex];
		vertexByteSizes[component.GetStreamIndex()] += component.GetByteSize();
	}

	// setup elements
    for (compIndex = 0; compIndex < this->components.Size(); compIndex++)
    {
		const VertexComponent& component = this->components[compIndex];
		GLint numComponents = OGL4Types::AsOGL4NumComponents(component.GetFormat());
		GLenum type = OGL4Types::AsOGL4SymbolicType(component.GetFormat());

		// bind buffer
		glBindBuffer(GL_ARRAY_BUFFER, this->vertexStreams[component.GetStreamIndex()]->GetOGL4VertexBuffer());

		// avoid working with empty array buffers
		if (this->vertexStreams[component.GetStreamIndex()] == 0) continue;

#if __OGL43__
		// activate vertex attribute
		glEnableVertexAttribArray(compIndex);

		// format vertex attributes
		if (component.GetFormat() == VertexComponentBase::UByte4	||
			component.GetFormat() == VertexComponentBase::UByte4N ||
			component.GetFormat() == VertexComponentBase::Short2	||
			component.GetFormat() == VertexComponentBase::Short4	||
			component.GetFormat() == VertexComponentBase::Short2N ||
			component.GetFormat() == VertexComponentBase::Short4N)
		{
			glVertexAttribIFormat(compIndex, numComponents, type, curOffset[component.GetStreamIndex()]);
		}
		else
		{
			glVertexAttribFormat(compIndex, numComponents, type, GL_TRUE, curOffset[component.GetStreamIndex()]);
		}

		// now bind the component to the appropriate vertex buffer
		glVertexAttribBinding(compIndex, component.GetStreamIndex());
#else

		// activate vertex attribute
        glEnableVertexAttribArray(component.GetSemanticName());

		// bind vertex attribute
		if (component.GetFormat() == VertexComponent::UByte4 ||
			component.GetFormat() == VertexComponent::Short2 ||
			component.GetFormat() == VertexComponent::Short4)
		{
			glVertexAttribIPointer(component.GetSemanticName(), numComponents, type, vertexByteSizes[component.GetStreamIndex()], (GLvoid*)curOffset[component.GetStreamIndex()]);
		}
		else if (component.GetFormat() == VertexComponent::UByte4N ||
				 component.GetFormat() == VertexComponent::Byte4N  ||
				 component.GetFormat() == VertexComponent::Short2N ||
				 component.GetFormat() == VertexComponent::Short4N)
		{
            glVertexAttribPointer(component.GetSemanticName(), numComponents, type, GL_TRUE, vertexByteSizes[component.GetStreamIndex()], (GLvoid*)curOffset[component.GetStreamIndex()]);
		}
		else
		{
            glVertexAttribPointer(component.GetSemanticName(), numComponents, type, GL_FALSE, vertexByteSizes[component.GetStreamIndex()], (GLvoid*)curOffset[component.GetStreamIndex()]);
		}

		if (component.GetStrideType() == VertexComponent::PerInstance)
		{
			// set divisor for instancing, this code is fugly, but it sorta works although only if we have two streams...
            glVertexAttribDivisor(component.GetSemanticName(), component.GetStride());
		}		

		// unbind buffer
		glBindBuffer(GL_ARRAY_BUFFER, 0);
#endif
		curOffset[component.GetStreamIndex()] += component.GetByteSize();
		attribIndex[component.GetStreamIndex()]++;
    }
	n_assert(GLSUCCESS);

	// unbind VAO
	glBindVertexArray(0);
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4VertexLayout::Discard()
{
	n_assert(this->ogl4Vao);
	glDeleteVertexArrays(1, &this->ogl4Vao);
	this->ogl4Vao = 0;

    VertexLayoutBase::Discard();	
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4VertexLayout::Apply()
{
	n_assert(this->ogl4Vao);

	// bind vertex array
	glBindVertexArray(this->ogl4Vao);
}

} // namespace OpenGL4
