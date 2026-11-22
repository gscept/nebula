//------------------------------------------------------------------------------
//  @file editorcore.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/core/editorcore.h"
#include "neweditor/commands/addcomponentcommand.h"
#include "neweditor/commands/createentitycommand.h"
#include "neweditor/commands/deleteentitycommand.h"
#include "neweditor/commands/instantiateprefabcommand.h"
#include "neweditor/commands/removecomponentcommand.h"
#include "neweditor/commands/revertcomponentcommand.h"
#include "neweditor/commands/revertrecordcommand.h"
#include "neweditor/commands/setcomponentcommand.h"
#include "neweditor/commands/unpackprefabcommand.h"
#include "neweditor/core/editorpanel.h"
#include "neweditor/widgets/filebrowser.h"
#include "neweditor/widgets/filebrowsermodal.h"
#include "neweditor/widgets/confirmmodal.h"
#include "neweditor/widgets/commandpalette.h"
#include "neweditor/widgets/keybindsmodal.h"
#include "neweditor/base/drawsurface.h"
#include "neweditor/base/iconfont.h"
#include "neweditor/base/iconregistry.h"
#include "neweditor/base/palette.h"
#include "appgame/gameapplication.h"
#include "game/component.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "memdb/attributeregistry.h"

namespace Editor
{

__ImplementClass(Editor::EditorCore, 'ECOR', Shell);

EditorCore* EditorCore::instance = nullptr;

// The dockspace. The id is a constant rather than a hash of a name, because the layout the core
// builds and the
// layout a user saved both name the dockspace node by this id, and a saved layout is only read by a
// program that
// computes the same id
static const ImGuiID EditorDockspaceId = 0x65646974;   // 'edit'

// The toolbar glyphs for the viewport tools, in ToolId order
static const char* const ToolIconNames[NumToolIds] = { "mouse-pointer-2", "move-3-d", "rotate-3-d", "scale-3-d" };

// The popups the two viewport helper buttons open. A popup is named by its id, so these are the
// names
// and not the labels
static const char* GridSettingsPopup = "grid_settings";
static const char* SnapSettingsPopup = "snap_settings";
static const char* PanelsPopup = "panels";

//------------------------------------------------------------------------------
/**
*/
EditorCore::EditorCore() :
    focusedPanel(nullptr),
    activeScene(InvalidIndex),
    pendingClose(InvalidIndex),
    pendingQuit(false),
    quitSaveIndex(InvalidIndex),
    clipboard(nullptr),
    cameraCapture(false),
    opened(false),
    layoutBuilt(false)
{
    // The core a feature reaches is published here rather than at Setup, because a feature
    // contributes at its
    // own attach and asks for the core before anything is opened
    EditorCore::instance = this;
}
//------------------------------------------------------------------------------
/**
*/
EditorCore::~EditorCore()
{
    this->Close();
}

//------------------------------------------------------------------------------
/**
    The style is put on the draw context here, because the look is the editor's and a host that has a
    style of its own applied it before.
*/
void
EditorCore::Setup()
{
    ApplyStyle();

    this->log = EditorLog::Create();

    this->statusBar = StatusBar::Create();

    this->commands = EditorCommandSystem::Create();
    this->commands->Setup(this);

    this->keybinds = EditorKeybinds::Create();
    this->keybinds->Setup(this);

    this->clipboard = Clipboard::Create();
    this->clipboard->Setup(this);

    // The tools run against whichever scene is active, the same way the undo commands do
    this->tools = ToolContext::Create();
    this->tools->Setup(this);

    // The file dialog. It belongs to the shell rather than to a panel, because it is not a window a
    // user
    // docks, and the core draws it after the panels so the popup sits above them
    this->fileBrowserModal = FileBrowserModal::Create();
    this->fileBrowserModal->Setup(this);

    this->confirmModal = ConfirmModal::Create();
    this->confirmModal->Setup(this);

    // The command palette. Like the file dialog it is not a window a user docks, so the core owns
    // it and draws
    // it after the panels. Its command and its keybind are registered by its Setup
    this->commandPalette = CommandPalette::Create();
    this->commandPalette->Setup(this);

    // The keybinds table, the same shape: not a window a user docks, owned by the core, drawn after
    // the panels,
    // opened by a command its Setup registers
    this->keybindsModal = KeybindsModal::Create();
    this->keybindsModal->Setup(this);

    // The undo stacks and the selection are the scene's, so the commands that run them are
    // registered once
    // here and resolved through whichever scene is active. A delegate cannot point at a manager
    // that comes
    // and goes with a tab
    this->commands->Register("undo", "undo the last editor action",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<EditorCore, &EditorCore::HandleUndo>(this));
    this->commands->Register("redo", "redo the last undone editor action",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<EditorCore, &EditorCore::HandleRedo>(this));
    this->commands->Register("copy", "copy the selection to the system clipboard as a scene document",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<EditorCore, &EditorCore::HandleCopy>(this));
    this->commands->Register("paste", "paste the system clipboard into the active scene",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<EditorCore, &EditorCore::HandlePaste>(this));
    this->commands->Register("duplicate", "duplicate the selection",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<EditorCore, &EditorCore::HandleDuplicate>(this));
    this->commands->Register("delete", "delete the selection",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<EditorCore, &EditorCore::HandleDelete>(this));
    this->commands->Register("scene", "scene [new|close|list|prev|next|<name>] - the open scenes",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<EditorCore, &EditorCore::HandleScene>(this));
    this->commands->Register("prefab", "prefab [list|show <instance>|info <entity>|chain <entity>|sources <entity> <component>|markers <entity>|instantiate <path> [name]|unpack [name]|revert <name> [component]|revert-record <instance> <marker>|update <path>|apply [name]] - the scene's prefab instances",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<EditorCore, &EditorCore::HandlePrefab>(this));
    this->commands->Register("set", "set <entity> <component> <field> <value> - write one component field",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<EditorCore, &EditorCore::HandleSetComponent>(this));
    this->commands->Register("remove", "remove <entity> <component> - take a component off an entity",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<EditorCore, &EditorCore::HandleRemoveComponent>(this));
    this->commands->Register("get", "get <entity> <component> [field] - print what a component holds",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<EditorCore, &EditorCore::HandleGetComponent>(this));
    this->commands->Register("keybind", "keybind [list|bind <action> <combo>|unbind <action>|reset [action]|save [path]|load [path]] - the editor shortcuts",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<EditorCore, &EditorCore::HandleKeybind>(this));

    // A bind matches when its modifiers are a subset of the pressed ones, so the more specific
    // ctrl+shift+z
    // has to be found before plain ctrl+z. The registry keeps that order whatever the file says, so
    // the order
    // these are registered in no longer decides it
    this->keybinds->Register("undo", Input::Key::Z, Input::Modifier::ControlModifier, "undo");
    this->keybinds->Register("redo", Input::Key::Z, Input::Modifier::ControlModifier | Input::Modifier::ShiftModifier, "redo");
    this->keybinds->Register("redo.alt", Input::Key::Y, Input::Modifier::ControlModifier, "redo");
    this->keybinds->Register("copy", Input::Key::C, Input::Modifier::ControlModifier, "copy");
    this->keybinds->Register("paste", Input::Key::V, Input::Modifier::ControlModifier, "paste");
    this->keybinds->Register("duplicate", Input::Key::D, Input::Modifier::ControlModifier, "duplicate");
    this->keybinds->Register("delete", Input::Key::Delete, Input::Modifier::NoModifier, "delete");
    this->keybinds->Register("scene.save", Input::Key::S, Input::Modifier::ControlModifier, "scene save");
    this->keybinds->Register("scene.prev", Input::Key::PageUp, Input::Modifier::ControlModifier, "scene prev");
    this->keybinds->Register("scene.next", Input::Key::PageDown, Input::Modifier::ControlModifier, "scene next");
}

//------------------------------------------------------------------------------
/**
    The editor a feature reaches. A build with no editor has no core, and a feature that checks this
    gets null and registers nothing.
*/
EditorCore*
EditorCore::Instance()
{
    return EditorCore::instance;
}

//------------------------------------------------------------------------------
/**
*/
bool
EditorCore::IsBlocked(Game::Entity entity, Game::ComponentId component) const
{
    return EditorFeatureRegistry::IsBlocked(this->GetScene(), entity, component);
}

//------------------------------------------------------------------------------
/**
*/
bool
EditorCore::Open()
{
    // The parts that nothing injected read the shell through the registry
    EditorFeatureRegistry::SetShell(this);

    // The face the glyphs draw with. It needs an ImGui context, which a host has made before the
    // editor opens
    LoadIconFont();

    // The panels the features pushed. Every feature is attached before the editor opens, so this is
    // the point
    // where the editor becomes whatever the game attached, and the order the features came in does
    // not matter
    const Util::Array<Ptr<EditorPanel>>& pushed = EditorFeatureRegistry::GetPanels();
    for (IndexT i = 0; i < pushed.Size(); i++)
        this->panels.Append(pushed[i]);

    // The bar resolves the cvar it reads here, which is the same point at which a panel resolves
    // what it needs
    this->statusBar->Setup(this);

    for (IndexT i = 0; i < this->panels.Size(); i++)
    {
        Ptr<EditorPanel> panel = this->panels[i];
        if (!panel->Setup(this))
            n_warning("Could not setup panel '%s'\n", panel->GetName().AsCharPtr());
    }

    this->opened = true;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
EditorCore::Close()
{
    if (EditorCore::instance == this)
        EditorCore::instance = nullptr;

    // What a feature pushed goes with the feature. A delegate holds the object it was bound to, so
    // a registry
    // that outlived the feature would be a delegate pointing at something that is gone. This
    // happens before the
    // guard, because a feature can push onto a core that was never opened
    EditorFeatureRegistry::Clear();

    if (!this->opened)
        return;

    this->panels.Clear();
    this->actions.Clear();
    this->layoutBuilt = false;

    if (this->tools != nullptr)
    {
        this->tools->Discard();
        this->tools = nullptr;
    }

    if (this->fileBrowserModal != nullptr)
    {
        this->fileBrowserModal->Discard();
        this->fileBrowserModal = nullptr;
    }

    if (this->confirmModal != nullptr)
    {
        this->confirmModal->Discard();
        this->confirmModal = nullptr;
    }

    if (this->commandPalette != nullptr)
    {
        this->commandPalette->Discard();
        this->commandPalette = nullptr;
    }

    if (this->keybindsModal != nullptr)
    {
        this->keybindsModal->Discard();
        this->keybindsModal = nullptr;
    }

    this->pendingClose = InvalidIndex;
    this->pendingQuit = false;
    this->quitSaveIndex = InvalidIndex;
    this->cameraCapture = false;

    for (IndexT i = 0; i < this->scenes.Size(); i++)
        this->scenes[i]->Discard();
    this->scenes.Clear();
    this->activeScene = InvalidIndex;

    if (this->commands != nullptr)
        this->commands->Discard();
    if (this->keybinds != nullptr)
        this->keybinds->Discard();
    if (this->clipboard != nullptr)
        this->clipboard->Discard();

    this->focusedPanel = nullptr;
    this->log = nullptr;
    this->statusBar = nullptr;
    this->opened = false;
}

//------------------------------------------------------------------------------
/**
    The dockspace and the toolbar. The dockspace has to be created before the panels open, so that
    the panels land in it rather than floating over it. The dockspace is opened here rather than
    through DockSpaceOverViewport(), because that fills the whole work area and the status bar is
    not something a panel can be docked against. The id is the one both the built shape and a shape
    a user saved name, so a saved layout is still read.
*/
void
EditorCore::DrawShell()
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float barHeight = this->statusBar->Height();

    // The bar takes the bottom of the work area, so the host keeps the top and only gives up the
    // height the bar
    // takes. Insetting the position as well would move the editor down and leave a gap at the top
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, viewport->WorkSize.y - barHeight), ImGuiCond_Always);
    ImGui::SetNextWindowViewport(viewport->ID);

    const ImGuiWindowFlags hostFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    // The host is the area the panels fill, so it draws without its own frame
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    if (ImGui::Begin("Dockspace", nullptr, hostFlags))
    {
        ImGui::DockSpace(EditorDockspaceId, ImVec2(0.0f, 0.0f));

        // The shape the editor starts in, built once per open. It has to be built after the node
        // has a size,
        // which is what DockSpace gives it, because a split of a node that has no size does not
        // produce a
        // reliable shape. The flag is what keeps it from running again when a user un-splits the
        // tree, because
        // that leaves a node with no shape, which is the condition BuildDefaultLayout answers on
        if (!this->layoutBuilt)
        {
            this->layoutBuilt = true;
            this->BuildDefaultLayout();
        }
    }
    ImGui::End();

    ImGui::PopStyleVar(3);

    // Begin() returns false for a window that is collapsed, or for a docked window whose tab is not
    // the visible
    // one. End() still has to be called for it, so the return value only decides whether the
    // contents are drawn
    const bool visible = ImGui::Begin("Editor");

    if (visible)
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New Scene"))
                    this->ExecuteCommand("scene new");
                // The item that wants a typed path puts the command into the console's input. Save
                // Scene
                // As... asks the file dialog instead, which is the way to pick a path with spaces
                // rather than
                // type it in quotes
                if (ImGui::MenuItem("Open Scene..."))
                    this->SeedConsoleCommand("scene open ");
                if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
                    this->ExecuteCommand("scene save");
                if (ImGui::MenuItem("Save Scene As..."))
                    this->OpenSaveSceneDialog();
                if (ImGui::MenuItem("Close Scene"))
                    this->ExecuteCommand("scene close");
                ImGui::Separator();
                if (ImGui::MenuItem("Exit"))
                    this->ExecuteCommand("exit");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Undo"))
                    this->ExecuteCommand("undo");
                if (ImGui::MenuItem("Redo"))
                    this->ExecuteCommand("redo");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Window"))
            {
                for (IndexT i = 0; i < this->panels.Size(); i++)
                {
                    Ptr<EditorPanel> panel = this->panels[i];
                    if (ImGui::MenuItem(panel->GetName().AsCharPtr(), nullptr, panel->IsOpen()))
                        panel->SetOpen(!panel->IsOpen());
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help"))
            {
                if (ImGui::MenuItem("Commands"))
                    this->ExecuteCommand("help");
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        this->DrawSceneTabs();

        // The toolbar buttons are glyphs, and a name has to be in the generated table

        // The file side first, the way the File menu has it: Save writes the scene's own file, Save
        // As asks
        // the file dialog where to write
        if (IconButton(IconGlyph("save"), "Save scene"))
            this->ExecuteCommand("scene save");
        ImGui::SameLine();
        if (IconButton(IconGlyph("save-all"), "Save scene as"))
            this->OpenSaveSceneDialog();

        ImGui::SameLine();

        if (IconButton(IconGlyph("undo-2"), "Undo"))
            this->ExecuteCommand("undo");
        ImGui::SameLine();
        if (IconButton(IconGlyph("redo-2"), "Redo"))
            this->ExecuteCommand("redo");

        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        if (IconButton(IconGlyph("play"), "Play"))
            this->ExecuteCommand("pie start");
        ImGui::SameLine();
        if (IconButton(IconGlyph("square"), "Stop"))
            this->ExecuteCommand("pie stop");

        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        // The viewport tools. The active one carries the accent color, the same way the view mode
        // row marks its mode
        for (IndexT i = 0; i < NumToolIds; i++)
        {
            const char* glyph = IconGlyph(ToolIconNames[i]);
            if (glyph == nullptr)
                continue;

            const bool active = this->tools->GetActiveTool() == (ToolId)i;
            if (active)
                ImGui::PushStyleColor(ImGuiCol_Text, ColorFromHex(PaletteAccent));

            if (IconButton(glyph, this->tools->ToolName((ToolId)i)))
                this->tools->SetActiveTool((ToolId)i);

            if (active)
                ImGui::PopStyleColor();

            if (i < NumToolIds - 1)
                ImGui::SameLine();
        }

        // The frame the tools work in. It is a dropdown rather than a button because a frame is a
        // choice
        // between more than two things, and it belongs with the tools it decides the basis for
        ImGui::SameLine();
        this->DrawFrameDropdown();

        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        // The canvas' helpers last: the grid the scene is measured against and the lattice a drag
        // lands
        // on. Each is a toggle with the settings it uses in a popup beside it, so both states read
        // off
        // the bar without anything having to be opened
        this->DrawGridButton();
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        this->DrawSnapButton();

        ImGui::SameLine();

        const char* panelsGlyph = IconGlyph("layout-panel-left");
        const char* paletteGlyph = IconGlyph("square-terminal");
        const char* keybindGlyph = IconGlyph("keyboard");

        if (panelsGlyph != nullptr && paletteGlyph != nullptr && keybindGlyph != nullptr)
        {
            // An icon button costs the glyph plus the frame padding on one side, and the group is put
            // at the right end of the bar by what the three measure. The last button has the window
            // padding after it
            const ImGuiStyle& style = ImGui::GetStyle();
            const float groupWidth = ImGui::CalcTextSize(panelsGlyph).x + ImGui::CalcTextSize(paletteGlyph).x +
                ImGui::CalcTextSize(keybindGlyph).x + 3 * style.FramePadding.x + 2 * style.ItemSpacing.x +
                style.WindowPadding.x + style.FramePadding.x;

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - groupWidth);

            // The glyph is what the button is drawn as, so each is id by what it opens rather than
            // by the glyph
            ImGui::PushID("palette");
            if (IconButton(paletteGlyph, "Command palette"))
                this->ExecuteCommand("palette");
            ImGui::PopID();

            ImGui::SameLine();

            ImGui::PushID("keybinds");
            if (IconButton(keybindGlyph, "Keybinds"))
                this->ExecuteCommand("keybinds");
            ImGui::PopID();

            ImGui::SameLine();

            ImGui::PushID("panels");
            this->DrawPanelsButton(panelsGlyph);
            ImGui::PopID();
        }
    }

    ImGui::End();
}

