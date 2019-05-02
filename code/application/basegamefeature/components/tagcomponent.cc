//------------------------------------------------------------------------------
//  tagcomponent.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "tagcomponent.h"
#include "basegamefeature/managers/componentmanager.h"

namespace Game
{

typedef Component<Attr::Tag> ComponentData;
static ComponentData* data;

__ImplementComponent(TagComponent, data);

//------------------------------------------------------------------------------
/**
*/
void
TagComponent::Create()
{
	if (data != nullptr)
	{
		data->DestroyAll();
	}
	else
	{
		data = new ComponentData({ true });
	}

	__SetupDefaultComponentBundle(data);
	Game::ComponentManager::Instance()->RegisterComponent(data, "TagComponent"_atm, 'tagc');
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