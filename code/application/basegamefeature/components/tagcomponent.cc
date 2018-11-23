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

static TagComponentData data;

__ImplementComponent(TagComponent, data);

//------------------------------------------------------------------------------
/**
*/
void
TagComponent::Create()
{
	data = TagComponentData();

	__SetupDefaultComponentBundle(data);
	data.functions.Optimize = Optimize;
	__RegisterComponent(&data);
}

//------------------------------------------------------------------------------
/**
*/
void
TagComponent::Discard()
{
	// Empty
}

//------------------------------------------------------------------------------
/**
*/
SizeT
TagComponent::Optimize()
{
	return data.Optimize();
}

} // namespace Game