#pragma once
//------------------------------------------------------------------------------
/**
    The resource memory pool performs loading immediately using local memory buffers.
    
    This means, the ResourceMemoryCache is immediate, and requires a previously created 
    resource, using Resources::ReserveResource, followed by the attachment of a loader,
    and then perform a load. Therefore, it requires no ResourceServer Update() to trigger.

    This type of loader exists to provide code-local data, like a const float* mesh, or texture,
    or for example font data loaded as a const char* from some foreign library. 

    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resourcecache.h"
#include "resourceid.h"
#include "resource.h"


namespace Resources
{
class ResourceServer;
class ResourceMemoryCache : public ResourceCache
{
    __DeclareAbstractClass(ResourceMemoryCache);
public:

    /// constructor
    ResourceMemoryCache();
    /// destructor
    virtual ~ResourceMemoryCache();

    /// reserve resource, which then allows you to modify it 
    Resources::ResourceId ReserveResource(const ResourceName& res, const Util::StringAtom& tag);
    /// discard resource instance
    void DiscardResource(const Resources::ResourceId id);
    /// discard all resources associated with a tag
    void DiscardByTag(const Util::StringAtom& tag);

    /// update reserved resource, the info struct is loader dependent (overload to implement resource deallocation, remember to set resource state!)
    virtual LoadStatus LoadFromMemory(const Resources::ResourceId id, const void* info) = 0;

private:
    friend class ResourceServer;
};
} // namespace Resources
