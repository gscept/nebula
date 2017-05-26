//------------------------------------------------------------------------------
//  indexbufferbase.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/base/indexbufferbase.h"

namespace Base
{
__ImplementClass(Base::IndexBufferBase, 'IXBB', Base::ResourceBase);

using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
IndexBufferBase::IndexBufferBase() :
    indexType(IndexType::None),
    numIndices(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
IndexBufferBase::~IndexBufferBase()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Make the index buffer content accessible by the CPU. The index buffer
    must have been initialized with the right Access and Usage flags 
    (see parent class for details). There are several reasons why a mapping
    the resource may fail, this depends on the platform (for instance, the
    resource may currently be busy, or selected for rendering).
*/
void*
IndexBufferBase::Map(MapType mapType)
{
    n_error("IndexBufferBase::Map() called!");
    return 0;
}

//------------------------------------------------------------------------------
/**
    Give up CPU access on the index buffer content.
*/
void
IndexBufferBase::Unmap()
{
    n_error("IndexBufferBase::Unmap() called!");
}

} // namespace Base