//------------------------------------------------------------------------------
/**
    What a tab calls a scene: the file's name without its extension when the scene has a file, and
    the scene's own name while it hasn't got one. A scene saved somewhere moves its tab to the new
    file's name on its own, since this is read from the file path each frame rather than stored.
*/
static Util::String
TabTitle(const Ptr<Scene>& scene)
{
    const Util::String& path = scene->GetFilePath();
    if (path.IsEmpty())
        return scene->GetName();

    // The file's base name: the assign off first, since a file right at an assign root,
    // 'work:thescene.json', has no slash for ExtractFileName to cut on
    Util::String name = path.ExtractFileName();
    name.StripAssignPrefix();
    name.StripFileExtension();
    return name.IsEmpty() ? scene->GetName() : name;
}

//------------------------------------------------------------------------------
/**
    The shape the editor starts in when the user has no saved layout.

    It is built from the panel list rather than from a shape written for a particular set of panels,
    because a game that takes a panel out or adds one has a shape the editor cannot know in advance.
    The rule the panel list alone can answer with is the order the panels were registered in: the
    first is the main area and the ones after it are stacked on the right, each with an equal share.

    A layout a user moved is theirs and is read from the file, so this only runs when the layout
    that was read has no shape in it. A node that came from a saved layout has children, which is
    what tells the two apart. The caller runs it once per open, so a user who un-splits the tree
    does not get the default shape back.
*/
void
EditorCore::BuildDefaultLayout()
{
    if (this->panels.IsEmpty())
        return;

    ImGuiDockNode* dockspace = ImGui::DockBuilderGetNode(EditorDockspaceId);
    if (dockspace == nullptr)
        return;

    if (dockspace->IsSplitNode())
        return;

    ImGuiID main = EditorDockspaceId;
    ImGuiID column = 0;

    // A quarter is what keeps the main panel the one with room, and the stack wide enough to read
    // once it has
    // more than one row in it
    if (this->panels.Size() > 1)
        ImGui::DockBuilderSplitNode(EditorDockspaceId, ImGuiDir_Right, 0.25f, &column, &main);

    for (IndexT i = 0; i < this->panels.Size(); i++)
    {
        const char* name = this->panels[i]->GetName().AsCharPtr();

        if (column == 0)
        {
            ImGui::DockBuilderDockWindow(name, main);
            continue;
        }

        // An equal share of what is left, which is what makes the stack even whatever the count is.
        // The last
        // panel takes the rest of the column, so it is docked to it directly rather than split off
        // it
        const IndexT remaining = this->panels.Size() - i;
        if (remaining == 1)
        {
            ImGui::DockBuilderDockWindow(name, column);
            break;
        }

        ImGuiID slot = 0;
        ImGui::DockBuilderSplitNode(column, ImGuiDir_Up, 1.0f / float(remaining), &slot, &column);
        ImGui::DockBuilderDockWindow(name, slot);
    }

    ImGui::DockBuilderFinish(EditorDockspaceId);
}

//------------------------------------------------------------------------------
/**
    The tabs. One per open scene, in the order they were added. The core draws them rather than a
    panel, because the scene a tab selects is not something a panel owns: every panel draws the
    active scene, so the bar that picks it has to sit above all of them. The tab bar is the drawing,
    the model is the core's. ImGuiTabItemFlags_SetSelected is what makes the tab that is drawn the
    one the core has active, so ImGui cannot pick a scene on its own, and a scene that is renamed or
    reordered by the core shows up the way the core has it. Each tab is an ID scope, because a tab
    id is the name the tab was opened with and two scenes with the same name would otherwise be the
    same tab. What a click changes is applied after the bar has been closed, because a close takes a
    scene out of the array the loop is still walking, and the tab that is active is the one that was
    active when it was drawn.
*/
void
EditorCore::DrawSceneTabs()
{
    if (!ImGui::BeginTabBar("scene_tabs"))
        return;

    IndexT clicked = InvalidIndex;
    IndexT closed = InvalidIndex;

    for (IndexT i = 0; i < this->scenes.Size(); i++)
    {
        ImGui::PushID(i);

        const Ptr<Scene> scene = this->scenes[i];

        const ImGuiTabItemFlags flags = (i == this->activeScene) ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;

        // The close button is ImGui's, and p_open is how it reports the press. Only the press is
        // read here, the
        // scene itself is removed after the bar has been closed
        bool open = true;

        // The dirty mark rides the tab title rather than the model name, so commands still name
        // scenes
        // the way they always have and only what is read off the bar changes
        const Util::String title = TabTitle(scene) + (scene->IsDirty() ? "*" : "");
        const bool selected = ImGui::BeginTabItem(title.AsCharPtr(), &open, flags);

        if (ImGui::IsItemClicked())
            clicked = i;

        // Two scenes may be called the same thing, so the tab says which file it is when the title
        // had to hide one
        const Util::String& path = scene->GetFilePath();
        ImGui::SetItemTooltip("%s", path.IsEmpty() ? scene->GetName().AsCharPtr() : path.AsCharPtr());

        if (selected)
            ImGui::EndTabItem();

        if (!open)
            closed = i;

        ImGui::PopID();
    }

    // The new scene button is a tab bar item rather than a tab, because it is not a scene and
    // cannot be selected
    const char* plus = IconGlyph("plus");
    const bool create = plus != nullptr && ImGui::TabItemButton(plus);
    if (plus != nullptr)
        ImGui::SetItemTooltip("new scene");

    ImGui::EndTabBar();

    if (create)
        this->AddScene("Scene");

    if (clicked != InvalidIndex)
        this->SetActiveScene(clicked);

    if (closed != InvalidIndex)
        this->RequestCloseScene(closed);
}

