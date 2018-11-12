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
	
	static void OnActivate(const uint32_t& instance);
	static void OnDeactivate(const uint32_t& instance);

	static void UpdateTransform(const Game::Entity&, const Math::matrix44&);
	static void SetModel(const Game::Entity&, const Util::String&);


private:

};

} // namespace GraphicsFeature
