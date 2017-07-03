#pragma once
//------------------------------------------------------------------------------
/**
	Base class for resource loaders.

	Contains the names for the placeholder and failed-to-load resource names.
	When inheriting from this class, make sure to provide proper resource ids for:
		1. Placeholder resource
		2. Error resource

	If no placeholder resource is provided, the loader cannot execute asynchronously.
	If no error resource is provided and the resource fails to load, then the ResourceManager
	will raise an assertion. 

	Each resource loader also keeps a list of the resources loaded by it. That is,
	the ResourceManager is not responsible for maintaining which resources are loaded.

	The resource loader also maintains a tag-to-resource database, which associates
	the resources loaded by this loader with a tag, and allows all resources for that
	specific tag to be deleted in one swipe, instead of having to release them individually.

	Resources created with tags must also be removed using the tag. A tagged resource can only
	be discarded by using that tag. 

	TODO:
		Because pending resources are put in a dictionary, they will be continuously sorted,
		which means an early loaded resource might get postponed when the dictionary is sorted.
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "util/stringatom.h"
#include "io/stream.h"
#include "jobs/jobport.h"
#include "util/set.h"
#include "resourcecontainer.h"
#include "resource.h"
#include <tuple>
#include <functional>

namespace Resources
{
class Resource;
class ResourceLoader : public Core::RefCounted
{
	__DeclareAbstractClass(ResourceLoader);

public:
	/// constructor
	ResourceLoader();
	/// destructor
	virtual ~ResourceLoader();

	/// setup resource loader, initiates the placeholder and error resources if valid
	virtual void Setup();
	/// discard resource loader
	virtual void Discard();

protected:
	friend class ResourceManager;

	enum LoadStatus
	{
		Success,		/// resource is properly loaded
		Failed,			/// resource loading failed
		Delay,			/// resource is loaded at some later point
		Threaded		/// resource is loaded from a thread, which is like Delay, but is no longer pending
	};

	struct _PendingResource
	{
		Ptr<Resource> res;
		std::function<void(const Ptr<Resource>&)> success;
		std::function<void(const Ptr<Resource>&)> failed;
		Util::StringAtom tag;
		bool inflight;
		std::function<void()> loadFunc;
		bool immediate;
	};

	/// create a container with a tag associated with it, if no tag is provided, the resource will be untagged
	template <class RESOURCE_TYPE> Ptr<ResourceContainer<RESOURCE_TYPE>> CreateContainer(const ResourceId& res, const Util::StringAtom& tag, std::function<void(const Ptr<Resource>&)> success, std::function<void(const Ptr<Resource>&)> failed, bool immediate);
	/// discard container
	template <class RESOURCE_TYPE> void DiscardContainer(const Ptr<ResourceContainer<RESOURCE_TYPE>>& res);
	/// discard all resources associated with a tag
	void DiscardByTag(const Util::StringAtom& tag);
	/// start loading
	LoadStatus PrepareLoad(_PendingResource& res);

	/// perform actual load, override in subclass
	virtual LoadStatus Load(const Ptr<Resource>& res, const Ptr<IO::Stream>& stream) = 0;
	/// unload resource
	virtual void Unload(const Ptr<Resource>& res) = 0;

	/// update the resource loader, this is done every frame
	void Update(IndexT frameIndex);

	/// these types need to be properly initiated in a subclass Setup function
	Util::StringAtom placeholderResourceId;
	Util::StringAtom errorResourceId;
	Core::Rtti* resourceClass;

	Ptr<Resource> placeholderResource;
	Ptr<Resource> errorResource;

	bool async;
	Ptr<Jobs::JobPort> jobport;

	Util::Dictionary<Util::StringAtom, _PendingResource> pending;
	Util::Dictionary<Util::StringAtom, Ptr<Resource>> loaded;
	Util::Dictionary<Util::StringAtom, Util::StringAtom> tags;
	Util::Dictionary<Util::StringAtom, int> usage;

	Threading::CriticalSection asyncSection;
};

//------------------------------------------------------------------------------
/**
*/
template <class RESOURCE_TYPE> 
inline Ptr<ResourceContainer<RESOURCE_TYPE>>
Resources::ResourceLoader::CreateContainer(const ResourceId& res, const Util::StringAtom& tag, std::function<void(const Ptr<Resource>&)> success, std::function<void(const Ptr<Resource>&)> failed, bool immediate)
{
	static_assert(std::is_base_of<Resource, RESOURCE_TYPE>::value, "Type is not a subclass of Resources::Resource");
	n_assert(this->resourceClass != nullptr);	
	n_assert(this->resourceClass->IsDerivedFrom(RESOURCE_TYPE::RTTI));
	Ptr<ResourceContainer<RESOURCE_TYPE>> container = ResourceContainer<RESOURCE_TYPE>::Create();
	container->error = this->errorResource;
	container->placeholder = this->placeholderResource;
	IndexT i = this->loaded.FindIndex(res);
	if (i != InvalidIndex)
	{
		container->resource = this->loaded.ValueAtIndex(i);

		// run success callback immediately
		if (success != nullptr) success(container->resource);

		// we can guarantee usage is valid already
		this->usage.ValueAtIndex(i)++;
	}
	else
	{
		IndexT usage = this->usage.FindIndex(res);
		if (usage == InvalidIndex) this->usage.Add(res, 1);
		else					   this->usage.ValueAtIndex(usage)++;

		// in case the resource is being loaded asynchronously, it should still be in pending
		this->criticalSection.Enter();
		IndexT i = this->pending.FindIndex(res);
		if (i != InvalidIndex)
		{
			const _PendingResource& pres = this->pending.ValueAtIndex(i);
			container->resource = pres.res;
		}
		else
		{
			container->resource = this->resourceClass->Create();
			container->resource->resourceId = res;
			container->resource->tag = tag;
			this->pending.Add(res, { container->resource, success, failed, tag, false, nullptr, immediate });
		}		
		this->criticalSection.Leave();
	}
	return container;
}

