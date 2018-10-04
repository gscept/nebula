#pragma once
//------------------------------------------------------------------------------
/** 
    @class OpenGL4::OGL4VertexBuffer
  
    OGL4/Xbox360 implementation of VertexBuffer.
    
    (C) 2007 Radon Labs GmbH
	(C) 2013-2018 Individual contributors, see AUTHORS file
*/    
#include "coregraphics/base/vertexbufferbase.h"

//------------------------------------------------------------------------------
namespace OpenGL4
{
class OGL4VertexBuffer : public Base::VertexBufferBase
{
    __DeclareClass(OGL4VertexBuffer);
public:
    /// constructor
    OGL4VertexBuffer();
    /// destructor
    virtual ~OGL4VertexBuffer();

    /// unload the resource, or cancel the pending load
    virtual void Unload();
    /// map the vertices for CPU access
    void* Map(MapType mapType);
    /// unmap the resource
    void Unmap();

    /// set ogl4 vertex buffer pointer
    void SetOGL4VertexBuffer(const GLuint& vb);
    /// get pointer to ogl4 vertex buffer object
    const GLuint& GetOGL4VertexBuffer() const;

private:
	GLuint ogl4VertexBuffer;
    int mapCount;
};

//------------------------------------------------------------------------------
/**
*/
inline const GLuint&
OGL4VertexBuffer::GetOGL4VertexBuffer() const
{
    n_assert(0 != this->ogl4VertexBuffer);
    return this->ogl4VertexBuffer;
}

} // namespace OpenGL4
//------------------------------------------------------------------------------

