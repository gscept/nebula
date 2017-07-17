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

int32_t ResourceManager::UniqueLoaderCounter = 0;
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
	this->open = true;
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
	this->loaders.Clear();
	this->extensionMap.Clear();
	this->open = false;
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceManager::RegisterFileLoader(const Util::StringAtom& ext, const Core::Rtti& loaderClass)
{
	n_assert(this->open);
	Core::RefCounted* obj = loaderClass.Create();
	Ptr<ResourceLoader> loader((ResourceLoader*)obj);
	loader->uniqueId = UniqueLoaderCounter++;
	loader->Setup();
	this->loaders.Append(loader);
	this->extensionMap.Add(ext, this->loaders.Size() - 1);
	this->typeMap.Add(&loaderClass, this->loaders.Size() - 1);
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceManager::RegisterLoader(const Core::Rtti& loaderClass)
{
	n_assert(this->open);
	Core::RefCounted* obj = loaderClass.Create();
	Ptr<ResourceLoader> loader((ResourceLoader*)obj);
	loader->uniqueId = UniqueLoaderCounter++;
	loader->Setup();
	this->loaders.Append(loader);
	this->typeMap.Add(&loaderClass, this->loaders.Size() - 1);
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceManager::Update(IndexT frameIndex)
{
	IndexT i;
	for (i = 0; i < this->loaders.Size(); i++)
	{
		const Ptr<ResourceLoader>& loader = this->loaders[i];
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
	for (i = 0; i < this->loaders.Size(); i++)
	{
		const Ptr<ResourceLoader>& loader = this->loaders[i];
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
	n_assert(this->loaders.Size() > loaderid);
	const Ptr<ResourceLoader>& loader = this->loaders[loaderid];
	return loader->resourceClass;
}

//------------------------------------------------------------------------------
/**
*/
template <class TYPE>
const Ptr<TYPE>&
ResourceManager::GetResource(const Resources::ResourceId id)
{
	static_assert(std::is_base_of<TYPE, Resource>::value, "Template argument is not a Resource type!");

	// get id of loader
	const Ids::Id32 lower = Ids::Id::GetLow(id);
	const Ids::Id8 loaderid = Ids::Id::GetTiny(lower);
	const Ids::Id24 resourceId = Ids::Id::GetBig(lower);

	// get resource loader by extension
	n_assert(this->loaders.Size() > loaderid);
	const Ptr<ResourceLoader>& loader = this->loaders[loaderid];
	return loader->containers[resourceId].GetResource();
}

} // namespace Resources