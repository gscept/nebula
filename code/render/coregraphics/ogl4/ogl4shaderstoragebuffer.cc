//------------------------------------------------------------------------------
//  ogl4shaderbuffer.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "ogl4shaderstoragebuffer.h"
#include "../renderdevice.h"



namespace OpenGL4
{
__ImplementClass(OpenGL4::OGL4ShaderStorageBuffer, 'O4SB', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
OGL4ShaderStorageBuffer::OGL4ShaderStorageBuffer() :
	ogl4Buffer(0),
	handle(NULL)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
OGL4ShaderStorageBuffer::~OGL4ShaderStorageBuffer()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderStorageBuffer::Setup(const SizeT numBackingBuffers)
{
	n_assert(this->size > 0);
	ShaderReadWriteBufferBase::Setup(numBackingBuffers);
	glGenBuffers(1, &this->ogl4Buffer);

	GLint maxBufferSize;
	GLint offsetAlignment;
	glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &maxBufferSize);
	glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &offsetAlignment);

	// calculate aligned size
	this->size = (this->size + offsetAlignment - 1) - (this->size + offsetAlignment - 1) % offsetAlignment;
	n_assert(this->size < GLuint(maxBufferSize));

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->ogl4Buffer);
#ifdef OGL4_SHADER_BUFFER_ALWAYS_MAPPED
	GLenum mapFlags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, this->size * this->numBuffers, NULL, mapFlags);
	this->buf = (GLubyte*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, this->size * this->numBuffers, mapFlags);
	this->bufferLock = CoreGraphics::BufferLock::Create();
#else
    glBufferData(GL_SHADER_STORAGE_BUFFER, this->size * this->numBuffers, NULL, GL_STREAM_DRAW);
#endif
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	n_assert(GLSUCCESS);

	// setup handle
    this->handle = n_new(AnyFX::OpenGLBufferBinding);
    this->handle->size = this->size;
	this->handle->handle = this->ogl4Buffer;
	this->handle->bindRange = true;
	this->handle->offset = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderStorageBuffer::Discard()
{
	n_assert(this->ogl4Buffer != 0);
#ifdef OGL4_SHADER_BUFFER_ALWAYS_MAPPED
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->ogl4Buffer);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    this->buf = 0;
	this->bufferLock = 0;
#endif
	glDeleteBuffers(1, &this->ogl4Buffer);
	this->ogl4Buffer = 0;
	n_delete(this->handle);
	this->handle = 0;
	ShaderReadWriteBufferBase::Discard();
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderStorageBuffer::CycleBuffers()
{
    ShaderReadWriteBufferBase::CycleBuffers();
    this->handle->offset = this->size * this->bufferIndex;
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderStorageBuffer::Update(void* data, uint offset, uint length)
{
	n_assert(offset + length <= this->size);
	ShaderReadWriteBufferBase::Update(data, offset, length);
	
#ifdef OGL4_SHADER_BUFFER_ALWAYS_MAPPED
    this->bufferLock->WaitForRange(this->handle->offset, length);
	GLubyte* currentBuf = this->buf + this->handle->offset;
	memcpy(currentBuf + offset, data, length);
	CoreGraphics::RenderDevice::EnqueueBufferLockIndex(this->bufferLock.downcast<CoreGraphics::BufferLock>(), this->bufferIndex);
#else
	//glInvalidateBufferSubData(this->ogl4Buffer, this->handle->offset, length);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->ogl4Buffer);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, this->handle->offset + offset, length, data);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
#endif
}

} // namespace OpenGL4