#pragma once
//------------------------------------------------------------------------------
/**
	@class	Demo::InputManager

	(C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "game/manager.h"

namespace Tests
{

class InputManager : public Game::Manager
{
	__DeclareClass(InputManager)
	__DeclareSingleton(InputManager);
public:
	InputManager();
	virtual ~InputManager();

	void OnActivate() override;
	void OnDeactivate() override;;
};

} // namespace Demo
