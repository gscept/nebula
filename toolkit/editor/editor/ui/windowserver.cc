//------------------------------------------------------------------------------
//  windowmanager.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "components/lighting.h"
#include "foundation/stdneb.h"
#include "windowserver.h"
#include "imgui.h"
#include "io/jsonwriter.h"
#include "io/jsonreader.h"
#include "io/ioserver.h"
#include "app/application.h"
#include "input/inputserver.h"
#include "input/keyboard.h"
#include "io/filedialog.h"
#include "editor/entityloader.h"
#include "editor/editor.h"
#include "editor/commandmanager.h"
#include "editor/editor.h"
#include "editor/cmds.h"
#include "editor/tools/selectioncontext.h"
#include "uimanager.h"

#include "toolkit-common/logger.h"
#include "io/ioserver.h"
#include "io/textwriter.h"
#include "io/textreader.h"

#include "dynui/imguicontext.h"
#include "toolkitutil/asset/assetimporter.h"
#include "toolkitutil/asset/assetpackager.h"

#include "windows/console.h"
#include "windows/outline.h"
#include "windows/history.h"
#include "windows/styleeditor.h"
#include "windows/toolbar.h"
#include "windows/environment.h"
#include "windows/scene.h"
#include "windows/inspector.h"
#include "windows/assetbrowser.h"
#include "windows/asseteditor/asseteditor.h"
#include "windows/resourcebrowser.h"
#include "windows/profiler.h"
#include "windows/physics.h"
#include "windows/navigation.h"
#include "windows/settings.h"
#include "windows/importwindow.h"
#include "windows/terraineditor/terraineditor.h"
#include "editor/tools/livebatcher.h"

#include "dynui/nebula_icons.h"

using namespace Util;

