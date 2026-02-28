//------------------------------------------------------------------------------
//  windowmanager.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "windowserver.h"
#include "imgui.h"
#include "io/jsonwriter.h"
#include "io/jsonreader.h"
#include "io/ioserver.h"
#include "app/application.h"
#include "input/inputserver.h"
#include "input/keyboard.h"
#include "windows/scriptedwindow.h"
#include "io/filedialog.h"
#include "editor/entityloader.h"
#include "editor/editor.h"
#include "editor/commandmanager.h"
#include "editor/editor.h"
#include "editor/cmds.h"
#include "uimanager.h"

using namespace Util;

namespace Presentation
{

__ImplementClass(Presentation::WindowServer, 'wSrv', Core::RefCounted);
__ImplementSingleton(Presentation::WindowServer);

//------------------------------------------------------------------------------
/**
*/
WindowServer::WindowServer()
{
    __ConstructSingleton;
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
                    Ptr<Editor::EntityLoader> loader = Editor::EntityLoader::Create();
                    loader->SetWorld(Editor::state.editorWorld);
                    Ptr<IO::JsonReader> reader = IO::JsonReader::Create();
                    reader->SetStream(IO::IoServer::Instance()->CreateStream(path));
                    if (reader->Open())
                    {
                        loader->LoadJsonLevel(reader);
                    }
                    reader->Close();
                }
            }
            if (ImGui::MenuItem("Save", "Ctrl+S"))
            {
                Presentation::WindowServer::Instance()->BroadcastSave(Presentation::BaseWindow::SaveMode::SaveActive);
            }

            if (ImGui::MenuItem("Save All", "Ctrl+Shift+S"))
            {
                Presentation::WindowServer::Instance()->BroadcastSave(Presentation::BaseWindow::SaveMode::SaveAll);
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
            if (ImGui::MenuItem("Entity", "Ctrl+C"))
            {
                Presentation::WindowServer::Instance()->GetWindow("Create Object")->Open() = true;
            }
            if (ImGui::BeginMenu("Light"))
            {
                if (ImGui::MenuItem("Point Light"))
                {
                    Edit::CommandManager::BeginMacro("Create point light", true);
                    Editor::Entity newEntity = Edit::CreateEntity("PointLight");
                    Edit::SetSelection({ newEntity });
                    Edit::CommandManager::EndMacro();
                }
                if (ImGui::MenuItem("Spot Light"))
                {
                    Edit::CommandManager::BeginMacro("Create spot light", true);
                    Editor::Entity newEntity = Edit::CreateEntity("SpotLight");
                    Edit::SetSelection({ newEntity });
                    Edit::CommandManager::EndMacro();
                }
                if (ImGui::MenuItem("Area Light"))
                {
                    Edit::CommandManager::BeginMacro("Create area light", true);
                    Editor::Entity newEntity = Edit::CreateEntity("AreaLight");
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
                const IO::URI path(Editor::UIManager::GetEditorUIIniPath());
                ImGui::SaveIniSettingsToDisk(path.LocalPath().c_str());
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    //Run all windows
    for (SizeT i = 0; i < this->windows.Size(); i++)
    {
        auto it = this->windows[i];
        
        N_SCOPE_DYN(it->name.AsCharPtr(), UI)
        if (it->Open())
        {
            if (it->usesCustomWindowPadding)
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {it->windowPadding.x, it->windowPadding.y});

            if (ImGui::Begin(it->GetName().AsCharPtr(), &it->Open(), it->GetAdditionalFlags()))
            {
                it->Run(this->save);
            }
            ImGui::End();

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
*/
void
WindowServer::RegisterWindow(const Util::String & className, const char * label, const char* category)
{
    Ptr<BaseWindow> intFace((BaseWindow*)Core::Factory::Instance()->Create(className));
    n_assert2(intFace != nullptr, "Interface could not be found by provided class name!");

    intFace->SetName(label);
    intFace->SetCategory(category);
    this->windowByName.Add(label, intFace);
    this->windows.Append(intFace);
    this->AddCategory(category);
}

//------------------------------------------------------------------------------
/**
*/
void
WindowServer::RegisterWindow(const Util::FourCC fourcc, const char * label, const char* category)
{
    Ptr<BaseWindow> intFace((BaseWindow*)Core::Factory::Instance()->Create(fourcc));
    n_assert2(intFace != nullptr, "Interface could not be found by provided FourCC");

    intFace->SetName(label);
    intFace->SetCategory(category);
    this->windowByName.Add(label, intFace);
    this->windows.Append(intFace);
    this->AddCategory(category);
}

//------------------------------------------------------------------------------
/**
*/
void
WindowServer::RegisterWindow(const Ptr<BaseWindow>& base)
{
    this->windowByName.Add(base->GetName(), base);
    this->windows.Append(base);
    this->AddCategory(base->GetCategory());
}

//------------------------------------------------------------------------------
/**
*/
void
WindowServer::RegisterWindowScript(const char* script, const char* label)
{
	Ptr<ScriptedWindow> wnd = ScriptedWindow::Create();
	wnd->SetName(label);
	if (wnd->LoadModule(script))
	{
		this->RegisterWindow(wnd.upcast<BaseWindow>());
	}
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
Ptr<BaseWindow>
WindowServer::GetWindow(const Util::String & name)
{
    auto i = this->windowByName.FindIndex(name);
    if (i != InvalidIndex)
    {
        return this->windowByName.ValueAtIndex(name, i);
    }

    return nullptr;
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
