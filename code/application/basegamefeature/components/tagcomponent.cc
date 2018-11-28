//------------------------------------------------------------------------------
//  tagcomponent.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "tagcomponent.h"
#include "basegamefeature/managers/componentmanager.h"
#include "basegamefeature/components/tagdata.h"

namespace Game
{

static TagComponentAllocator data;

__ImplementComponent(TagComponent, data);

//------------------------------------------------------------------------------
/**
*/
void
TagComponent::Create()
{
	data = TagComponentAllocator();

	__SetupDefaultComponentBundle(data);
	__RegisterComponent(&data, "TagComponent"_atm);
}

//------------------------------------------------------------------------------
/**
*/
void
TagComponent::Discard()
{
	// Empty
}

} // namespace Game