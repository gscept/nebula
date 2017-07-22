#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::MemoryVertexBufferLoaderBase
    
    Base resource loader class for initializing an vertex buffer from
    data in memory.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "resources/resourcememorypool.h"
#include "coregraphics/vertexcomponent.h"
#include "coregraphics/vertexbuffer.h"

//------------------------------------------------------------------------------
namespace Base
{
class MemoryVertexBufferPoolBase : public Resources::ResourceMemoryPool
{
public:
	struct VertexBufferLoadInfo
	{
		Util::Array<CoreGraphics::VertexComponent> vertexComponents;
		SizeT numVertices;
		void* vertexDataPtr;
		SizeT vertexDataSize;
		CoreGraphics::IndexBuffer::Usage usage = CoreGraphics::IndexBuffer::UsageImmutable;
		CoreGraphics::IndexBuffer::Access access = CoreGraphics::IndexBuffer::AccessNone;
		CoreGraphics::IndexBuffer::Syncing syncing = CoreGraphics::IndexBuffer::SyncingFlush;
	};
};

} // namespace Base
//------------------------------------------------------------------------------

