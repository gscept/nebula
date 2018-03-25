#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::Character
    
    Couples a skin and skeleton
    
    (C) 2012 gscept
*/
#include "core/refcounted.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class Character : public Core::RefCounted
{
	__DeclareClass(Character);
public:
	/// constructor
	Character();
	/// destructor
	virtual ~Character();
}; 
} // namespace ToolkitUtil
//------------------------------------------------------------------------------