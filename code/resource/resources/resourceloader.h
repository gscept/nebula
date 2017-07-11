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

	Each resource loader also keeps a list of the resources loaded by it. Therefore,
	the ResourceManager is not responsible for maintaining which resources are loaded.

	The loader associates a resource name (StringAtom) with an id, such that it can be quickly
	retrieved. 
	
	When creating an instance of a resource, an ID is returned, this ID contains the following:
	32 bits (resource instance id), 24 bits (resource id) and 8 bits (loader id). The instance id
	is a recyclable number which uniquely identifies a single allocation. The next 24 bits is the
	internal ID for the resource, which is only loaded once. The last 8 bits identifies which loader
	created the resource. 

	Resources created with tags must also be removed using the tag. A tagged resource can only
	be discarded by using that tag. 
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/id.h"
#include "util/stringatom.h"
#include "io/stream.h"
#include "jobs/jobport.h"
#include "util/set.h"
#include "resourcecontainer.h"
#include "resource.h"
#include <tuple>
#include <functional>
#include "core/idpool.h"

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
		Core::Id id;
		Ptr<Resource> res;
		std::function<void(const Core::Id&)> success;
		std::function<void(const Core::Id&)> failed;
		Util::StringAtom tag;
		bool inflight;
		std::function<void()> loadFunc;
		bool immediate;
	};

	/// create a container with a tag associated with it, if no tag is provided, the resource will be untagged
	Core::Id CreateResource(const ResourceId& res, const Util::StringAtom& tag, std::function<void(const Core::Id&)> success, std::function<void(const Core::Id&)> failed, bool immediate);
	/// discard container
	void DiscardResource(const Core::Id& res);
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
	int32_t uniqueId;

	Ptr<Resource> placeholderResource;
	Ptr<Resource> errorResource;

	bool async;
	Ptr<Jobs::JobPort> jobport;

	// old way
	Util::Dictionary<Util::StringAtom, _PendingResource> pending;
	//Util::Dictionary<Util::StringAtom, Ptr<Resource>> loaded;
	//Util::Dictionary<Util::StringAtom, Util::StringAtom> tags;
	//Util::Dictionary<Util::StringAtom, int> usage;

	// 
	Util::Dictionary<Resources::ResourceId, uint32_t> ids;
	Core::IdPool resourceInstanceIndexPool;
	Core::IdPool resourceIndexPool;

	Util::Array<Ptr<Resource>> loaded;
	Util::FixedArray<ResourceContainer> containers;
	Util::FixedArray<uint32_t> usage;
	uint32_t uniqueResourceId;

	Threading::CriticalSection asyncSection;
};


} // namespace Resources