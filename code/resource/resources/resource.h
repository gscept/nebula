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
        Pending,    /// Resource has some loading to be performed
        Loaded,     /// Resource is done loading all of its requested subresources
        Failed,     /// Resource loading failed
        Unloaded    /// Resource has been unloaded and deinitialized
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
