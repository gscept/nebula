#pragma once
//------------------------------------------------------------------------------
/**
    @class OpenGL4::OGL4IndexBuffer
  
    OGL4 implementation of index buffer.
    
    (C) 2014-2016 Individual contributors, see AUTHORS file
*/
#include "coregraphics/base/indexbufferbase.h"

//------------------------------------------------------------------------------
namespace OpenGL4
{
class OGL4IndexBuffer : public Base::IndexBufferBase
{
    __DeclareClass(OGL4IndexBuffer);
public:
    /// constructor
    OGL4IndexBuffer();
    /// destructor
    virtual ~OGL4IndexBuffer();
    
    /// unload the resource, or cancel the pending load
    virtual void Unload();
    /// map index buffer for CPU access
    void* Map(MapType mapType);
    /// unmap the resource
    void Unmap();

    /// set ogl4 index buffer pointer
    void SetOGL4IndexBuffer(const GLuint& ib);
    /// get ogl4 index buffer pointer
    const GLuint& GetOGL4IndexBuffer() const;

	/// get pointer to the raw data
	void* GetRawBuffer();
	/// get raw buffer size
	uint GetRawBufferSize();
	/// set the pointer to the raw data
	void SetRawBuffer( void* data );
	/// set the size of the raw buffer
	void SetRawBufferSize( uint size );


private:
	GLuint ogl4IndexBuffer;
    int mapCount;
};



//------------------------------------------------------------------------------
/**
*/
inline const GLuint&
OGL4IndexBuffer::GetOGL4IndexBuffer() const
{
    n_assert(0 != this->ogl4IndexBuffer);
    return this->ogl4IndexBuffer;
}

} // namespace OpenGL4
//------------------------------------------------------------------------------
