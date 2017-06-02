#pragma once
//------------------------------------------------------------------------------
/**
	Description
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include <functional>
#include "core/refcounted.h"
#include "core/singleton.h"
#include "resourcecontainer.h"
#include "resourceid.h"
#include "resourceloader.h"
namespace Resources
{
class ResourceManager : public Core::RefCounted
{
	__DeclareClass(ResourceManager);
	__DeclareSingleton(ResourceManager);
public:
	/// constructor
	ResourceManager();
	/// destructor
	virtual ~ResourceManager();

	/// update resource manager, call each frame
	void Update(IndexT frameIndex);

	/// create a new resource, which will be loaded at some later point, if not already loaded
	template <class RESOURCE_TYPE> Ptr<ResourceContainer<RESOURCE_TYPE>> CreateResource(const ResourceId& id, std::function<void(const Ptr<RESOURCE_TYPE>&)> success, std::function<void(const Ptr<RESOURCE_TYPE>&)> failed);
	/// overload which also takes an identifying tag, which is used to group-discard resources
	template <class RESOURCE_TYPE> Ptr<ResourceContainer<RESOURCE_TYPE>> CreateResource(const ResourceId& id, const Util::StringAtom& tag, std::function<void(const Ptr<RESOURCE_TYPE>&)> success, std::function<void(const Ptr<RESOURCE_TYPE>&)> failed);
	/// discard resource
	template <class RESOURCE_TYPE> void DiscardResource(const Ptr<ResourceContainer<RESOURCE_TYPE>>& res);
	/// discard all resources by tag
	void DiscardResources(const Util::StringAtom& tag);

	/// register a resource loader by using RTTI, this function will also create the loader
	void RegisterLoader(const Util::StringAtom& ext, const Core::Rtti& loaderClass);
private:

	Util::Dictionary<Util::StringAtom, Ptr<ResourceLoader>> loaders;
};

//------------------------------------------------------------------------------
/**
*/
template <class RESOURCE_TYPE> 	
inline Ptr<ResourceContainer<RESOURCE_TYPE>>
Resource::ResourceManager::CreateResource(const ResourceId& id, std::function<void(const Ptr<RESOURCE_TYPE>&)> success, std::function<void(const Ptr<RESOURCE_TYPE>&)> failed)
{
	return this->CreateResource(id, "", success, failed);
}

//------------------------------------------------------------------------------
/**
*/
template <class RESOURCE_TYPE> 
inline Ptr<ResourceContainer<RESOURCE_TYPE>>
Resource::ResourceManager::CreateResource(const ResourceId& id, const Util::StringAtom& tag, std::function<void(const Ptr<RESOURCE_TYPE>&)> success, std::function<void(const Ptr<RESOURCE_TYPE>&)> failed)
{
	static_assert(std::is_base_of(Resource, RESOURCE_TYPE));

	// get resource loader by extension
	Util::String ext = id.AsString().GetFileExtension();
	IndexT i = this->loaders.FindIndex(ext);
	n_assert_fmt(i != InvalidIndex, "No resource loader is associated with file extension '%s'", ext.AsCharPtr());
	const Ptr<ResourceLoader>& loader = this->loaders.ValueAtIndex(i);

	// create container and cast to actual resource type
	Ptr<ResourceContainer<RESOURCE_TYPE>> container = loader->CreateContainer<RESOURCE_TYPE>(id, tag, success, failed);
	return container;
}

//------------------------------------------------------------------------------
/**
*/
template <class RESOURCE_TYPE>
inline void
Resource::ResourceManager::DiscardResource(const Ptr<ResourceContainer<RESOURCE_TYPE>>& res)
{
	static_assert(std::is_base_of(Resource, RESOURCE_TYPE));

	// get resource loader by extension
	Util::String ext = res->resource->resourceId.AsString().GetFileExtension();
	IndexT i = this->loaders.FindIndex(ext);
	n_assert_fmt(i != InvalidIndex, "No resource loader is associated with file extension '%s'", ext.AsCharPtr());
	const Ptr<ResourceLoader>& loader = this->loaders.ValueAtIndex(i);

	// discard container
	loader->DiscardContainer(res);
}

} // namespace Resources