//------------------------------------------------------------------------------
/**
*/
void
EditorCore::DrawFrameDropdown()
{
    const Ptr<ToolContext> tools = this->tools;

    ImGui::PushID("frame");

    const char* glyph = IconGlyph("axis-3-d");

    float widest = 0.0f;
    for (IndexT i = 0; i < NumFrames; i++)
    {
        const float width = ImGui::CalcTextSize(tools->FrameName(i)).x + 25;
        if (width > widest)
            widest = width;
    }

    ImGui::SetNextItemWidth((glyph != nullptr ? ImGui::CalcTextSize(glyph).x : 0.0f) + widest + ImGui::GetFrameHeight());

    Util::String label;
    if (glyph != nullptr)
        label.Format("%s %s", glyph, tools->FrameName(tools->GetFrame()));
    else
        label = tools->FrameName(tools->GetFrame());

    const bool open = ImGui::BeginCombo("##frame", label.AsCharPtr());

    ImGui::SetItemTooltip("the basis the gizmo is drawn and dragged in (G)");

    if (open)
    {
        for (IndexT i = 0; i < NumFrames; i++)
        {
            const bool current = tools->GetFrame() == i;

            if (current)
                ImGui::PushStyleColor(ImGuiCol_Text, ColorFromHex(PaletteAccent));

            if (ImGui::Selectable(tools->FrameName(i), current))
                tools->SetFrame(i);

            if (current)
            {
                ImGui::PopStyleColor();
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
    The grid button and the settings it opens. The button is the grid's visibility, and the accent
    color is what says the grid is drawing, the same way it marks the active tool.
*/
void
EditorCore::DrawGridButton()
{
    const Ptr<ToolContext> tools = this->tools;

    // The style color is pushed and popped off what the visibility was when the button was drawn,
    // not
    // off what it is after the click: the click turns the grid over, and re-reading it in between
    // would
    // leave the pop behind on exactly the frames the button was pressed
    const bool visible = tools->GetGrid().visible;

    ImGui::PushID("grid");

    if (visible)
        ImGui::PushStyleColor(ImGuiCol_Text, ColorFromHex(PaletteAccent));

    if (IconButton(IconGlyph("grid-3-x-3"), "Grid (Shift+G)"))
        tools->ToggleGrid();

    if (visible)
        ImGui::PopStyleColor();

    ImGui::SameLine();
    if (IconButton(IconGlyph("sliders-horizontal"), "Grid settings"))
        ImGui::OpenPopup(GridSettingsPopup);

    if (ImGui::BeginPopup(GridSettingsPopup))
    {
        GridSettings grid = tools->GetGrid();
        bool edited = false;

        edited |= ImGui::Checkbox("Show grid", &grid.visible);

        edited |= ImGui::DragFloat("Step", &grid.step, 0.05f, 0.0f, 100.0f, "%.2f");
        ImGui::SetItemTooltip("the distance between the major lines");

        edited |= ImGui::DragInt("Subdivisions", &grid.subdivisions, 1.0f, 1, 10);
        ImGui::SetItemTooltip("how many times the step is cut up between the major lines");

        edited |= ImGui::DragFloat("Extent", &grid.extent, 1.0f, 0.0f, 1000.0f, "%.0f");
        ImGui::SetItemTooltip("how far the grid reaches out from the origin, in world units");

        // The colors carry their alpha, and the minor lattice gets its own because a grid wants its
        // measure lines to sit back from the major ones. The bar beside each picker is what makes
        // the
        // transparency something you can set and see, not just a component in the row
        edited |= ImGui::ColorEdit4("Major color", &grid.majorColor.x, ImGuiColorEditFlags_AlphaBar);
        ImGui::SetItemTooltip("the color of the major lines, with its transparency");

        edited |= ImGui::ColorEdit4("Minor color", &grid.minorColor.x, ImGuiColorEditFlags_AlphaBar);
        ImGui::SetItemTooltip("the color of the subdivision lines, set apart from the major ones");

        if (edited)
            tools->SetGrid(grid);

        // The snapping to the grid lands on the minor lines, so the grid says what it currently
        // means to
        // the snapping rather than leaving that to be worked out from the two settings above
        ImGui::Separator();
        Util::String note;
        note.Format("Minor step %.2f", tools->GridMinorStep());
        ImGui::TextDisabled("%s", note.AsCharPtr());

        ImGui::EndPopup();
    }

    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
void
EditorCore::DrawSnapButton()
{
    const Ptr<ToolContext> tools = this->tools;
    const bool snapping = tools->SnapsToGrid() || tools->SnapsToIncrements();

    ImGui::PushID("snap");

    if (snapping)
        ImGui::PushStyleColor(ImGuiCol_Text, ColorFromHex(PaletteAccent));

    if (IconButton(IconGlyph("magnet"), "Snap (X), ctrl inverts it during a drag"))
        tools->ToggleSnapping();

    if (snapping)
        ImGui::PopStyleColor();

    ImGui::SameLine();
    if (IconButton(IconGlyph("sliders-horizontal"), "Snap settings"))
        ImGui::OpenPopup(SnapSettingsPopup);

    if (ImGui::BeginPopup(SnapSettingsPopup))
    {
        SnapSettings snap = tools->GetSnapping();
        bool edited = false;

        edited |= ImGui::Checkbox("Snap to grid", &snap.toGrid);
        ImGui::SetItemTooltip("the position lands on the grid's minor lines");

        edited |= ImGui::Checkbox("Snap to increments", &snap.toIncrements);
        ImGui::SetItemTooltip("each operation lands on the increment below it");

        ImGui::Separator();

        edited |= ImGui::DragFloat("Translate", &snap.translate, 0.02f, 0.0f, 1000.0f, "%.2f");
        ImGui::SetItemTooltip("how far a drag moves the height, in world units. The grid decides the other two axes");

        edited |= ImGui::DragFloat("Rotate", &snap.rotate, 0.5f, 0.0f, 360.0f, "%.1f");
        ImGui::SetItemTooltip("the turn a drag makes, in degrees");

        edited |= ImGui::DragFloat("Scale", &snap.scale, 0.01f, 0.0f, 10.0f, "%.2f");
        ImGui::SetItemTooltip("the stretch a drag makes, as a ratio, so 0.05 steps through 1.05 and 1.10");

        if (edited)
            tools->SetSnapping(snap);

        ImGui::Separator();
        Util::String note;
        note.Format("%s, on a %.2f grid", tools->SnapSourceName(), tools->GridMinorStep());
        ImGui::TextDisabled("%s", note.AsCharPtr());
        ImGui::TextDisabled("ctrl held through a drag turns the two sources the other way");

        ImGui::EndPopup();
    }

    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
    The panels button and the dropdown it opens. The rows are the core's panel list rather than a set
    written for a particular game, because a game can add a panel at its own attach and the core
    cannot know which ones it has. Each row is that panel's window, and the tick is whether the core
    draws it this frame.

    Closing a row does not take the panel out of the layout, it only stops the core from drawing it,
    which is the same thing the Window menu does, and both read the same flag, so the two agree.
*/
void
EditorCore::DrawPanelsButton(const char* glyph)
{
    if (IconButton(glyph, "Panels"))
        ImGui::OpenPopup(PanelsPopup);

    if (!ImGui::BeginPopup(PanelsPopup))
        return;

    for (IndexT i = 0; i < this->panels.Size(); i++)
    {
        Ptr<EditorPanel> panel = this->panels[i];

        // The value is read into a local and written back when the row is ticked, because the panel
        // keeps its own open flag and there is nothing here to write through
        bool open = panel->IsOpen();
        if (ImGui::Checkbox(panel->GetName().AsCharPtr(), &open))
            panel->SetOpen(open);
    }

    ImGui::EndPopup();
}

//------------------------------------------------------------------------------
/**
*/
void
EditorCore::Draw()
{
    if (!this->opened)
        return;

    this->DrawShell();

    EditorPanel* focused = nullptr;
    for (IndexT i = 0; i < this->panels.Size(); i++)
    {
        Ptr<EditorPanel> panel = this->panels[i];

        const bool wasOpen = panel->IsOpen();
        const bool visible = panel->BeginWindow();

        if (visible)
        {
            panel->Draw();

            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
                focused = panel;
        }

        if (wasOpen)
            panel->EndWindow();
    }

    // The file dialog draws after the panels so its popup ends up above them, and before the status
    // bar so a
    // message the dialog makes this frame shows this frame
    if (this->fileBrowserModal != nullptr)
        this->fileBrowserModal->Draw();

    if (this->confirmModal != nullptr)
        this->confirmModal->Draw();

    // The palette draws with the other modals, so that its popup sits above the panels it is opened
    // from
    if (this->commandPalette != nullptr)
        this->commandPalette->Draw();

    if (this->keybindsModal != nullptr)
        this->keybindsModal->Draw();

    this->SetFocusedPanel(focused);

    // The bar draws after the panels, so a message a panel put on it during this frame shows on
    // this frame
    this->statusBar->Draw();

    this->FlushActions();
}

//------------------------------------------------------------------------------
/**
    The actions are run after every panel has drawn. A panel that changed the model while it was
    drawing would change the arrays it is still walking, so the change is deferred to here.
*/
void
EditorCore::FlushActions()
{
    if (this->actions.IsEmpty())
        return;

    // The array is copied before the actions run, because an action can queue another one
    Util::Array<Util::Delegate<void()>> pending = this->actions;
    this->actions.Clear();

    for (IndexT i = 0; i < pending.Size(); i++)
    {
        if (pending[i].IsValid())
            pending[i]();
    }
}

//------------------------------------------------------------------------------
/**
    The status bar is view state rather than model state, so a panel can put a message on it while
    it draws.
*/
void
EditorCore::SetStatus(const Util::String& msg, float duration)
{
    this->statusBar->SetStatus(msg, duration);
}

//------------------------------------------------------------------------------
/**
*/
Ptr<StatusBar>
EditorCore::GetStatusBar() const
{
    return this->statusBar;
}

//------------------------------------------------------------------------------
/**
*/
Ptr<ConfirmModal>
EditorCore::GetConfirmModal() const
{
    return this->confirmModal;
}

//------------------------------------------------------------------------------
/**
*/
void
EditorCore::QueueAction(Util::Delegate<void()> action)
{
    if (!action.IsValid())
        return;
    this->actions.Append(action);
}

//------------------------------------------------------------------------------
/**
    The key the host did not take for a text field. The host gives it through the shell.
*/
bool
EditorCore::OnKey(Input::Key::Code key, bool isPress, Input::Modifier::Code mods)
{
    if (this->keybinds == nullptr)
        return false;

    return this->keybinds->Process(key, isPress, mods);
}

//------------------------------------------------------------------------------
/**
*/
EditorPanel*
EditorCore::GetPanel(const char* name) const
{
    for (IndexT i = 0; i < this->panels.Size(); i++)
    {
        if (this->panels[i]->GetName() == name)
            return this->panels[i];
    }
    return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
Ptr<EditorLog>
EditorCore::GetLog() const
{
    return this->log;
}

//------------------------------------------------------------------------------
/**
    The scene is created here rather than by the feature unit, because the core is what owns the tabs.
*/
IndexT
EditorCore::AddScene(const Util::String& name)
{
    // The scene that was showing is no longer the one the sim world holds. This is also the first
    // scene, where
    // there is nothing to deactivate
    if (!this->scenes.IsEmpty())
        this->scenes[this->activeScene]->Deactivate();

    Ptr<Scene> scene = Scene::Create();
    scene->Setup(this, name);

    this->scenes.Append(scene);
    this->activeScene = (IndexT)this->scenes.Size() - 1;
    scene->Activate();

    this->Log(LogMessageType::N_SYSTEM, Util::String("Opened ") + name);
    return this->activeScene;
}

//------------------------------------------------------------------------------
/**
    Closing a tab takes the scene and its history with it. The tab next to it becomes the showing
    one.
*/
void
EditorCore::RemoveScene(IndexT index)
{
    if (index == InvalidIndex || index >= (IndexT)this->scenes.Size())
        return;

    Ptr<Scene> scene = this->scenes[index];
    const bool wasActive = index == this->activeScene;
    this->scenes.EraseIndex(index);

    if (wasActive)
        this->activeScene = this->scenes.IsEmpty() ? InvalidIndex : (index < (IndexT)this->scenes.Size() ? index : (IndexT)this->scenes.Size() - 1);
    else if (index < this->activeScene)
        this->activeScene--;

    this->Log(LogMessageType::N_SYSTEM, Util::String("Closed ") + scene->GetName());

    scene->Discard();

    if (this->scenes.IsEmpty())
    {
        // The editor always has a scene to draw. A panel resolves the scene through a Ptr and
        // converts it to a
        // pointer, which asserts on an empty one, so closing the last tab opens a new scene rather
        // than leaving
        // the panels with nothing
        this->AddScene("Scene");
        return;
    }

    // Only the tab that was showing has to be copied into the sim world again
    if (wasActive)
        this->scenes[this->activeScene]->Activate();
}

//------------------------------------------------------------------------------
/**
    The sim world is shared, so showing a scene means copying it over the scene that was showing.
    Sim state does not survive the switch, the authored state, the history and the selection do.
*/
void
EditorCore::SetActiveScene(IndexT index)
{
    if (index == InvalidIndex || index >= (IndexT)this->scenes.Size())
        return;

    if (index == this->activeScene)
        return;

    // A drag that is still running previews into the sim world this tab is about to replace, so it
    // is put back rather than left to write into the wrong scene
    this->tools->AbortActiveTool();

    this->GetScene()->Deactivate();

    this->activeScene = index;
    this->scenes[index]->Activate();
}

//------------------------------------------------------------------------------
/**
*/
IndexT
EditorCore::GetActiveSceneIndex() const
{
    return this->activeScene;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Ptr<Scene>>&
EditorCore::GetScenes() const
{
    return this->scenes;
}

//------------------------------------------------------------------------------
/**
*/
Ptr<Scene>
EditorCore::GetScene() const
{
    // The core always has at least one scene, which is what makes this a pointer a panel can use
    // without
    // checking it. A Ptr asserts when it is converted to a pointer, so a check here would never be
    // reached
    n_assert(this->activeScene != InvalidIndex && this->activeScene < (IndexT)this->scenes.Size());

    return this->scenes[this->activeScene];
}

//------------------------------------------------------------------------------
/**
*/
Ptr<SelectionContext>
EditorCore::GetSelection() const
{
    return this->GetScene()->GetSelection();
}

//------------------------------------------------------------------------------
/**
*/
Ptr<ToolContext>
EditorCore::GetTools() const
{
    return this->tools;
}

//------------------------------------------------------------------------------
/**
*/
Ptr<EditCommandManager>
EditorCore::GetCommandManager() const
{
    return this->GetScene()->GetCommandManager();
}

//------------------------------------------------------------------------------
/**
*/
Ptr<Clipboard>
EditorCore::GetClipboard() const
{
    return this->clipboard;
}

//------------------------------------------------------------------------------
/**
*/
IndexT
EditorCore::FindScene(const Util::String& name) const
{
    for (IndexT i = 0; i < this->scenes.Size(); i++)
    {
        if (this->scenes[i]->GetName() == name)
            return i;
    }
    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
EditorPanel*
EditorCore::GetFocusedPanel() const
{
    return this->focusedPanel;
}

//------------------------------------------------------------------------------
/**
*/
void
EditorCore::SetFocusedPanel(EditorPanel* panel)
{
    if (panel == this->focusedPanel)
        return;

    if (this->focusedPanel != nullptr)
        this->focusedPanel->OnFocus(false);
    this->focusedPanel = panel;
    if (panel != nullptr)
        panel->OnFocus(true);
}

//------------------------------------------------------------------------------
/**
*/
EditorCommandSystem&
EditorCore::GetCommands()
{
    return *this->commands;
}

//------------------------------------------------------------------------------
/**
*/
EditorKeybinds&
EditorCore::GetKeybinds()
{
    return *this->keybinds;
}

//------------------------------------------------------------------------------
/**
*/
bool
EditorCore::IsCameraCapturing() const
{
    return this->cameraCapture;
}

//------------------------------------------------------------------------------
/**
*/
void
EditorCore::SetCameraCapture(bool on)
{
    this->cameraCapture = on;
}

//------------------------------------------------------------------------------
/**
*/
bool
EditorCore::ExecuteCommand(const Util::String& command)
{
    return this->commands->Execute(command);
}

//------------------------------------------------------------------------------
/**
    The panels that have no command entry answer with nothing, which is what makes this a call to
    the console rather than a lookup of it.
*/
void
EditorCore::SeedConsoleCommand(const Util::String& command)
{
    for (IndexT i = 0; i < this->panels.Size(); i++)
        this->panels[i]->SeedCommand(command);
}

//------------------------------------------------------------------------------
/**
    The scene that wrote the file has what it wrote already. The other tabs that instance the file
    are the ones that have to answer to it.
*/
void
EditorCore::PrefabFileWritten(const Util::String& path, Scene* from)
{
    for (IndexT i = 0; i < this->scenes.Size(); i++)
    {
        if (this->scenes[i].get() != from)
            this->scenes[i]->UpdatePrefabInstances(path);
    }
}

//------------------------------------------------------------------------------
/**
    Without an argument the command prints the tabs, which is what makes it usable from a command
    script. A word that is not one of the verbs is read as the name of an open scene.
*/
void
EditorCore::HandleScene(const Util::Array<Util::String>& args)
{
    if (args.IsEmpty() || args[0] == "list")
    {
        for (IndexT i = 0; i < this->scenes.Size(); i++)
        {
            Util::String line;
            line.Format("%s%s (%d entities)", i == this->activeScene ? "* " : "  ",
                this->scenes[i]->GetName().AsCharPtr(), (int)this->scenes[i]->NumEntities());
            this->Log(LogMessageType::N_SYSTEM, line);
        }
        return;
    }

    const Util::String& verb = args[0];

    if (verb == "new")
    {
        this->AddScene(args.Size() > 1 ? args[1] : Util::String("Scene"));
        return;
    }

    if (verb == "open")
    {
        if (args.Size() < 2)
        {
            this->Log(LogMessageType::N_WARNING, "scene open expects a path, like 'scene open work:levels/test.json'");
            return;
        }

        this->OpenSceneFile(args[1]);
        return;
    }

    if (verb == "save" || verb == "save-as")
    {
        const Util::String path = args.Size() > 1 ? args[1] : this->GetScene()->GetFilePath();
        if (path.IsEmpty())
        {
            // A scene without a file has nothing to save into, so the save turns into a Save As and
            // asks where to write
            this->OpenSaveSceneDialog();
            return;
        }

        this->SaveSceneFile(path);
        return;
    }

    if (verb == "close")
    {
        const IndexT index = args.Size() > 1 ? this->FindScene(args[1]) : this->activeScene;
        if (index == InvalidIndex)
        {
            this->Log(LogMessageType::N_WARNING, "no open scene with that name");
            return;
        }
        this->RequestCloseScene(index);
        return;
    }

    if (verb == "prev" || verb == "next")
    {
        if (this->scenes.IsEmpty())
            return;

        IndexT index = this->activeScene;
        if (verb == "prev")
            index = index <= 0 ? (IndexT)this->scenes.Size() - 1 : index - 1;
        else
            index = index + 1 >= (IndexT)this->scenes.Size() ? 0 : index + 1;

        this->SetActiveScene(index);
        return;
    }

    const IndexT index = this->FindScene(verb);
    if (index == InvalidIndex)
    {
        this->Log(LogMessageType::N_WARNING, "scene is one of: new, close, list, prev, next, or the name of an open scene");
        return;
    }

    this->SetActiveScene(index);
}

//------------------------------------------------------------------------------
/**
    The body of 'scene open' taken out of the command, so that the file dialog and the command reach
    it the same way. The dialog hands the path over as a string, and the command line carries one in
    quotes.
*/
void
EditorCore::OpenSceneFile(const Util::String& path)
{
    // A tab named for the file it holds. The scene reads the file into itself, which is what
    // a scene without one would otherwise have to do by hand
    Util::String name = path;
    name.ExtractFileName();
    name.StripFileExtension();

    const IndexT index = this->AddScene(name);
    if (!this->scenes[index]->Load(path))
    {
        // the reason the file was refused is already in the log by now: the read itself and
        // the prefab chain check both say what was missing. This is the line that says the
        // file is not coming into the editor
        this->Log(LogMessageType::N_ERROR, "Not opening " + path + ", the file is unreadable or broken");
        this->RemoveScene(index);
        return;
    }

    // The loaded state is what the sim world shows
    this->scenes[index]->Activate();
    this->Log(LogMessageType::N_SYSTEM, "Opened " + path);
}

//------------------------------------------------------------------------------
/**
    A file that already has a tab goes to that tab rather than reading it a second time, which is
    what keeps two tabs off the same file. The comparison is the whole assign-form path, so the same
    file opened through different routes, a resolved path against 'work:', is not recognized as one
    file.
*/
void
EditorCore::OpenOrFocusSceneFile(const Util::String& path)
{
    for (IndexT i = 0; i < this->scenes.Size(); i++)
    {
        if (this->scenes[i]->GetFilePath() == path)
        {
            this->SetActiveScene(i);
            return;
        }
    }

    this->OpenSceneFile(path);
}

//------------------------------------------------------------------------------
/**
    The body of 'scene save-as', out of the command for the same reason as the open side. False when
    the file could not be written, which is what tells a caller that was closing the tab to keep it
    open.
*/
bool
EditorCore::SaveSceneFile(const Util::String& path)
{
    return this->SaveSceneFile(this->activeScene, path);
}

//------------------------------------------------------------------------------
/**
    The same write for a chosen tab. A close or an exit can be about a scene that is not the active
    one, and it is that scene's work that has to reach the file.
*/
bool
EditorCore::SaveSceneFile(IndexT index, const Util::String& path)
{
    if (index == InvalidIndex || index >= (IndexT)this->scenes.Size())
        return false;

    const Ptr<Scene> scene = this->scenes[index];

    if (!scene->Save(path))
    {
        this->Log(LogMessageType::N_WARNING, "could not write " + path);
        return false;
    }

    // A saved scene file is what reaches the instances of it, wherever they are open
    this->PrefabFileWritten(path, scene.get());

    this->Log(LogMessageType::N_SYSTEM, "Saved " + path);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
EditorCore::OpenSaveSceneDialog()
{
    this->OpenSaveSceneDialogFor(this->activeScene,
        Util::Delegate<void(const Util::String&)>::FromMethod<EditorCore, &EditorCore::HandleSaveSceneFile>(this));
}

//------------------------------------------------------------------------------
/**
*/
void
EditorCore::HandleSaveSceneFile(const Util::String& path)
{
    this->SaveSceneFile(path);
}

//------------------------------------------------------------------------------
/**
    The dialog starts at the folder the scene's file is in and offers the scene's name as the file.
    A scene that has no file yet starts at 'work:', the folder the authored files live in. What an
    accepted save means and what a refused one means is the caller's business, so both callbacks are
    the caller's.
*/
void
EditorCore::OpenSaveSceneDialogFor(IndexT index, Util::Delegate<void(const Util::String&)> onAccepted,
    Util::Delegate<void()> onCanceled)
{
    if (index == InvalidIndex || index >= (IndexT)this->scenes.Size())
        return;

    const Ptr<Scene> scene = this->scenes[index];

    Util::String startDir = "work:";
    const Util::String& filePath = scene->GetFilePath();
    if (!filePath.IsEmpty())
        startDir = FileBrowser::ParentDir(filePath);

    this->fileBrowserModal->ShowSave("Save scene as", startDir, TabTitle(scene) + ".json", "*.json", onAccepted, onCanceled);
}

//------------------------------------------------------------------------------
/**
    The gate every close goes through. A scene with unsaved work is asked about first.
*/
void
EditorCore::RequestCloseScene(IndexT index)
{
    if (index == InvalidIndex || index >= (IndexT)this->scenes.Size())
        return;

    const Ptr<Scene> scene = this->scenes[index];
    if (scene->IsDirty())
    {
        this->pendingClose = index;
        this->confirmModal->Show("Close scene",
            TabTitle(scene) + " has changes that are not saved. Save before closing?",
            "Save", "Don't Save",
            Util::Delegate<void()>::FromMethod<EditorCore, &EditorCore::ConfirmCloseSave>(this),
            Util::Delegate<void()>::FromMethod<EditorCore, &EditorCore::ConfirmCloseSkip>(this),
            Util::Delegate<void()>::FromMethod<EditorCore, &EditorCore::CloseDismissed>(this));
        return;
    }

    this->RemoveScene(index);
}

//------------------------------------------------------------------------------
/**
    'Save' on the question. A scene with a file writes that file and goes with it written. A scene
    that never had one is the Save As case again, so the Save As dialog opens for it and the tab
    waits for the written file's body. A write that fails keeps the tab, which is the whole point of
    the question.
*/
void
EditorCore::ConfirmCloseSave()
{
    const IndexT index = this->pendingClose;
    this->pendingClose = InvalidIndex;
    if (index == InvalidIndex || index >= (IndexT)this->scenes.Size())
        return;

    const Util::String& path = this->scenes[index]->GetFilePath();
    if (!path.IsEmpty())
    {
        if (this->SaveSceneFile(index, path))
            this->RemoveScene(index);
        return;
    }

    this->pendingClose = index;
    this->OpenSaveSceneDialogFor(index,
        Util::Delegate<void(const Util::String&)>::FromMethod<EditorCore, &EditorCore::SaveAsThenClose>(this));
}

//------------------------------------------------------------------------------
/**
    'Don't Save' on the question: the work goes and the tab goes with it.
*/
void
EditorCore::ConfirmCloseSkip()
{
    const IndexT index = this->pendingClose;
    this->pendingClose = InvalidIndex;
    if (index == InvalidIndex || index >= (IndexT)this->scenes.Size())
        return;

    this->RemoveScene(index);
}

//------------------------------------------------------------------------------
/**
    The close question put away without an answer: nothing was agreed to, so the tab stays and the
    question is simply over.
*/
void
EditorCore::CloseDismissed()
{
    this->pendingClose = InvalidIndex;
}

//------------------------------------------------------------------------------
/**
    The Save As callback for a close that needed a file first: the save answers through the same
    body the command uses, and the close goes on top of the saved state.
*/
void
EditorCore::SaveAsThenClose(const Util::String& path)
{
    const IndexT index = this->pendingClose;
    this->pendingClose = InvalidIndex;
    if (index == InvalidIndex || index >= (IndexT)this->scenes.Size())
        return;

    if (!this->SaveSceneFile(index, path))
        return;

    this->RemoveScene(index);
}

//------------------------------------------------------------------------------
/**
    The exit request: the 'exit' command, the File menu, and the window's X all come through here.
    Unsaved work is asked about first. Asking twice in a row does not raise a second question while
    the first one is still up or still saving its way out.
*/
void
EditorCore::RequestQuit(bool force)
{
    if (force)
    {
        this->QuitNow();
        return;
    }

    if (this->pendingQuit)
        return;

    IndexT dirty = 0;
    for (IndexT i = 0; i < this->scenes.Size(); i++)
    {
        if (this->scenes[i]->IsDirty())
            dirty++;
    }

    if (dirty == 0)
    {
        this->QuitNow();
        return;
    }

    Util::String question;
    if (dirty == 1)
        question = "An open scene has changes that are not saved. Save before exiting?";
    else
        question.Format("%d open scenes have changes that are not saved. Save them before exiting?", (int)dirty);

    this->pendingQuit = true;
    this->confirmModal->Show("Exit editor", question, "Save", "Don't Save",
        Util::Delegate<void()>::FromMethod<EditorCore, &EditorCore::ConfirmQuitSave>(this),
        Util::Delegate<void()>::FromMethod<EditorCore, &EditorCore::ConfirmQuitSkip>(this),
        Util::Delegate<void()>::FromMethod<EditorCore, &EditorCore::QuitDismissed>(this));
}

//------------------------------------------------------------------------------
/**
    'Save' on the exit question: walk the tabs and write every dirty one. A scene with a file is
    written where it lives; a scene without one is the Save As case, and the walk stops at its
    dialog until the path is given. A write that fails ends the walk with the editor still open,
    because an exit that throws work away after being told to keep it is the one thing the question
    was asked to prevent.
*/
void
EditorCore::ConfirmQuitSave()
{
    for (IndexT i = 0; i < this->scenes.Size(); i++)
    {
        const Ptr<Scene> scene = this->scenes[i];
        if (!scene->IsDirty())
            continue;

        const Util::String& path = scene->GetFilePath();
        if (!path.IsEmpty())
        {
            if (!this->SaveSceneFile(i, path))
            {
                this->pendingQuit = false;
                return;
            }
            continue;
        }

        this->quitSaveIndex = i;
        this->OpenSaveSceneDialogFor(i,
            Util::Delegate<void(const Util::String&)>::FromMethod<EditorCore, &EditorCore::QuitSaveAsCurrent>(this),
            Util::Delegate<void()>::FromMethod<EditorCore, &EditorCore::QuitSaveAborted>(this));
        return;
    }

    this->pendingQuit = false;
    this->QuitNow();
}

//------------------------------------------------------------------------------
/**
    'Don't Save' on the exit question: the work goes and the editor goes with it.
*/
void
EditorCore::ConfirmQuitSkip()
{
    this->pendingQuit = false;
    this->quitSaveIndex = InvalidIndex;
    this->QuitNow();
}

//------------------------------------------------------------------------------
/**
    The exit question put away without an answer: the request goes back to waiting. This is what
    keeps the re-entry guard from swallowing every later X and Alt-F4 after one dismissed question.
*/
void
EditorCore::QuitDismissed()
{
    this->pendingQuit = false;
    this->quitSaveIndex = InvalidIndex;
}

//------------------------------------------------------------------------------
/**
    A path was given for a dirty scene without a file: write it and pick the walk back up where it
    stopped. A write that fails is an unanswered question again, so the exit stops with it.
*/
void
EditorCore::QuitSaveAsCurrent(const Util::String& path)
{
    const IndexT index = this->quitSaveIndex;
    this->quitSaveIndex = InvalidIndex;
    if (!this->pendingQuit || index == InvalidIndex)
        return;

    if (!this->SaveSceneFile(index, path))
    {
        this->pendingQuit = false;
        return;
    }

    this->ConfirmQuitSave();
}

//------------------------------------------------------------------------------
/**
    The Save As dialog closed without an answer during the exit walk: the exit was agreed to a save
    that was never given, so the whole exit goes back to waiting and the editor stays open.
*/
void
EditorCore::QuitSaveAborted()
{
    this->pendingQuit = false;
    this->quitSaveIndex = InvalidIndex;
}

//------------------------------------------------------------------------------
/**
    End the app. The game loop reads the input server's quit flag as its own, so the paths that ask
    the question clear that flag when they take the request over; this is what sets the app's own
    flag, the one the loop cannot be talked out of.
*/
void
EditorCore::QuitNow()
{
    this->Log(LogMessageType::N_SYSTEM, "Exiting");
    App::GameApplication::Instance()->SetQuitRequested(true);
}

//------------------------------------------------------------------------------
/**
    The prefab instances of the active scene, and the actions that work on them. An entity is taken
    from the name when one is given, and from the selection when it is not, which is the way the
    outline's own commands read it.
*/
void
EditorCore::HandlePrefab(const Util::Array<Util::String>& args)
{
    const Ptr<Scene> scene = this->GetScene();

    if (args.IsEmpty() || args[0] == "list")
    {
        const Util::Array<PrefabInstance>& instances = scene->GetPrefabInstances();
        IndexT shown = 0;
        for (IndexT i = 0; i < instances.Size(); i++)
        {
            if (!instances[i].valid)
                continue;

            Util::String line;
            line.Format("%d: %s (%d entities)", (int)i, instances[i].source.AsCharPtr(),
                (int)instances[i].entities.Size());
            this->Log(LogMessageType::N_SYSTEM, line);
            shown++;
        }
        if (0 == shown)
            this->Log(LogMessageType::N_SYSTEM, "The scene has no prefab instances");
        return;
    }

    const Util::String& verb = args[0];

    if (verb == "instantiate")
    {
        if (args.Size() < 2)
        {
            this->Log(LogMessageType::N_WARNING, "prefab instantiate expects a path, like 'prefab instantiate work:prefabs/enemy.json'");
            return;
        }

        Ptr<InstantiatePrefabCommand> command = InstantiatePrefabCommand::Create();
        command->scene = scene;
        command->path = args[1];
        if (args.Size() > 2)
            command->rootName = args[2];

        // A file that cannot be read, or one whose own prefab chains do not resolve, says
        // so through the scene's log, and the command is left on the stack as one that
        // made nothing. The line about instantiating only happens when something was made
        this->GetCommandManager()->Execute(command);
        if (!command->created.IsEmpty())
            this->Log(LogMessageType::N_SYSTEM, "Instantiated " + args[1]);
        return;
    }

    if (verb == "unpack")
    {
        Game::Entity entity = Game::Entity::Invalid();
        if (args.Size() > 1)
            entity = this->EntityNamed(args[1]);
        else
        {
            const Util::Array<Game::Entity>& selection = this->GetSelection()->GetSelection();
            if (!selection.IsEmpty())
                entity = selection[0];
        }

        if (entity == Game::Entity::Invalid())
        {
            this->Log(LogMessageType::N_WARNING, "prefab unpack takes the name of an instance entity, or uses the selection");
            return;
        }
        if (!scene->IsPrefabInstanceEntity(entity))
        {
            this->Log(LogMessageType::N_WARNING, "that entity is not part of a prefab instance");
            return;
        }

        Ptr<UnpackPrefabCommand> command = UnpackPrefabCommand::Create();
        command->scene = scene;
        command->entity = entity;
        this->GetCommandManager()->Execute(command);

        this->Log(LogMessageType::N_SYSTEM, "Unpacked the prefab instance of " + scene->GetEditable(entity).name);
        return;
    }

    if (verb == "revert")
    {
        // The last word is the component name when it names one, and the entity name is the words
        // before
        // it, because an entity name can hold a space and the command line splits on it. Without a
        // component the revert is the whole entity, which is what the panel's Reset does
        Game::ComponentId component = Game::ComponentId::Invalid();
        if (args.Size() > 1)
            component = Game::GetComponentId(args[args.Size() - 1]);

        Util::String entityName;
        const IndexT nameEnd = component == Game::ComponentId::Invalid() ? args.Size() : args.Size() - 1;
        for (IndexT i = 1; i < nameEnd; i++)
        {
            if (i > 1)
                entityName += " ";
            entityName += args[i];
        }

        Game::Entity entity = Game::Entity::Invalid();
        if (!entityName.IsEmpty())
            entity = this->EntityNamed(entityName);
        else
        {
            const Util::Array<Game::Entity>& selection = this->GetSelection()->GetSelection();
            if (!selection.IsEmpty())
                entity = selection[0];
        }

        if (entity == Game::Entity::Invalid())
        {
            this->Log(LogMessageType::N_WARNING, "prefab revert takes the name of an instance entity, or uses the selection");
            return;
        }
        if (!scene->IsPrefabInstanceEntity(entity))
        {
            this->Log(LogMessageType::N_WARNING, "that entity is not part of a prefab instance");
            return;
        }

        if (component == Game::ComponentId::Invalid())
        {
            // The whole entity. The managed properties are not part of this, because the command
            // that
            // reverts one needs the extension's own resolve, and that is the C# side
            const Editable& editable = scene->GetEditable(entity);

            Util::Array<Game::ComponentId> entries;
            for (IndexT i = 0; i < editable.prefabOverrides.Size(); i++)
            {
                if (editable.prefabOverrides[i].component != Game::ComponentId::Invalid())
                    entries.Append(editable.prefabOverrides[i].component);
            }

            if (entries.IsEmpty())
            {
                this->Log(LogMessageType::N_WARNING, "that entity has nothing authored over its chain");
                return;
            }

            // The entries are read whole before the commands run, because a command writes the
            // marker the
            // list is read from
            Ptr<EditCommandManager> manager = this->GetCommandManager();
            manager->BeginMacro("Revert entity");
            for (IndexT i = 0; i < entries.Size(); i++)
            {
                Ptr<RevertComponentCommand> command = RevertComponentCommand::Create();
                command->scene = scene;
                command->entity = entity;
                command->component = entries[i];
                manager->Execute(command);
            }
            manager->EndMacro();

            this->Log(LogMessageType::N_SYSTEM, "Reverted " + editable.name);
            return;
        }

        Ptr<RevertComponentCommand> command = RevertComponentCommand::Create();
        command->scene = scene;
        command->entity = entity;
        command->component = component;
        this->GetCommandManager()->Execute(command);

        this->Log(LogMessageType::N_SYSTEM,
            Util::String("Reverted ") + args[args.Size() - 1] + " on " + scene->GetEditable(entity).name);
        return;
    }

    if (verb == "apply")
    {
        Game::Entity entity = Game::Entity::Invalid();
        if (args.Size() > 1)
            entity = this->EntityNamed(args[1]);
        else
        {
            const Util::Array<Game::Entity>& selection = this->GetSelection()->GetSelection();
            if (!selection.IsEmpty())
                entity = selection[0];
        }

        if (entity == Game::Entity::Invalid())
        {
            this->Log(LogMessageType::N_WARNING, "prefab apply takes the name of an instance entity, or uses the selection");
            return;
        }
        if (!scene->IsPrefabInstanceEntity(entity))
        {
            this->Log(LogMessageType::N_WARNING, "that entity is not part of a prefab instance");
            return;
        }

        // The file write is what the apply is, and the open scenes that instance the file
        // re-merge against it the way they do when the prefab itself is saved. The write is
        // not an undoable scene action: the file it changes is not scene state
        if (!scene->ApplyInstanceToPrefab(entity))
        {
            this->Log(LogMessageType::N_WARNING, "could not write " + scene->GetPrefabInstance(entity).source);
            return;
        }

        const Util::String source = scene->GetPrefabInstance(entity).source;
        this->PrefabFileWritten(source, scene.get());

        this->Log(LogMessageType::N_SYSTEM, "Applied " + scene->GetEditable(entity).name + " to " + source);
        return;
    }

    if (verb == "show")
    {
        if (args.Size() < 2 || !args[1].IsValidInt())
        {
            this->Log(LogMessageType::N_WARNING, "prefab show expects the instance index from 'prefab list'");
            return;
        }

        const IndexT index = args[1].AsInt();
        const Util::Array<PrefabInstance>& instances = scene->GetPrefabInstances();
        if (index < 0 || index >= (IndexT)instances.Size())
        {
            this->Log(LogMessageType::N_WARNING, "no prefab instance with that index");
            return;
        }

        const PrefabInstance& instance = instances[index];

        Util::String line;
        line.Format("%d: %s", (int)index, instance.source.AsCharPtr());
        this->Log(LogMessageType::N_SYSTEM, line);

        // The record an entity is read from is the first row of its chain, which is the instance's
        // own
        // file. An entity with no chain has no record, which is what makes it the instance's own
        // addition
        // The instance's set is what its tree holds, so an entity a command keeps for a redo is not
        // shown. The marker is what says the instance differs from the file for that entity
        for (IndexT i = 0; i < instance.entities.Size(); i++)
        {
            if (!scene->HasInstance(instance.entities[i]))
                continue;

            Util::Array<ChainRecord> chain;
            scene->GetPrefabChain(instance.entities[i], chain);

            const Editable& editable = scene->GetEditable(instance.entities[i]);
            Util::String row;
            row.Format("  %s -> %s", editable.name.AsCharPtr(),
                chain.IsEmpty() ? "added" : chain[0].record.AsCharPtr());
            this->Log(LogMessageType::N_SYSTEM, row);
        }

        for (IndexT i = 0; i < instance.removedRecords.Size(); i++)
        {
            Util::String row;
            row.Format("  removed %d: %s", (int)i, instance.removedRecords[i].name.AsCharPtr());
            this->Log(LogMessageType::N_SYSTEM, row);
        }
        return;
    }

    if (verb == "info")
    {
        const Game::Entity entity = this->EntityFromArgs(args);
        if (entity == Game::Entity::Invalid())
        {
            this->Log(LogMessageType::N_WARNING, "prefab info takes the name of an instance entity, or uses the selection");
            return;
        }

        const Editable& editable = scene->GetEditable(entity);
        if (editable.prefabInstance == InvalidIndex)
        {
            Util::String line;
            line.Format("%s: not part of a prefab instance", editable.name.AsCharPtr());
            this->Log(LogMessageType::N_SYSTEM, line);
            return;
        }

        const PrefabInstance& instance = scene->GetPrefabInstances()[editable.prefabInstance];

        Util::Array<ChainRecord> chain;
        scene->GetPrefabChain(entity, chain);

        Util::String line;
        // An entity with no record is the instance's own addition, which is the same thing the
        // chain
        // says, so the line says it once
        line.Format("%s: instance %d, %s, record %s%s", editable.name.AsCharPtr(), (int)editable.prefabInstance,
            instance.source.AsCharPtr(), chain.IsEmpty() ? "added" : chain[0].record.AsCharPtr(),
            scene->IsPrefabInstanceRoot(entity) ? ", root" : "");
        this->Log(LogMessageType::N_SYSTEM, line);
        return;
    }

    if (verb == "chain")
    {
        const Game::Entity entity = this->EntityFromArgs(args);
        if (entity == Game::Entity::Invalid())
        {
            this->Log(LogMessageType::N_WARNING, "prefab chain takes the name of an instance entity, or uses the selection");
            return;
        }

        Util::Array<ChainRecord> chain;
        scene->GetPrefabChain(entity, chain);
        if (chain.IsEmpty())
        {
            this->Log(LogMessageType::N_SYSTEM, "that entity has no chain");
            return;
        }

        for (IndexT i = 0; i < chain.Size(); i++)
        {
            Util::String line;
            line.Format("%s / %s", chain[i].source.AsCharPtr(), chain[i].record.AsCharPtr());
            this->Log(LogMessageType::N_SYSTEM, line);
        }
        return;
    }

    if (verb == "sources")
    {
        if (args.Size() < 3)
        {
            this->Log(LogMessageType::N_WARNING, "prefab sources expects an entity and a component");
            return;
        }

        const Game::Entity entity = this->EntityNamed(args[1]);
        const Game::ComponentId component = Game::GetComponentId(args[2]);
        if (component == Game::ComponentId::Invalid())
        {
            this->Log(LogMessageType::N_WARNING, Util::String("No component named ") + args[2]);
            return;
        }

        Game::ComponentInterface* interface =
            static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(component));
        if (interface == nullptr)
        {
            this->Log(LogMessageType::N_WARNING, Util::String("No component interface for ") + args[2]);
            return;
        }

        Util::Array<ChainRecord> sources;
        scene->GetFieldSources(entity, component, sources);

        for (IndexT field = 0; field < sources.Size(); field++)
        {
            Util::String source = sources[field].source;
            if (!sources[field].record.IsEmpty())
                source += " / " + sources[field].record;

            Util::String line;
            line.Format("%s.%s = %s", interface->GetName(), interface->GetFieldNames()[field],
                source.IsEmpty() ? "this scene" : source.AsCharPtr());
            this->Log(LogMessageType::N_SYSTEM, line);
        }
        return;
    }

    if (verb == "markers")
    {
        const Game::Entity entity = this->EntityFromArgs(args);
        if (entity == Game::Entity::Invalid())
        {
            this->Log(LogMessageType::N_WARNING, "prefab markers takes the name of an instance entity, or uses the selection");
            return;
        }

        const Editable& editable = scene->GetEditable(entity);
        if (editable.prefabOverrides.IsEmpty())
        {
            Util::String line;
            line.Format("%s: nothing authored over its chain", editable.name.AsCharPtr());
            this->Log(LogMessageType::N_SYSTEM, line);
            return;
        }

        for (IndexT i = 0; i < editable.prefabOverrides.Size(); i++)
        {
            const PrefabOverride& state = editable.prefabOverrides[i];

            // An entry with nothing in it is what a comparison leaves behind when nothing differed,
            // so
            // there is nothing to say. This is the same rule the inspector's rows use
            bool saysSomething = state.componentAdded || state.componentRemoved;

            Game::ComponentInterface* interface = nullptr;
            if (state.component != Game::ComponentId::Invalid())
            {
                interface = static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(state.component));
                if (interface == nullptr)
                    continue;

                for (IndexT field = 0; field < (IndexT)interface->GetNumFields(); field++)
                {
                    if (state.overriddenFields.IsSet((uint64_t)field))
                        saysSomething = true;
                }
            }
            else if (!state.fields.IsEmpty())
            {
                saysSomething = true;
            }

            if (!saysSomething)
                continue;

            Util::String line;
            if (state.component != Game::ComponentId::Invalid())
            {
                Game::ComponentInterface* interface =
                    static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(state.component));
                if (interface == nullptr)
                    continue;

                line.Format("%s:", interface->GetName());
                if (state.componentAdded)
                    line += " added";
                if (state.componentRemoved)
                    line += " removed";

                for (IndexT field = 0; field < (IndexT)interface->GetNumFields(); field++)
                {
                    if (!state.overriddenFields.IsSet((uint64_t)field))
                        continue;
                    line += " ";
                    line += interface->GetFieldNames()[field];
                }
            }
            else
            {
                line.Format("%s:", state.extension.Value());
                for (IndexT f = 0; f < state.fields.Size(); f++)
                    line += " " + state.fields[f];
            }

            this->Log(LogMessageType::N_SYSTEM, line);
        }
        return;
    }

    if (verb == "update")
    {
        if (args.Size() < 2)
        {
            this->Log(LogMessageType::N_WARNING, "prefab update expects the path of a prefab file");
            return;
        }

        scene->UpdatePrefabInstances(args[1]);
        return;
    }

    if (verb == "revert-record")
    {
        if (args.Size() < 3 || !args[1].IsValidInt() || !args[2].IsValidInt())
        {
            this->Log(LogMessageType::N_WARNING,
                "prefab revert-record expects the instance index and the marker index from 'prefab show'");
            return;
        }

        const IndexT index = args[1].AsInt();
        const IndexT marker = args[2].AsInt();
        const Util::Array<PrefabInstance>& instances = scene->GetPrefabInstances();
        if (index < 0 || index >= (IndexT)instances.Size())
        {
            this->Log(LogMessageType::N_WARNING, "no prefab instance with that index");
            return;
        }

        const PrefabInstance& instance = instances[index];
        if (marker < 0 || marker >= (IndexT)instance.removedRecords.Size())
        {
            this->Log(LogMessageType::N_WARNING, "that instance has no marker with that index");
            return;
        }

        Ptr<RevertRecordCommand> command = RevertRecordCommand::Create();
        command->scene = scene;
        command->instanceIndex = index;
        command->record = instance.removedRecords[marker].record;
        this->GetCommandManager()->Execute(command);

        this->Log(LogMessageType::N_SYSTEM, "Reverted " + instance.removedRecords[marker].name + " from " + instance.source);
        return;
    }

    this->Log(LogMessageType::N_WARNING, "prefab is one of: list, show <instance>, info <entity>, chain <entity>, sources <entity> <component>, markers <entity>, instantiate <path> [name], unpack [name], revert <name> [component], revert-record <instance> <marker>, update <path>, apply [name]");
}

//------------------------------------------------------------------------------
/**
    Names are what the console finds entities by, which is the same way the outline's select command
    reads them. A released record is not an entity of the scene, so it is skipped.
*/
Game::Entity
EditorCore::EntityNamed(const Util::String& name) const
{
    const Scene* scene = this->GetScene();
    const Util::Array<Editable>& editables = scene->GetEditables();
    for (IndexT i = 0; i < editables.Size(); i++)
    {
        if (editables[i].guid.IsValid() && editables[i].name == name && scene->HasInstance(editables[i].entity))
            return editables[i].entity;
    }
    return Game::Entity::Invalid();
}

//------------------------------------------------------------------------------
/**
    The entity a prefab query names. A name is the first word after the verb, and the selection is
    what the query falls back to, which is what lets a script select once and then ask about the
    same entity several times.
*/
Game::Entity
EditorCore::EntityFromArgs(const Util::Array<Util::String>& args) const
{
    if (args.Size() > 1)
        return this->EntityNamed(args[1]);

    const Util::Array<Game::Entity>& selection = this->GetSelection()->GetSelection();
    if (!selection.IsEmpty())
        return selection[0];

    return Game::Entity::Invalid();
}

//------------------------------------------------------------------------------
/**
    One component field read from the command line. The value is written the way the inspector shows
    it: a number for the scalars, x,y,z for the vectors, and the name of an entity for an entity
    reference, which the caller resolves because only the scene knows what an entity is called.
*/
static bool
ParseFieldValue(const char* typeName, const Util::String& text, void* field)
{
    const Util::StringAtom type(typeName);

    if (type == "bool"_atm)
    {
        if (!text.IsValidBool())
            return false;
        *(bool*)field = text.AsBool();
    }
    else if (type == "int"_atm || type == "int32_t"_atm)
    {
        if (!text.IsValidInt())
            return false;
        *(int*)field = text.AsInt();
    }
    else if (type == "uint"_atm || type == "uint32_t"_atm)
    {
        if (!text.IsValidInt())
            return false;
        *(unsigned int*)field = (unsigned int)text.AsInt();
    }
    else if (type == "int16_t"_atm)
    {
        if (!text.IsValidInt())
            return false;
        *(int16_t*)field = (int16_t)text.AsInt();
    }
    else if (type == "uint16_t"_atm)
    {
        if (!text.IsValidInt())
            return false;
        *(uint16_t*)field = (uint16_t)text.AsInt();
    }
    else if (type == "uint64_t"_atm)
    {
        if (!text.IsValidInt())
            return false;
        *(uint64_t*)field = (uint64_t)text.AsInt();
    }
    else if (type == "float"_atm)
    {
        if (!text.IsValidFloat())
            return false;
        *(float*)field = text.AsFloat();
    }
    else if (type == "Math::vec2"_atm)
    {
        const Util::Array<Util::String> tokens = text.Tokenize(",");
        if (tokens.Size() != 2)
            return false;
        *(Math::vec2*)field = Math::vec2(tokens[0].AsFloat(), tokens[1].AsFloat());
    }
    else if (type == "Math::vec3"_atm)
    {
        const Util::Array<Util::String> tokens = text.Tokenize(",");
        if (tokens.Size() != 3)
            return false;
        *(Math::vec3*)field = Math::vec3(tokens[0].AsFloat(), tokens[1].AsFloat(), tokens[2].AsFloat());
    }
    else if (type == "Math::vec4"_atm)
    {
        const Util::Array<Util::String> tokens = text.Tokenize(",");
        if (tokens.Size() != 4)
            return false;
        *(Math::vec4*)field = Math::vec4(tokens[0].AsFloat(), tokens[1].AsFloat(), tokens[2].AsFloat(), tokens[3].AsFloat());
    }
    else if (type == "Math::quat"_atm)
    {
        const Util::Array<Util::String> tokens = text.Tokenize(",");
        if (tokens.Size() != 4)
            return false;
        *(Math::quat*)field = Math::quat(tokens[0].AsFloat(), tokens[1].AsFloat(), tokens[2].AsFloat(), tokens[3].AsFloat());
    }
    else if (type == "Util::StringAtom"_atm)
    {
        *(Util::StringAtom*)field = Util::StringAtom(text.AsCharPtr());
    }
    else
        return false;

    return true;
}

//------------------------------------------------------------------------------
/**
    The other direction: a component field as text, which is what makes a value that a prefab gave
    an entity readable from a command script.
*/
static Util::String
FieldValueText(const char* typeName, const void* field)
{
    const Util::StringAtom type(typeName);
    Util::String out;

    if (type == "bool"_atm)
        out = *(const bool*)field ? "true" : "false";
    else if (type == "int"_atm || type == "int32_t"_atm)
        out.Format("%d", *(const int*)field);
    else if (type == "uint"_atm || type == "uint32_t"_atm)
        out.Format("%u", *(const unsigned int*)field);
    else if (type == "int16_t"_atm)
        out.Format("%d", (int)*(const int16_t*)field);
    else if (type == "uint16_t"_atm)
        out.Format("%u", (unsigned int)*(const uint16_t*)field);
    else if (type == "uint64_t"_atm)
        out.Format("%llu", (unsigned long long)*(const uint64_t*)field);
    else if (type == "float"_atm)
        out.Format("%g", *(const float*)field);
    else if (type == "Math::vec2"_atm)
    {
        const Math::vec2 v = *(const Math::vec2*)field;
        out.Format("%g, %g", v.x, v.y);
    }
    else if (type == "Math::vec3"_atm)
    {
        const Math::vec3 v = *(const Math::vec3*)field;
        out.Format("%g, %g, %g", v.x, v.y, v.z);
    }
    else if (type == "Math::vec4"_atm)
    {
        const Math::vec4 v = *(const Math::vec4*)field;
        out.Format("%g, %g, %g, %g", v.x, v.y, v.z, v.w);
    }
    else if (type == "Math::quat"_atm)
    {
        const Math::quat v = *(const Math::quat*)field;
        out.Format("%g, %g, %g, %g", v.x, v.y, v.z, v.w);
    }
    else if (type == "Util::StringAtom"_atm)
        out = ((const Util::StringAtom*)field)->Value();
    else if (type == "Game::Entity"_atm)
    {
        const Game::Entity entity = *(const Game::Entity*)field;
        if (entity == Game::Entity::Invalid())
            out = "-";
        else
            out.Format("%d", (int)entity.index);
    }
    else
        out = "?";

    return out;
}

//------------------------------------------------------------------------------
/**
    set <entity> <component> <field> <value> - writes one field and leaves the rest of the component
    alone, which is the same as changing one widget in the inspector.
*/
void
EditorCore::HandleSetComponent(const Util::Array<Util::String>& args)
{
    if (args.Size() < 4)
    {
        this->Log(LogMessageType::N_WARNING, "set expects an entity, a component, a field and a value");
        return;
    }

    const Ptr<Scene> scene = this->GetScene();

    const Game::ComponentId component = Game::GetComponentId(args[1]);
    if (component == Game::ComponentId::Invalid())
    {
        this->Log(LogMessageType::N_WARNING, Util::String("No component named ") + args[1]);
        return;
    }

    const Game::Entity entity = this->EntityNamed(args[0]);
    if (entity == Game::Entity::Invalid())
    {
        this->Log(LogMessageType::N_WARNING, Util::String("No entity named ") + args[0]);
        return;
    }

    Game::ComponentInterface* interface =
        static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(component));
    const SizeT typeSize = MemDb::AttributeRegistry::TypeSize(component);

    IndexT field = InvalidIndex;
    for (IndexT i = 0; i < (IndexT)interface->GetNumFields(); i++)
    {
        if (args[2] == interface->GetFieldNames()[i])
        {
            field = i;
            break;
        }
    }
    if (field == InvalidIndex)
    {
        this->Log(LogMessageType::N_WARNING, Util::String(args[1]) + " has no field named " + args[2]);
        return;
    }

    // A component the entity does not have is given to it with the field written into the
    // default value, which is what lets one command line both give an entity a component and
    // say what it should hold
    const bool adding = !scene->GetAuthoringWorld()->HasComponent(entity, component);

    Util::Blob value;
    if (adding)
    {
        if (typeSize > 0)
            value = Util::Blob(MemDb::AttributeRegistry::DefaultValue(component), typeSize);
    }
    else
        value = Util::Blob(scene->GetComponentData(entity, component), typeSize);

    if (typeSize == 0)
    {
        this->Log(LogMessageType::N_WARNING, Util::String(args[1]) + " has no fields to write");
        return;
    }

    byte* at = (byte*)value.GetPtr() + interface->GetFieldByteOffsets()[field];

    if (Util::StringAtom(interface->GetFieldTypenames()[field]) == "Game::Entity"_atm)
    {
        const Game::Entity target = this->EntityNamed(args[3]);
        if (target == Game::Entity::Invalid())
        {
            this->Log(LogMessageType::N_WARNING, Util::String("No entity named ") + args[3]);
            return;
        }
        Memory::Copy(&target, at, sizeof(Game::Entity));
    }
    else if (!ParseFieldValue(interface->GetFieldTypenames()[field], args[3], at))
    {
        this->Log(LogMessageType::N_WARNING,
            Util::String("'") + args[3] + "' is not a value " + args[2] + " can hold");
        return;
    }

    if (adding)
    {
        Ptr<AddComponentCommand> add = AddComponentCommand::Create();
        add->scene = scene;
        add->entity = entity;
        add->component = component;
        add->value = value;
        this->GetCommandManager()->Execute(add);
        return;
    }

    Ptr<SetComponentCommand> command = SetComponentCommand::Create();
    command->scene = scene;
    command->entity = entity;
    command->component = component;
    command->newValue = value;
    this->GetCommandManager()->Execute(command);
}

//------------------------------------------------------------------------------
/**
    remove <entity> <component> - the component goes, which is the same action the inspector's row
    runs with a button. For an instance entity this is a change against its chain, and the marker is
    what says whether the chain had it.
*/
void
EditorCore::HandleRemoveComponent(const Util::Array<Util::String>& args)
{
    if (args.Size() < 2)
    {
        this->Log(LogMessageType::N_WARNING, "remove expects an entity and a component");
        return;
    }

    const Ptr<Scene> scene = this->GetScene();

    const Game::ComponentId component = Game::GetComponentId(args[1]);
    if (component == Game::ComponentId::Invalid())
    {
        this->Log(LogMessageType::N_WARNING, Util::String("No component named ") + args[1]);
        return;
    }

    const Game::Entity entity = this->EntityNamed(args[0]);
    if (entity == Game::Entity::Invalid())
    {
        this->Log(LogMessageType::N_WARNING, Util::String("No entity named ") + args[0]);
        return;
    }

    // The fixed columns of every entity table cannot come off an entity, so the command says so
    // rather
    // than doing nothing
    const Game::ComponentId ownerId = Game::GetComponentId<Game::Entity>();
    if (component == ownerId || scene->IsTransformComponent(component))
    {
        this->Log(LogMessageType::N_WARNING,
            Util::String(args[1]) + " is a fixed column of every entity table, so it cannot be removed");
        return;
    }

    Ptr<RemoveComponentCommand> command = RemoveComponentCommand::Create();
    command->scene = scene;
    command->entity = entity;
    command->component = component;
    this->GetCommandManager()->Execute(command);
}

//------------------------------------------------------------------------------
/**
    get <entity> <component> [field] - the component's fields and what they hold, which is what lets
    a command script check what a prefab gave an instance.
*/
void
EditorCore::HandleGetComponent(const Util::Array<Util::String>& args)
{
    if (args.Size() < 2)
    {
        this->Log(LogMessageType::N_WARNING, "get expects an entity and a component, and optionally a field");
        return;
    }

    const Ptr<Scene> scene = this->GetScene();

    const Game::ComponentId component = Game::GetComponentId(args[1]);
    if (component == Game::ComponentId::Invalid())
    {
        this->Log(LogMessageType::N_WARNING, Util::String("No component named ") + args[1]);
        return;
    }

    const Game::Entity entity = this->EntityNamed(args[0]);
    if (entity == Game::Entity::Invalid())
    {
        this->Log(LogMessageType::N_WARNING, Util::String("No entity named ") + args[0]);
        return;
    }
    if (!scene->GetAuthoringWorld()->HasComponent(entity, component))
    {
        this->Log(LogMessageType::N_WARNING, Util::String(args[0]) + " does not have " + args[1]);
        return;
    }

    Game::ComponentInterface* interface =
        static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(component));
    const byte* value = (const byte*)scene->GetComponentData(entity, component);

    for (IndexT i = 0; i < (IndexT)interface->GetNumFields(); i++)
    {
        if (args.Size() > 2 && args[2] != interface->GetFieldNames()[i])
            continue;

        const void* at = value + interface->GetFieldByteOffsets()[i];
        Util::String text;

        // An entity reference is worth more as the name it points at than as an index, which
        // is what makes a prefab's hierarchy readable from a command script
        if (Util::StringAtom(interface->GetFieldTypenames()[i]) == "Game::Entity"_atm)
        {
            const Game::Entity target = *(const Game::Entity*)at;
            if (target == Game::Entity::Invalid())
                text = "-";
            else if (target.index < scene->GetEditables().Size() && scene->GetEditables()[target.index].guid.IsValid())
                text = scene->GetEditables()[target.index].name;
            else
                text.Format("%d", (int)target.index);
        }
        else
            text = FieldValueText(interface->GetFieldTypenames()[i], at);

        Util::String line;
        line.Format("%s.%s = %s", args[1].AsCharPtr(), interface->GetFieldNames()[i], text.AsCharPtr());
        this->Log(LogMessageType::N_SYSTEM, line);
    }
}

//------------------------------------------------------------------------------
/**
    The stacks are the active scene's, so a tab that is not showing cannot be undone from here.
*/
void
EditorCore::HandleUndo(const Util::Array<Util::String>& args)
{
    (void)args;

    if (!this->GetCommandManager()->Undo())
        this->Log(LogMessageType::N_WARNING, "Nothing to undo");
}

//------------------------------------------------------------------------------
/**
*/
void
EditorCore::HandleRedo(const Util::Array<Util::String>& args)
{
    (void)args;

    if (!this->GetCommandManager()->Redo())
        this->Log(LogMessageType::N_WARNING, "Nothing to redo");
}

//------------------------------------------------------------------------------
/**
    The selection is the active scene's, the same way the undo stacks are. A copy is the selection
    plus the subtree under each of it, written as a scene document
*/
void
EditorCore::HandleCopy(const Util::Array<Util::String>& args)
{
    (void)args;

    if (!this->clipboard->Copy(this->GetSelection()->GetSelection()))
        this->Log(LogMessageType::N_WARNING, "Nothing to copy");
}

//------------------------------------------------------------------------------
/**
*/
void
EditorCore::HandlePaste(const Util::Array<Util::String>& args)
{
    (void)args;

    Ptr<SceneDocument> doc = this->clipboard->Read();
    if (!doc.isvalid())
    {
        this->Log(LogMessageType::N_WARNING, "Clipboard does not hold a scene document");
        return;
    }

    if (!this->clipboard->Paste(doc, this->GetScene()->GetActiveCollection(), Game::Entity::Invalid(), "Paste"))
        this->Log(LogMessageType::N_WARNING, "Nothing pasted");
}

//------------------------------------------------------------------------------
/**
*/
void
EditorCore::HandleDuplicate(const Util::Array<Util::String>& args)
{
    (void)args;

    if (!this->clipboard->Duplicate(this->GetSelection()->GetSelection(),
        this->GetScene()->GetActiveCollection(), Game::Entity::Invalid()))
        this->Log(LogMessageType::N_WARNING, "Nothing to duplicate");
}

//------------------------------------------------------------------------------
/**
*/
void
EditorCore::HandleDelete(const Util::Array<Util::String>& args)
{
    (void)args;

    if (!this->DeleteEntities(this->GetSelection()->GetSelection()))
        this->Log(LogMessageType::N_WARNING, "Nothing to delete");
}

//------------------------------------------------------------------------------
/**
    The description the command registry has for the first word of a command line. A keybind runs a
    command line rather than a command name, so the word is what a description can be looked up for.
    Empty when the line starts with something the registry does not have.
*/
Util::String
EditorCore::CommandDescription(const Util::String& line)
{
    Util::Array<Util::String> tokens;
    line.Tokenize(" ", tokens);
    if (tokens.IsEmpty())
        return Util::String();

    const EditorCommandSystem& commands = this->GetCommands();
    for (IndexT i = 0; i < commands.GetCommands().Size(); i++)
    {
        if (commands.GetCommands()[i].name == tokens[0])
            return commands.GetCommands()[i].description;
    }

    return Util::String();
}

//------------------------------------------------------------------------------
/**
    The keybinds, and the rebinding.

    The action is the name the file uses, so what is typed here is what the file holds, and a bind
    changed here is written back straight away rather than kept for the session.
*/
void
EditorCore::HandleKeybind(const Util::Array<Util::String>& args)
{
    EditorKeybinds& keybinds = this->GetKeybinds();

    const Util::String verb = args.IsEmpty() ? Util::String("list") : args[0];

    if (verb == "list")
    {
        this->Log(LogMessageType::N_SYSTEM, Util::String("Keybinds, read from ") + keybinds.GetPath());

        const Util::Array<EditorKeyBind>& binds = keybinds.GetKeybinds();
        if (binds.IsEmpty())
        {
            this->Log(LogMessageType::N_SYSTEM, "Nothing registered");
            return;
        }

        for (IndexT i = 0; i < binds.Size(); i++)
        {
            const EditorKeyBind& bind = binds[i];

            // An action with no key can still be run, just not with a key
            const Util::String combo = bind.key == Input::Key::InvalidKey
                ? Util::String("(none)")
                : EditorKeybinds::ComboString(bind.key, bind.mods);

            Util::String line;
            line.Format("%-14s  %-14s  %-16s  %s", bind.action.AsCharPtr(), combo.AsCharPtr(), bind.command.AsCharPtr(),
                this->CommandDescription(bind.command).AsCharPtr());
            this->Log(LogMessageType::N_SYSTEM, line);
        }
        return;
    }

    if (verb == "bind")
    {
        if (args.Size() < 3)
        {
            this->Log(LogMessageType::N_WARNING, "keybind bind expects an action and a combo, like 'keybind bind undo ctrl+z'");
            return;
        }

        if (!keybinds.BindCombo(args[1], args[2]))
        {
            this->Log(LogMessageType::N_WARNING, Util::String("Could not bind '") + args[1] +
                "'. Either nothing registers that action or the combo cannot be read");
            return;
        }

        keybinds.Save();
        this->Log(LogMessageType::N_SYSTEM, Util::String("Bound ") + args[1] + " to " + args[2]);
        return;
    }

    if (verb == "unbind")
    {
        if (args.Size() < 2)
        {
            this->Log(LogMessageType::N_WARNING, "keybind unbind expects an action");
            return;
        }

        if (!keybinds.Unbind(args[1]))
        {
            this->Log(LogMessageType::N_WARNING, Util::String("Nothing registers the action ") + args[1]);
            return;
        }

        keybinds.Save();
        this->Log(LogMessageType::N_SYSTEM, Util::String("Unbound ") + args[1]);
        return;
    }

    if (verb == "reset")
    {
        const Util::String action = args.Size() > 1 ? args[1] : Util::String();
        if (!action.IsEmpty() && keybinds.Find(action) == nullptr)
        {
            this->Log(LogMessageType::N_WARNING, Util::String("Nothing registers the action ") + action);
            return;
        }

        keybinds.Reset(action);
        keybinds.Save();
        this->Log(LogMessageType::N_SYSTEM, action.IsEmpty() ? "All keybinds reset to their defaults" : Util::String("Reset ") + action);
        return;
    }

    if (verb == "save")
    {
        if (args.Size() > 1)
            keybinds.SetPath(args[1]);

        keybinds.Save();
        return;
    }

    if (verb == "load")
    {
        if (args.Size() > 1)
            keybinds.SetPath(args[1]);

        if (!keybinds.Load())
            this->Log(LogMessageType::N_WARNING, Util::String("Nothing read from ") + keybinds.GetPath());
        return;
    }

    this->Log(LogMessageType::N_WARNING, "keybind expects list, bind, unbind, reset, save or load");
}

//------------------------------------------------------------------------------
/**
    The name is one that no other authored entity has, so that a create does not make two entities
    that cannot be told apart by 'select'. The create and the selection that follows it are one
    macro, because a create that selects is one action rather than two, and undoing it takes both
    back. This is the create that both the outline's New Entity button and the 'create' command run.
    It is here rather than on the panel, because the command runs it as well. What the panel adds on
    top of it is the tree: the outline reveals the selection it draws.
*/
Game::Entity
EditorCore::CreateEntity(const Util::String& baseName, const Util::Guid& collection)
{
    Scene* scene = this->GetScene();

    Ptr<CreateEntityCommand> command = CreateEntityCommand::Create();
    command->scene = scene;
    command->record.name = scene->UniqueEntityName(baseName);
    command->record.collection = collection;

    Ptr<EditCommandManager> manager = this->GetCommandManager();

    manager->BeginMacro("Create entity");
    manager->Execute(command);

    if (!scene->HasInstance(command->record.entity))
    {
        // A create that failed put nothing in the macro, so EndMacro does not make an entry, but
        // macro mode still
        // has to be left, otherwise the next action would be bundled onto it
        manager->EndMacro();
        this->Log(LogMessageType::N_WARNING, "Entity not created");
        return Game::Entity::Invalid();
    }

    Util::Array<Game::Entity> entities;
    entities.Append(command->record.entity);
    this->GetSelection()->SetSelection(entities);

    manager->EndMacro();

    this->Log(LogMessageType::N_SYSTEM, Util::String("Created ") + command->record.name);
    return command->record.entity;
}

//------------------------------------------------------------------------------
/**
*/
bool
EditorCore::DeleteEntities(const Util::Array<Game::Entity>& entities)
{
    Scene* scene = this->GetScene();

    // Only the authored entities can be deleted. An entity a command holds an id for is not
    // authored, and a
    // delete that takes nothing out is not an action
    Util::Array<Game::Entity> targets;
    for (IndexT i = 0; i < entities.Size(); i++)
    {
        if (scene->HasInstance(entities[i]))
            targets.Append(entities[i]);
    }
    if (targets.IsEmpty())
        return false;

    // The names are read before the delete, because the record is cleared by it
    Util::String msg;
    if (targets.Size() == 1)
        msg.Format("Deleted %s", scene->GetEditable(targets[0]).name.AsCharPtr());
    else
        msg.Format("Deleted %d entities", (int)targets.Size());

    Ptr<EditCommandManager> manager = this->GetCommandManager();
    Ptr<SelectionContext> selection = this->GetSelection();

    manager->BeginMacro("Delete entities");
    for (IndexT i = 0; i < targets.Size(); i++)
    {
        Ptr<DeleteEntityCommand> command = DeleteEntityCommand::Create();
        command->scene = scene;
        command->entity = targets[i];
        manager->Execute(command);
    }

    // The selection is the same macro. The entities that were not part of it stay, which is what
    // makes a
    // delete of one row out of a selection of ten keep the other nine
    Util::Array<Game::Entity> remaining = selection->GetSelection();
    for (IndexT i = 0; i < targets.Size(); i++)
    {
        const IndexT index = remaining.BinarySearchIndex(targets[i]);
        if (index != InvalidIndex)
            remaining.EraseIndex(index);
    }
    selection->SetSelection(remaining);

    manager->EndMacro();

    this->Log(LogMessageType::N_SYSTEM, msg);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
EditorCore::Log(LogMessageType type, const Util::String& msg)
{
    if (this->log != nullptr)
        this->log->Append(type, msg);

    for (IndexT i = 0; i < this->panels.Size(); i++)
        this->panels[i]->OnLog(type, msg);
}

} // namespace Editor
