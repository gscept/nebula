#pragma once
//------------------------------------------------------------------------------
/**
	Description
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include <functional>
#include "core/refcounted.h"
#include "core/singleton.h"
#include "resourcecontainer.h"
namespace Resource
{
class ResourceManager : public Core::RefCounted
{
	__DeclareClass(ResourceManager);
	__DeclareSingleton(ResourceManager);
public:
	/// constructor
	ResourceManager();
	/// destructor
	virtual ~ResourceManager();

	/// initiate the loading of a resource, immediately returns a container which is appropriate for the resource type getting loaded
	Ptr<ResourceContainer> CreateResource(const ResourceId& id, std::function<void(const Ptr<ResourceContainer>&)> success, std::function<void(const Ptr<ResourceContainer>&)> failed);
private:
};
} // namespace Resource