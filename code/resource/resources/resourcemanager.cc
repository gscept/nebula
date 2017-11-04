//------------------------------------------------------------------------------
// resourcemanager.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "resourcemanager.h"

namespace Resources
{

__ImplementClass(Resources::ResourceManager, 'RMGR', Core::RefCounted);
__ImplementSingleton(Resources::ResourceManager);

int32_t ResourceManager::UniquePoolCounter = 0;
//------------------------------------------------------------------------------
/**
*/
ResourceManager::ResourceManager()
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
ResourceManager::~ResourceManager()
{
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceManager::Open()
{
	n_assert(!this->open);
	this->loaderThread = ResourceLoaderThread::Create();
	this->loaderThread->SetPriority(Threading::Thread::Normal);
	this->loaderThread->SetCoreId(System::Cpu::IoThreadCore);
	this->loaderThread->SetName("Resources::ResourceLoaderThread");
	this->loaderThread->Start();
	this->pools.Reserve(256); // lower 8 bits of resource id can only get to 256
	this->open = true;
	UniquePoolCounter = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceManager::Close()
{
	n_assert(this->open);
	this->loaderThread->Stop();
	this->loaderThread = nullptr;
	this->pools.Clear();
	this->extensionMap.Clear();
	this->open = false;
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceManager::RegisterStreamPool(const Util::StringAtom& ext, const Core::Rtti& loaderClass)
{
	n_assert(this->open);
	n_assert(loaderClass.IsDerivedFrom(ResourceStreamPool::RTTI));
	void* obj = loaderClass.Create();
	Ptr<ResourceStreamPool> loader((ResourceStreamPool*)obj);
	loader->uniqueId = UniquePoolCounter++;
	loader->Setup();
	this->pools.Append(loader);
	this->extensionMap.Add(ext, this->pools.Size() - 1);
	this->typeMap.Add(&loaderClass, this->pools.Size() - 1);
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceManager::RegisterMemoryPool(const Core::Rtti& loaderClass)
{
	n_assert(this->open);
	n_assert(loaderClass.IsDerivedFrom(ResourceMemoryPool::RTTI));
	void* obj = loaderClass.Create();
	Ptr<ResourceMemoryPool> loader((ResourceMemoryPool*)obj);
	loader->uniqueId = UniquePoolCounter++;
	loader->Setup();
	this->pools.Append(loader);
	this->typeMap.Add(&loaderClass, this->pools.Size() - 1);
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceManager::Update(IndexT frameIndex)
{
	IndexT i;
	for (i = 0; i < this->pools.Size(); i++)
	{
		const Ptr<ResourcePool>& loader = this->pools[i];
		loader->Update(frameIndex);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceManager::DiscardResources(const Util::StringAtom& tag)
{
	IndexT i;
	for (i = 0; i < this->pools.Size(); i++)
	{
		const Ptr<ResourcePool>& loader = this->pools[i];
		loader->DiscardByTag(tag);
	}
}

//------------------------------------------------------------------------------
/**
*/
Core::Rtti*
ResourceManager::GetType(const Resources::ResourceId id)
{
	// get id of loader
	const Ids::Id8 loaderid = Ids::Id::GetTiny(Ids::Id::GetLow(id));

	// get resource loader by extension
	n_assert(this->pools.Size() > loaderid);
	const Ptr<ResourcePool>& loader = this->pools[loaderid];
	return loader->GetRtti();
}

} // namespace Resources