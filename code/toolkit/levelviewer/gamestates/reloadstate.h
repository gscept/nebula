#pragma once
//------------------------------------------------------------------------------
/**
    @class Tools::ReloadState
    
    A transition game state
    
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "statehandlers/gamestatehandler.h"
#include "game/entity.h"


//------------------------------------------------------------------------------
namespace Tools
{
class ReloadState : public BaseGameFeature::GameStateHandler
{
	__DeclareClass(ReloadState);
public:
	/// constructor
	ReloadState();
	/// destructor
	virtual ~ReloadState();
	
	/// called when the state represented by this state handler is entered
	virtual void OnStateEnter(const Util::String& prevState);
	/// called each frame as long as state is current, return new state
	virtual Util::String OnFrame();

private:
	Util::String nextLevelName;

	
}; 
} // namespace Tools
//------------------------------------------------------------------------------