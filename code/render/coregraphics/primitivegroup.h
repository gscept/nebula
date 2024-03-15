#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::PrimitiveGroup
  
    Defines a group of primitives as a subset of a vertex buffer and index
    buffer plus the primitive topology (triangle list, etc...).
    
    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/    
#include "coregraphics/primitivetopology.h"
#include "math/bbox.h"

//------------------------------------------------------------------------------
namespace CoreGraphics
{
class PrimitiveGroup
{
public:
    /// constructor
    PrimitiveGroup();

    /// set base vertex index
    void SetBaseVertex(IndexT i);
    /// get index of base vertex
    IndexT GetBaseVertex() const;
    /// set number of vertices
    void SetNumVertices(SizeT n);
    ///get number of vertices
    SizeT GetNumVertices() const;
    /// set base index index
    void SetBaseIndex(IndexT i);
    /// get base index index
    IndexT GetBaseIndex() const;
    /// set number of indices
    void SetNumIndices(SizeT n);
    /// get number of indices
    SizeT GetNumIndices() const;
    /// get computed number of primitives
    SizeT GetNumPrimitives(const CoreGraphics::PrimitiveTopology::Code& topo) const;

private:
    IndexT baseVertex;
    SizeT numVertices;
    IndexT baseIndex;
    SizeT numIndices;
};

//------------------------------------------------------------------------------
/**
*/
inline
PrimitiveGroup::PrimitiveGroup() :
    baseVertex(0),
    numVertices(0),
    baseIndex(0),
    numIndices(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline void
PrimitiveGroup::SetBaseVertex(IndexT i)
{
    this->baseVertex = i;
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
PrimitiveGroup::GetBaseVertex() const
{
    return this->baseVertex;
}

//------------------------------------------------------------------------------
/**
*/
inline void
PrimitiveGroup::SetNumVertices(SizeT n)
{
    this->numVertices = n;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
PrimitiveGroup::GetNumVertices() const
{
    return this->numVertices;
}

//------------------------------------------------------------------------------
/**
*/
inline void
PrimitiveGroup::SetBaseIndex(IndexT i)
{
    this->baseIndex = i;
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
PrimitiveGroup::GetBaseIndex() const
{
    return this->baseIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline void
PrimitiveGroup::SetNumIndices(SizeT n)
{
    this->numIndices = n;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
PrimitiveGroup::GetNumIndices() const
{
    return this->numIndices;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
PrimitiveGroup::GetNumPrimitives(const CoreGraphics::PrimitiveTopology::Code& topo) const
{
    if (this->numIndices > 0)
    {
        return PrimitiveTopology::NumberOfPrimitives(topo, this->numIndices);
    }
    else
    {
        return PrimitiveTopology::NumberOfPrimitives(topo, this->numVertices);
    }
}

} // namespace PrimitiveGroup
//------------------------------------------------------------------------------

