#pragma once
//------------------------------------------------------------------------------
/**
	The ResourceId type is just a StringAtom, but is primarily meant for resources.

	The first 32 bits of the resource id is used internally for the pool to relate the shared
	resource with the pool, since a pool can share allocator with another pool. The 24 following 
	bytes is the actual resource handle, which can be used with the many different pools to fetch the
	data underlying that resource. The last 8 bits is the type of the loader, which makes it 
	fast to lookup and delete said resource instance.

	24 bytes ---------------8 bytes --------------- 24 bytes -------------- 8 bytes
	Pool storage id			Pool id					Allocator id			Allocator resource type

	Example: If we allocate a texture and want to use TextureId, then we take the 24 bytes part (id.id24)
	and use it to construct a Texture id, but we still need the ResourceId if we want to deallocate that
	texture at some later point. To convert to such an id, use the SpecializedId.
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/stringatom.h"
#include "ids/id.h"
#include "core/debug.h"
namespace Resources
{
typedef Util::StringAtom ResourceName;
ID_24_8_24_8_TYPE(ResourceId);				// 24 bits: pool-resource id, 8 bits: pool id, 24 bits: allocator id, 8 bits: allocator type

// define a generic typed ResourceId, this is so we can have specialized allocators, but have a common pool implementation...
ID_24_8_TYPE(ResourceUnknownId);

} // namespace Resource


#define RESOURCE_ID_TYPE(type, bit) struct type : public Resources::ResourceId { \
		const decltype(Resources::ResourceId::id24_0)& poolId = Resources::ResourceId::id24_0;\
		const decltype(Resources::ResourceId::id8_0)& poolIndex = Resources::ResourceId::id8_0;\
		const decltype(Resources::ResourceId::id24_1)& allocId = Resources::ResourceId::id24_1;\
		const decltype(Resources::ResourceId::id8_1)& allocType = Resources::ResourceId::id8_1;\
		constexpr type() : poolId(Ids::InvalidId24), poolIndex(Ids::InvalidId8), allocId(Ids::InvalidId24), allocType(bit) {};\
		constexpr type(const Resources::ResourceId& res) : poolId(res.id24_0), poolIndex(res.id8_0), allocId(res.id24_1), allocType(bit) { n_assert(res.id8_1 == bit); };\
		constexpr type(const Ids::Id32 id) : allocId(Ids::Id::GetBig(id)), allocType(Ids::Id::GetTiny(id)) { n_assert(this->id8_1 == bit); };\
		static constexpr type Invalid() { return Ids::Id::MakeId24_8_24_8(Ids::InvalidId24, Ids::InvalidId8, Ids::InvalidId24, bit); }\
		constexpr operator Ids::Id32() const { return Ids::Id::MakeId24_8(allocId, allocType); }\
		constexpr IndexT HashCode() const { return (IndexT)(allocId & 0xFFFFFF00 | allocType); }\
	};