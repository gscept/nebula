#pragma once
//------------------------------------------------------------------------------
/**
    Game managers
    
    Managers are defined via a simple function API.
    They are meant to care about "specific global stuff".
    They should be defined by applications to implement globals aspects of
    the application (mainly global game play related).
    
    Managers should be created and triggered by game features. The frame event
    functions are invoked when the gameserver triggers the game feature.
    
    (C) 2020 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "ids/id.h"

//------------------------------------------------------------------------------
namespace Game
{

ID_32_TYPE(ManagerHandle)

struct ManagerAPI
{
    /// called when attached to game server
    void(*OnActivate)() = nullptr;
    /// called when removed from game server
    void(*OnDeactivate)() = nullptr;
    /// called before frame by the game server
    void(*OnBeginFrame)() = nullptr;
    /// called per-frame by the game server
    void(*OnFrame)() = nullptr;
    /// called after frame by the game server
    void(*OnEndFrame)() = nullptr;
    /// called after loading game state
    void(*OnLoad)() = nullptr;
    /// called before saving game state
    void(*OnSave)() = nullptr;
    /// called by Game::Server::Start()
    void(*OnStart)() = nullptr;
    /// render a debug visualization 
    void(*OnRenderDebug)() = nullptr;
};

}; // namespace Game
//------------------------------------------------------------------------------
 
    
