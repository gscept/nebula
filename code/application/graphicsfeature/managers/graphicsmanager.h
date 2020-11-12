#pragma once
//------------------------------------------------------------------------------
/**
	@class	GraphicsFeature::GraphicsManager

	Handles logic for connecting the game layer with the render layer.
	Also handles simple graphics entities such as static models and similar.

	(C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "game/manager.h"
#include "graphicsfeature/properties/graphics.h"
#include "game/category.h"
#include "basegamefeature/messages/entitymessages.h"

namespace GraphicsFeature
{

class GraphicsManager
{
	__DeclareSingleton(GraphicsManager);
public:
	/// retrieve the api
	static Game::ManagerAPI Create();

	/// destroy entity manager
	static void Destroy();

private:
	/// constructor
	GraphicsManager();
	/// destructor
	~GraphicsManager();

	static void OnBeginFrame();

	static void InitModelEntities();
	
	
	struct Pids
	{
		Game::PropertyId owner;
		Game::PropertyId worldTransform;
		Game::PropertyId modelResource;
		Game::PropertyId graphicsId;
		Game::PropertyId modelEntityData;
	} pids;

	Msg::AddProperty::MessageQueueId addPropertyMsgQueue;
};

} // namespace GraphicsFeature
