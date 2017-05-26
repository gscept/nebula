//------------------------------------------------------------------------------
//  ogl4indexbuffer.cc
//  (C) 2014-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/ogl4/ogl4indexbuffer.h"
#include "coregraphics/ogl4/ogl4renderdevice.h"
#include "coregraphics/ogl4/ogl4types.h"

namespace OpenGL4
{
__ImplementClass(OpenGL4::OGL4IndexBuffer, 'O4IB', Base::IndexBufferBase);

using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
OGL4IndexBuffer::OGL4IndexBuffer() :
    ogl4IndexBuffer(0),
    mapCount(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
OGL4IndexBuffer::~OGL4IndexBuffer()
{
    n_assert(0 == this->ogl4IndexBuffer);
    n_assert(0 == this->mapCount);
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4IndexBuffer::Unload()
{
    n_assert(0 == this->mapCount);
	n_assert(0 != this->ogl4IndexBuffer);

	glDeleteBuffers(1, &this->ogl4IndexBuffer);
	this->ogl4IndexBuffer = 0;
	
    IndexBufferBase::Unload();
}

//------------------------------------------------------------------------------
/**
*/
void*
OGL4IndexBuffer::Map(MapType mapType)
{
    n_assert(0 != this->ogl4IndexBuffer);
    this->mapCount++;
	
	GLenum mapFlags = 0;
	switch (mapType)
	{
	case MapRead:
		mapFlags = GL_MAP_READ_BIT;
		break;
	case MapWrite:
		mapFlags = GL_MAP_WRITE_BIT;
		break;
	case MapReadWrite:
		mapFlags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
		break;
	case MapWriteDiscard:
		mapFlags = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
		break;
	}

	switch (this->syncing)
	{
	case SyncingSimple:		// do nothing
		break;
	case SyncingPersistent:
		mapFlags |= GL_MAP_PERSISTENT_BIT;
		break;
	case SyncingCoherentPersistent:
		mapFlags |= GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT;
		break;
	}

	// calculate size
	SizeT size = this->numIndices * IndexType::SizeOf(this->indexType);

	// bind index buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ogl4IndexBuffer);

	// only map range of buffer
	void* data = glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, size, mapFlags);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	n_assert(data);
	return data;
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4IndexBuffer::Unmap()
{
    n_assert(0 != this->ogl4IndexBuffer);
    n_assert(this->mapCount > 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ogl4IndexBuffer);
	GLboolean result = glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	n_assert(result);
    this->mapCount--;
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4IndexBuffer::SetOGL4IndexBuffer(const GLuint& ib)
{
    n_assert(0 != ib);
    n_assert(0 == this->ogl4IndexBuffer);
    this->ogl4IndexBuffer = ib;
}

} // namespace CoreGraphics

