#pragma once
//------------------------------------------------------------------------------
/**
    Presentation::WindowServer

    (C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "window.h"
#include "util/string.h"
#include "io/uri.h"
#include "util/delegate.h"
#include "input/key.h"

//------------------------------------------------------------------------------
namespace Presentation
{

class WindowServer : public Core::RefCounted
{   
    __DeclareClass(WindowServer);
    __DeclareSingleton(WindowServer);
public:
    WindowServer();
    ~WindowServer();

    /// Render all windows
    void RunAll();

    /// update all windows
    void Update();

    /// register an interface by class name (RTTI)
    void RegisterWindow(const Util::String& className, const char* label, const char* category = NULL);
    /// register an interface by fourcc
    void RegisterWindow(const Util::FourCC fourcc, const char* label, const char* category = NULL);
    /// register an interface by pointer
    void RegisterWindow(const Ptr<BaseWindow>& base);
    /// register a scripted window
    void RegisterWindowScript(const char* script, const char* label);

    /// register a executable command shotcut. this is also placed in the menu bar tab and category
    /// shortcut is a single, or combination of keys, ex. "Ctrl+S", "A", "Left" "Ctrl+Shift+F10", etc.
    void RegisterCommand(Util::Delegate<void()> func, Util::String const& label, Util::String const& shortcut, const char* menu = NULL, const char* category = NULL);

    /// Get window by name
    Ptr<BaseWindow> GetWindow(const Util::String& name);

private:
    void AddCategory(const Util::String& category);

    Util::HashTable<Util::String, Ptr<BaseWindow>> windowByName;
    Util::Array<Util::String> categories;
    Util::Array<Ptr<BaseWindow>> windows;

    struct CommandInfo
    {
        Util::Delegate<void()> func;
        Util::String label;
        Util::String shortcut;
        Util::FixedArray<Input::Key::Code> keys;
        Util::String menu;
        Util::String category;
    };

    Util::Dictionary<Util::String, CommandInfo> commands;
};

} // namespace Interface
