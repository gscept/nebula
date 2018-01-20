//------------------------------------------------------------------------------
// resourcemanager.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "resourcemanager.h"

#if NEBULA_DEBUG
#include "core/sysfunc.h"
#endif
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
	n_assert(!this->open); // make sure to call close before destroying the object
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

#if NEBULA_DEBUG
	// report any resources which have not been unloaded
	bool hasLeaks = false;
	for (IndexT i = 0; i < this->pools.Size(); i++)
	{
		ResourcePool* pool = this->pools[i];
		for (IndexT j = 0; j < pool->usage.Size(); j++)
		{
			if (pool->usage[j] != 0)
			{
				const Resources::ResourceName& name = pool->names[j];
				Util::String msg = Util::String::Sprintf("\n\n******** NEBULA T RESOURCE MANAGER ********\n Resource <%s> (id %d) from pool %d is not unloaded, usage is %d!\n\n", name.Value(), j, pool->uniqueId, pool->usage[j]);
				Core::SysFunc::DebugOut(msg.AsCharPtr());
				hasLeaks = true;
			}
		}
	}

	if (!hasLeaks)
	{
		Util::String msg = Util::String::Sprintf("\n\n******** NEBULA T RESOURCE MANAGER ********\n All resources are properly freed!\n\n");
		Core::SysFunc::DebugOut(msg.AsCharPtr());
	}

#endif
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
bool
ResourceManager::HasPendingResources()
{
	IndexT i;
	for (i = 0; i < this->pools.Size(); i++)
	{
		const Ptr<ResourcePool>& loader = this->pools[i];
		if (loader->IsA(ResourceStreamPool::RTTI))
		{
			const Ptr<ResourceStreamPool>& pool = loader.cast<ResourceStreamPool>();
			if (!pool->pendingLoadMap.IsEmpty())
				return true;
		}
	}
	return false;
}

//------------------------------------------------------------------------------
/**
*/
Core::Rtti*
ResourceManager::GetType(const Resources::ResourceId id)
{
	// get id of loader
	const Ids::Id8 loaderid = id.poolIndex;

	// get resource loader by extension
	n_assert(this->pools.Size() > loaderid);
	const Ptr<ResourcePool>& loader = this->pools[loaderid];
	return loader->GetRtti();
}

} // namespace Resources