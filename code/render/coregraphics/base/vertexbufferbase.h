#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::VertexBufferBase
  
    A resource which holds an array of vertices.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "coregraphics/vertexlayout.h"
#include "coregraphics/base/bufferbase.h"
#include "coregraphics/bufferlock.h"

//------------------------------------------------------------------------------
namespace Base
{
class VertexBufferBase : public BufferBase
{
    __DeclareClass(VertexBufferBase);
public:
    /// constructor
    VertexBufferBase();
    /// destructor
    virtual ~VertexBufferBase();

    /// unload the resource, or cancel the pending load
    virtual void Unload();
    /// map the vertices for CPU access
    void* Map(MapType mapType);
    /// unmap the resource
    void Unmap();
    
    /// set vertex layout (set by resource loader)
    void SetVertexLayout(const Ptr<CoreGraphics::VertexLayout>& vertexLayout);
    /// get the vertex layout
    const Ptr<CoreGraphics::VertexLayout>& GetVertexLayout() const;
    /// set number of vertices (set by resource loader)
    void SetNumVertices(SizeT numVertices);
    /// get number of vertices in the buffer
    SizeT GetNumVertices() const;
	/// set the vertex byte size
	void SetVertexByteSize(SizeT size);
	/// get the vertex byte size
	SizeT GetVertexByteSize() const;

protected:
    Ptr<CoreGraphics::VertexLayout> vertexLayout;
	SizeT numVertices;
	SizeT vertexByteSize;
};

//------------------------------------------------------------------------------
/**
*/
inline void
VertexBufferBase::SetVertexLayout(const Ptr<CoreGraphics::VertexLayout>& l)
{
    this->vertexLayout = l;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::VertexLayout>&
VertexBufferBase::GetVertexLayout() const
{
    return this->vertexLayout;
}

//------------------------------------------------------------------------------
/**
*/
inline void
VertexBufferBase::SetNumVertices(SizeT num)
{
    n_assert(num > 0);
    this->numVertices = num;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
VertexBufferBase::GetNumVertices() const
{
    return this->numVertices;
}

//------------------------------------------------------------------------------
/**
*/
inline void
VertexBufferBase::SetVertexByteSize(SizeT size)
{
	this->vertexByteSize = size;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
VertexBufferBase::GetVertexByteSize() const
{
	return this->vertexByteSize;
}

} // namespace Base
//------------------------------------------------------------------------------

