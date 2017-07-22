#pragma once
//------------------------------------------------------------------------------
/**
	The resource pool implements a resource specific pool, which implements
	loading, unloading and fetching of resource.
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resourcecontainer.h"
#include "ids/idpool.h"
namespace Resources
{
class ResourcePool : public Core::RefCounted
{
	__DeclareAbstractClass(ResourcePool);
public:
	/// constructor
	ResourcePool();
	/// destructor
	virtual ~ResourcePool();

	/// setup resource loader, initiates the placeholder and error resources if valid
	virtual void Setup();
	/// discard resource loader
	virtual void Discard();

	/// discard resource instance
	virtual void DiscardResource(const Resources::ResourceId id);
	/// discard all resources associated with a tag
	virtual void DiscardByTag(const Util::StringAtom& tag);

	/// update the resource loader, this is done every frame
	virtual void Update(IndexT frameIndex);

	enum LoadStatus
	{
		Success,		/// resource is properly loaded
		Failed,			/// resource loading failed
		Delay,			/// resource is loaded at some later point
		Threaded		/// resource is loaded from a thread, which is like Delay, but is no longer pending
	};
protected:
	friend class ResourceManager;

	/// id in resource manager
	int32_t uniqueId;

	Util::Dictionary<Resources::ResourceName, Ids::Id24> ids;
	Ids::IdPool resourceInstanceIndexPool;
	Ids::IdPool resourceIndexPool;

	Util::FixedArray<ResourceContainer> containers;
	Util::FixedArray<uint32_t> usage;
	Util::FixedArray<Resources::ResourceName> names;
	uint32_t uniqueResourceId;

	Core::Rtti* resourceClass;
};
} // namespace Resources