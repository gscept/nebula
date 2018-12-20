//------------------------------------------------------------------------------
//  ogl4uniformbuffer.cc
//  (C) 2015-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "ogl4uniformbuffer.h"
#include "coregraphics/shadervariable.h"
#include "coregraphics/constantbuffer.h"
#include "coregraphics/bufferlock.h"
#include "coregraphics/renderdevice.h"


namespace OpenGL4
{
__ImplementClass(OpenGL4::OGL4UniformBuffer, 'O4UB', Base::ConstantBufferBase);

//------------------------------------------------------------------------------
/**
*/
OGL4UniformBuffer::OGL4UniformBuffer() : 
	ogl4Buffer(0),
	handle(NULL),
	bufferLock(NULL)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
OGL4UniformBuffer::~OGL4UniformBuffer()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4UniformBuffer::Setup(const SizeT numBackingBuffers)
{
	n_assert(this->size > 0);
	n_assert(this->handle == NULL);
    ConstantBufferBase::Setup(numBackingBuffers);
    glGenBuffers(1, &this->ogl4Buffer);

	GLint maxBufferSize;
	GLint offsetAlignment;
	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxBufferSize);
	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &offsetAlignment);

    // calculate aligned size
	this->size = (this->size + offsetAlignment - 1) - (this->size + offsetAlignment - 1) % offsetAlignment;
	n_assert(this->size < GLuint(maxBufferSize) * this->numBuffers);

    glBindBuffer(GL_UNIFORM_BUFFER, this->ogl4Buffer);
    if (!this->sync)
    {
        GLenum mapFlags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
		glBufferStorage(GL_UNIFORM_BUFFER, this->size * this->numBuffers, NULL, mapFlags);
		this->buffer = glMapBufferRange(GL_UNIFORM_BUFFER, 0, this->size * this->numBuffers, mapFlags);
    }
    else
    {
		glBufferStorage(GL_UNIFORM_BUFFER, this->size * this->numBuffers, NULL, GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT);
		//glBufferData(GL_UNIFORM_BUFFER, this->size * this->numBuffers, NULL, GL_STREAM_DRAW);
        this->buffer = n_new_array(byte, this->size);
    }
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// create new bufferlock
	this->bufferLock = CoreGraphics::BufferLock::Create();

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
OGL4UniformBuffer::Discard()
{
	n_assert(this->ogl4Buffer != 0);

	// free lock and release variables
	this->bufferLock = 0;

	IndexT i;
	for (i = 0; i < this->variables.Size(); i++)
	{
		this->variables[i]->Cleanup();
	}
	this->variables.Clear();
	this->variablesByName.Clear();

    if (!this->sync)
    {
#if OGL4_BINDLESS
        glUnmapNamedBuffer(this->ogl4Buffer);
#else
        glBindBuffer(GL_UNIFORM_BUFFER, this->ogl4Buffer);
        glUnmapBuffer(GL_UNIFORM_BUFFER);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
#endif
    }
    else
    {
        n_delete_array(this->buffer);
    }
    
    this->buffer = 0;
	n_delete(this->handle);
	this->handle = 0;
    glDeleteBuffers(1, &this->ogl4Buffer);
	this->ogl4Buffer = 0;
    ConstantBufferBase::Discard();
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4UniformBuffer::SetupFromBlockInShader(const Ptr<CoreGraphics::Shader>& shader, const Util::String& blockName, const SizeT numBackingBuffers)
{
    n_assert(!this->isSetup);
    AnyFX::EffectVarblock* block = shader->GetOGL4Effect()->GetVarblockByName(blockName.AsCharPtr());
    this->size = block->GetSize();

    // setup buffer which initializes GL buffer
    this->Setup(numBackingBuffers);

	this->BeginUpdateSync();
    const eastl::vector<AnyFX::VarblockVariableBinding>& perFrameBinds = block->GetVariables();
    for (unsigned i = 0; i < perFrameBinds.size(); i++)
    {
        const AnyFX::VarblockVariableBinding& binding = perFrameBinds[i];
        Ptr<CoreGraphics::ShaderVariable> var = CoreGraphics::ShaderVariable::Create();
        Ptr<OGL4UniformBuffer> thisPtr(this);
        var->BindToUniformBuffer(thisPtr.downcast<CoreGraphics::ConstantBuffer>(), binding.offset, binding.size, binding.value);

        // add to variable dictionary
        this->variables.Append(var);
        this->variablesByName.Add(binding.name.c_str(), var);
    }
	this->EndUpdateSync();
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4UniformBuffer::CycleBuffers()
{
    ConstantBufferBase::CycleBuffers();
    this->handle->offset = this->size * this->bufferIndex;
	/*
	this->handle->offset =
		(this->handle->offset + OGL4UniformBuffer::ogl4OffsetAlignment - 1) -
		(this->handle->offset + OGL4UniformBuffer::ogl4OffsetAlignment - 1) %
		OGL4UniformBuffer::ogl4OffsetAlignment;
		*/
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4UniformBuffer::EndUpdateSync()
{
	if (this->sync)
	{
		// only sync if we made changes
		if (this->isDirty)
		{
			//glInvalidateBufferSubData(this->ogl4Buffer, this->handle->offset, this->size);
#if OGL4_BINDLESS
			glNamedBufferSubData(this->ogl4Buffer, this->handle->offset, this->size, this->buffer);
#else
			glBindBuffer(GL_UNIFORM_BUFFER, this->ogl4Buffer);
			glBufferSubData(GL_UNIFORM_BUFFER, this->handle->offset, this->size, this->buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
#endif
		}
	}
	else
	{
		CoreGraphics::RenderDevice::EnqueueBufferLockIndex(this->bufferLock.downcast<CoreGraphics::BufferLock>(), this->bufferIndex);
	}
	
	ConstantBufferBase::EndUpdateSync();
}

} // namespace OpenGL4 