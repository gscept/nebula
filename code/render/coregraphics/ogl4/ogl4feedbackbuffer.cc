//------------------------------------------------------------------------------
//  ogl4vertexbuffer.cc
//  (C) 2007 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/ogl4/ogl4feedbackbuffer.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/ogl4/ogl4types.h"
#include "../memoryvertexbufferloader.h"

namespace OpenGL4
{
__ImplementClass(OpenGL4::OGL4FeedbackBuffer, 'O4FB', Base::FeedbackBufferBase);

using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
OGL4FeedbackBuffer::OGL4FeedbackBuffer() :
	ogl4TransformFeedback(0),
	bufferIndex(0),
	bufferOffset(0),
    mapCount(0),
	ogl4TransformFeedbackBuffer(NumBuffers),
	vbos(NumBuffers),
	layouts(NumBuffers)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
OGL4FeedbackBuffer::~OGL4FeedbackBuffer()
{
	n_assert(0 == this->ogl4TransformFeedback);
    n_assert(0 == this->mapCount);
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4FeedbackBuffer::Setup()
{
	FeedbackBufferBase::Setup();
	glGenBuffers(NumBuffers, &this->ogl4TransformFeedbackBuffer[0]);

	IndexT i;
	for (i = 0; i < NumBuffers; i++)
	{
		// load vertex buffers
		Ptr<MemoryVertexBufferLoader> vboLoader = MemoryVertexBufferLoader::Create();
		vboLoader->Setup(this->components, this->numElements, NULL, 0, VertexBuffer::UsageDynamic, VertexBuffer::AccessRead);
		this->vbos[i]->SetLoader(vboLoader.upcast<Resources::ResourceLoader>());
		this->vbos[i]->Load();
		n_assert(this->vbos[i]->IsLoaded());
		this->vbos[i]->SetLoader(NULL);

		// save layout
		this->layouts[i] = this->vbos[i]->GetVertexLayout();
	}
	

	glGenTransformFeedbacks(1, &this->ogl4TransformFeedback);
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, this->ogl4TransformFeedback);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, this->ogl4TransformFeedbackBuffer[0]);
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
	this->layout = this->layouts[0];

	// setup layout, we will treat our transform buffer as an array buffer to render with it
	//this->layout->SetStreamBuffer(0, this->ogl4TransformFeedbackBuffer);
	//this->layout->Setup(this->components);
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4FeedbackBuffer::Discard()
{
    n_assert(0 == this->mapCount);
	n_assert(0 != this->ogl4TransformFeedback);
	n_assert(0 != this->ogl4TransformFeedbackBuffer[0]);

	glDeleteTransformFeedbacks(1, &this->ogl4TransformFeedback);
	this->ogl4TransformFeedback = 0;
	glDeleteBuffers(NumBuffers, &this->ogl4TransformFeedbackBuffer[0]);
	this->ogl4TransformFeedbackBuffer.Clear();
	IndexT i;
	for (i = 0; i < this->vbos.Size(); i++)
	{
		this->vbos[i]->Unload();
		this->layouts[i]->Discard();
	}
	this->vbos.Clear();
	this->layouts.Clear();
	//this->layout->Discard();
	this->layout = 0;
	this->ogl4TransformFeedback = 0;
}

//------------------------------------------------------------------------------
/**
*/
void*
OGL4FeedbackBuffer::Map(Base::ResourceBase::MapType mapType)
{
	n_assert(0 != this->ogl4TransformFeedbackBuffer[0]);
	return this->vbos[this->bufferIndex]->Map(mapType);
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4FeedbackBuffer::Unmap()
{
	n_assert(0 != this->ogl4TransformFeedback);
    n_assert(this->mapCount > 0);
	this->vbos[this->bufferIndex]->Unmap();
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4FeedbackBuffer::Swap()
{
	this->bufferIndex = (this->bufferIndex + 1) % NumBuffers;
	this->bufferOffset = this->bufferIndex * this->size * this->numElements;
	//this->primGroup.SetBaseVertex(this->bufferIndex * this->size);

	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, this->ogl4TransformFeedback);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, this->ogl4TransformFeedbackBuffer[this->bufferIndex]);
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
	this->layout = this->layouts[this->bufferIndex];
}

} // namespace OpenGL4
