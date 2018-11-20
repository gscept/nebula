#pragma once
//------------------------------------------------------------------------------
/**
	GraphicsComponent

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "game/component/component.h"

namespace GraphicsFeature
{

class GraphicsComponent
{
	__DeclareComponent(GraphicsComponent)
public:
	static void SetupAcceptedMessages();
	
	static void OnActivate(uint32_t instance);
	static void OnDeactivate(uint32_t instance);

	static void UpdateTransform(Game::Entity, const Math::matrix44&);
	static void SetModel(Game::Entity, const Util::String&);

private:

};

} // namespace GraphicsFeature
