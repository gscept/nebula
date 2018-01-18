#pragma once
//------------------------------------------------------------------------------
/**
	The resource memory pool performs loading immediately using local memory buffers.
	
	This means, the ResourceMemoryPool is immediate, and requires a previously created 
	resource, using Resources::ReserveResource, followed by the attachment of a loader,
	and then perform a load. Therefore, it requires no ResourceManager Update() to trigger.

	This type of loader exists to provide code-local data, like a const float* mesh, or texture,
	or for example font data loaded as a const char* from some foreign library. 

	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resourcepool.h"
#include "resourceid.h"
#include "resource.h"


namespace Resources
{
class ResourceManager;
class ResourceMemoryPool : public ResourcePool
{
	__DeclareAbstractClass(ResourceMemoryPool);
public:

	/// constructor
	ResourceMemoryPool();
	/// destructor
	virtual ~ResourceMemoryPool();

	/// reserve resource, which then allows you to modify it 
	Resources::ResourceId ReserveResource(const ResourceName& res, const Util::StringAtom& tag);
	/// discard resource instance
	void DiscardResource(const Resources::ResourceId id);
	/// discard all resources associated with a tag
	void DiscardByTag(const Util::StringAtom& tag);

	/// update reserved resource, the info struct is loader dependent (overload to implement resource deallocation, remember to set resource state!)
	virtual LoadStatus LoadFromMemory(const Ids::Id24 id, const void* info) = 0;


private:
	friend class ResourceManager;
};
} // namespace Resources