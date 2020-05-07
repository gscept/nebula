#pragma once
//------------------------------------------------------------------------------
/**
	The ResourceServer marks the central entry point into the Resource subsystem.
	It contains a set of convenience functions (which is just a proxy for the Singleton),
	and should be updated at least once per frame using Update().
	
	(C)2017-2020 Individual contributors, see AUTHORS file
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
class ResourceServer : public Core::RefCounted
{
	__DeclareClass(ResourceServer);
	__DeclareInterfaceSingleton(ResourceServer);
public:
	/// constructor
	ResourceServer();
	/// destructor
	virtual ~ResourceServer();

	/// open manager
	void Open();
	/// close manager
	void Close();

	/// update resource manager, call each frame
	void Update(IndexT frameIndex);

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
	/// reload resource
	void ReloadResource(const ResourceName& res, std::function<void(const Resources::ResourceId)> success = nullptr, std::function<void(const Resources::ResourceId)> failed = nullptr);
	/// stream in a new LOD
	void StreamLOD(const ResourceId& id, IndexT lod, bool immediate);

	/// reserve resource (for self-managed resources)
	Resources::ResourceId ReserveResource(const ResourceName& res, const Util::StringAtom& tag, const Core::Rtti& type);
	/// update resource (for self-managed resources), info pointer is a struct specific to the loader
	ResourcePool::LoadStatus LoadFromMemory(const Resources::ResourceId id, void* info);

	/// get type of resource pool this resource was allocated with
	Core::Rtti* GetType(const Resources::ResourceId id);

	/// wait for resource thread to finish the current job, and then pause the thread
	void WaitForLoaderThread();

	/// get resource name
	const Resources::ResourceName GetName(const Resources::ResourceId id) const;
	/// get tag resource was first registered with
	const Util::StringAtom GetTag(const Resources::ResourceId id) const;
	/// get resource state
	const Resource::State GetState(const Resources::ResourceId id) const;
	/// get usage 
	const SizeT GetUsage(const Resources::ResourceId id) const;
	/// check if resource id is valid
	bool HasResource(const Resources::ResourceId id) const;
	/// get id from name
	const Resources::ResourceId GetId(const Resources::ResourceName& name) const;

	/// register a stream pool, which takes an extension and the RTTI of the resource type to create
	void RegisterStreamPool(const Util::StringAtom& ext, const Core::Rtti& loaderClass);
	/// get stream pool for later use
	template <class POOL_TYPE> POOL_TYPE* GetStreamPool() const;
	/// register a memory pool, which maps only a resource type (RTTI)
	void RegisterMemoryPool(const Core::Rtti& loaderClass);
	/// get memory pool for later use
	template <class POOL_TYPE> POOL_TYPE* GetMemoryPool() const;

	/// goes through all pools and sets up their default resources
	void LoadDefaultResources();
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
Resources::ResourceServer::CreateResource(const ResourceName& id, std::function<void(const Resources::ResourceId)> success, std::function<void(const Resources::ResourceId)> failed, bool immediate)
{
	return this->CreateResource(id, "", success, failed, immediate);
}

//------------------------------------------------------------------------------
/**
	If a previous call to CreateResources triggered a resource load, and this evocation enforces the resource loading to be immediate, then despite
	this call not actually triggering a resource to be loaded, the referenced resource will be loaded immediately nonetheless.
*/
inline Resources::ResourceId
Resources::ResourceServer::CreateResource(const ResourceName& res, const Util::StringAtom& tag, std::function<void(const Resources::ResourceId)> success, std::function<void(const Resources::ResourceId)> failed, bool immediate)
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
*/
inline void
ResourceServer::ReloadResource(const ResourceName& res, std::function<void(const Resources::ResourceId)> success, std::function<void(const Resources::ResourceId)> failed)
{
	// get resource loader by extension
	Util::String ext = res.AsString().GetFileExtension();
	IndexT i = this->extensionMap.FindIndex(ext);
	n_assert_fmt(i != InvalidIndex, "No resource loader is associated with file extension '%s'", ext.AsCharPtr());
	const Ptr<ResourceStreamPool>& loader = this->pools[this->extensionMap.ValueAtIndex(i)].downcast<ResourceStreamPool>();

	// create container and cast to actual resource type
	loader->ReloadResource(res, success, failed);
}

//------------------------------------------------------------------------------
/**
*/
inline void 
ResourceServer::StreamLOD(const ResourceId& id, IndexT lod, bool immediate)
{
	// get id of loader
	const Ids::Id8 loaderid = id.poolIndex;

	// get resource loader by extension
	n_assert(this->pools.Size() > loaderid);
	const Ptr<ResourceStreamPool>& loader = this->pools[loaderid].downcast<ResourceStreamPool>();

	// update LOD
	loader->UpdateResourceLOD(id, lod, immediate);
}

