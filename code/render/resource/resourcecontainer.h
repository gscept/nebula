#pragma once
//------------------------------------------------------------------------------
/**
	Implements a container for pending resources, which means they are currently in-flight, bu
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
namespace Resource		
{
template <class RESOURCE>
class ResourceContainer : public Core::RefCounted
{
	__DeclareClass(ResourceContainer);
public:
	/// constructor
	ResourceContainer();
	/// destructor
	virtual ~ResourceContainer();
private:

	Ptr<RESOURCE> resource;
	Ptr<RESOURCE> placeholder;
	Ptr<RESOURCE> failed;
};
} // namespace Resource