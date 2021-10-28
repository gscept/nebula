#pragma once
//------------------------------------------------------------------------------
/**
    The material pool provides a chunk allocation source for material types and instances

    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/resourcestreamcache.h"
#include "shaderconfig.h"
#include "coregraphics/config.h"
#include "threading/criticalsection.h"

namespace Materials
{

struct MaterialInfo
{
    Resources::ResourceName materialType;
};

struct MaterialResourceId;
RESOURCE_ID_TYPE(MaterialResourceId);

class MaterialCache : public Resources::ResourceStreamCache
{
    __DeclareClass(MaterialCache);
public:

    /// setup resource loader, initiates the placeholder and error resources if valid, so don't forget to run!
    virtual void Setup() override;

    /// update reserved resource, the info struct is loader dependent (overload to implement resource deallocation, remember to set resource state!)
    Resources::ResourceCache::LoadStatus LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate = false) override;

    /// get material id
    const MaterialId GetId(const MaterialResourceId id);
    /// get material type
    ShaderConfig* const GetType(const MaterialResourceId id);
    /// update lod for textures in surface 
    void SetMaxLOD(const MaterialResourceId id, const float lod);
private:

    /// unload resource (overload to implement resource deallocation)
    void Unload(const Resources::ResourceId id);

    enum
    {
        Surface_MaterialId,
        Surface_MaterialType,
        Surface_Textures,
        Surface_MinLOD
    };

    Ids::IdAllocator<
        MaterialId,
        ShaderConfig*,
        Util::Array<CoreGraphics::TextureId>,
        float
    > allocator;

    Threading::CriticalSection textureLoadSection;
    __ImplementResourceAllocatorTyped(allocator, CoreGraphics::MaterialIdType);
};

//------------------------------------------------------------------------------
/**
*/
inline const MaterialId
MaterialCache::GetId(const MaterialResourceId id)
{
    const MaterialId ret = allocator.Get<Surface_MaterialId>(id.resourceId);
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
inline ShaderConfig* const
MaterialCache::GetType(const MaterialResourceId id)
{
    ShaderConfig* const ret = allocator.Get<Surface_MaterialType>(id.resourceId);
    return ret;
}


} // namespace Materials
