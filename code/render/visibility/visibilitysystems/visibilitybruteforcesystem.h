#pragma once
//------------------------------------------------------------------------------
/**
	Implements a brute-force visibility system which just goes through all entities
	and performs a visibility check.
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "visibility/visibilitysystems/visibilitysystembase.h"
namespace Visibility
{
class VisibilityBruteForceSystem : public VisibilitySystemBase
{
	__DeclareClass(VisibilityBruteForceSystem);
public:
	/// constructor
	VisibilityBruteForceSystem();
	/// destructor
	virtual ~VisibilityBruteForceSystem();
private:
};
} // namespace Visibility