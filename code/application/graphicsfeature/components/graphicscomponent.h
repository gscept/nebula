#pragma once
//------------------------------------------------------------------------------
/**
	GraphicsComponent

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphicscomponentbase.h"

namespace GraphicsFeature
{

class GraphicsComponent : public GraphicsComponentBase
{
	__DeclareClass(GraphicsComponent)
public:
	GraphicsComponent();
	~GraphicsComponent();
	
	void SetupAcceptedMessages();

	void OnActivate(const uint32_t& instance);
	void OnDeactivate(const uint32_t& instance);

	void UpdateTransform(const Game::Entity&, const Math::matrix44&);
	void SetModel(const Game::Entity&, const Util::String&);

private:

};

} // namespace GraphicsFeature
