#pragma once
//------------------------------------------------------------------------------
/**
	@class OpenGL4::OGL4ShaderReadWriteBuffer
	
	Implements an OpenGL4 shader buffer, which is basically a shader storage buffer.

	Hmm, if one defines the OGL4_SHADER_BUFFER_ALWAYS_MAPPED, one must also make sure to lock the buffer after rendering is done.
	
	(C) 2012-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

#define OGL4_SHADER_BUFFER_ALWAYS_MAPPED

#include "coregraphics/base/shaderreadwritebufferbase.h"
#include "coregraphics/bufferlock.h"
#include "afxapi.h"
namespace OpenGL4
{
class OGL4ShaderStorageBuffer : public Base::ShaderReadWriteBufferBase
{
	__DeclareClass(OGL4ShaderStorageBuffer);
public:
	/// constructor
	OGL4ShaderStorageBuffer();
	/// destructor
	virtual ~OGL4ShaderStorageBuffer();

	/// setup buffer
	void Setup(const SizeT numBackingBuffers = DefaultNumBackingBuffers);
	/// discard buffer
	void Discard();

	/// update buffer region (offset = 0, length = size means the entire buffer)
    void Update(void* data, uint offset, uint length);
    /// cycle buffers
    void CycleBuffers();

	/// return handle
	AnyFX::Handle* GetHandle() const;

private:

    AnyFX::OpenGLBufferBinding* handle;

#ifdef OGL4_SHADER_BUFFER_ALWAYS_MAPPED
	GLubyte* buf;
	Ptr<CoreGraphics::BufferLock> bufferLock;
#endif
	GLuint ogl4Buffer;
};

//------------------------------------------------------------------------------
/**
*/
inline AnyFX::Handle*
OGL4ShaderStorageBuffer::GetHandle() const
{
	return this->handle;
}

} // namespace OpenGL4