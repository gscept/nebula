#pragma once
//------------------------------------------------------------------------------
/**
	The resource pool implements a resource specific pool, which implements
	loading, unloading and fetching of resource.

	To implement a custom pool, look at ResourceStreamPool for file loading,
	and ResourceMemoryPool for memory loading.

	For ResourceStreamPool, Load and Unload must be implemented.
	For ResourceMemoryPool, UpdateResource and Unload must be implemented.

	For both, the following functions must be implemented:
		AllocResource
		DeallocResource

	The ResourceAllocator class lets you implement an efficient resource allocator
	straight away, by simply putting the __ResourceAllocator macro in your class,
	however it is also viable to implement your own ResourceAllocator type if 
	necessary. This macro implements the Alloc and Dealloc functions.

	Use the __ResourceAllocatorType to construct an allocator, it takes a list
	of types which is then implemented as separate arrays. Couple it with the
	__ImplementResourceAllocator to implement the allocator functions.

	is not, because the comma after uint32_t will end the first macro argument, and Util::Array<VkDescriptorSetLayoutBinding>>
	becomes the second, and setBindings the third (when the macro only takes two arguments).

	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "ids/idpool.h"
#include "ids/idallocator.h"
#include "rttiarray.h"
#include "resource.h"
#include "resourceid.h"
#include <tuple>


#define __ImplementResourceAllocator(name) \
	inline Ids::Id32 AllocResource() { return name.AllocResource(); } \
	inline void DeallocResource(const Ids::Id32 id) { name.DeallocResource(id); } \
	template<int MEMBER> inline auto& Get(const Ids::Id24 id) { return name.Get<MEMBER>(id); } \
	template<int MEMBER> inline auto& Get(const Resources::ResourceId id) { return name.Get<MEMBER>(id); }

#define __ImplementResourceAllocatorSafe(name) \
	inline Ids::Id32 AllocResource() { return name.AllocResource(); } \
	inline void DeallocResource(const Ids::Id32 id) { name.DeallocResource(id); } \
	inline void EnterGet() { name.EnterGet(); } \
	inline void LeaveGet() { name.LeaveGet(); } \
	template<int MEMBER> inline auto& Get(const Ids::Id24 id) { return name.Get<MEMBER>(id); } \
	template<int MEMBER> inline auto& Get(const Resources::ResourceId id) { return name.Get<MEMBER>(id); } \
	template<int MEMBER> inline auto& GetUnsafe(const Ids::Id24 id) { return name.GetUnsafe<MEMBER>(id); } \
	template<int MEMBER> inline auto& GetUnsafe(const Resources::ResourceId id) { return name.GetUnsafe<MEMBER>(id); } \
	template<int MEMBER> inline auto& GetSafe(const Ids::Id24 id) { return name.GetSafe<MEMBER>(id); } \
	template<int MEMBER> inline auto& GetSafe(const Resources::ResourceId id) { return name.GetSafe<MEMBER>(id); }

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

	/// get resource name
	const Resources::ResourceName GetName(const Resources::ResourceId id);
	/// get resource name directly
	const Resources::ResourceName GetName(const Ids::Id24 id);
	/// get resource tag was first registered with
	const Util::StringAtom GetTag(const Resources::ResourceId id);
	/// get resource tag directly
	const Util::StringAtom GetTag(const Ids::Id24 id);
	/// get resource state
	const Resource::State GetState(const Resources::ResourceId id);
	/// get resource state directly
	const Resource::State GetState(const Ids::Id24 id);

	/// update the resource loader, this is done every frame
	virtual void Update(IndexT frameIndex);

	enum LoadStatus
	{
		Success,		/// resource is properly loaded
		Failed,			/// resource loading failed
		Delay,			/// resource is loaded at some later point
		Threaded		/// resource is loaded from a thread, which is like Delay, but is no longer pending
	};
	static const uint32_t ResourceIndexGrow = 512;
protected:
	friend class ResourceManager;

	/// request new resource and generate id for it, implement in subclass
	virtual Ids::Id32 AllocResource() = 0;
	/// deallocate resource
	virtual void DeallocResource(const Ids::Id32 id) = 0;
	/// unload resource (overload to implement resource deallocation)
	virtual void Unload(const Ids::Id24 id) = 0;

	/// id in resource manager
	int32_t uniqueId;

	Util::Dictionary<Resources::ResourceName, Ids::Id24> ids;
	Ids::IdPool resourceInstanceIndexPool;

	Util::FixedArray<uint32_t> usage;
	Util::FixedArray<Resources::ResourceName> names;
	Util::FixedArray<Util::StringAtom> tags;
	Util::FixedArray<Resource::State> states;
	uint32_t uniqueResourceId;
};


//------------------------------------------------------------------------------
/**
*/
inline const Resources::ResourceName
ResourcePool::GetName(const Resources::ResourceId id)
{
	const Ids::Id24 resId = Ids::Id::GetBig(Ids::Id::GetLow(id));
	return this->names[resId];
}

//------------------------------------------------------------------------------
/**
*/
inline const Resources::ResourceName
ResourcePool::GetName(const Ids::Id24 id)
{
	return this->names[id];
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom
ResourcePool::GetTag(const Resources::ResourceId id)
{
	const Ids::Id24 resId = Ids::Id::GetBig(Ids::Id::GetLow(id));
	return this->tags[resId];
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom
ResourcePool::GetTag(const Ids::Id24 id)
{
	return this->tags[id];
}

//------------------------------------------------------------------------------
/**
*/
inline const Resources::Resource::State
ResourcePool::GetState(const Resources::ResourceId id)
{
	const Ids::Id24 resId = Ids::Id::GetBig(Ids::Id::GetLow(id));
	return this->states[resId];
}

//------------------------------------------------------------------------------
/**
*/
inline const Resources::Resource::State
ResourcePool::GetState(const Ids::Id24 id)
{
	return this->states[id];
}

} // namespace Resources