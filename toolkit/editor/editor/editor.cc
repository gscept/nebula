//------------------------------------------------------------------------------
//  editor.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "editor.h"

#include "io/assignregistry.h"
#include "memdb/database.h"
#include "game/api.h"
#include "basegamefeature/basegamefeatureunit.h"
#include "basegamefeature/managers/timemanager.h"
#include "graphicsfeature/graphicsfeatureunit.h"
#include "imgui.h"
#include "util/random.h"
#include "commandmanager.h"
#include "cmds.h"
#include "entityloader.h"
#include "io/filedialog.h"
#include "io/jsonreader.h"
#include "io/ioserver.h"

#include "editor/components/editorcomponents.h"
#include "tools/pathconverter.h"
#include "io/assignregistry.h"
#include "tools/livebatcher.h"

#include "game/editorstate.h"
#include "game/component.h"

#include "toolkit-common/projectinfo.h"

namespace Editor
{

//------------------------------------------------------------------------------
/**
*/
State state;

//------------------------------------------------------------------------------
/**
*/
static void
ClearLevel()
{
    Edit::SetSelection({});

    Game::Filter filter = Game::FilterBuilder().Including<Game::Entity>().Build();
    Game::Dataset data = state.editorWorld->Query(filter);
    Util::Array<Editor::Entity> entities;
    for (int v = 0; v < data.numViews; v++)
    {
        Game::Dataset::View const& view = data.views[v];
        Editor::Entity const* const viewEntities = (Editor::Entity*)view.buffers[0];
        entities.AppendArray(viewEntities, view.numInstances);
    }
    Game::DestroyFilter(filter);

    for (Editor::Entity entity : entities)
    {
        Edit::DeleteEntity(entity);
    }

    Edit::CommandManager::Clear();
    state.editables.Clear();
    state.collections.Clear();
    state.activeCollection = Util::Guid();
}

//------------------------------------------------------------------------------
/**
*/
void
Create()
{
    IO::AssignRegistry::Instance()->SetAssign(IO::Assign("edscr", "bin:editorscripts"));
    IO::AssignRegistry::Instance()->SetAssign(IO::Assign("work", "proj:work"));
    IO::AssignRegistry::Instance()->SetAssign(IO::Assign("assets", "work:assets"));
    IO::AssignRegistry::Instance()->SetAssign(IO::Assign("src", "proj:work"));

    Game::TimeSourceCreateInfo editorTimeSourceInfo;
    editorTimeSourceInfo.hash = TIMESOURCE_EDITOR;
    Game::Time::CreateTimeSource(editorTimeSourceInfo);

    ToolkitUtil::ProjectInfo projectInfo;
    ToolkitUtil::ProjectInfo::Result res = projectInfo.Setup();
    n_assert(res == ToolkitUtil::ProjectInfo::Success);
    IO::AssignRegistry::Instance()->SetAssign(IO::Assign("int", projectInfo.GetAttr("IntermediateDir")));
    IO::IoServer::Instance()->CreateDirectory("int:");

    LiveBatcher::Setup();

    Game::TimeSource* gameTimeSource = Game::Time::GetTimeSource(TIMESOURCE_GAMEPLAY);
    gameTimeSource->timeFactor = 0.0f;

    state.editorWorld = Game::GameServer::Instance()->CreateWorld(WORLD_EDITOR);
    state.editorWorld->componentInitializationEnabled = false;

    Game::GameServer::Instance()->SetupEmptyWorld(state.editorWorld);

    // Create a command manager with a 20MB buffer
    Edit::CommandManager::Create(20_MB);
    CreatePathConverter({});

    Game::EditorState::Singleton = new Game::EditorState();
}

//------------------------------------------------------------------------------
/**
*/
void
Start()
{
    Game::EditorState::Instance()->isRunning = true;
}

//------------------------------------------------------------------------------
/**
*/
void
Destroy()
{
    LiveBatcher::Discard();
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
    Game::TimeSource* gameTimeSource = Game::Time::GetTimeSource(TIMESOURCE_GAMEPLAY);
    gameTimeSource->timeFactor = 1.0f;
}

//------------------------------------------------------------------------------
/**
*/
void
PauseGame()
{
    Game::EditorState::Instance()->isPlaying = false;
    Game::TimeSource* gameTimeSource = Game::Time::GetTimeSource(TIMESOURCE_GAMEPLAY);
    gameTimeSource->timeFactor = 0.0f;
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

            gameWorld->SetComponent<Game::Entity>(edit.gameEntity, edit.gameEntity);
        }
    }

    for (int v = 0; v < data.numViews; v++)
    {
        Game::Dataset::View const& view = data.views[v];
        Editor::Entity const* const entities = (Editor::Entity*)view.buffers[0];
        for (IndexT i = 0; i < view.numInstances; ++i)
        {
            Editor::Entity const editorEntity = entities[i];
            Editable& edit = state.editables[editorEntity.index];
            Editor::RemapInstanceToGame(gameWorld, edit.gameEntity);
            Game::EntityMapping const mapping = gameWorld->GetEntityMapping(edit.gameEntity);
            gameWorld->InitializeInstance(edit.gameEntity, mapping.table, mapping.instance);
            Editor::EditorEntity* editorEntityComponent = gameWorld->AddComponent<Editor::EditorEntity>(edit.gameEntity);
            editorEntityComponent->id = (uint64_t)editorEntity;
        }
    }
    Game::DestroyFilter(filter);

    Game::TimeSource* gameTimeSource = Game::Time::GetTimeSource(TIMESOURCE_GAMEPLAY);
    gameTimeSource->timeFactor = 0.0f;
}

