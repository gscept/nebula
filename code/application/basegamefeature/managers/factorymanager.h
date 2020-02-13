#pragma once
//------------------------------------------------------------------------------
/**
	@class	Game::FactoryManager

	(C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "game/entity.h"
#include "game/manager.h"

namespace Game
{

class FactoryManager : public Game::Manager
{
	__DeclareClass(FactoryManager)
	__DeclareSingleton(FactoryManager)
public:
	/// constructor
	FactoryManager();
	/// destructor
	~FactoryManager();

	/// create a new entity from its category name
	Game::Entity CreateEntityByCategory(Util::StringAtom const categoryName) const;

private:
};

} // namespace Game
