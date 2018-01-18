#pragma once
//------------------------------------------------------------------------------
/**
	The ResourceManager marks the central entry point into the Resource subsystem.
	It contains a set of convenience functions (which is just a proxy for the Singleton),
	and should be updated at least once per frame using Update().
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include <functional>
#include "core/refcounted.h"
#include "ids/id.h"
#include "core/singleton.h"
#include "resourceid.h"
#include "resourcestreampool.h"
#include "resourceloaderthread.h"
#include "resourcememorypool.h"
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

	/// open manager
	void Open();
	/// close manager
	void Close();

	/// enter lock-step mode, during this phase, resources may not be discarded
	void EnterLockstep();
	/// update resource manager, call each frame
	void Update(IndexT frameIndex);
	/// exit lock-step mode
	void ExitLockstep();

	/// create a new resource (stream-managed), which will be loaded at some later point, if not already loaded
	Resources::ResourceId CreateResource(const ResourceName& id, std::function<void(const Resources::ResourceId)> success = nullptr, std::function<void(const Resources::ResourceId)> failed = nullptr, bool immediate = false);
	/// overload which also takes an identifying tag, which is used to group-discard resources
	Resources::ResourceId CreateResource(const ResourceName& id, const Util::StringAtom& tag, std::function<void(const Resources::ResourceId)> success = nullptr, std::function<void(const Resources::ResourceId)> failed = nullptr, bool immediate = false);
	/// discard resource (stream-managed)
	void DiscardResource(const Resources::ResourceId res);
	/// discard all resources by tag (stream-managed)
	void DiscardResources(const Util::StringAtom& tag);
	/// returns true if there are pending resources in-flight
	bool HasPendingResources();

	/// reserve resource (for self-managed resources)
	Resources::ResourceId ReserveResource(const ResourceName& res, const Util::StringAtom& tag, const Core::Rtti& type);
	/// update resource (for self-managed resources), info pointer is a struct specific to the loader
	ResourcePool::LoadStatus LoadFromMemory(const Resources::ResourceId id, void* info);

	/// get type of resource pool this resource was allocated with
	Core::Rtti* GetType(const Resources::ResourceId id);

	/// get resource name
	const Resources::ResourceName GetName(const Resources::ResourceId id);
	/// get tag resource was first registered with
	const Util::StringAtom GetTag(const Resources::ResourceId id);
	/// get resource state
	const Resource::State GetState(const Resources::ResourceId id);
	/// check if resource id is valid
	bool HasResource(const Resources::ResourceId id);
	/// get id from name
	const Resources::ResourceId GetId(const Resources::ResourceName& name);

	/// register a stream pool, which takes an extension and the RTTI of the resource type to create
	void RegisterStreamPool(const Util::StringAtom& ext, const Core::Rtti& loaderClass);
	/// get stream pool for later use
	template <class POOL_TYPE> POOL_TYPE* GetStreamPool() const;
	/// register a memory pool, which maps only a resource type (RTTI)
	void RegisterMemoryPool(const Core::Rtti& loaderClass);
	/// get memory pool for later use
	template <class POOL_TYPE> POOL_TYPE* GetMemoryPool() const;
private:
	friend class ResourceStreamPool;

	bool open;
	Ptr<ResourceLoaderThread> loaderThread;
	Util::Dictionary<Util::StringAtom, IndexT> extensionMap;
	Util::Dictionary<const Core::Rtti*, IndexT> typeMap;
	Util::Array<Ptr<ResourcePool>> pools;

	static int32_t UniquePoolCounter;
};

//------------------------------------------------------------------------------
/**
	If a previous call to CreateResources triggered a resource load, and this evocation enforces the resource loading to be immediate, then despite
	this call not actually triggering a resource to be loaded, the referenced resource will be loaded immediately nonetheless.
*/
inline Resources::ResourceId
Resources::ResourceManager::CreateResource(const ResourceName& id, std::function<void(const Resources::ResourceId)> success, std::function<void(const Resources::ResourceId)> failed, bool immediate)
{
	return this->CreateResource(id, "", success, failed, immediate);
}

//------------------------------------------------------------------------------
/**
	If a previous call to CreateResources triggered a resource load, and this evocation enforces the resource loading to be immediate, then despite
	this call not actually triggering a resource to be loaded, the referenced resource will be loaded immediately nonetheless.
*/
inline Resources::ResourceId
Resources::ResourceManager::CreateResource(const ResourceName& res, const Util::StringAtom& tag, std::function<void(const Resources::ResourceId)> success, std::function<void(const Resources::ResourceId)> failed, bool immediate)
{
	// get resource loader by extension
	Util::String ext = res.AsString().GetFileExtension();
	IndexT i = this->extensionMap.FindIndex(ext);
	n_assert_fmt(i != InvalidIndex, "No resource loader is associated with file extension '%s'", ext.AsCharPtr());
	const Ptr<ResourceStreamPool>& loader = this->pools[this->extensionMap.ValueAtIndex(i)].downcast<ResourceStreamPool>();

	// create container and cast to actual resource type
	Resources::ResourceId id = loader->CreateResource(res, tag, success, failed, immediate);
	return id;
}

//------------------------------------------------------------------------------
/**
	Discards a single resource, and removes the callbacks to it from
*/
inline void
Resources::ResourceManager::DiscardResource(const Resources::ResourceId id)
{
	// get id of loader
	const Ids::Id8 loaderid = id.id8_0;

	// get resource loader by extension
	n_assert(this->pools.Size() > loaderid);
	const Ptr<ResourceStreamPool>& loader = this->pools[loaderid].downcast<ResourceStreamPool>();

	// discard container
	loader->DiscardResource(id);
}

