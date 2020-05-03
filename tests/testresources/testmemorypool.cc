//------------------------------------------------------------------------------
// testloader.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "testmemorypool.h"
#include "testresource.h"
#include "resources/resourcemanager.h"

namespace Test
{

__ImplementClass(Test::TestMemoryPool, 'TMPO', Resources::ResourceMemoryPool);
//------------------------------------------------------------------------------
/**
*/
TestMemoryPool::TestMemoryPool() :
	alloc(0x00FFFFFF)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
TestMemoryPool::~TestMemoryPool()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
TestMemoryPool::Setup()
{
	ResourceMemoryPool::Setup();
}

//------------------------------------------------------------------------------
/**
*/
void
TestMemoryPool::Discard()
{
	ResourceMemoryPool::Discard();
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourcePool::LoadStatus
TestMemoryPool::LoadFromMemory(const Ids::Id24 id, const void* info)
{
	TestResourceData& res = this->alloc.Get<0>(id);
	const UpdateInfo* upd = static_cast<const UpdateInfo*>(info);
	res.data.Set(upd->buf, upd->len);
	return ResourcePool::Success;
}

//------------------------------------------------------------------------------
/**
*/
void
TestMemoryPool::Unload(const Ids::Id24 id)
{
	// meh, do nothing here since Util::String is recyclabe
}

//------------------------------------------------------------------------------
/**
*/
const TestResourceData&
TestMemoryPool::GetResource(const Resources::ResourceId id)
{
	return this->Get<0>(id.allocId);
}

} // namespace Test