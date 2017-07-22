#pragma once
//------------------------------------------------------------------------------
/**
	Implements a container for pending resources, which means they are currently in-flight, 
	but can still be retrieved. 

	When the request resource is in it's state, the ResourceContainer behaves accordingly:
		1. Pending load		- the resource is being loaded, so the 'placeholder' resource is returned
		2. Loaded			- the actual requested resource is returned
		3. Failed			- the resource failed to load, and the 'error' resource will be returned
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resource.h"
namespace Resources
{
class ResourceStreamPool;
class ResourceMemoryPool;
class ResourcePool;
class ResourceManager;
class ResourceContainer
{
public:
	/// constructor
	ResourceContainer();
	/// destructor
	virtual ~ResourceContainer();

	/// get resource, may return placeholder if pending, or failed if load failed
	const Ptr<Resource>& GetResource();
private:
	friend class ResourceStreamPool;
	friend class ResourceMemoryPool;
	friend class ResourcePool;
	friend class ResourceManager;

	Ptr<Resource> resource;
	Ptr<Resource> placeholder;
	Ptr<Resource> error;
};

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Resource>&
Resources::ResourceContainer::GetResource()
{
	switch (this->resource->state)
	{
	case Resource::Pending:
		return this->placeholder;
		break;
	case Resource::Loaded:
		return this->resource;
		break;
	case Resource::Failed:
		return this->error;
		break;
	}
	return this->error;
}

} // namespace Resources