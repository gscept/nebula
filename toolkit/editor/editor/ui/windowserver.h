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
#include "toolkit-common/logger.h"

//------------------------------------------------------------------------------
namespace Presentation
{

extern BaseWindow* ConsoleWindow;
extern BaseWindow* OutlineWindow;
extern BaseWindow* HistoryWindow;
extern BaseWindow* StyleEditorWindow;
extern BaseWindow* ToolbarWindow;
extern BaseWindow* EnvironmentWindow;
extern BaseWindow* SceneWindow;
extern BaseWindow* InspectorWindow;
extern BaseWindow* AssetBrowserWindow;
extern BaseWindow* AssetBrowserPickerWindow;
extern BaseWindow* AssetEditorWindow;
extern BaseWindow* ResourceBrowserWindow;
extern BaseWindow* ProfilerWindow;
extern BaseWindow* PhysicsWindow;
extern BaseWindow* NavigationWindow;
extern BaseWindow* SettingsWindow;
extern BaseWindow* TerrainEditorWindow;
extern BaseWindow* BatcherWindow;
extern BaseWindow* ImporterWindow;
class WindowServer : public Core::RefCounted
{   
    __DeclareClass(WindowServer);
    __DeclareSingleton(WindowServer);
public:

    /// Constructor
    WindowServer();

    /// Destructor
    ~WindowServer();

    /// Save state of windows
    void SaveState();
    /// Load state of windows
    void LoadState();

    /// Render all windows
    void RunAll();

    /// update all windows
    void Update();

    /// Broadcast save 
    void BroadcastSave(BaseWindow::SaveMode mode);

    /// register a executable command shotcut. this is also placed in the menu bar tab and category
    /// shortcut is a single, or combination of keys, ex. "Ctrl+S", "A", "Left" "Ctrl+Shift+F10", etc.
    void RegisterCommand(Util::Delegate<void()> func, Util::String const& label, Util::String const& shortcut, const char* menu = NULL, const char* category = NULL);

private:
    void AddCategory(const Util::String& category);

    Util::HashTable<Util::String, BaseWindow*> windowByName;
    Util::Array<Util::String> categories;
    Util::Array<BaseWindow*> windows;
    BaseWindow::SaveMode save;

    struct CommandInfo
    {
        Util::Delegate<void()> func;
        Util::String label;
        Util::String shortcut;
        Util::FixedArray<Input::Key::Code> keys;
        Util::String menu;
        Util::String category;
    };

    ToolkitUtil::Logger logger;
    Util::Dictionary<Util::String, CommandInfo> commands;
};

} // namespace Interface
