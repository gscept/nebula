//------------------------------------------------------------------------------
//  editor.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "editor.h"

#include "io/assignregistry.h"
#include "scripting/scriptserver.h"
#include "memdb/database.h"
#include "game/api.h"
#include "basegamefeature/basegamefeatureunit.h"
#include "basegamefeature/managers/timemanager.h"
#include "graphicsfeature/graphicsfeatureunit.h"
#include "imgui.h"
#include "util/random.h"
#include "commandmanager.h"

#include "editor/components/editorcomponents.h"
#include "tools/pathconverter.h"
#include "io/assignregistry.h"

#include "game/editorstate.h"

namespace Editor
{

//------------------------------------------------------------------------------
/**
*/
State state;

//------------------------------------------------------------------------------
/**
*/
void
Create()
{
    IO::AssignRegistry::Instance()->SetAssign(IO::Assign("edscr", "bin:editorscripts"));
    IO::AssignRegistry::Instance()->SetAssign(IO::Assign("work", "proj:work"));
    IO::AssignRegistry::Instance()->SetAssign(IO::Assign("assets", "work:assets"));

    Scripting::ScriptServer::Instance()->AddModulePath("edscr:");
    Scripting::ScriptServer::Instance()->EvalFile("edscr:bootstrap.py");

    /// Import reload to be able to reload modules.
    Scripting::ScriptServer::Instance()->Eval("from importlib import reload");

    Game::TimeManager::SetGlobalTimeFactor(0.0f);

    state.editorWorld = Game::GameServer::Instance()->CreateWorld(WORLD_EDITOR);
    state.editorWorld->componentInitializationEnabled = false;

    Game::GameServer::Instance()->SetupEmptyWorld(state.editorWorld);

    // Create a command manager with a 20MB buffer
    Edit::CommandManager::Create(20_MB);
    CreatePathConverter({});

    Game::EditorState::Singleton = new Game::EditorState();
    Game::EditorState::Instance()->isRunning = true;    
}

//------------------------------------------------------------------------------
/**
*/
void
Destroy()
{
    Edit::CommandManager::Discard();
    delete Game::EditorState::Singleton;
}

//------------------------------------------------------------------------------
/**
*/
void
PlayGame()
{
    Game::EditorState::Instance()->isPlaying = true;
    Game::TimeManager::SetGlobalTimeFactor(1.0f);
}

//------------------------------------------------------------------------------
/**
*/
void
PauseGame()
{
    Game::EditorState::Instance()->isPlaying = false;
    Game::TimeManager::SetGlobalTimeFactor(0.0f);
}

//------------------------------------------------------------------------------
/**
*/
void
StopGame()
{
    Game::EditorState::Instance()->isPlaying = false;

    Game::World* gameWorld = Game::GetWorld(WORLD_DEFAULT);
    Game::GameServer::Instance()->CleanupWorld(gameWorld);
    Game::GameServer::Instance()->SetupEmptyWorld(gameWorld);
    
    Game::World::Override(state.editorWorld, gameWorld);

    // update the editables so that they point to the correct game entities.
    Game::Filter filter = Game::FilterBuilder().Including<Game::Entity>().Build();
    Game::Dataset data = state.editorWorld->Query(filter);

    for (int v = 0; v < data.numViews; v++)
    {
        Game::Dataset::View const& view = data.views[v];
        Editor::Entity const* const entities = (Editor::Entity*)view.buffers[0];

        for (IndexT i = 0; i < view.numInstances; ++i)
        {
            Editor::Entity const& editorEntity = entities[i];
            Editable& edit = state.editables[editorEntity.index];
            edit.gameEntity = editorEntity;
            edit.gameEntity.world = gameWorld->GetWorldId();

            Editor::EditorEntity* editorEntityComponent = gameWorld->AddComponent<Editor::EditorEntity>(edit.gameEntity);
            editorEntityComponent->id = (uint64_t)editorEntity;
        }
    }

    Game::DestroyFilter(filter);

    Game::TimeManager::SetGlobalTimeFactor(0.0f);
}

} // namespace Editor

