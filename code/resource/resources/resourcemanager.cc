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
ResourceManager::RegisterLoader(const Util::StringAtom& ext, const Core::Rtti& loaderClass)
{
	n_assert(this->open);
	Core::RefCounted* obj = loaderClass.Create();
	Ptr<ResourceLoader> loader((ResourceLoader*)obj);
	loader->uniqueId = UniqueLoaderCounter++;
	loader->Setup();
	this->loaders.Append(loader);
	this->extensionMap.Add(ext, this->loaders.Size() - 1);
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



} // namespace Resources