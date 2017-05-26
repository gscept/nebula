#pragma once
//------------------------------------------------------------------------------
/**
	@class OpenGL4::OGL4UniformBuffer
	
	Wraps an OpenGL4 uniform buffer to enable updating and comitting shader variables in a buffered manner.
	
	(C) 2015-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/shader.h"
#include "coregraphics/base/constantbufferbase.h"
#include "coregraphics/bufferlock.h"

#define OGL4_UNIFORM_BUFFER_ALWAYS_MAPPED (1)
//#define OGL4_BINDLESS (1)
namespace CoreGraphics
{
class BufferLock;
class Shader;
}

namespace OpenGL4
{
class OGL4UniformBuffer : public Base::ConstantBufferBase
{
	__DeclareClass(OGL4UniformBuffer);
public:
	/// constructor
	OGL4UniformBuffer();
	/// destructor
	virtual ~OGL4UniformBuffer();

    /// setup buffer
	void Setup(const SizeT numBackingBuffers = DefaultNumBackingBuffers);
    /// bind variables in a block with a name in a shader to this buffer
	void SetupFromBlockInShader(const Ptr<CoreGraphics::Shader>& shader, const Util::String& blockName, const SizeT numBackingBuffers = DefaultNumBackingBuffers);
    /// discard buffer
    void Discard();

    /// get the handle for this buffer
	AnyFX::Handle* GetHandle() const;
     
	/// begin updating a segment of the buffer, will effectively lock the buffer
	void BeginUpdateSync();
    /// update segment of buffer asynchronously, which might overwrite data if it hasn't been used yet
    void UpdateAsync(void* data, uint offset, uint size);
    /// update segment of buffer as array asynchronously, which might overwrite data if it hasn't been used yet
    void UpdateArrayAsync(void* data, uint offset, uint size, uint count);
	/// update buffer synchronously
	void UpdateSync(void* data, uint offset, uint size);
	/// update buffer synchronously using an array of data
	void UpdateArraySync(void* data, uint offset, uint size, uint count);
    /// end updating asynchronously, which updates the GL buffer
    void EndUpdateSync();

    /// cycle buffers
    void CycleBuffers();

private:

    GLuint ogl4Buffer;
    AnyFX::OpenGLBufferBinding* handle;
	Ptr<CoreGraphics::BufferLock> bufferLock;
};

//------------------------------------------------------------------------------
/**
*/
inline AnyFX::Handle*
OGL4UniformBuffer::GetHandle() const
{
    return this->handle;
}

//------------------------------------------------------------------------------
/**
*/
inline void
OGL4UniformBuffer::UpdateAsync(void* data, uint offset, uint size)
{
	n_assert(size + offset <= this->size);
    GLubyte* currentBuf = (GLubyte*)this->buffer;
	if (!this->sync) currentBuf += this->handle->offset + this->baseOffset;
    memcpy(currentBuf + offset, data, size);
}

//------------------------------------------------------------------------------
/**
*/
inline void
OGL4UniformBuffer::UpdateArrayAsync(void* data, uint offset, uint size, uint count)
{
	n_assert(size * count + offset <= this->size);
    GLubyte* currentBuf = (GLubyte*)this->buffer;
	if (!this->sync) currentBuf += this->handle->offset + this->baseOffset;
	memcpy(currentBuf + offset, data, size * count);
}

//------------------------------------------------------------------------------
/**
*/
inline void
OGL4UniformBuffer::UpdateSync(void* data, uint offset, uint size)
{
	n_assert(size + offset <= this->size);
	GLubyte* currentBuf = (GLubyte*)this->buffer;
	if (!this->sync) currentBuf += this->handle->offset + this->baseOffset;
	memcpy(currentBuf + offset, data, size);
}

//------------------------------------------------------------------------------
/**
*/
inline void
OGL4UniformBuffer::UpdateArraySync(void* data, uint offset, uint size, uint count)
{
	n_assert(size * count + offset <= this->size);
	GLubyte* currentBuf = (GLubyte*)this->buffer;
	if (!this->sync) currentBuf += this->handle->offset + this->baseOffset;
	memcpy(currentBuf + offset, data, size * count);
}

//------------------------------------------------------------------------------
/**
*/
inline void
OGL4UniformBuffer::BeginUpdateSync()
{
	ConstantBufferBase::BeginUpdateSync();
	if (!this->sync)
	{
		this->bufferLock->WaitForBuffer(this->bufferIndex);
	}
}

} // namespace OpenGL4