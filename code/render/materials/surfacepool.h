#pragma once
//------------------------------------------------------------------------------
/**
	The material pool provides a chunk allocation source for material types and instances

	(C)2017-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/resourcestreampool.h"
#include "materialtype.h"
#include "coregraphics/config.h"

namespace Materials
{

struct MaterialInfo
{
	Resources::ResourceName materialType;
};

struct SurfaceRuntime // consider splitting into runtime and setup
{
	SurfaceId id;
	MaterialType* type;
};

struct SurfaceResourceId;
RESOURCE_ID_TYPE(SurfaceResourceId);

class SurfacePool : public Resources::ResourceStreamPool
{
	__DeclareClass(SurfacePool);
public:

	/// update reserved resource, the info struct is loader dependent (overload to implement resource deallocation, remember to set resource state!)
	Resources::ResourcePool::LoadStatus LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream);

	/// get material id
	const SurfaceId GetId(const SurfaceResourceId id);
	/// get material type
	MaterialType* const GetType(const SurfaceResourceId id);
private:

	/// unload resource (overload to implement resource deallocation)
	void Unload(const Resources::ResourceId id);

	Ids::IdAllocatorSafe<SurfaceRuntime> allocator;
	__ImplementResourceAllocatorTypedSafe(allocator, MaterialIdType);
};

//------------------------------------------------------------------------------
/**
*/
inline const SurfaceId
SurfacePool::GetId(const SurfaceResourceId id)
{
	allocator.EnterGet();
	const SurfaceId ret = allocator.Get<0>(id.allocId).id;
	allocator.LeaveGet();
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
inline MaterialType* const
SurfacePool::GetType(const SurfaceResourceId id)
{
	allocator.EnterGet();
	MaterialType* const ret = allocator.Get<0>(id.allocId).type;
	allocator.LeaveGet();
	return ret;
}

} // namespace Materials