//------------------------------------------------------------------------------
/**
	Reserve a resource for direct updating through a memory pool.
	The type is the RTTI of the pool to use.
*/
inline Resources::ResourceId
ResourceManager::ReserveResource(const ResourceName& res, const Util::StringAtom& tag, const Core::Rtti& type)
{
	n_assert(type.IsDerivedFrom(ResourceMemoryPool::RTTI));
	const Ptr<ResourceMemoryPool>& loader = this->pools[this->typeMap[&type]].downcast<ResourceMemoryPool>();
	return loader->ReserveResource(res, tag);
}

//------------------------------------------------------------------------------
/**
	Update resource previously reserved.
	The info pointer is a struct containing pool specific information.
*/
inline Resources::ResourcePool::LoadStatus
ResourceManager::LoadFromMemory(const Resources::ResourceId id, void* info)
{
	const Ptr<ResourceMemoryPool>& loader = this->pools[id.id8_0].downcast<ResourceMemoryPool>();
	return loader->LoadFromMemory(id.id24_1, info);
}

//------------------------------------------------------------------------------
/**
*/
inline const Resources::ResourceName
ResourceManager::GetName(const Resources::ResourceId id)
{
	// get resource loader by extension
	n_assert(this->pools.Size() > id.id8_0);
	const Ptr<ResourcePool>& loader = this->pools[id.id8_0];
	return loader->GetName(id);
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom
ResourceManager::GetTag(const Resources::ResourceId id)
{
	// get resource loader by extension
	n_assert(this->pools.Size() > id.id8_0);
	const Ptr<ResourcePool>& loader = this->pools[id.id8_0];
	return loader->GetTag(id);
}

//------------------------------------------------------------------------------
/**
*/
inline const Resources::Resource::State
ResourceManager::GetState(const Resources::ResourceId id)
{
	// get resource loader by extension
	n_assert(this->pools.Size() > id.id8_0);
	const Ptr<ResourcePool>& loader = this->pools[id.id8_0];
	return loader->GetState(id);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
ResourceManager::HasResource(const Resources::ResourceId id)
{
	if (this->pools.Size() <= id.id8_0) return false;
	{
		const Ptr<ResourcePool>& loader = this->pools[id.id8_0];
		if (loader->HasResource(id)) return true;
		return false;		
	}
}

//------------------------------------------------------------------------------
/**
*/
inline const Resources::ResourceId
ResourceManager::GetId(const Resources::ResourceName& name)
{
	return Resources::ResourceId();
}

//------------------------------------------------------------------------------
/**
*/
template <class POOL_TYPE>
inline POOL_TYPE*
Resources::ResourceManager::GetStreamPool() const
{
	static_assert(std::is_base_of<ResourceStreamPool, POOL_TYPE>::value, "Type requested is not a stream pool");
	IndexT i;
	for (i = 0; i < this->pools.Size(); i++)
	{
		if (this->pools[i]->GetRtti()->IsDerivedFrom(POOL_TYPE::RTTI)) return static_cast<POOL_TYPE*>(this->pools[i].get());
	}

	n_error("No loader registered for this type");
	return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
template <class POOL_TYPE>
inline POOL_TYPE*
Resources::ResourceManager::GetMemoryPool() const
{
	static_assert(std::is_base_of<ResourceMemoryPool, POOL_TYPE>::value, "Type requested is not a memory pool");
	IndexT i;
	for (i = 0; i < this->pools.Size(); i++)
	{
		if (this->pools[i]->GetRtti()->IsDerivedFrom(POOL_TYPE::RTTI)) return static_cast<POOL_TYPE*>(this->pools[i].get());
	}

	n_error("No loader registered for this type");
	return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
inline Resources::ResourceId
CreateResource(const ResourceName& res, const Util::StringAtom& tag, std::function<void(const Resources::ResourceId)> success = nullptr, std::function<void(const Resources::ResourceId)> failed = nullptr, bool immediate = false)
{
	return ResourceManager::Instance()->CreateResource(res, tag, success, failed, immediate);
}

//------------------------------------------------------------------------------
/**
*/
inline void
DiscardResource(const Resources::ResourceId id)
{
	ResourceManager::Instance()->DiscardResource(id);
}

//------------------------------------------------------------------------------
/**
	Reserve a resource for direct updating through a memory pool.
	The type is the RTTI of the pool to use.
*/
inline Resources::ResourceId
ReserveResource(const ResourceName& res, const Util::StringAtom& tag, const Core::Rtti& type)
{
	return ResourceManager::Instance()->ReserveResource(res, tag, type);
}

//------------------------------------------------------------------------------
/**
	Update resource previously reserved.
	The info pointer is a struct containing pool specific information.
*/
inline ResourcePool::LoadStatus
LoadFromMemory(const Resources::ResourceId id, void* info)
{
	return ResourceManager::Instance()->LoadFromMemory(id, info);
}

//------------------------------------------------------------------------------
/**
	Get the type of pool used to create the resource.
*/
inline Core::Rtti*
GetType(const Resources::ResourceId id)
{
	ResourceManager::Instance()->GetType(id);
}

//------------------------------------------------------------------------------
/**
*/
template <class POOL_TYPE>
inline POOL_TYPE*
GetMemoryPool()
{
	static_assert(std::is_base_of<ResourcePool, TYPE>::value, "Template argument is not a Resource pool type!");
	return ResourceManager::Instance()->GetMemoryPool<POOL_TYPE>();
}

//------------------------------------------------------------------------------
/**
*/
template <class POOL_TYPE>
inline POOL_TYPE*
GetStreamPool()
{
	static_assert(std::is_base_of<ResourcePool, TYPE>::value, "Template argument is not a Resource pool type!");
	return ResourceManager::Instance()->GetStreamPool<POOL_TYPE>();
}

} // namespace Resources