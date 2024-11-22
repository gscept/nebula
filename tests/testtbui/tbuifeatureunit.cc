//------------------------------------------------------------------------------
//  tbuifeatureunit.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "tbuifeatureunit.h"
#include "basegamefeature/basegamefeatureunit.h"
#include "gamestatemanager.h"
#include "profiling/profiling.h"
#include "managers/inputmanager.h"
#include "game/api.h"
#include "game/world.h"

namespace Tests
{

__ImplementClass(TBUIFeatureUnit, 'TBFU', Game::FeatureUnit);
__ImplementSingleton(TBUIFeatureUnit);

//------------------------------------------------------------------------------
/**
*/
TBUIFeatureUnit::TBUIFeatureUnit()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
TBUIFeatureUnit::~TBUIFeatureUnit()
{
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
TBUIFeatureUnit::OnAttach()
{
}

//------------------------------------------------------------------------------
/**
*/
void
TBUIFeatureUnit::OnActivate()
{
    FeatureUnit::OnActivate();

    // Setup game state
    this->AttachManager(Tests::GameStateManager::Create());
    this->AttachManager(Tests::InputManager::Create());
}

//------------------------------------------------------------------------------
/**
*/
void
TBUIFeatureUnit::OnBeginFrame()
{
    FeatureUnit::OnBeginFrame();
}

//------------------------------------------------------------------------------
/**
*/
void
TBUIFeatureUnit::OnDeactivate()
{
    FeatureUnit::OnDeactivate();
}
} // namespace Tests
