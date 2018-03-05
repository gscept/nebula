#pragma once
//------------------------------------------------------------------------------
/**
	The material pool provides a chunk allocation source for material types and instances

	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/resourcestreampool.h"
namespace Materials
{

struct MaterialInfo
{
	Resources::ResourceName materialType;
};

class MaterialPool : public Resources::ResourceStreamPool
{
	__DeclareClass(MaterialPool);
public:

	/// update reserved resource, the info struct is loader dependent (overload to implement resource deallocation, remember to set resource state!)
	Resources::ResourcePool::LoadStatus LoadFromStream(const Ids::Id24 id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream);

private:

	/// allocate object by probing the material type from the file
	Resources::ResourceUnknownId AllocObject();
	/// deallocate resource
	void DeallocObject(const Resources::ResourceUnknownId id);
	/// unload resource (overload to implement resource deallocation)
	void Unload(const Ids::Id24 id);
};

} // namespace Materials
