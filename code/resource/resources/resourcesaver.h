#pragma once
//------------------------------------------------------------------------------
/**
	Saves resource to stream
	
	(C)2017-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "io/uri.h"
#include "resourceid.h"
namespace Resources
{
class ResourceSaver : public Core::RefCounted
{
	__DeclareAbstractClass(ResourceSaver);
public:
	/// constructor
	ResourceSaver();
	/// destructor
	virtual ~ResourceSaver();

	enum SaveStatus
	{
		Success,		// save successful
		Failed			// save failed
	};

	/// save resource to path
	virtual SaveStatus Save(const Resources::ResourceId id, IO::URI path) = 0;
};
} // namespace Resources