//------------------------------------------------------------------------------
//  ogl4memoryindexbufferloader.cc
//  (C) 2007 Radon Labs GmbH
//  (C)2013 - 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/ogl4/ogl4memoryindexbufferloader.h"
#include "coregraphics/indexbuffer.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/ogl4/ogl4types.h"

namespace OpenGL4
{
__ImplementClass(OpenGL4::OGL4MemoryIndexBufferLoader, 'OMIL', Base::MemoryIndexBufferLoaderBase);

using namespace Resources;
using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
    This will create a OGL4 IndexBuffer using the data provided by our
    Setup() method and set our resource object (which must be a
    OGL4IndexBuffer object).
*/
bool
OGL4MemoryIndexBufferLoader::OnLoadRequested()
{
    n_assert(this->GetState() == Resource::Initial);
    n_assert(this->resource.isvalid());
    n_assert(!this->resource->IsAsyncEnabled());
    n_assert(this->indexType != IndexType::None);
    n_assert(this->numIndices > 0);
    if (IndexBuffer::UsageImmutable == this->usage)
    {
        n_assert(this->indexDataSize == (this->numIndices * IndexType::SizeOf(this->indexType)));
        n_assert(0 != this->indexDataPtr);
        n_assert(0 < this->indexDataSize);
    }
	
	GLuint ogl4IndexBuffer = 0;
	GLenum usage = OGL4Types::AsOGL4Usage(this->usage, this->access);
	GLenum sync = OGL4Types::AsOGL4Syncing(this->syncing);
	glGenBuffers(1, &ogl4IndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ogl4IndexBuffer);
	if (this->syncing == IndexBuffer::SyncingFlush)	glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->numIndices * IndexType::SizeOf(this->indexType), this->indexDataPtr, usage | sync);
	else											glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, this->numIndices * IndexType::SizeOf(this->indexType), this->indexDataPtr, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | sync);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	n_assert(GLSUCCESS);

    // setup our IndexBuffer resource
    const Ptr<IndexBuffer>& res = this->resource.downcast<IndexBuffer>();
    n_assert(!res->IsLoaded());
	res->SetUsage(this->usage);
	res->SetAccess(this->access);
	res->SetSyncing(this->syncing);
    res->SetIndexType(this->indexType);
    res->SetNumIndices(this->numIndices);
	res->SetByteSize(this->indexDataSize);
    res->SetOGL4IndexBuffer(ogl4IndexBuffer);

    // invalidate setup data (because we don't own our data)
    this->indexDataPtr = 0;
    this->indexDataSize = 0;

    this->SetState(Resource::Loaded);
    return true;
}

} // namespace OpenGL4
