#pragma once
//------------------------------------------------------------------------------
/**
	A resource is a container for some type of file which is loaded.

	Resources can be loaded asynchronously, and through the ResourceContainer 
	class, a resource can be used before it is loaded.
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resourceid.h"
namespace Resources
{
class ResourceStreamPool;
class ResourceMemoryPool;
class ResourcePool;
class ResourceManager;
class ResourceContainer;
class Resource
{
public:

	enum State
	{
		Pending,	/// resource is being loaded
		Loaded,		/// resource is done loading, and is successful
		Failed,		/// resource loading failed
		Unloaded	/// resource has been unloaded
	};
	/// constructor
	Resource();
	/// destructor
	virtual ~Resource();

protected:
	friend class ResourceStreamPool;
	friend class ResourceMemoryPool;
	friend class ResourcePool;
	friend class ResourceManager;
	friend class ResourceContainer;
};

} // namespace Resources