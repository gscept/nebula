#pragma once
//------------------------------------------------------------------------------
/**
    @class Tools::LevelViewerGameState
    
    A basic game state
    
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "statehandlers/gamestatehandler.h"
#include "graphics/spotlightentity.h"
#include "graphics/globallightentity.h"
#include "graphics/pointlightentity.h"
#include "graphics/modelentity.h"
#include "game/entity.h"
#include "posteffect/posteffectentity.h"

//------------------------------------------------------------------------------
namespace Tools
{
class LevelViewerGameState : public BaseGameFeature::GameStateHandler
{
    __DeclareClass(LevelViewerGameState);
public:
    /// constructor
    LevelViewerGameState();
    /// destructor
    virtual ~LevelViewerGameState();

    /// called when the state represented by this state handler is entered
    virtual void OnStateEnter(const Util::String& prevState);
    /// called when the state represented by this state handler is left
    virtual void OnStateLeave(const Util::String& nextState);
    /// called each frame as long as state is current, return new state
    virtual Util::String OnFrame();
    /// called after Db is opened, and before entities are loaded
    virtual void OnLoadBefore();
    /// called after entities are loaded
    virtual void OnLoadAfter();

    // handle all user input; called @ LevelEditorState::OnFrame()
    void HandleInput();

    //
    void LoadLevel(const Util::String &level, bool applyTransform = false);
    //
    void ReloadLevel(bool keepTransform);

private:
    
    Math::matrix44 focusTransform;
    Util::String lastLevel;
    bool applyTransform;
    bool entitiesLoaded;    
}; 
} // namespace Tools
//------------------------------------------------------------------------------