//------------------------------------------------------------------------------
//  reloadstate.cc
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "reloadstate.h"
#include "statehandlers/gamestatehandler.h"
#include "appgame/gameapplication.h"

namespace Tools
{
__ImplementClass(Tools::ReloadState, 'RELO', BaseGameFeature::GameStateHandler);

using namespace BaseGameFeature;
using namespace Util;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
ReloadState::ReloadState()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ReloadState::~ReloadState()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
ReloadState::OnStateEnter(const Util::String& prevState)
{
	this->nextLevelName = this->GetLevelName();
	GameStateHandler::OnStateEnter(prevState);
}
//------------------------------------------------------------------------------
/**
*/
Util::String 
ReloadState::OnFrame( )
{		
	const Ptr<BaseGameFeature::GameStateHandler>& state = App::GameApplication::Instance()->FindStateHandlerByName("LevelViewerGameState").cast<BaseGameFeature::GameStateHandler>();
	state->SetSetupMode(GameStateHandler::LoadLevel);
	state->SetLevelName(this->nextLevelName);
	//App::GameApplication::Instance()->RequestState("LevelViewerGameState");
	GameStateHandler::OnFrame();
	return "LevelViewerGameState";

}

} // namespace Tools