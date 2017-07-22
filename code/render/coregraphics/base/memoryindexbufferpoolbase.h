#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::MemoryIndexBufferLoaderBase
    
    Base resource loader class for initializing an index buffer from
    data in memory.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "resources/resourcememorypool.h"
#include "coregraphics/indexbuffer.h"
#include "coregraphics/indextype.h"

//------------------------------------------------------------------------------
namespace Base
{
class MemoryIndexBufferPoolBase : public Resources::ResourceMemoryPool
{
public:
	struct IndexBufferLoadInfo
	{
		CoreGraphics::IndexType::Code indexType;
		SizeT numIndices;
		void* indexDataPtr;
		SizeT indexDataSize;
		CoreGraphics::IndexBuffer::Usage usage = CoreGraphics::IndexBuffer::UsageImmutable;
		CoreGraphics::IndexBuffer::Access access = CoreGraphics::IndexBuffer::AccessNone;
		CoreGraphics::IndexBuffer::Syncing syncing = CoreGraphics::IndexBuffer::SyncingFlush;
	};
};
        
} // namespace CoreGraphics
//------------------------------------------------------------------------------

