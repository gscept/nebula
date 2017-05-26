#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::IndexBufferBase
  
    A resource which holds an array of indices into an array of vertices.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "coregraphics/indextype.h"
#include "coregraphics/base/bufferbase.h"
#include "coregraphics/bufferlock.h"

//------------------------------------------------------------------------------
namespace Base
{
class IndexBufferBase : public BufferBase
{
    __DeclareClass(IndexBufferBase);
public:
    /// constructor
    IndexBufferBase();
    /// destructor
    virtual ~IndexBufferBase();

    /// map index buffer for CPU access
    void* Map(MapType mapType);
    /// unmap the resource
    void Unmap();
    /// set the index type (Index16 or Index32)
    void SetIndexType(CoreGraphics::IndexType::Code type);
    /// get the index type (Index16 or Index32)
    CoreGraphics::IndexType::Code GetIndexType() const;
    /// set number of indices
    void SetNumIndices(SizeT num);
    /// get number of indices
    SizeT GetNumIndices() const;

protected:
    CoreGraphics::IndexType::Code indexType;
    SizeT numIndices;
};

//------------------------------------------------------------------------------
/**
*/
inline void
IndexBufferBase::SetIndexType(CoreGraphics::IndexType::Code type)
{
    this->indexType = type;
}

//------------------------------------------------------------------------------
/**
*/
inline CoreGraphics::IndexType::Code
IndexBufferBase::GetIndexType() const
{
    return this->indexType;
}

//------------------------------------------------------------------------------
/**
*/
inline void
IndexBufferBase::SetNumIndices(SizeT num)
{
    n_assert(num > 0);
    this->numIndices = num;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
IndexBufferBase::GetNumIndices() const
{
    return this->numIndices;
}

} // namespace Base
//------------------------------------------------------------------------------