//------------------------------------------------------------------------------
/**
*/
template <class RESOURCE_TYPE>
inline void
Resources::ResourceLoader::DiscardContainer(const Ptr<ResourceContainer<RESOURCE_TYPE>>& res)
{
	static_assert(std::is_base_of<Resource, RESOURCE_TYPE>::value, "Type is not a subclass of Resources::Resource");
	n_assert(this->resourceClass != nullptr);
	n_assert(this->resourceClass->IsDerivedFrom(RESOURCE_TYPE::RTTI));	
	
	IndexT i = this->loaded.FindIndex(res->resource->resourceId);
	if (i == InvalidIndex)
	{
		this->criticalSection.Enter();
		IndexT j = this->pending.FindIndex(res->resource->resourceId);
		n_assert(j != InvalidIndex);
		const _PendingResource& res = this->pending.ValueAtIndex(j);

		// hmm, resource is being loaded by a thread
		if (res.inflight)
		{
			// wait for resource loader thread to finish, then delete it
			// this is very unfortunate, but we cannot abort the thread job safely without a sync point
			n_assert(res.loadFunc != nullptr);
			ResourceManager::Instance()->loaderThread->Wait();
		}
		else
		{
			// call failed function to indicate the load was aborted
			if (res.failed != nullptr) res.failed(res.res);

			// just remove from pending, easy peasy

			this->pending.EraseAtIndex(j);
		}
		this->criticalSection.Leave();
	}
	else
	{
		n_assert_fmt(!res->resource->tag.IsValid(), "Resource with tag can not be individually deleted");

		// if usage is 1, and we are just about to discard it, unload resource and cleanup dictionaries
		this->usage.ValueAtIndex(i)--;
		if (this->usage.ValueAtIndex(i) == 0)
		{
			this->Unload(this->loaded.ValueAtIndex(i));
			this->loaded.EraseAtIndex(i);
			this->usage.EraseAtIndex(i);
			this->tags.EraseAtIndex(i);
		}
	}	
}

} // namespace Resources