namespace Presentation
{

BaseWindow* ConsoleWindow;
BaseWindow* OutlineWindow;
BaseWindow* HistoryWindow;
BaseWindow* StyleEditorWindow;
BaseWindow* ToolbarWindow;
BaseWindow* EnvironmentWindow;
BaseWindow* SceneWindow;
BaseWindow* InspectorWindow;
BaseWindow* AssetBrowserWindow;
BaseWindow* AssetBrowserPickerWindow;
BaseWindow* AssetEditorWindow;
BaseWindow* ResourceBrowserWindow;
BaseWindow* ProfilerWindow;
BaseWindow* PhysicsWindow;
BaseWindow* NavigationWindow;
BaseWindow* SettingsWindow;
BaseWindow* TerrainEditorWindow;
BaseWindow* BatcherWindow;
BaseWindow* ImporterWindow;

__ImplementClass(Presentation::WindowServer, 'wSrv', Core::RefCounted);
__ImplementSingleton(Presentation::WindowServer);

#define SETUP_WINDOW(var, type, name, category)\
    var = new type(); var->SetName(#name); var->SetCategory(#category);\
    this->windowByName.Add(#name, var);\
    this->windows.Append(var);\
    this->AddCategory(#category);


//------------------------------------------------------------------------------
/**
*/
WindowServer::WindowServer()
{
    __ConstructSingleton;
    SETUP_WINDOW(ConsoleWindow, Console, Console, Debug);
    SETUP_WINDOW(OutlineWindow, Outline, Outline, );
    SETUP_WINDOW(HistoryWindow, History, History, Editor);
    SETUP_WINDOW(StyleEditorWindow, StyleEditor, Style Editor, Editor);
    SETUP_WINDOW(ToolbarWindow, Toolbar, Toolbar, );
    SETUP_WINDOW(EnvironmentWindow, Environment, Environment, );
    SETUP_WINDOW(SceneWindow, Scene, Scene, );
    SETUP_WINDOW(InspectorWindow, Inspector, Inspector, );
    SETUP_WINDOW(AssetBrowserWindow, AssetBrowser, Asset Browser, );
    SETUP_WINDOW(AssetBrowserPickerWindow, AssetBrowser, Asset Picker, );
    SETUP_WINDOW(AssetEditorWindow, AssetEditor, Asset Editor, Editor);
    SETUP_WINDOW(ResourceBrowserWindow, ResourceBrowser, Resource Browser, );
    SETUP_WINDOW(ProfilerWindow, Profiler, Profiler, );
    SETUP_WINDOW(PhysicsWindow, Physics, Physics, );
    SETUP_WINDOW(NavigationWindow, Navigation, Navigation, );
    SETUP_WINDOW(SettingsWindow, Settings, Settings, );
    SETUP_WINDOW(TerrainEditorWindow, TerrainEditor, Terrain Editor, );
    SETUP_WINDOW(BatcherWindow, LiveBatcherWindow, Live Batcher, );
    SETUP_WINDOW(ImporterWindow, AssetImporterWindow, Importer, );

    this->LoadState();

    AssetBrowserPickerWindow->Modal();
    AssetBrowserPickerWindow->open = false;
}

//------------------------------------------------------------------------------
/**
*/
WindowServer::~WindowServer()
{
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void 
WindowServer::SaveState()
{
    IO::CreateDirectory("user:nebula-editor"_uri.LocalPath());
    IO::URI settings("user:nebula-editor/window_state.ini");
    Ptr<IO::Stream> state = IO::CreateStream(settings);
    state->SetAccessMode(IO::Stream::AccessMode::WriteAccess);
    if (state->Open())
    {
        Ptr<IO::TextWriter> writer = IO::TextWriter::Create();
        writer->SetStream(state);
        writer->Open();
        auto it = windowByName.Begin();
        while (it != windowByName.End())
        {
            writer->WriteString(*it.key);
            writer->WriteString("=");
            writer->WriteChar((*it.val)->open ? '1' : '0');
            writer->WriteChar('\n');
            it++;
        }
        state->Close();
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
WindowServer::LoadState()
{
    IO::URI settings("user:nebula-editor/window_state.ini");
    Ptr<IO::Stream> state = IO::CreateStream(settings);
    state->SetAccessMode(IO::Stream::AccessMode::ReadAccess);
    if (state->Open())
    {
        Ptr<IO::TextReader> reader = IO::TextReader::Create();
        reader->SetStream(state);

        Util::Array<Util::String> lines = reader->ReadAllLines();
        for (const Util::String& line : lines)
        {
            Util::Array<Util::String> parts = line.Tokenize("=");
            const Util::String& window = parts[0];
            const Util::String& flag = parts[1];

            IndexT windowIndex = this->windowByName.FindIndex(window);
            if (windowIndex != InvalidIndex)
            {
                const bool open = flag.AsInt();
                this->windowByName.ValueAtIndex(window, windowIndex)->open = open;
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
WindowServer::RunAll()
{
    //List all windows in windows menu
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("    File    "))
        {
            if (ImGui::MenuItem("Load", "Ctrl+O"))
            {
                static Util::String localpath = IO::URI("proj:").LocalPath();
                Util::String path;
                if (IO::FileDialog::OpenFile("Select Nebula Level", localpath, { "*.json" }, path))
                {
                    Editor::LoadLevel(path);
                }
            }
            if (ImGui::MenuItem("Instantiate Level"))
            {
                static Util::String localpath = IO::URI("proj:work/levels").LocalPath();
                Util::String path;
                if (IO::FileDialog::OpenFile("Select Nebula Level", localpath, { "*.json" }, path))
                {
                    Editor::LoadLevel(path, true);
                }
            }
            if (ImGui::MenuItem("Save", "Ctrl+S"))
            {
                Editor::SaveLevelWithDialog();
                Presentation::WindowServer::Instance()->BroadcastSave(Presentation::BaseWindow::SaveMode::SaveActive);
            }

            if (ImGui::MenuItem("Save All", "Ctrl+Shift+S"))
            {
                Editor::SaveLevelWithDialog();
                Presentation::WindowServer::Instance()->BroadcastSave(Presentation::BaseWindow::SaveMode::SaveAll);
            }

            if (ImGui::MenuItem("Save As"))
            {
                Editor::SaveLevelWithDialog(true);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("    Edit    "))
        {
            if (ImGui::MenuItem("Undo", "Ctrl+Z"))
            {
                Edit::CommandManager::Undo();
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z"))
            {
                Edit::CommandManager::Redo();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("    Create    "))
        {
            if (ImGui::MenuItem("Entity", "Ctrl+A"))
            {
                Edit::CreateEntity();
            }
            if (ImGui::BeginMenu("Light"))
            {
                if (ImGui::MenuItem("Point Light"))
                {
                    Edit::CommandManager::BeginMacro("Create point light", true);
                    Editor::Entity newEntity = Edit::CreateEntity();
                    Edit::AddComponent(newEntity, Game::GetComponentId<GraphicsFeature::PointLight>());
                    Util::String name = "Point Light ";
                    name.AppendInt(newEntity.index);
                    Edit::SetEntityName(newEntity, name);
                    Edit::SetSelection({ newEntity });
                    Edit::CommandManager::EndMacro();
                }
                if (ImGui::MenuItem("Spot Light"))
                {
                    Edit::CommandManager::BeginMacro("Create spot light", true);
                    Editor::Entity newEntity = Edit::CreateEntity();
                    Edit::AddComponent(newEntity, Game::GetComponentId<GraphicsFeature::SpotLight>());
                    Util::String name = "Spot Light ";
                    name.AppendInt(newEntity.index);
                    Edit::SetEntityName(newEntity, name);
                    Edit::SetSelection({ newEntity });
                    Edit::CommandManager::EndMacro();
                }
                if (ImGui::MenuItem("Area Light"))
                {
                    Edit::CommandManager::BeginMacro("Create area light", true);
                    Editor::Entity newEntity = Edit::CreateEntity();
                    Edit::AddComponent(newEntity, Game::GetComponentId<GraphicsFeature::AreaLight>());
                    Util::String name = "Area Light ";
                    name.AppendInt(newEntity.index);
                    Edit::SetEntityName(newEntity, name);
                    Edit::SetSelection({ newEntity });
                    Edit::CommandManager::EndMacro();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("    Game    "))
        {
            if (ImGui::MenuItem("Play", "Ctrl+P"))
            {
                Editor::PlayGame();
            }
            if (ImGui::MenuItem("Pause", "Ctrl+Shift+P"))
            {
                Editor::PauseGame();
            }
            if (ImGui::MenuItem("Stop", "Ctrl+S"))
            {
                Editor::StopGame();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("    Window    "))
        {
            if (ImGui::BeginMenu("Show"))
            {
                // TODO: there's WAY better ways to do this.

                // First, show all sub categories
                for (SizeT i = 0; i < this->categories.Size(); i++)
                {
                    auto& category = this->categories[i];
                    if (ImGui::BeginMenu(category.AsCharPtr()))
                    {
                        for (SizeT j = 0; j < this->windows.Size(); j++)
                        {
                            auto it = this->windows[j];
                            if (it->GetCategory() == category)
                            {
                                ImGui::MenuItem(it->GetName().AsCharPtr(), NULL, &it->Open());
                            }
                        }
                        ImGui::EndMenu();
                    }
                }

                // last, show categoryless windows.
                for (SizeT i = 0; i < this->windows.Size(); i++)
                {
                    auto it = this->windows[i];
                    if (it->GetCategory().IsEmpty())
                        ImGui::MenuItem(it->GetName().AsCharPtr(), NULL, &it->Open());
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Shortcuts"))
            {
                for (SizeT i = 0; i < this->commands.Size(); i++)
                {
                    auto const& shortcutStr = this->commands.ValueAtIndex(i).shortcut;
                    const char* shortcut = shortcutStr.IsEmpty() ? NULL : shortcutStr.AsCharPtr();
                    if (ImGui::MenuItem(this->commands.ValueAtIndex(i).label.AsCharPtr(), shortcut))
                    {
                        this->commands.ValueAtIndex(i).func();
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Save Window Layout"))
            {
                const IO::URI path(Dynui::EditorUIPath);
                ImGui::SaveIniSettingsToDisk(path.LocalPath().c_str());
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    // clear transient hover state, it's re-evaluated as windows run
    Tools::SelectionContext::ClearHovered();

    //Run all windows
    for (SizeT i = 0; i < this->windows.Size(); i++)
    {
        auto it = this->windows[i];

        if (!it->ShouldRun())
            continue;
        
        N_SCOPE_DYN(it->name.AsCharPtr(), UI)

        if (it->popupThisFrame)
        {
            ImGui::OpenPopup(it->GetName().AsCharPtr());

        }
        if (it->Open())
        {
            if (it->usesCustomWindowPadding)
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {it->windowPadding.x, it->windowPadding.y});

            if (it->modal)
            {
                if (ImGui::BeginPopupModal(it->GetName().AsCharPtr(), &it->open, it->GetAdditionalFlags()))
                {
                    it->Run(this->save);

                    if (!it->open)
                    {
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }
            }
            else
            {
                if (ImGui::Begin(it->GetName().AsCharPtr(), &it->Open(), it->GetAdditionalFlags()))
                {
                    if (it->focusThisFrame)
                    {
                        ImGui::SetWindowFocus(it->GetName().AsCharPtr());
                        ImGuiViewport* viewport = ImGui::GetWindowViewport();
                        ImGui::GetPlatformIO().Platform_SetWindowFocus(viewport);
                        it->focusThisFrame = false;
                    }
                    it->Run(this->save);
                }
                ImGui::End();
            }

            if (it->usesCustomWindowPadding)
                ImGui::PopStyleVar();
        }
    }
    this->save = BaseWindow::SaveMode::None;
}

//------------------------------------------------------------------------------
/**
*/
void
WindowServer::Update()
{
    for (SizeT i = 0; i < this->windows.Size(); i++)
    {
        auto it = this->windows[i];
        it->Update();
    }

    auto const& keyboard = Input::InputServer::Instance()->GetDefaultKeyboard();

    // check for shortcuts
    IndexT cmdIndex = -1;
    int longestShortcut = 0;
    for (IndexT i = 0; i < this->commands.Size(); ++i)
    {
        CommandInfo const& cmd = this->commands.ValueAtIndex(i);
        bool exec = true;
        for (IndexT k = 0; k < cmd.keys.Size(); ++k)
        {
            auto key = cmd.keys[k];

            // if we're processing the last key, we check if its been pressed once, other keys if they're held down.
            if (k == (cmd.keys.Size() - 1))
            {
                if (!keyboard->KeyDown(key))
                {
                    exec = false;
                    break;
                }
            }
            else
            {
                bool pressed = keyboard->KeyPressed(key);
                if (!pressed)
                {
                    if (key == Input::Key::Code::Control)
                    {
                        // special case, check both left and right key
                        if (!(keyboard->KeyPressed(Input::Key::Code::LeftControl) || keyboard->KeyPressed(Input::Key::Code::RightControl)))
                        {
                            exec = false;
                            break;
                        }
                    }
                    else if (key == Input::Key::Code::Shift)
                    {
                        // special case, check both left and right key
                        if (!(keyboard->KeyPressed(Input::Key::Code::LeftShift) || keyboard->KeyPressed(Input::Key::Code::RightShift)))
                        {
                            exec = false;
                            break;
                        }
                    }
                    else if (key == Input::Key::Code::Menu)
                    {
                        // special case, check both left and right key
                        if (!(keyboard->KeyPressed(Input::Key::Code::LeftMenu) || keyboard->KeyPressed(Input::Key::Code::RightMenu)))
                        {
                            exec = false;
                            break;
                        }
                    }
                    else
                    {
                        exec = false;
                        break;
                    }
                }
            }
        }

        // run command
        if (exec && cmd.keys.Size() > longestShortcut)
        {
            longestShortcut = cmd.keys.Size();
            cmdIndex = i;
        }
    }

    if (cmdIndex != -1)
    {
        this->commands.ValueAtIndex(cmdIndex).func();
    }

}

//------------------------------------------------------------------------------
/**
*/
void 
WindowServer::BroadcastSave(BaseWindow::SaveMode mode)
{
    this->save = mode;
}


//------------------------------------------------------------------------------
/**
    If menu is NULL, the command won't show up in the menu.
    If category is NULL, the command is placed directly in the menu tab
*/
void
WindowServer::RegisterCommand(Util::Delegate<void()> func, Util::String const& label, Util::String const& shortcut, const char* menu, const char* category)
{
    if (this->commands.Contains(label))
    {
        n_warning("Command delegate with label %s already exists!", label.AsCharPtr());
        return;
    }

    // Split the shortcut into keycodes and validate
    Util::Array<Util::String> keyTokens = shortcut.Tokenize("+");

    Util::FixedArray<Input::Key::Code> keys;
    keys.SetSize(keyTokens.Size());

    for (IndexT i = 0; i < keyTokens.Size(); ++i)
    {
        if (!Input::Key::IsValid(keyTokens[i]))
        {
            n_warning("Command: \"%s\", Shortcut: \"%s\" - %s is not a valid keycode!", label.AsCharPtr(), shortcut.AsCharPtr(), keyTokens[i].AsCharPtr());
            return;
        }

        auto key = Input::Key::FromString(keyTokens[i]);
        keys[i] = key;
    }

    CommandInfo info = {
        func,
        label,
        shortcut,
        keys,
        menu,
        category
    };

    this->commands.Add(label, info);
}

//------------------------------------------------------------------------------
/**
*/
void
WindowServer::AddCategory(const Util::String & category)
{
    if (!category.IsEmpty())
    {
        if (this->categories.FindIndex(category) == InvalidIndex)
        {
            this->categories.Append(category);
        }
    }
}

} // namespace Presentation