//------------------------------------------------------------------------------
/**
*/
bool
LoadLevel(const Util::String& path, bool instantiate)
{
    Ptr<IO::JsonReader> reader = IO::JsonReader::Create();
    reader->SetStream(IO::IoServer::Instance()->CreateStream(path));
    if (!reader->Open() || !reader->HasNode("/level"))
    {
        n_warning("Could not open JSON level '%s'\n", path.AsCharPtr());
        if (reader->IsOpen())
            reader->Close();
        return false;
    }

    reader->SetToNode("/level");
    if (reader->GetInt("version") != 100)
    {
        n_warning("Unsupported JSON level version in '%s'\n", path.AsCharPtr());
        reader->Close();
        return false;
    }

    if (!instantiate)
    {
        ClearLevel();
    }

    Ptr<Editor::EntityLoader> loader = Editor::EntityLoader::Create();
    loader->SetWorld(Editor::state.editorWorld);
    loader->SetGenerateGuids(instantiate);
    loader->LoadJsonLevel(reader);
    loader->LoadCollections(reader);
    reader->Close();

    if (!instantiate)
    {
        Editor::state.levelPath = path;
        Edit::CommandManager::SetClean();
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
SaveLevel()
{
    if (!Editor::state.levelPath.IsValid())
    {
        return false;
    }
    if (Editor::SaveEntities(Editor::state.levelPath.AsCharPtr()))
    {
        Edit::CommandManager::SetClean();
        return true;
    }
    n_warning("Could not save JSON level '%s'\n", Editor::state.levelPath.AsCharPtr());
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
SaveLevelAs(const Util::String& path)
{
    Util::String levelPath = path;
    if (levelPath.GetFileExtension() != "json")
    {
        levelPath.Append(".json");
    }
    if (Editor::SaveEntities(levelPath.AsCharPtr()))
    {
        Editor::state.levelPath = levelPath;
        Edit::CommandManager::SetClean();
        return true;
    }
    n_warning("Could not save JSON level '%s'\n", levelPath.AsCharPtr());
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
SaveLevelWithDialog(bool saveAs)
{
    if (!saveAs && SaveLevel())
    {
        return true;
    }

    static Util::String localPath = IO::URI("proj:work/levels").LocalPath();
    Util::String path;
    IO::IoServer::Instance()->CreateDirectory(localPath);
    if (!IO::FileDialog::SaveFile("Save Nebula Level", localPath, {"*.json"}, path))
    {
        return false;
    }
    return SaveLevelAs(path);
}

//------------------------------------------------------------------------------
/**
*/
IndexT
FindCollection(const Util::Guid& guid)
{
    if (!guid.IsValid())
    {
        return InvalidIndex;
    }
    for (IndexT i = 0; i < state.collections.Size(); i++)
    {
        if (state.collections[i].guid == guid)
        {
            return i;
        }
    }
    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
Game::Entity
ToGameEntity(Game::Entity entity)
{
    if (entity == Game::Entity::Invalid())
    {
        return entity;
    }
    if (entity.world == state.editorWorld->GetWorldId() && state.editorWorld->IsValid(entity) && entity.index < state.editables.Size())
    {
        return state.editables[entity.index].gameEntity;
    }
    return entity;
}

//------------------------------------------------------------------------------
/**
*/
Game::Entity
ToEditorEntity(Game::Entity entity)
{
    if (entity == Game::Entity::Invalid())
    {
        return entity;
    }
    if (entity.world == state.editorWorld->GetWorldId())
    {
        return entity;
    }
    Game::World* gameWorld = Game::GetWorld(WORLD_DEFAULT);
    if (entity.world == gameWorld->GetWorldId() && gameWorld->IsValid(entity) && gameWorld->HasComponent<Editor::EditorEntity>(entity))
    {
        Editor::EditorEntity const link = gameWorld->GetComponent<Editor::EditorEntity>(entity);
        Game::Entity const editorEntity = (Game::Entity)link.id;
        if (state.editorWorld->IsValid(editorEntity))
        {
            return editorEntity;
        }
    }
    return entity;
}

//------------------------------------------------------------------------------
/**
*/
static void
RemapComponentEntities(Game::ComponentId component, void* data, bool toGame)
{
    if (component == Game::GetComponentId<Game::Entity>())
    {
        return;
    }
    Game::ComponentInterface const* componentInterface =
        static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(component));
    for (IndexT field = 0; field < componentInterface->GetNumFields(); field++)
    {
        if (Util::StringAtom(componentInterface->GetFieldTypenames()[field]) == "Game::Entity"_atm)
        {
            Game::Entity* entity = (Game::Entity*)((byte*)data + componentInterface->GetFieldByteOffsets()[field]);
            *entity = toGame ? ToGameEntity(*entity) : ToEditorEntity(*entity);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
RemapComponentToGame(Game::ComponentId component, void* data)
{
    RemapComponentEntities(component, data, true);
}

//------------------------------------------------------------------------------
/**
*/
void
RemapComponentToEditor(Game::ComponentId component, void* data)
{
    RemapComponentEntities(component, data, false);
}

//------------------------------------------------------------------------------
/**
*/
void
RemapInstanceToGame(Game::World* world, Game::Entity entity)
{
    Game::EntityMapping const mapping = world->GetEntityMapping(entity);
    MemDb::Table const& table = world->GetDatabase()->GetTable(mapping.table);
    for (Game::ComponentId component : table.GetAttributes())
    {
        SizeT const typeSize = MemDb::AttributeRegistry::TypeSize(component);
        if (typeSize == 0 || component == Game::GetComponentId<Game::Entity>())
        {
            continue;
        }
        void* buffer = world->GetInstanceBuffer(mapping.table, mapping.instance.partition, component);
        RemapComponentToGame(component, (byte*)buffer + mapping.instance.index * typeSize);
    }
}

} // namespace Editor
