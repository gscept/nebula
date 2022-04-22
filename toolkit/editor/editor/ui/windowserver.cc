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
#include "pybind11/pybind11.h"
#include "pybind11/embed.h"

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
        if (ImGui::BeginMenu("Window"))
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
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Commands"))
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

        ImGui::EndMainMenuBar();
    }

    //Run all windows
    for (SizeT i = 0; i < this->windows.Size(); i++)
    {
        auto it = this->windows[i];
        
        if (it->Open())
        {
            if (ImGui::Begin(it->GetName().AsCharPtr(), &it->Open(), it->GetAdditionalFlags()))
            {
                it->Run();
            }
            ImGui::End();
        }
    }
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
                    else
                    {
                        exec = false;
                        break;
                    }
                }
            }
        }

        // run command
        if (exec)
            cmd.func();
    }
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
    if (IO::IoServer::Instance()->FileExists(script))
    {
        Ptr<ScriptedWindow> wnd = ScriptedWindow::Create();
        wnd->SetName(label);
        if (wnd->LoadModule(script))
        {
            this->RegisterWindow(wnd.upcast<BaseWindow>());
        }
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
