//------------------------------------------------------------------------------
// testloader.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "teststreampool.h"
#include "testresource.h"

namespace Test
{

__ImplementClass(Test::TestStreamPool, 'TSPO', Resources::ResourceStreamPool);
//------------------------------------------------------------------------------
/**
*/
TestStreamPool::TestStreamPool() 
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
TestStreamPool::~TestStreamPool()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
TestStreamPool::Setup()
{
	ResourceStreamPool::Setup();
	this->async = true;
}

//------------------------------------------------------------------------------
/**
*/
void
TestStreamPool::Discard()
{
	ResourceStreamPool::Discard();
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourcePool::LoadStatus
TestStreamPool::LoadFromStream(const Ids::Id24 id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream)
{
	void* buf = stream->Map();
	SizeT size = stream->GetSize();
	TestResourceData& res = this->alloc.Get<0>(id);
	res.data.Set((char*)buf, size);
	stream->Unmap();
	return ResourcePool::Success;
}

//------------------------------------------------------------------------------
/**
*/
void
TestStreamPool::Unload(const Ids::Id24 id)
{

}

//------------------------------------------------------------------------------
/**
*/
const TestResourceData&
TestStreamPool::GetResource(const Resources::ResourceId id)
{
	return this->alloc.Get<0>(id.allocId);
}

} // namespace Test