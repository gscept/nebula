#pragma once
//------------------------------------------------------------------------------
/**
	@class	Game::TransformManager

	(C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "game/entity.h"
#include "game/manager.h"

namespace Game
{

class TransformManager : public Game::Manager
{
	__DeclareClass(TransformManager)
	__DeclareSingleton(TransformManager)
public:
	/// constructor
	TransformManager();
	/// destructor
	~TransformManager();

};

} // namespace Game
