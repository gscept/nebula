#pragma once
//------------------------------------------------------------------------------
/**
	The material pool provides a chunk allocation source for material types and instances

	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/resourcestreampool.h"
#include "materialtype.h"

namespace Materials
{

struct MaterialInfo
{
	Resources::ResourceName materialType;
};

struct MaterialRuntime // consider splitting into runtime and setup
{
	MaterialId id;
	MaterialType* type;
};

struct MaterialId;

class MaterialPool : public Resources::ResourceStreamPool
{
	__DeclareClass(MaterialPool);
public:

	/// update reserved resource, the info struct is loader dependent (overload to implement resource deallocation, remember to set resource state!)
	Resources::ResourcePool::LoadStatus LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream);

	/// get material id
	const MaterialId& GetId(const Resources::ResourceId& id);
private:

	/// unload resource (overload to implement resource deallocation)
	void Unload(const Resources::ResourceId id);

	Ids::IdAllocatorSafe<MaterialRuntime> allocator;
	__ImplementResourceAllocatorSafe(allocator);
};

//------------------------------------------------------------------------------
/**
*/
inline const MaterialId&
MaterialPool::GetId(const Resources::ResourceId& id)
{
	return allocator.Get<0>(id.allocId).id;
}

} // namespace Materials
