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

struct MaterialRuntime // consider splitting into runtime and setup
{
	MaterialInstanceId id;
	MaterialType* type;
};

struct MaterialId;
RESOURCE_ID_TYPE(MaterialId);

class MaterialPool : public Resources::ResourceStreamPool
{
	__DeclareClass(MaterialPool);
public:

	/// update reserved resource, the info struct is loader dependent (overload to implement resource deallocation, remember to set resource state!)
	Resources::ResourcePool::LoadStatus LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream);

	/// get material id
	const MaterialInstanceId& GetId(const MaterialId& id);
	/// get material type
	MaterialType* const GetType(const MaterialId& id);
private:

	/// unload resource (overload to implement resource deallocation)
	void Unload(const Resources::ResourceId id);

	Ids::IdAllocatorSafe<MaterialRuntime> allocator;
	__ImplementResourceAllocatorTypedSafe(allocator, MaterialIdType);
};

//------------------------------------------------------------------------------
/**
*/
inline const MaterialInstanceId&
MaterialPool::GetId(const MaterialId& id)
{
	allocator.EnterGet();
	const MaterialInstanceId& ret = allocator.Get<0>(id.allocId).id;
	allocator.LeaveGet();
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
inline MaterialType* const
MaterialPool::GetType(const MaterialId& id)
{
	allocator.EnterGet();
	MaterialType* const ret = allocator.Get<0>(id.allocId).type;
	allocator.LeaveGet();
	return ret;
}

} // namespace Materials
