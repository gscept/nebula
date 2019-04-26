//------------------------------------------------------------------------------
//  tagcomponent.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "tagcomponent.h"
#include "basegamefeature/managers/componentmanager.h"

namespace Game
{

static Component<Attr::Tag> data;

__ImplementComponent(TagComponent, data);

//------------------------------------------------------------------------------
/**
*/
void
TagComponent::Create()
{
	data.DestroyAll();

	__SetupDefaultComponentBundle(data);
	__RegisterComponent(&data, "TagComponent"_atm, 'tagc');
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