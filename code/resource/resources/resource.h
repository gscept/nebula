#pragma once
//------------------------------------------------------------------------------
/**
    A resource is a container for some type of file which is loaded.

    Resources can be loaded asynchronously, and through the ResourceContainer 
    class, a resource can be used before it is loaded.
    
    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resourceid.h"
namespace Resources
{
class ResourceLoader;
class ResourceMemoryCache;
class ResourceServer;
class ResourceContainer;
class Resource
{
public:

    enum State
    {
        Pending,    /// resource is being loaded
        Loaded,     /// resource is done loading, and is successful
        Failed,     /// resource loading failed
        Unloaded    /// resource has been unloaded
    };
    /// constructor
    Resource();
    /// destructor
    virtual ~Resource();

protected:
    friend class ResourceLoader;
    friend class ResourceMemoryCache;
    friend class ResourceServer;
    friend class ResourceContainer;
};

} // namespace Resources
