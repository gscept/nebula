//------------------------------------------------------------------------------
//  @file editorfeatureunit.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/core/editorfeatureunit.h"
#include "neweditor/api/editorfeature.h"
#include "neweditor/base/editorlog.h"
#include "neweditor/console/commandscript.h"
#include "neweditor/console/editorconsolehandler.h"
#include "neweditor/core/editorcore.h"
#include "neweditor/extensions/propertyrecorder.h"
#include "neweditor/input/editorkeybinds.h"
#include "neweditor/model/scene.h"
#include "neweditor/panels/consolepanel.h"
#include "neweditor/panels/filebrowserpanel.h"
#include "neweditor/panels/inspectorpanel.h"
#include "neweditor/panels/outlinepanel.h"
#include "neweditor/panels/profilerpanel.h"
#include "input/inputserver.h"
#include "util/delegate.h"

namespace Editor
{

__ImplementClass(Editor::EditorFeatureUnit, 'EDEF', Game::FeatureUnit);

//------------------------------------------------------------------------------
/**
*/
EditorFeatureUnit::EditorFeatureUnit()
{
}

//------------------------------------------------------------------------------
/**
*/
EditorFeatureUnit::~EditorFeatureUnit()
{
}

//------------------------------------------------------------------------------
/**
    The value that follows a flag. CommandLineArgs::GetString() reads past the end of the argument
    list when a flag has no value, so this walks the arguments itself.
*/
Util::String
EditorFeatureUnit::FlagValue(const Util::String& name) const
{
    const Util::CommandLineArgs& args = this->GetCmdLineArgs();
    const IndexT numArgs = (IndexT)args.GetNumArgs();

    for (IndexT i = 0; i < numArgs; i++)
    {
        if (args.GetStringAtIndex(i) == name)
        {
            if (i + 1 < numArgs)
                return args.GetStringAtIndex(i + 1);
            break;
        }
    }

    return Util::String();
}

//------------------------------------------------------------------------------
/**
    The core is created here rather than at activation, because a feature contributes at its own
    attach and asks for the core.
*/
void
EditorFeatureUnit::OnAttach()
{
    FeatureUnit::OnAttach();

    this->core = EditorCore::Create();

    // The panels this editor has. They are registered here rather than at activation, so they come
    // before what a feature registers at its own attach, because a feature's contribution is an
    // addition to this set rather than part of it. The order the registry is read at Open is the
    // order the default layout is built from, so this is also the shape the editor starts in: the
    // first panel is the main area and the rest are stacked on the right, with the contributions at
    // the bottom of that stack
    EditorFeatureRegistry::RegisterPanel(ConsolePanel::Create());
    EditorFeatureRegistry::RegisterPanel(InspectorPanel::Create());
    EditorFeatureRegistry::RegisterPanel(OutlinePanel::Create());
    EditorFeatureRegistry::RegisterPanel(FileBrowserPanel::Create());
    EditorFeatureRegistry::RegisterPanel(ProfilerPanel::Create());
}

//------------------------------------------------------------------------------
/**
*/
void
EditorFeatureUnit::OnActivate()
{
    FeatureUnit::OnActivate();

    if (this->core == nullptr)
        return;

    this->core->Setup();

    // The scene has to exist before the panels are prepared, because a panel resolves what it needs
    // in OnSetup
    this->core->AddScene("Scene");

    if (!this->core->Open())
    {
        this->core = nullptr;
        return;
    }

    this->consoleHandler = EditorConsoleHandler::Create();
    this->consoleHandler->Setup(this->core);

    this->core->GetCommands().Register("pie", "start, stop or restart play-in-editor",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<EditorFeatureUnit, &EditorFeatureUnit::CommandPie>(this));
    this->core->GetCommands().Register("create", "create an entity in the scene and select it",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<EditorFeatureUnit, &EditorFeatureUnit::CommandCreate>(this));
    this->core->GetCommands().Register("exit", "exit [force] - quit the editor",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<EditorFeatureUnit, &EditorFeatureUnit::CommandExit>(this));

    // Play-in-editor keybinds. A bind with no modifiers matches any modifier, so the modified one
    // wins the lookup
    EditorKeybinds& keybinds = this->core->GetKeybinds();
    keybinds.Register("pie.stop", Input::Key::F5, Input::Modifier::ShiftModifier, "pie stop");
    keybinds.Register("pie.start", Input::Key::F5, Input::Modifier::NoModifier, "pie start");
    keybinds.Register("pie.stop.alt", Input::Key::F8, Input::Modifier::NoModifier, "pie stop");

    // The keybind for the command registered above, which is what makes Ctrl+N the same route as
    // typing 'create'
    keybinds.Register("create", Input::Key::N, Input::Modifier::ControlModifier, "create");

    // The keybind file is read here rather than in Open, because this is the last thing that
    // registers an action. Read earlier, the file would be applied over a registry that does not
    // have these actions yet
    keybinds.Load();

    this->core->Log(LogMessageType::N_SYSTEM, "Editor session started");
    this->core->Log(LogMessageType::N_SYSTEM,
        "Console ready. Type a cvar to read it, or 'cl_debug 1' to set one. 'help' lists the commands");

    const Util::String scriptPath = this->FlagValue("--commands");
    if (!scriptPath.IsEmpty())
    {
        this->script = CommandScript::Create();
        if (!this->script->Setup(this->core, scriptPath))
            this->script = nullptr;    // Setup logs what went wrong
        else
            this->core->Log(LogMessageType::N_SYSTEM, Util::String("Running commands from ") + scriptPath);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
EditorFeatureUnit::OnBeginFrame()
{
    FeatureUnit::OnBeginFrame();

    if (this->core == nullptr)
        return;

    // The window's X and Alt-F4 land as the input server's quit flag, and the app loop reads that
    // flag as its own. Take it here, before the loop does, so the editor gets to ask about unsaved
    // work first. Only the editor's own exit paths can end the app after this
    if (Input::InputServer::HasInstance() && Input::InputServer::Instance()->IsQuitRequested())
    {
        Input::InputServer::Instance()->SetQuitRequested(false);
        this->core->RequestQuit(false);
    }

    // The commands run before the panels draw, so whatever a command changes is shown this frame
    if (this->script != nullptr)
        this->script->Update();

    this->core->Draw();
}

//------------------------------------------------------------------------------
/**
*/
void
EditorFeatureUnit::OnDeactivate()
{
    FeatureUnit::OnDeactivate();

    if (this->script != nullptr)
    {
        this->script->Discard();
        this->script = nullptr;
    }

    if (this->consoleHandler != nullptr)
    {
        this->consoleHandler->Discard();
        this->consoleHandler = nullptr;
    }

    PropertyRecorder::Discard();

    if (this->core != nullptr)
    {
        // Close discards the open scenes, which is what releases their authoring worlds
        this->core->Close();
        this->core = nullptr;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
EditorFeatureUnit::CommandPie(const Util::Array<Util::String>& args)
{
    const Ptr<Scene> scene = this->core->GetScene();

    if (args.IsEmpty())
    {
        if (scene->IsPlaying())
            scene->StopPlay();
        else
            scene->StartPlay();
        return;
    }

    const Util::String& verb = args[0];
    if (verb == "start")
        scene->StartPlay();
    else if (verb == "stop")
        scene->StopPlay();
    else if (verb == "restart")
        scene->RestartPlay();
    else
        this->core->Log(LogMessageType::N_WARNING, "pie expects start, stop or restart");
}

//------------------------------------------------------------------------------
/**
    The create is the core's, which is the same one the outline's New Entity button runs. The
    outline reveals the selection it draws, so a create through the console or Ctrl+N opens the node
    the way a create from the tree does.
*/
void
EditorFeatureUnit::CommandCreate(const Util::Array<Util::String>& args)
{
    const Util::String base = args.IsEmpty() ? Util::String("Entity") : args[0];
    this->core->CreateEntity(base, this->core->GetScene()->GetActiveCollection());
}

//------------------------------------------------------------------------------
/**
    A scene whose file does not hold what the tab holds now is asked about before the editor goes,
    which is what 'force' is for: a command script that does not care.
*/
void
EditorFeatureUnit::CommandExit(const Util::Array<Util::String>& args)
{
    const bool force = !args.IsEmpty() && args[0] == "force";
    if (!args.IsEmpty() && !force)
    {
        this->core->Log(LogMessageType::N_WARNING, "exit takes no arguments, or the optional 'force'");
        return;
    }

    this->core->RequestQuit(force);
}

} // namespace Editor
//------------------------------------------------------------------------------
