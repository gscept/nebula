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
class ResourceLoader;
class ResourceManager;
class ResourceContainer;
class Resource : public Core::RefCounted
{
	__DeclareClass(Resource);
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

	/// get state
	const State& GetState() const;
	/// get name
	const ResourceName& GetResourceName() const;

protected:
	friend class ResourceLoader;
	friend class ResourceManager;
	friend class ResourceContainer;

	State state;
	ResourceName resourceName;
	Util::StringAtom tag;
};

//------------------------------------------------------------------------------
/**
*/
inline const Resource::State&
Resource::GetState() const
{
	return this->state;
}

//------------------------------------------------------------------------------
/**
*/
inline const Resources::ResourceName&
Resource::GetResourceName() const
{
	return this->resourceName;
}

} // namespace Resources