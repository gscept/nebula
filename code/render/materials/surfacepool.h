#pragma once
//------------------------------------------------------------------------------
/**
	The material pool provides a chunk allocation source for material types and instances

	(C)2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/resourcestreampool.h"
#include "materialtype.h"
#include "coregraphics/config.h"
#include "threading/criticalsection.h"

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

	/// setup resource loader, initiates the placeholder and error resources if valid, so don't forget to run!
	virtual void Setup() override;

	/// update reserved resource, the info struct is loader dependent (overload to implement resource deallocation, remember to set resource state!)
	Resources::ResourcePool::LoadStatus LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate = false) override;

	/// get material id
	const SurfaceId GetId(const SurfaceResourceId id);
	/// get material type
	MaterialType* const GetType(const SurfaceResourceId id);
	/// update lod for textures in surface 
	void SetMaxLOD(const SurfaceResourceId id, const float lod);
private:

	/// unload resource (overload to implement resource deallocation)
	void Unload(const Resources::ResourceId id);

	enum
	{
		Surface_SurfaceId,
		Surface_MaterialType,
		Surface_Textures,
		Surface_MinLOD
	};

	Ids::IdAllocator<
		SurfaceId,
		MaterialType*,
		Util::Array<CoreGraphics::TextureId>,
		float
	> allocator;

	Threading::CriticalSection textureLoadSection;
	__ImplementResourceAllocatorTyped(allocator, CoreGraphics::MaterialIdType);
};

//------------------------------------------------------------------------------
/**
*/
inline const SurfaceId
SurfacePool::GetId(const SurfaceResourceId id)
{
	const SurfaceId ret = allocator.Get<Surface_SurfaceId>(id.resourceId);
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
inline MaterialType* const
SurfacePool::GetType(const SurfaceResourceId id)
{
	MaterialType* const ret = allocator.Get<Surface_MaterialType>(id.resourceId);
	return ret;
}


} // namespace Materials
