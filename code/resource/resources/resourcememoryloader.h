#pragma once
//------------------------------------------------------------------------------
/**
	The resource memory loader performs loading immediately using local memory buffers.
	
	This means, the ResourceMemoryLoader is immediate, and requires a previously created 
	resource, using Resources::ReserveResource, followed by the attachment of a loader,
	and then perform a load.
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resourceid.h"
namespace Resources
{
class ResourceMemoryLoader : public Core::RefCounted
{
	__DeclareAbstractClass(ResourceMemoryLoader);
public:
	enum LoadStatus
	{
		Success,		/// resource is properly loaded
		Failed,			/// resource loading failed
	};

	/// constructor
	ResourceMemoryLoader();
	/// destructor
	virtual ~ResourceMemoryLoader();

	/// perform actual load
	virtual LoadStatus Load(const Resources::ResourceId id) = 0;
private:
};
} // namespace Resources