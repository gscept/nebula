//------------------------------------------------------------------------------
//  editor.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "editor.h"

#include "appgame/gameapplication.h"
#include "game/modulemanager.h"
#include "entityloader.h"
#include "io/assignregistry.h"
#include "io/fswrapper.h"
#include "io/ioserver.h"
#include "io/jsonreader.h"
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
#include "tools/livebatcher.h"
#include "editor/ui/windows/navigation.h"

#include "game/editorstate.h"

#include "toolkit-common/projectinfo.h"

#include <cstdlib>

namespace Editor
{

namespace
{

const char* ReloadSnapshotUri = "user:nebula/editor/hotreload_snapshot.json";

//------------------------------------------------------------------------------
/**
*/
Util::String
GetEditorBuildDir()
{
#if defined(NEBULA_BINARY_FOLDER)
    Util::String buildDir = NEBULA_BINARY_FOLDER;
    buildDir.ConvertBackslashes();
    buildDir.SubstituteString("/fips-deploy/", "/fips-build/");
    return buildDir;
#else
    return "";
#endif
}

//------------------------------------------------------------------------------
/**
*/
bool
GetWorkspaceAndProjectFromBinaryDir(Util::String& outWorkspaceDir, Util::String& outProjectName)
{
#if defined(NEBULA_BINARY_FOLDER)
    Util::String binaryDir = NEBULA_BINARY_FOLDER;
    binaryDir.ConvertBackslashes();
    const Util::String marker = "/fips-deploy/";

    const IndexT markerPos = binaryDir.FindStringIndex(marker);
    if (markerPos == InvalidIndex)
        return false;

    outWorkspaceDir = binaryDir.ExtractRange(0, markerPos);

    const IndexT remainderStart = markerPos + (IndexT)marker.Length();
    const Util::String remainder = binaryDir.ExtractToEnd(remainderStart);
    const IndexT slashPos = remainder.FindCharIndex('/');
    if (slashPos == InvalidIndex)
        return false;

    outProjectName = remainder.ExtractRange(0, slashPos);
    if (!outProjectName.IsValid())
        return false;

    return true;
#else
    return false;
#endif
}

//------------------------------------------------------------------------------
/**
*/
bool
BuildModuleTargetWithCMake(const Util::String& buildTarget, Util::String& outStatus)
{
    Util::String buildDir = GetEditorBuildDir();
    if (!buildDir.IsValid() || !IO::FSWrapper::DirectoryExists(buildDir))
    {
        outStatus = "Reload failed: build directory not found";
        return false;
    }

    Util::String cmd = Util::String::Sprintf(
        "cmake --build \"%s\" --target \"%s\"",
        buildDir.AsCharPtr(),
        buildTarget.AsCharPtr()
    );

    int buildResult = std::system(cmd.AsCharPtr());
    if (buildResult != 0)
    {
        outStatus = "Reload failed: module build failed";
        return false;
    }

    outStatus = "Build succeeded, reload queued...";
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
BuildModuleTarget(const Util::String& buildTarget, Util::String& outStatus)
{
    Util::String workspaceDir;
    Util::String projectName;
    if (!GetWorkspaceAndProjectFromBinaryDir(workspaceDir, projectName))
    {
        return BuildModuleTargetWithCMake(buildTarget, outStatus);
    }

    Util::String projectDir = Util::String::AppendPath(workspaceDir, projectName);
    if (!IO::FSWrapper::DirectoryExists(projectDir))
    {
        return BuildModuleTargetWithCMake(buildTarget, outStatus);
    }

    Util::String localFips = Util::String::AppendPath(projectDir, "fips");
    Util::String workspaceFips = Util::String::AppendPath(Util::String::AppendPath(workspaceDir, "fips"), "fips");

    Util::String fipsScript;
    if (IO::FSWrapper::FileExists(localFips))
    {
        fipsScript = localFips;
    }
    else if (IO::FSWrapper::FileExists(workspaceFips))
    {
        fipsScript = workspaceFips;
    }
    else
    {
        return BuildModuleTargetWithCMake(buildTarget, outStatus);
    }

    Util::String cmd = Util::String::Sprintf(
        "cd \"%s\" && \"%s\" modulebuild build \"%s\"",
        projectDir.AsCharPtr(),
        fipsScript.AsCharPtr(),
        buildTarget.AsCharPtr()
    );

    int buildResult = std::system(cmd.AsCharPtr());
    if (buildResult != 0)
    {
        Util::String fallbackStatus;
        if (!BuildModuleTargetWithCMake(buildTarget, fallbackStatus))
        {
            outStatus = "Reload failed: fips modulebuild and cmake fallback both failed";
            return false;
        }
    }

    outStatus = "Build succeeded, reload queued...";
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
RestoreReloadSnapshot()
{
    if (!Game::EditorState::HasInstance())
        return false;

    Game::EditorState* editorState = Game::EditorState::Instance();
    if (!editorState->reloadSnapshotPending || !editorState->reloadSnapshotPath.IsValid())
        return false;

    IO::URI snapshotUri = IO::URI(editorState->reloadSnapshotPath);
    if (!IO::IoServer::Instance()->FileExists(snapshotUri))
    {
        editorState->reloadSnapshotPending = false;
        return false;
    }

    Game::World* gameWorld = Game::GetWorld(WORLD_DEFAULT);
    n_assert(gameWorld != nullptr);

    Game::GameServer::Instance()->CleanupWorld(gameWorld);
    Game::GameServer::Instance()->SetupEmptyWorld(gameWorld);

    Ptr<EntityLoader> loader = EntityLoader::Create();
    loader->SetWorld(state.editorWorld);

    Ptr<IO::JsonReader> reader = IO::JsonReader::Create();
    reader->SetStream(IO::IoServer::Instance()->CreateStream(snapshotUri));
    if (!reader->Open())
    {
        editorState->reloadSnapshotPending = false;
        return false;
    }

    loader->LoadJsonLevel(reader);
    reader->Close();

    Edit::CommandManager::Clear();
    Edit::CommandManager::SetClean();

    editorState->reloadSnapshotPending = false;
    return true;
}

}

//------------------------------------------------------------------------------
/**
*/
State state;

//------------------------------------------------------------------------------
/**
*/
bool
IsCreated()
{
    return state.editorWorld != nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
Create()
{
    if (IsCreated())
        return;

    IO::AssignRegistry::Instance()->SetAssign(IO::Assign("edscr", "bin:editorscripts"));
    IO::AssignRegistry::Instance()->SetAssign(IO::Assign("work", "proj:work"));
    IO::AssignRegistry::Instance()->SetAssign(IO::Assign("assets", "work:assets"));
    IO::AssignRegistry::Instance()->SetAssign(IO::Assign("src", "proj:work"));

    if (!Game::Time::HasTimeSource(TIMESOURCE_EDITOR))
    {
        Game::TimeSourceCreateInfo editorTimeSourceInfo;
        editorTimeSourceInfo.hash = TIMESOURCE_EDITOR;
        Game::Time::CreateTimeSource(editorTimeSourceInfo);
    }

    ToolkitUtil::ProjectInfo projectInfo;
    ToolkitUtil::ProjectInfo::Result res = projectInfo.Setup();
    n_assert(res == ToolkitUtil::ProjectInfo::Success);
    IO::AssignRegistry::Instance()->SetAssign(IO::Assign("int", projectInfo.GetAttr("IntermediateDir")));
    IO::IoServer::Instance()->CreateDirectory("int:");

    // Load optional runtime hooks up-front so tool windows do not trigger
    // first-use loading in the middle of interaction.
    Presentation::EnsureNavigationUiHookLoaded();

    LiveBatcher::Setup();

    Game::TimeSource* gameTimeSource = Game::Time::GetTimeSource(TIMESOURCE_GAMEPLAY);
    gameTimeSource->timeFactor = 0.0f;

    state.editorWorld = Game::GameServer::Instance()->CreateWorld(WORLD_EDITOR);
    state.editorWorld->componentInitializationEnabled = false;

    Game::GameServer::Instance()->SetupEmptyWorld(state.editorWorld);

    // Create a command manager with a 20MB buffer
    Edit::CommandManager::Create(20_MB);
    CreatePathConverter({});

    if (!Game::EditorState::HasInstance())
    {
        Game::EditorState::Singleton = new Game::EditorState();
    }

    if (RestoreReloadSnapshot())
    {
        state.lastReloadStatus = "Reload restore complete";
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Start()
{
    if (!Game::EditorState::Instance()->pythonBootstrapInitialized)
    {
        Scripting::ScriptServer::Instance()->AddModulePath("edscr:");
        Scripting::ScriptServer::Instance()->EvalFile("edscr:bootstrap.py");

        /// Import reload to be able to reload modules.
        Scripting::ScriptServer::Instance()->Eval("from importlib import reload");

        Game::EditorState::Instance()->pythonBootstrapInitialized = true;
    }

    Game::EditorState::Instance()->isRunning = true;
}

//------------------------------------------------------------------------------
/**
*/
void
Destroy()
{
    if (Game::EditorState::HasInstance())
    {
        Game::EditorState::Instance()->isRunning = false;
        Game::EditorState::Instance()->isPlaying = false;
    }

    if (state.editorWorld != nullptr)
    {
        Game::GameServer::Instance()->DestroyWorld(WORLD_EDITOR);
        state.editorWorld = nullptr;
        state.editables.Reset();
    }

    LiveBatcher::Discard();
    Edit::CommandManager::Discard();

    Game::Time::DestroyTimeSource(TIMESOURCE_EDITOR);

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
void
RequestModuleReload(const Util::String& moduleName, const Util::String& buildTarget)
{
    if (!moduleName.IsValid())
    {
        state.lastReloadStatus = "Reload failed: module name is empty";
        n_printf("Editor: %s\n", state.lastReloadStatus.AsCharPtr());
        return;
    }

    if (!Game::EditorState::HasInstance() || Game::EditorState::Instance()->isPlaying)
    {
        state.lastReloadStatus = "Reload blocked: play-in-editor is active";
        n_printf("Editor: %s\n", state.lastReloadStatus.AsCharPtr());
        return;
    }

    Ptr<Game::ModuleManager> mm = App::GameApplication::Instance()->GetModuleManager();
    if (!mm.isvalid())
    {
        state.lastReloadStatus = "Reload failed: no module manager";
        n_printf("Editor: %s\n", state.lastReloadStatus.AsCharPtr());
        return;
    }

    Util::String resolvedBuildTarget = buildTarget.IsValid() ? buildTarget : moduleName;
    if (!BuildModuleTarget(resolvedBuildTarget, state.lastReloadStatus))
    {
        n_printf("Editor: %s\n", state.lastReloadStatus.AsCharPtr());
        return;
    }

    IO::IoServer::Instance()->CreateDirectory("user:nebula/editor/");
    if (!SaveEntities(ReloadSnapshotUri))
    {
        state.lastReloadStatus = "Reload failed: could not snapshot current level";
        n_printf("Editor: %s\n", state.lastReloadStatus.AsCharPtr());
        return;
    }

    Game::EditorState::Instance()->reloadSnapshotPath = ReloadSnapshotUri;
    Game::EditorState::Instance()->reloadSnapshotPending = true;

    mm->QueueModuleReload(moduleName);
    state.lastReloadStatus = "Build succeeded, reload queued...";
    n_printf("Editor: module '%s' reload queued\n", moduleName.AsCharPtr());
}

//------------------------------------------------------------------------------
/**
*/
const Util::String&
GetLastReloadStatus()
{
    return state.lastReloadStatus;
}

} // namespace Editor

