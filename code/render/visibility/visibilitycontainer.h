#pragma once
//------------------------------------------------------------------------------
/**
	A VisibilityContainer groups visible entities in a form which allows them 
	to be traversed and rendered. 
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
namespace Visibility
{
class VisibilityContainer : public Core::RefCounted
{
	__DeclareClass(VisibilityContainer);
public:
	/// constructor
	VisibilityContainer();
	/// destructor
	virtual ~VisibilityContainer();
private:

	Util::Array<bool> visible;
};
} // namespace Visibility