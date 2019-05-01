#pragma once
//------------------------------------------------------------------------------
/**
	GraphicsComponent

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "game/component/component.h"
#include "game/component/attribute.h"

namespace GraphicsFeature
{

class GraphicsComponent
{
	__DeclareComponent(GraphicsComponent)
public:
	static void SetupAcceptedMessages();
	
	static void OnActivate(Game::InstanceId instance);
	static void OnDeactivate(Game::InstanceId instance);

	static void UpdateTransform(Game::Entity, const Math::matrix44&);
	static void SetModel(Game::Entity, const Util::String&);

	/// Return this components fourcc
	static Util::FourCC GetFourCC();
private:

};

} // namespace GraphicsFeature
