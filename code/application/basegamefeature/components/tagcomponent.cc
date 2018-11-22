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

static TagComponentData component;

__ImplementComponent(TagComponent, component);

//------------------------------------------------------------------------------
/**
*/
void
TagComponent::Create()
{
	component = TagComponentData();

	__SetupDefaultComponentBundle(component);
	component.functions.Optimize = Optimize;
	__RegisterComponent(&component);
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
	return component.Optimize();
}

} // namespace Game