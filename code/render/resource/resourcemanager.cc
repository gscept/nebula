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
ResourceManager::RegisterLoader(const Util::StringAtom& ext, const Core::Rtti& loaderClass)
{
	Core::RefCounted* obj = loaderClass.Create();
	Ptr<ResourceLoader> loader((ResourceLoader*)obj);
	this->loaders.Add(ext, loader);
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
		const Ptr<ResourceLoader>& loader = this->loaders.ValueAtIndex(i);
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
		const Ptr<ResourceLoader>& loader = this->loaders.ValueAtIndex(i);
		loader->DiscardByTag(tag);
	}
}

} // namespace Resources