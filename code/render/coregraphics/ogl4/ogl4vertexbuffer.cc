//------------------------------------------------------------------------------
//  ogl4vertexbuffer.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/ogl4/ogl4vertexbuffer.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/ogl4/ogl4types.h"

namespace OpenGL4
{
__ImplementClass(OpenGL4::OGL4VertexBuffer, 'O4VB', Base::VertexBufferBase);

using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
OGL4VertexBuffer::OGL4VertexBuffer() :
    ogl4VertexBuffer(0),
    mapCount(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
OGL4VertexBuffer::~OGL4VertexBuffer()
{
    n_assert(0 == this->ogl4VertexBuffer);
    n_assert(0 == this->mapCount);
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4VertexBuffer::Unload()
{
    n_assert(0 == this->mapCount);
	n_assert(0 != this->ogl4VertexBuffer);

	glDeleteBuffers(1, &this->ogl4VertexBuffer);
	this->ogl4VertexBuffer = 0;

    VertexBufferBase::Unload();
}

//------------------------------------------------------------------------------
/**
*/
void*
OGL4VertexBuffer::Map(MapType mapType)
{
    n_assert(0 != this->ogl4VertexBuffer);
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

	// bind buffer prior to mapping
	glBindBuffer(GL_ARRAY_BUFFER, this->ogl4VertexBuffer);

	// glMapBufferRange is a more modern way of mapping buffers, this can be done without any implicit synchronization, which is nice and fast!
	void* data = glMapBufferRange(GL_ARRAY_BUFFER, 0, this->numVertices * this->vertexByteSize, mapFlags);
	glBindBuffer(GL_ARRAY_BUFFER, 0);	
	n_assert(data);
	return data;
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4VertexBuffer::Unmap()
{
    n_assert(0 != this->ogl4VertexBuffer);
    n_assert(this->mapCount > 0);
	glBindBuffer(GL_ARRAY_BUFFER, this->ogl4VertexBuffer);
	GLboolean result = glUnmapBuffer(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	n_assert(result);
    this->mapCount--;
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4VertexBuffer::SetOGL4VertexBuffer(const GLuint& vb)
{
    n_assert(0 != vb);
    n_assert(0 == this->ogl4VertexBuffer);
    this->ogl4VertexBuffer = vb;
}

} // namespace OpenGL4
