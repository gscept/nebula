//------------------------------------------------------------------------------
//  memoryvertexbufferloaderbase.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/base/memoryvertexbufferloaderbase.h"

namespace Base
{
__ImplementAbstractClass(Base::MemoryVertexBufferLoaderBase, 'MVLB', Resources::ResourceLoader);

using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
MemoryVertexBufferLoaderBase::MemoryVertexBufferLoaderBase() :
    numVertices(0),
    vertexDataPtr(0),
    vertexDataSize(0),
    usage(CoreGraphics::VertexBuffer::UsageImmutable),
    access(CoreGraphics::VertexBuffer::AccessNone),
	syncing(CoreGraphics::VertexBuffer::SyncingFlush)
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Setup all information needed to initialize the VertexBuffer resource.
    The data must remain valid until OnLoadRequested() is called (which
    will invaliate the data).
*/
void
MemoryVertexBufferLoaderBase::Setup(const Util::Array<CoreGraphics::VertexComponent>& vertexComponents_, 
                                    SizeT numVertices_, 
                                    void* vertexDataPtr_, 
                                    SizeT vertexDataSize_, 
                                    VertexBuffer::Usage usage_, 
                                    VertexBuffer::Access access_,
									VertexBuffer::Syncing syncing_)
{
    n_assert(numVertices_ > 0);
    if (VertexBuffer::UsageImmutable == usage_)
    {
        n_assert(0 != vertexDataPtr_);
        n_assert(0 < vertexDataSize_);
    }

    this->vertexComponents	= vertexComponents_;
    this->numVertices		= numVertices_;
    this->vertexDataPtr		= vertexDataPtr_;
    this->vertexDataSize	= vertexDataSize_;
    this->usage				= usage_;
    this->access			= access_;
	this->syncing			= syncing_;
}

} // namespace Base