#pragma once
//------------------------------------------------------------------------------
/** 
    @class OpenGL4::OGL4VertexBuffer
  
    OGL4/Xbox360 implementation of VertexBuffer.
    
    (C) 2007 Radon Labs GmbH
*/    
#include "coregraphics/base/feedbackbufferbase.h"
#include "coregraphics/base/resourcebase.h"

//------------------------------------------------------------------------------
namespace OpenGL4
{
class OGL4FeedbackBuffer : public Base::FeedbackBufferBase
{
	__DeclareClass(OGL4FeedbackBuffer);
public:
    /// constructor
	OGL4FeedbackBuffer();
    /// destructor
	virtual ~OGL4FeedbackBuffer();

	/// setup feedback buffer
	void Setup();
    /// discard feedback buffer and unload the feedback buffer
	void Discard();
    /// map the vertices for CPU access
    void* Map(Base::ResourceBase::MapType mapType);
    /// unmap the resource
    void Unmap();

	/// swap buffers
	void Swap();

	/// get pointer to ogl4 feedback object
	const GLuint& GetOGL4TransformFeedback() const;

	static const SizeT NumBuffers = 3;

private:
	Util::FixedArray<Ptr<CoreGraphics::VertexBuffer>> vbos;
	Util::FixedArray<Ptr<CoreGraphics::VertexLayout>> layouts;
	Util::FixedArray<GLuint> ogl4TransformFeedbackBuffer;
	GLuint ogl4TransformFeedback;
	IndexT bufferIndex;
	SizeT bufferOffset;
    int mapCount;
};

//------------------------------------------------------------------------------
/**
*/
inline const GLuint&
OGL4FeedbackBuffer::GetOGL4TransformFeedback() const
{
	n_assert(0 != this->ogl4TransformFeedback);
	return this->ogl4TransformFeedback;
}

} // namespace OpenGL4
//------------------------------------------------------------------------------