//------------------------------------------------------------------------------
/**
	Discards a single resource, and removes the callbacks to it from
*/
inline void
ResourceServer::DiscardResource(const Resources::ResourceId id)
{
	// get id of loader
	const Ids::Id8 loaderid = id.poolIndex;

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
ResourceServer::ReserveResource(const ResourceName& res, const Util::StringAtom& tag, const Core::Rtti& type)
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
ResourceServer::LoadFromMemory(const Resources::ResourceId id, void* info)
{
	const Ptr<ResourceMemoryPool>& loader = this->pools[id.poolIndex].downcast<ResourceMemoryPool>();
	return loader->LoadFromMemory(id.resourceId, info);
}

//------------------------------------------------------------------------------
/**
*/
inline const Resources::ResourceName
ResourceServer::GetName(const Resources::ResourceId id) const
{
	// get resource loader by extension
	n_assert(this->pools.Size() > id.poolIndex);
	const Ptr<ResourcePool>& loader = this->pools[id.poolIndex];
	return loader->GetName(id);
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom
ResourceServer::GetTag(const Resources::ResourceId id) const
{
	// get resource loader by extension
	n_assert(this->pools.Size() > id.poolIndex);
	const Ptr<ResourcePool>& loader = this->pools[id.poolIndex];
	return loader->GetTag(id);
}

//------------------------------------------------------------------------------
/**
*/
inline const Resources::Resource::State
ResourceServer::GetState(const Resources::ResourceId id) const
{
	// get resource loader by extension
	n_assert(this->pools.Size() > id.poolIndex);
	const Ptr<ResourcePool>& loader = this->pools[id.poolIndex];
	return loader->GetState(id);
}

//------------------------------------------------------------------------------
/**
*/
inline const SizeT
ResourceServer::GetUsage(const Resources::ResourceId id) const
{
	// get resource loader by extension
	n_assert(this->pools.Size() > id.poolIndex);
	const Ptr<ResourcePool>& loader = this->pools[id.poolIndex];
	return loader->GetUsage(id);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
ResourceServer::HasResource(const Resources::ResourceId id) const
{
	if (this->pools.Size() <= id.poolIndex) return false;
	{
		const Ptr<ResourcePool>& loader = this->pools[id.poolIndex];
		if (loader->HasResource(id)) return true;
		return false;		
	}
}

//------------------------------------------------------------------------------
/**
*/
inline const Resources::ResourceId
ResourceServer::GetId(const Resources::ResourceName& name) const
{
	IndexT i;
	for (i = 0; i < this->pools.Size(); i++)
	{
		Resources::ResourceId id = this->pools[i]->GetId(name);
		if (id != Resources::ResourceId::Invalid()) return id;
	}
	return Resources::ResourceId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
template <class POOL_TYPE>
inline POOL_TYPE*
ResourceServer::GetStreamPool() const
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
ResourceServer::GetMemoryPool() const
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
	return ResourceServer::Instance()->CreateResource(res, tag, success, failed, immediate);
}

//------------------------------------------------------------------------------
/**
*/
inline void
StreamLOD(const ResourceId& id, IndexT lod, bool immediate)
{
	return ResourceServer::Instance()->StreamLOD(id, lod, immediate);
}

//------------------------------------------------------------------------------
/**
*/
inline void
DiscardResource(const Resources::ResourceId id)
{
	ResourceServer::Instance()->DiscardResource(id);
}

//------------------------------------------------------------------------------
/**
	Reserve a resource for direct updating through a memory pool.
	The type is the RTTI of the pool to use.
*/
inline Resources::ResourceId
ReserveResource(const ResourceName& res, const Util::StringAtom& tag, const Core::Rtti& type)
{
	return ResourceServer::Instance()->ReserveResource(res, tag, type);
}

//------------------------------------------------------------------------------
/**
	Update resource previously reserved.
	The info pointer is a struct containing pool specific information.
*/
inline ResourcePool::LoadStatus
LoadFromMemory(const Resources::ResourceId id, void* info)
{
	return ResourceServer::Instance()->LoadFromMemory(id, info);
}

//------------------------------------------------------------------------------
/**
*/
inline void 
ReloadResource(const ResourceName& res)
{
	return ResourceServer::Instance()->ReloadResource(res);
}

//------------------------------------------------------------------------------
/**
*/
inline void
WaitForLoaderThread()
{
	ResourceServer::Instance()->WaitForLoaderThread();
}

//------------------------------------------------------------------------------
/**
*/
template <class POOL_TYPE>
inline POOL_TYPE*
GetMemoryPool()
{
	static_assert(std::is_base_of<ResourcePool, POOL_TYPE>::value, "Template argument is not a ResourcePool type!");
	return ResourceServer::Instance()->GetMemoryPool<POOL_TYPE>();
}

//------------------------------------------------------------------------------
/**
*/
template <class POOL_TYPE>
inline POOL_TYPE*
GetStreamPool()
{
	static_assert(std::is_base_of<ResourcePool, POOL_TYPE>::value, "Template argument is not a ResourcePool type!");
	return ResourceServer::Instance()->GetStreamPool<POOL_TYPE>();
}

} // namespace Resources