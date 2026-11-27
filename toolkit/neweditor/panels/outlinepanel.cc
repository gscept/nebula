//------------------------------------------------------------------------------
//  @file outlinepanel.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/panels/outlinepanel.h"
#include "neweditor/model/clipboard.h"
#include "neweditor/commands/createcollectioncommand.h"
#include "neweditor/commands/deletecollectioncommand.h"
#include "neweditor/commands/instantiateprefabcommand.h"
#include "neweditor/commands/moveentitiestocollectioncommand.h"
#include "neweditor/commands/setcollectionnamecommand.h"
#include "neweditor/commands/setcollectionparentcommand.h"
#include "neweditor/commands/setcomponentcommand.h"
#include "neweditor/commands/setentitynamecommand.h"
#include "neweditor/commands/setparentcommand.h"
#include "neweditor/commands/unpackprefabcommand.h"
#include "neweditor/model/editcommandmanager.h"
#include "neweditor/api/shell.h"
#include "neweditor/base/editorlog.h"
#include "neweditor/base/drawsurface.h"
#include "neweditor/console/editorcommands.h"
#include "neweditor/input/editorkeybinds.h"
#include "neweditor/model/scene.h"
#include "neweditor/model/selectioncontext.h"
#include "neweditor/widgets/filebrowser.h"
#include "neweditor/base/iconregistry.h"
#include "basegamefeature/components/position.h"
#include "neweditor/base/palette.h"
#include "imgui.h"

namespace Editor
{

__ImplementClass(Editor::OutlinePanel, 'OTLP', Editor::EditorPanel);

// The columns. The kind and the guid are fixed so that the tree stays aligned when a row is longer
// than the
// one before it. MarkerWidth is the room the marker takes inside the name column, so that a row
// with a marker
// and a row without one put the name at the same column
static const float MarkerWidth = 22.0f;
static const float KindWidth = 30.0f;
static const float GuidWidth = 170.0f;

// The icon the kind column draws. The order is the RowKind enum
static const char* RowKindIcons[] = { "network", "folder", "box" };

// The indent one level of the tree costs
static const int IndentWidth = 14;

// What the drag payload is called. It carries the entity id rather than a row index, because a
// rebuild moves the
// rows around while a drag is still running. The name is the surface's, because an entity row in the
// inspector is the other place this drag can land
static const char* const DragPayloadType = Editor::EntityDragPayload;

// The collection payload carries the guid of the collection the drag started on, which is what a
// drop reparents
static const char* CollectionPayloadType = "outline_collection";

//------------------------------------------------------------------------------
/**
    True when an entity is below another one in the parent chain, which is what makes a drop that
    would close a cycle not an action. The depth guard is there because a chain that has a cycle
    does not end.
*/
bool
IsUnder(const Scene* scene, Game::Entity entity, Game::Entity ancestor)
{
    SizeT depth = 0;
    while (entity != Game::Entity::Invalid() && depth++ <= scene->GetEditables().Size())
    {
        if (entity == ancestor)
            return true;
        entity = scene->GetParent(entity);
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    The filter is fuzzy: the characters of the filter have to appear in the name in the same order,
    but not next to each other, so 'plr' matches 'Player'.
*/
bool
FuzzyNameMatches(const Util::String& name, const Util::String& filter)
{
    if (filter.IsEmpty())
        return true;
    if (name.IsEmpty())
        return false;

    const char* nameCursor = name.AsCharPtr();
    const char* filterCursor = filter.AsCharPtr();
    while (*nameCursor != '\0' && *filterCursor != '\0')
    {
        if (Util::String::ToLower(*nameCursor) == Util::String::ToLower(*filterCursor))
            filterCursor++;
        nameCursor++;
    }
    return *filterCursor == '\0';
}

//------------------------------------------------------------------------------
/**
*/
const char*
OutlinePanel::RowKindName(IndexT kind)
{
    if (kind == RowRoot)
        return "root";
    if (kind == RowCollection)
        return "collection";
    return "entity";
}

//------------------------------------------------------------------------------
/**
*/
OutlinePanel::OutlinePanel() :
    mode(ModeCollections),
    showGuidCVar(nullptr),
    showGuid(false),
    rootOpen(true),
    anchorEntity(Game::Entity::Invalid()),
    dropTargetRow(InvalidIndex),
    contextRow(InvalidIndex),
    contextPopupRequested(false),
    pressedRow(InvalidIndex),
    pressedAdditive(false),
    pressedRanged(false),
    pressedDragged(false),
    renamingEntity(Game::Entity::Invalid()),
    renamingCollection(Util::Guid()),
    renameFocusRequested(false),
    newCollectionOpen(false),
    newCollectionFocusRequested(false),
    pendingParent(Game::Entity::Invalid()),
    pendingFileParent(Game::Entity::Invalid()),
    pendingFileReparent(false),
    pendingUnpack(Game::Entity::Invalid()),
    pendingTargetParent(Game::Entity::Invalid()),
    pendingSelectionAction(SelectionReplace),
    numCollections(0),
    rootBucket(0),
    treeVersion(0),
    rowsDirty(true)
{
    this->name = "outline";
    this->filterBuffer[0] = 0;
    this->renameBuffer[0] = 0;
    this->newCollectionBuffer[0] = 0;
}

//------------------------------------------------------------------------------
/**
*/
OutlinePanel::~OutlinePanel()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    The state the panel is holding belongs to the scene it last drew, so it goes back under that
    scene before the next one is read. Immediate mode needs the filter buffer to hold what the
    filter says, so the field is refilled as well, and the rows are dirtied because the tree they
    were built from is another scene's.
*/
void
OutlinePanel::SwitchState(const Scene* scene)
{
    if (scene->GetGuid() == this->stateScene)
        return;

    if (this->stateScene.IsValid())
    {
        SceneState& saved = this->sceneStates.Emplace(this->stateScene);
        saved.openCollections = this->openCollections;
        saved.openEntities = this->openEntities;
        saved.filter = this->filter;
        saved.selectedCollection = this->selectedCollection;
        saved.anchorEntity = this->anchorEntity;
        saved.lastSelection = this->lastSelection;
        saved.rootOpen = this->rootOpen;
    }

    this->stateScene = scene->GetGuid();
    const SceneState& loaded = this->sceneStates.Emplace(this->stateScene);

    this->openCollections = loaded.openCollections;
    this->openEntities = loaded.openEntities;
    this->filter = loaded.filter;
    this->selectedCollection = loaded.selectedCollection;
    this->anchorEntity = loaded.anchorEntity;
    this->lastSelection = loaded.lastSelection;
    this->rootOpen = loaded.rootOpen;

    IndexT i = 0;
    const char* src = this->filter.AsCharPtr();
    while (src[i] != '\0' && i < MaxFilterLength - 1)
    {
        this->filterBuffer[i] = src[i];
        i++;
    }
    this->filterBuffer[i] = '\0';

    // The drag and the rename field are rows of the tree that was drawn, and a row of another scene
    // is not a
    // row of this one
    this->rowsDirty = true;
    this->dropTargetRow = InvalidIndex;
    this->contextRow = InvalidIndex;
    this->pressedRow = InvalidIndex;
    this->renamingEntity = Game::Entity::Invalid();
    this->renamingCollection = Util::Guid();
    this->renameBuffer[0] = 0;

    // The prompt is not a row, but the parent it creates under belongs to the scene it was opened
    // on, so the
    // field is closed rather than left pointing at another scene's collection
    this->newCollectionOpen = false;
    this->newCollectionParent = Util::Guid();
    this->newCollectionBuffer[0] = 0;
}

//------------------------------------------------------------------------------
/**
    A closed scene is not drawn again, so its state goes here rather than at the close, which is the
    point where the panel knows the scene is gone.
*/
void
OutlinePanel::PruneStates()
{
    const Util::Array<Ptr<Scene>>& scenes = this->core->GetScenes();

    for (IndexT i = (IndexT)this->sceneStates.Size() - 1; i >= 0; i--)
    {
        const Util::Guid& guid = this->sceneStates.KeyAtIndex(i);

        bool open = false;
        for (IndexT j = 0; j < scenes.Size(); j++)
        {
            if (scenes[j]->GetGuid() == guid)
            {
                open = true;
                break;
            }
        }

        if (!open)
            this->sceneStates.EraseAtIndex(i);
    }
}

//------------------------------------------------------------------------------
/**
    Commands are registered here, which is what makes the panel extensible. The rows are not built
    here, because the first draw builds them from whatever the scene holds at that point.
*/
bool
OutlinePanel::OnSetup()
{
    EditorCommandSystem& commands = this->core->GetCommands();
    commands.Register("select", "select one or more entities in the outline by name. Without an argument it clears the selection",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<OutlinePanel, &OutlinePanel::CommandSelect>(this));
    commands.Register("collection", "create a collection in the outline and make it the active one, or make one the scene already has active",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<OutlinePanel, &OutlinePanel::CommandCollection>(this));
    commands.Register("parent", "parent <entity> [parent] - set or clear the parent of an entity",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<OutlinePanel, &OutlinePanel::CommandParent>(this));
    commands.Register("rename", "rename [name] - rename the selected entities. With more than one the name is the base and each entity gets a number: Foo 1, Foo 2. Without a name it opens the rename field on the first selected entity",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<OutlinePanel, &OutlinePanel::CommandRename>(this));

    // The rename key is a keybind against the command rather than a key read in the draw, which is
    // what keeps it
    // in the keybinds table and rebindable
    this->core->GetKeybinds().Register("outline.rename", Input::Key::F2, Input::Modifier::NoModifier, "rename");

    // The cvar is created here rather than by the feature unit, because a game that does not have
    // this panel
    // should not be creating a cvar for it. The inspector reads the same cvar and creates it as
    // well, which is
    // what makes either panel work on its own
    Core::CVarCreate(Core::CVarType::CVar_Int, "ed_show_entity_guid", "0",
        "Show the entity guid next to the entity name");

    this->showGuidCVar = Core::CVarGet("ed_show_entity_guid");
    this->showGuid = this->showGuidCVar != nullptr && Core::CVarReadInt(this->showGuidCVar) != 0;

    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
OutlinePanel::Draw()
{
    const Scene* scene = this->core->GetScene();

    this->PruneStates();
    this->SwitchState(scene);

    // The rows are built from the open state, so a selection that came from somewhere else has to
    // be revealed
    // before them
    this->RevealSelection();

    // The field is drawn on a row, so when the entity it was opened on is gone, the field goes with
    // it rather
    // than keeping a buffer that nothing can be given. The same for a collection row, where the
    // field is found
    // by the guid
    if (this->renamingEntity != Game::Entity::Invalid() && !scene->HasInstance(this->renamingEntity))
    {
        this->renamingEntity = Game::Entity::Invalid();
        this->renameBuffer[0] = 0;
    }

    if (this->renamingCollection.IsValid() && scene->FindCollection(this->renamingCollection) == InvalidIndex)
    {
        this->renamingCollection = Util::Guid();
        this->renameBuffer[0] = 0;
    }

    float avail = ImGui::GetContentRegionAvail().x;
    float xStart = ImGui::GetCursorPosX();

    ImGui::Text("OUTLINE");
    ImGui::SameLine();

    // The modes, right aligned on the same row as the title. Same styling as the console's filter
    // tabs
    const Mode modes[2] = { ModeCollections, ModeHierarchy };
    const char* names[2] = { "Collections  ", "Hierarchy" };

    ImGuiStyle& style = ImGui::GetStyle();
    float size = 0;
    for (int i = 0; i < 2; i++)
    {
        size += ImGui::CalcTextSize(names[i]).x;
    }

    // The spacing between the two and the margin on the right, so that the last one is not flush
    // with the edge
    size += style.FramePadding.x * 2.0f;

    float off = avail - size;
    if (off > 0.0f)
        ImGui::SetCursorPosX(xStart + off);

    for (int i = 0; i < 2; i++)
    {
        if (this->mode == modes[i])
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ColorFromHex(PaletteAccent));
            ImGui::Text(names[i]);
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::TextDisabled(names[i]);
            if (ImGui::IsItemHovered())
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }
            if (ImGui::IsItemClicked())
            {
                if (modes[i] != this->mode)
                {
                    this->mode = modes[i];
                    this->rowsDirty = true;
                }
            }
        }

        if (i < 1)
            ImGui::SameLine();
    }

    ImGui::Separator();

    // Immediate mode has no element to get a change event from, so the buffer is compared with the
    // filter the
    // rows were last built from
    if (ImGui::InputTextWithHint("##filter", "Filter by name...", this->filterBuffer, MaxFilterLength))
    {
        const Util::String filter(this->filterBuffer);
        if (filter != this->filter)
        {
            this->filter = filter;
            this->rowsDirty = true;
        }
    }

    if (ImGui::Button("New Entity"))
    {
        this->pendingCreateCollection = scene->GetActiveCollection();
        this->QueueAction(Util::Delegate<void()>::FromMethod<OutlinePanel, &OutlinePanel::ApplyCreate>(this));
    }

    if (this->mode == ModeCollections)
    {
        ImGui::SameLine();
        if (ImGui::Button("New Collection"))
        {
            this->newCollectionParent = scene->GetActiveCollection();
            this->newCollectionOpen = true;
            this->newCollectionFocusRequested = true;
        }
    }

    if (this->mode == ModeCollections && this->newCollectionOpen)
    {
        if (this->newCollectionFocusRequested)
        {
            ImGui::SetKeyboardFocusHere();
            this->newCollectionFocusRequested = false;
        }

        // The room the Create button takes on the row, so that the field and the button fit on it
        const float button = ImGui::CalcTextSize("Create").x + ImGui::GetStyle().FramePadding.x * 2.0f;

        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - button);
        const bool committed = ImGui::InputTextWithHint("##new_collection", "New collection name...",
            this->newCollectionBuffer, MaxNameLength, ImGuiInputTextFlags_EnterReturnsTrue);
        const bool deactivated = ImGui::IsItemDeactivated();
        ImGui::PopItemWidth();

        ImGui::SameLine();
        const bool pressed = ImGui::Button("Create");

        if (committed || pressed || deactivated)
        {
            const Util::String name(this->newCollectionBuffer);
            if (name.IsEmpty())
            {
                if (committed || pressed)
                    this->core->SetStatus("The name field is empty", 3.0f);
            }
            else
            {
                this->pendingNewCollectionName = name;
                this->pendingNewCollectionParent = this->newCollectionParent;
                this->QueueAction(Util::Delegate<void()>::FromMethod<OutlinePanel, &OutlinePanel::ApplyCreateCollection>(this));
            }

            this->newCollectionOpen = false;
            this->newCollectionParent = Util::Guid();
            this->newCollectionBuffer[0] = 0;
        }
    }

    if (scene->GetVersion() != this->treeVersion)
    {
        this->treeVersion = scene->GetVersion();
        this->rowsDirty = true;
    }

    if (this->rowsDirty)
    {
        this->BuildRows();

        // A rebuild moves the rows around, so the rows the menu and the drop target point at are
        // dropped rather
        // than remapped. The drag carries entities, so it survives
        this->contextRow = InvalidIndex;
        if (this->dropTargetRow >= (IndexT)this->rows.Size())
            this->dropTargetRow = InvalidIndex;
    }

    if (this->showGuidCVar != nullptr)
    {
        const bool showGuid = Core::CVarReadInt(this->showGuidCVar) != 0;
        if (showGuid != this->showGuid)
            this->showGuid = showGuid;
    }

    // The drag state. The entities are the truth rather than row indexes, because a rebuild moves
    // the rows
    // around while a drag is still running, and the row that started the drag may not be drawn this
    // frame
    if (ImGui::GetDragDropPayload() == nullptr && (!this->draggedEntities.IsEmpty() || this->draggedCollection.IsValid()))
    {
        this->draggedEntities.Clear();
        this->draggedCollection = Util::Guid();
        this->dropTargetRow = InvalidIndex;
    }

    // The tree is a scroll region of its own, so that the header and the filter do not move with
    // it. The footer is
    // the count and the separator above it, and the child gets the rest of the window
    const float footer = ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("tree", ImVec2(0, -footer));

    if (ImGui::BeginTable("outline", this->showGuid ? 4 : 3,
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoHostExtendX))
    {
        // The kind and the guid are fixed so that the tree stays aligned when a row is longer than
        // the one
        // before it. The indent is drawn inside the name column, because a fixed column that is as
        // wide as the
        // deepest row would move every row in the table when the tree gets one level deeper
        ImGui::TableSetupColumn("kind", ImGuiTableColumnFlags_WidthFixed, KindWidth);
        ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("suffix", ImGuiTableColumnFlags_WidthStretch);
        if (this->showGuid)
            ImGui::TableSetupColumn("guid", ImGuiTableColumnFlags_WidthFixed, GuidWidth);

        // Only the rows on screen are touched. The clipper counts rows, so a tree with a thousand
        // entities costs
        // the same as a tree with forty
        ImGuiListClipper clipper;
        clipper.Begin(this->rows.Size());
        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                this->DrawRow((IndexT)i);
        }

        ImGui::EndTable();
    }

    ImGui::EndChild();

    // A press is only a click when the release comes without a drag, and immediate mode only knows
    // the drag after
    // the mouse has moved off the row. The drag state is still the one the drag carries on the
    // frame the release
    // comes on, unless the drop landed in another panel: a delivery clears it inside that panel's
    // draw, and that
    // draw can come before this one, so the panel remembers the drag on the press rather than
    // reading it back
    // here. The row the press landed on is checked against the array, because a rebuild moves the
    // rows around
    // while the mouse is held
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        if (this->pressedRow >= 0 && this->pressedRow < (IndexT)this->rows.Size())
        {
            const Row& row = this->rows[this->pressedRow];
            if (!this->pressedDragged && !this->IsDraggedRow(row))
                this->SelectRow(this->pressedRow, this->pressedAdditive, this->pressedRanged);
        }

        this->pressedRow = InvalidIndex;
        this->pressedDragged = false;
    }

    // The popup is a window of its own, so it is opened from the flag the row set and drawn after
    // the tree. The
    // row it applies to is the one the panel remembered
    if (this->contextPopupRequested)
    {
        ImGui::OpenPopup("context");
        this->contextPopupRequested = false;
    }

    if (ImGui::BeginPopup("context"))
    {
        if (this->contextRow >= 0 && this->contextRow < (IndexT)this->rows.Size())
        {
            const Row& row = this->rows[this->contextRow];
            ImGui::TextUnformatted(row.name.AsCharPtr());
            ImGui::Separator();

            if (ImGui::MenuItem("Create entity"))
            {
                this->pendingCreateCollection = row.guid;
                this->QueueAction(Util::Delegate<void()>::FromMethod<OutlinePanel, &OutlinePanel::ApplyCreate>(this));
            }

            if (row.kind == RowEntity)
            {
                const Util::Array<Game::Entity> targets = this->RowTargets(row);
                const IndexT count = targets.Size();

                // The menu item opens the field rather than renaming, because immediate mode has no
                // element to get
                // a commit from. The popup closes when a menu item is pressed, so the field is
                // drawn on the row the
                // frame after. The commit applies to the row's targets, which is the whole
                // selection when the row
                // is part of it, and more than one target is numbered
                if (ImGui::MenuItem("Rename"))
                {
                    // The buffer is only filled when the field is opened, so opening it again on
                    // the row it is
                    // already open on does not throw away what has been typed
                    if (this->renamingEntity != row.entity)
                    {
                        row.name.CopyToBuffer(this->renameBuffer, MaxNameLength);
                        this->renamingEntity = row.entity;
                    }
                    this->renameFocusRequested = true;
                }

                // An instance entity is authored against another file, so the menu carries the two
                // ways of dealing with that: take the whole instantiation into the selection, or
                // drop the link and keep the values
                if (this->core->GetScene()->IsPrefabInstanceEntity(row.entity))
                {
                    if (ImGui::MenuItem("Select instance"))
                    {
                        Util::Array<Game::Entity> entities;
                        this->core->GetScene()->GetInstanceEntities(row.entity, entities);
                        this->pendingSelection = entities;
                        this->pendingSelectionAction = SelectionReplace;
                        this->QueueAction(Util::Delegate<void()>::FromMethod<OutlinePanel, &OutlinePanel::ApplySelection>(this));
                    }

                    if (ImGui::MenuItem("Unpack Prefab"))
                    {
                        this->pendingUnpack = row.entity;
                        this->QueueAction(Util::Delegate<void()>::FromMethod<OutlinePanel, &OutlinePanel::ApplyUnpack>(this));
                    }
                }

                if (ImGui::MenuItem("Copy"))
                {
                    this->pendingCopy = targets;
                    this->pendingTargetCollection = row.guid;
                    this->pendingTargetParent = row.entity;
                    this->QueueAction(Util::Delegate<void()>::FromMethod<OutlinePanel, &OutlinePanel::ApplyCopy>(this));
                }

                if (ImGui::MenuItem("Duplicate"))
                {
                    this->pendingDuplicate = targets;
                    this->pendingTargetCollection = row.guid;
                    this->pendingTargetParent = row.entity;
                    this->QueueAction(Util::Delegate<void()>::FromMethod<OutlinePanel, &OutlinePanel::ApplyDuplicate>(this));
                }

                if (ImGui::MenuItem("Paste"))
                {
                    this->pendingTargetCollection = row.guid;
                    this->pendingTargetParent = row.entity;
                    this->QueueAction(Util::Delegate<void()>::FromMethod<OutlinePanel, &OutlinePanel::ApplyPaste>(this));
                }

                Util::String label;
                if (count > 1)
                    label.Format("Delete %d entities", (int)count);
                else
                    label = "Delete entity";

                if (ImGui::MenuItem(label.AsCharPtr()))
                {
                    this->pendingDelete = targets;
                    this->QueueAction(Util::Delegate<void()>::FromMethod<OutlinePanel, &OutlinePanel::ApplyDelete>(this));
                }
            }
            else if (row.kind == RowCollection)
            {
                if (ImGui::MenuItem("Rename"))
                {
                    if (this->renamingCollection != row.guid)
                    {
                        row.name.CopyToBuffer(this->renameBuffer, MaxNameLength);
                        this->renamingCollection = row.guid;
                        this->renamingEntity = Game::Entity::Invalid();
                    }
                    this->renameFocusRequested = true;
                }

                // The collection's entities are the ones the row shows, which are the ones the
                // collection holds
                // directly: a nested collection is a row of its own, and its entities are on that
                // row. The two are
                // the same list with a different starting point, one replacing the selection and
                // the other adding
                // to it
                if (ImGui::MenuItem("Select all"))
                {
                    this->pendingSelection = this->CollectionEntities(row);
                    this->pendingSelectionAction = SelectionReplace;
                    this->QueueAction(Util::Delegate<void()>::FromMethod<OutlinePanel, &OutlinePanel::ApplySelection>(this));
                }

                if (ImGui::MenuItem("Add to selection"))
                {
                    this->pendingSelection = this->CollectionEntities(row);
                    this->pendingSelectionAction = SelectionAdd;
                    this->QueueAction(Util::Delegate<void()>::FromMethod<OutlinePanel, &OutlinePanel::ApplySelection>(this));
                }

                if (ImGui::MenuItem("Paste"))
                {
                    this->pendingTargetCollection = row.guid;
                    this->pendingTargetParent = Game::Entity::Invalid();
                    this->QueueAction(Util::Delegate<void()>::FromMethod<OutlinePanel, &OutlinePanel::ApplyPaste>(this));
                }

                if (ImGui::MenuItem("Delete collection"))
                {
                    this->pendingDeleteCollection = row.guid;
                    this->QueueAction(Util::Delegate<void()>::FromMethod<OutlinePanel, &OutlinePanel::ApplyDeleteCollection>(this));
                }
            }
            else
            {
                if (ImGui::MenuItem("Paste"))
                {
                    this->pendingTargetCollection = Util::Guid();
                    this->pendingTargetParent = Game::Entity::Invalid();
                    this->QueueAction(Util::Delegate<void()>::FromMethod<OutlinePanel, &OutlinePanel::ApplyPaste>(this));
                }
            }
        }

        ImGui::EndPopup();
    }

    this->DrawFooter();
}

//------------------------------------------------------------------------------
/**
    One row of the table. The row is drawn from the row array, and everything a click would change
    is queued, because the array being walked cannot change underneath it.
*/
void
OutlinePanel::DrawRow(IndexT rowIndex)
{
    const Row& row = this->rows[rowIndex];

    ImGui::TableNextRow();
    ImGui::PushID(rowIndex);

    ImGui::TableSetColumnIndex(0);

    // The kind is an icon rather than a word. A name that is not in the generated table falls back
    // to the word
    const char* icon = RowKindIcons[row.kind];

    // An entity that came from a prefab file is not the same kind of row as one the scene made:
    // most of what it holds belongs to that file, so the icon says so rather than the name
    Scene* scene = this->core->GetScene();
    const bool prefabRow = row.kind == RowEntity && scene->IsPrefabInstanceEntity(row.entity);
    if (prefabRow)
        icon = "boxes";

    const char* glyph = IconGlyph(icon);
    ImGui::TextUnformatted(glyph != nullptr ? glyph : RowKindName(row.kind));
    if (prefabRow && glyph != nullptr)
        ImGui::SetItemTooltip("instance of %s", scene->GetPrefabInstance(row.entity).source.AsCharPtr());
    ImGui::SameLine();

    // The name column is the tree column. The indent has to be drawn inside a column that is wide
    // enough for the
    // deepest row, and a fixed column sized for that would move every row in the table when the
    // tree gets one
    // level deeper, so the indent is drawn inside the stretch column. The marker keeps its width so
    // that the name
    // lands at the same place on a row that has one and on a row that does not
    ImGui::TableSetColumnIndex(1);
    const float indent = ImGui::GetCursorPosX() + row.depth * IndentWidth;
    ImGui::SetCursorPosX(indent);

    // The marker is the open and the close. It is checked before the row's selectable, because the
    // selectable
    // spans the whole row and would take the click
    if (row.hasChildren)
    {
        ImGui::TextUnformatted(row.open ? "-" : "+");
        ImGui::SameLine();
        if (ImGui::IsItemClicked())
            this->ToggleOpen(row);
    }
    else
    {
        ImGui::TextUnformatted(" ");
        ImGui::SameLine();
    }

    ImGui::SetCursorPosX(indent + MarkerWidth);

    const bool dragged = this->IsDraggedRow(row);

    // The rename field is drawn on the row it was opened on, and the row is found by the entity or
    // by the guid
    // rather than by an index, because a rebuild moves the rows around while the field is open.
    // While the field
    // is open it is the row, so the click and the drag that would have come from the selectable are
    // not drawn
    const bool renaming = (row.kind == RowEntity && row.entity == this->renamingEntity) ||
        (row.kind == RowCollection && row.guid == this->renamingCollection);

    if (renaming)
    {
        if (this->renameFocusRequested)
        {
            ImGui::SetKeyboardFocusHere();
            this->renameFocusRequested = false;
        }

        // The field is read after it is drawn, because the cell has to end with an item rather than
        // with a cursor
        // move. Reading Escape before the field would let the row end with the cursor move that put
        // the field at
        // the indent, which is what ImGui asserts on
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
        const bool committed = ImGui::InputTextWithHint("##rename", "Name", this->renameBuffer, MaxNameLength, ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::PopItemWidth();

        // Escape is the field's own while it has focus: it puts the buffer back to what the field
        // was opened with
        // and deactivates it. That is the frame the field closes on, and the name it holds then is
        // the one the
        // row already has, which is not a change
        if (committed || ImGui::IsItemDeactivated())
            this->CommitRename(row);
    }
    else
    {
        if (dragged)
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);

        ImGui::Selectable(row.name.AsCharPtr(), this->IsSelectedRow(rowIndex),
            ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);

        if (dragged)
            ImGui::PopStyleColor();

        // Ctrl adds to the selection and Shift ranges from the entity the last click landed on. The
        // modifiers are
        // read from the IO because the click is only a mouse button.
        //
        // The press is remembered rather than applied, because immediate mode only knows a drag
        // exists after the mouse has moved off the row. A press that becomes a drag is not a
        // selection click, so the selection is applied on the release, where the drag is known
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            this->pressedRow = rowIndex;
            this->pressedAdditive = ImGui::GetIO().KeyCtrl;
            this->pressedRanged = ImGui::GetIO().KeyShift;
            this->pressedDragged = false;
        }
        else if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            this->contextRow = rowIndex;
            this->contextPopupRequested = true;
        }
    }

    // Every row is a drop target. What a drop does is what the row represents in the tree the mode
    // is drawing:
    // an entity row is an entity in both modes, so a drop there makes it the parent of everything
    // the drag
    // carries, a collection row is a collection, and the root row is the scene in the collections
    // mode and the
    // top of the parent chain in the hierarchy mode. A row that is part of the drag is not a
    // target, because a
    // selection cannot be dropped into itself, and a row with the rename field open is not one
    // either, because
    // the field is what the row is while it is open. A file drag from the file browser is not one
    // of the
    // panel's own, so the row opens for any payload in flight
    if (!renaming && !dragged &&
        (!this->draggedEntities.IsEmpty() || this->draggedCollection.IsValid() ||
         ImGui::GetDragDropPayload() != nullptr))
    {
        if (ImGui::BeginDragDropTarget())
        {
            // The highlight is drawn here rather than from the style, because the color for the row
            // was
            // chosen before the hover was known. The style's fill is switched off for the row: it
            // is
            // opaque and would paint over the row's text. The row gets the same accent as a
            // translucent
            // band instead, and the accent border the target draw puts around it marks the row's
            // edge
            const ImU32 rowFill = (ImGui::GetColorU32(ImGuiCol_DragDropTargetBg) & 0xFFFFFF00) | 0x40;
            ImGui::PushStyleColor(ImGuiCol_DragDropTargetBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

            ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), rowFill);

            // The payload is what tells ImGui a drag is over this row. The entities the drag
            // carries are already
            // on the panel, because the row that started the drag may not be drawn this frame
            Scene* targetScene = this->core->GetScene();
            const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(DragPayloadType);
            if (payload != nullptr)
            {
                // The drop applies to everything the drag carries, which is the selection when the
                // drag
                // started on a selected row. The whole drop is one undo entry, because a drag that
                // moves ten
                // entities is one action
                const bool reparent = row.kind == RowEntity || this->mode == ModeHierarchy;

                this->pendingDrop.Clear();
                for (IndexT i = 0; i < this->draggedEntities.Size(); i++)
                {
                    const Game::Entity entity = this->draggedEntities[i];
                    if (reparent)
                    {
                        // An entity cannot be its own ancestor, which is the check the command does
                        // as well.
                        // It is done here too, because a drop that fails is not something the panel
                        // can
                        // report after the frame has drawn. The root row has no entity, so a drop
                        // there
                        // clears the parent instead
                        if (IsUnder(targetScene, row.entity, entity))
                            continue;
                    }
                    else if (targetScene->GetEditable(entity).collection == row.guid)
                    {
                        continue;
                    }

                    this->pendingDrop.Append(entity);
                }

                if (!this->pendingDrop.IsEmpty())
                {
                    this->dropTargetRow = rowIndex;
                    if (reparent)
                    {
                        // Invalid on the root row, which is what clears the parent
                        this->pendingParent = row.entity;
                        this->QueueAction(Util::Delegate<void()>::FromMethod<OutlinePanel, &OutlinePanel::ApplyParent>(this));
                    }
                    else
                    {
                        this->pendingDropCollection = row.guid;
                        this->QueueAction(Util::Delegate<void()>::FromMethod<OutlinePanel, &OutlinePanel::ApplyDrop>(this));
                    }
                }
            }

            // A collection drag carries one collection, and a drop on a collection row makes that
            // collection
            // its parent. The root row has no guid, so a drop there makes the collection a root
            // collection
            const ImGuiPayload* collectionPayload = ImGui::AcceptDragDropPayload(CollectionPayloadType);
            if (collectionPayload != nullptr)
            {
                const Util::Guid source = *((const Util::Guid*)collectionPayload->Data);

                // A collection cannot be nested inside itself, which is the check the command does
                // as well. It
                // is done here too, because a drop that fails is not something the panel can report
                // after the
                // frame has drawn
                bool nested = false;
                IndexT ancestor = targetScene->FindCollection(row.guid);
                SizeT guard = 0;
                while (ancestor != InvalidIndex && guard++ <= targetScene->GetCollections().Size())
                {
                    if (targetScene->GetCollections()[ancestor].guid == source)
                    {
                        nested = true;
                        break;
                    }
                    ancestor = targetScene->FindCollection(targetScene->GetCollections()[ancestor].parent);
                }

                if (!nested)
                {
                    this->pendingReparentCollection = source;
                    this->pendingReparentParent = row.guid;
                    this->dropTargetRow = rowIndex;
                    this->QueueAction(Util::Delegate<void()>::FromMethod<OutlinePanel, &OutlinePanel::ApplyReparentCollection>(this));
                }
            }

            // A file dragged in from the file browser turns into entities on this row, and the row
            // places
            // them the way it places dropped entities: an entity row parents them, a collection row
            // collects them, and the root row leaves them at the top of what the mode draws
            const ImGuiPayload* filePayload = ImGui::AcceptDragDropPayload(FileDragPayload);
            if (filePayload != nullptr)
            {
                this->pendingFilePath = (const char*)filePayload->Data;
                this->pendingFileParent = row.entity;       // invalid on the root and the collection rows
                this->pendingFileCollection = row.guid;     // invalid on the root and the entity rows
                this->pendingFileReparent = row.kind == RowEntity || this->mode == ModeHierarchy;
                this->dropTargetRow = rowIndex;
                this->QueueAction(Util::Delegate<void()>::FromMethod<OutlinePanel, &OutlinePanel::ApplyFileDrop>(this));
            }

            ImGui::PopStyleColor();
            ImGui::EndDragDropTarget();
        }
    }

    if (!renaming && row.kind == RowEntity)
    {
        if (ImGui::BeginDragDropSource())
        {
            // The press that turned into this drag is not a selection click. The panel remembers that
            // here, on the press, rather than reading the drag state back at the release, because a
            // drop that lands in another panel clears it inside that panel's draw, and that draw can
            // come before this one
            if (rowIndex == this->pressedRow)
                this->pressedDragged = true;

            if (this->draggedEntities.IsEmpty())
            {
                // A drag carries the whole selection when the row it started on is part of the
                // selection, and only
                // that row when it is not. The selection itself is not changed here, because
                // starting a drag is
                // not selecting
                Ptr<SelectionContext> selection = this->core->GetSelection();
                if (selection->IsSelected(row.entity))
                    this->draggedEntities = selection->GetSelection();
                else
                    this->draggedEntities.Append(row.entity);
            }

            const uint64_t id = Ids::Id64(row.entity);
            ImGui::SetDragDropPayload(DragPayloadType, &id, sizeof(id));

            Util::String tip;
            if (this->draggedEntities.Size() > 1)
                tip.Format("%d entities", (int)this->draggedEntities.Size());
            else
                tip = row.name;
            ImGui::TextUnformatted(tip.AsCharPtr());

            ImGui::EndDragDropSource();
        }
    }

    // A collection row drags the collection itself, which is what makes reparenting possible. A
    // collection is not
    // part of the selection, so the drag carries one collection rather than a set of them
    if (!renaming && this->mode == ModeCollections && row.kind == RowCollection)
    {
        if (ImGui::BeginDragDropSource())
        {
            // The same as the entity row: the press that turned into this drag is not a selection
            // click, and the panel has to remember that on the press
            if (rowIndex == this->pressedRow)
                this->pressedDragged = true;

            if (!this->draggedCollection.IsValid())
                this->draggedCollection = row.guid;

            ImGui::SetDragDropPayload(CollectionPayloadType, &row.guid, sizeof(row.guid));
            ImGui::TextUnformatted(row.name.AsCharPtr());

            ImGui::EndDragDropSource();
        }
    }

    // The suffix and the guid are the secondary information on the row, so they are drawn at half
    // opacity
    if (!row.suffix.IsEmpty())
    {
        ImGui::TableSetColumnIndex(2);
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
        ImGui::TextUnformatted(row.suffix.AsCharPtr());
        ImGui::PopStyleColor();
    }

    // Only an entity row has a guid
    if (this->showGuid && row.kind == RowEntity)
    {
        ImGui::TableSetColumnIndex(3);
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
        ImGui::TextUnformatted(row.entityGuid.AsString().AsCharPtr());
        ImGui::PopStyleColor();
    }

    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
    The rows the tree shows. Only the visible ones are in the array, so a collapsed node is not in
    it, and the clipper walks a slice of it.
*/
void
OutlinePanel::BuildRows()
{
    this->rows.Clear();
    this->rowsDirty = false;

    const Scene* scene = this->core->GetScene();

    const IndexT numEditables = scene->GetEditables().Size();
    this->numCollections = scene->GetCollections().Size();
    this->rootBucket = this->numCollections + numEditables;

    if (this->mode == ModeCollections)
        this->BuildCollectionNodes();
    else
        this->BuildHierarchyNodes();

    const char* rootName = this->mode == ModeCollections ? "Scene" : "Hierarchy";
    const IndexT rootRow = this->AppendRow(RowRoot, 0, InvalidIndex, Game::Entity::Invalid(), Util::Guid(), Util::Guid(),
        rootName, Util::String(), this->rootOpen, this->nodeHead[this->rootBucket] != -1);

    // A filter that is not empty forces every node open, otherwise there is nothing to match
    // against
    if (this->rootOpen || !this->filter.IsEmpty())
        this->WalkTree(rootRow);

    if (!this->filter.IsEmpty())
        this->FilterRows();
}

//------------------------------------------------------------------------------
/**
    The node lists for the collection tree. Collections are linked first so that a collection shows
    its child collections before its entities, which is the order the old outliner drew them in.
*/
void
OutlinePanel::BuildCollectionNodes()
{
    const Scene* scene = this->core->GetScene();
    const IndexT numEditables = scene->GetEditables().Size();

    this->nodeHead.Clear();
    this->nodeHead.Resize(this->rootBucket + 1, -1);
    this->nodeLast.Clear();
    this->nodeLast.Resize(this->rootBucket + 1, -1);
    this->nodeNext.Clear();
    this->nodeNext.Resize(this->rootBucket + 1, -1);

    for (IndexT i = 0; i < this->numCollections; i++)
    {
        IndexT parent = scene->FindCollection(scene->GetCollections()[i].parent);
        if (parent == InvalidIndex)
            parent = this->rootBucket;
        this->AppendNode(parent, i);
    }

    for (IndexT i = 0; i < numEditables; i++)
    {
        const Editable& editable = scene->GetEditables()[i];
        if (!editable.guid.IsValid() || !scene->HasInstance(editable.entity))
            continue;

        IndexT parent = scene->FindCollection(editable.collection);
        if (parent == InvalidIndex)
            parent = this->rootBucket;
        this->AppendNode(parent, this->numCollections + i);
    }
}

//------------------------------------------------------------------------------
/**
    The node lists for the hierarchy tree, from the parent field of Game::HTransform. An entity
    whose parent is not an authored entity is a root, which is what keeps entities created while
    playing out of the tree.
*/
void
OutlinePanel::BuildHierarchyNodes()
{
    const Scene* scene = this->core->GetScene();
    const IndexT numEditables = scene->GetEditables().Size();

    this->nodeHead.Clear();
    this->nodeHead.Resize(this->rootBucket + 1, -1);
    this->nodeLast.Clear();
    this->nodeLast.Resize(this->rootBucket + 1, -1);
    this->nodeNext.Clear();
    this->nodeNext.Resize(this->rootBucket + 1, -1);

    for (IndexT i = 0; i < numEditables; i++)
    {
        const Editable& editable = scene->GetEditables()[i];
        if (!editable.guid.IsValid() || !scene->HasInstance(editable.entity))
            continue;

        const Game::Entity parent = scene->GetParent(editable.entity);
        if (parent == Game::Entity::Invalid() || parent.index >= numEditables ||
            !scene->GetEditables()[parent.index].guid.IsValid() || !scene->HasInstance(parent))
        {
            this->AppendNode(this->rootBucket, this->numCollections + i);
            continue;
        }

        this->AppendNode(this->numCollections + parent.index, this->numCollections + i);
    }
}

//------------------------------------------------------------------------------
/**
    Appending to the end of the parent's list is what keeps the order the nodes were created in.
*/
void
OutlinePanel::AppendNode(IndexT parent, IndexT node)
{
    if (this->nodeHead[parent] == -1)
        this->nodeHead[parent] = node;
    else
        this->nodeNext[this->nodeLast[parent]] = node;
    this->nodeLast[parent] = node;
}

//------------------------------------------------------------------------------
/**
    The walk is iterative because the tree can be as deep as it wants, and a node is only walked
    when the path to it is open.
*/
void
OutlinePanel::WalkTree(IndexT rootRow)
{
    const Scene* scene = this->core->GetScene();
    const IndexT numEditables = scene->GetEditables().Size();
    const bool filtering = !this->filter.IsEmpty();

    this->path.Clear();

    IndexT node = this->nodeHead[this->rootBucket];
    int depth = 1;
    IndexT parentRow = rootRow;

    while (node != -1)
    {
        Game::Entity entity = Game::Entity::Invalid();
        Util::Guid guid;
        Util::Guid entityGuid;
        Util::String name;
        Util::String suffix;
        IndexT kind;
        bool open;

        if (node < this->numCollections)
        {
            kind = RowCollection;
            const Collection& collection = scene->GetCollections()[node];
            guid = collection.guid;
            name = collection.name;

            // Collections start open, entities start closed
            if (!this->openCollections.Contains(guid))
                this->openCollections.Add(guid, true);
            open = filtering || this->openCollections[guid];
        }
        else
        {
            kind = RowEntity;
            const Editable& editable = scene->GetEditables()[node - this->numCollections];
            entity = editable.entity;
            entityGuid = editable.guid;
            name = editable.name;

            // The guid of an entity row is the collection it is in, so that the context menu on the
            // row has it
            // without a second lookup. A drop on an entity row uses the entity rather than the
            // collection
            guid = editable.collection;

            if (!this->openEntities.Contains(entity))
                this->openEntities.Add(entity, false);
            open = filtering || this->openEntities[entity];

            // The suffix is what tells the two modes apart on an entity row: which collection it is
            // in when
            // the tree is grouped by collection, and who its parent is when the tree is grouped by
            // hierarchy.
            // It is kept apart from the name so that it can be drawn at half opacity, because it is
            // the
            // secondary information on the row
            if (this->mode == ModeHierarchy)
            {
                const IndexT collectionIndex = scene->FindCollection(editable.collection);
                if (collectionIndex != InvalidIndex)
                {
                    suffix.Append("[");
                    suffix.Append(scene->GetCollections()[collectionIndex].name);
                    suffix.Append("]");
                }
                else
                {
                    suffix.Append("[Scene]");
                }
            }
            else
            {
                const Game::Entity parent = scene->GetParent(editable.entity);
                if (parent != Game::Entity::Invalid() && parent.index < numEditables &&
                    scene->GetEditables()[parent.index].guid.IsValid())
                {
                    suffix.Append("[parent: ");
                    suffix.Append(scene->GetEditables()[parent.index].name);
                    suffix.Append("]");
                }
            }
        }

        const bool hasChildren = this->nodeHead[node] != -1;
        const IndexT row = this->AppendRow(kind, depth, parentRow, entity, guid, entityGuid, name, suffix, open, hasChildren);

        if (open && hasChildren)
        {
            this->path.Push(PathEntry{node, depth, row});
            node = this->nodeHead[node];
            depth++;
            parentRow = row;
            continue;
        }

        IndexT next = this->nodeNext[node];
        while (next == -1 && !this->path.IsEmpty())
        {
            const PathEntry entry = this->path.Pop();
            depth = entry.depth;
            parentRow = entry.row;
            next = this->nodeNext[entry.index];
        }
        node = next;
    }
}

//------------------------------------------------------------------------------
/**
    A row stays when it matches the filter or when something under it matches, which is what keeps
    the path to a match visible. Walking the rows backwards lets a row inherit the matches of
    everything below it, because the rows are in the order they were drawn and the descendants of a
    row come right after it.
*/
void
OutlinePanel::FilterRows()
{
    Util::Array<bool> keep;
    keep.Resize(this->rows.Size(), false);

    for (IndexT i = (IndexT)this->rows.Size() - 1; i >= 0; i--)
    {
        if (!this->rows[i].match)
            continue;

        IndexT row = i;
        while (row != InvalidIndex && !keep[row])
        {
            keep[row] = true;
            row = this->rows[row].parentRow;
        }
    }

    IndexT write = 0;
    for (IndexT read = 0; read < (IndexT)this->rows.Size(); read++)
    {
        if (!keep[read])
            continue;
        if (write != read)
            this->rows[write] = this->rows[read];
        write++;
    }
    this->rows.Resize(write);
}

//------------------------------------------------------------------------------
/**
*/
IndexT
OutlinePanel::AppendRow(IndexT kind, int depth, IndexT parentRow, Game::Entity entity, const Util::Guid& guid,
    const Util::Guid& entityGuid, const Util::String& name, const Util::String& suffix, bool open, bool hasChildren)
{
    Row row;
    row.kind = kind;
    row.depth = depth;
    row.parentRow = parentRow;
    row.entity = entity;
    row.guid = guid;
    row.entityGuid = entityGuid;
    row.name = name;
    row.suffix = suffix;
    row.open = open;
    row.hasChildren = hasChildren;

    // The suffix is part of what the filter can match, so that a filter can still find an entity by
    // the
    // collection it is in or the name of its parent
    row.match = this->MatchesFilter(name) || (!suffix.IsEmpty() && this->MatchesFilter(suffix));

    this->rows.Append(row);
    return (IndexT)this->rows.Size() - 1;
}

//------------------------------------------------------------------------------
/**
    Opening and closing is the panel's own state, so it is written here rather than queued. It is
    what makes a filter keep the path to a match, and what lets a command open a node.
*/
void
OutlinePanel::ToggleOpen(const Row& row)
{
    if (!row.hasChildren)
        return;

    if (row.kind == RowRoot)
    {
        this->rootOpen = !this->rootOpen;
    }
    else if (row.kind == RowCollection)
    {
        if (!this->openCollections.Contains(row.guid))
            this->openCollections.Add(row.guid, true);
        this->openCollections[row.guid] = !this->openCollections[row.guid];
    }
    else
    {
        if (!this->openEntities.Contains(row.entity))
            this->openEntities.Add(row.entity, false);
        this->openEntities[row.entity] = !this->openEntities[row.entity];
    }

    this->rowsDirty = true;
}

//------------------------------------------------------------------------------
/**
    A plain click replaces the selection and Ctrl+click adds or removes one entity. Shift+click
    selects the rows between the entity the last click landed on and this one, in the order the tree
    draws them.

    The selection goes through the command manager, which is what makes it undoable, so the change
    is queued rather than applied here.
*/
void
OutlinePanel::SelectRow(IndexT rowIndex, bool additive, bool ranged)
{
    const Row& row = this->rows[rowIndex];

    if (row.kind == RowEntity)
    {
        this->pendingSelection.Clear();

        const IndexT anchorRow = ranged ? this->RowOfEntity(this->anchorEntity) : InvalidIndex;

        if (anchorRow != InvalidIndex)
        {
            const IndexT first = anchorRow < rowIndex ? anchorRow : rowIndex;
            const IndexT last = anchorRow < rowIndex ? rowIndex : anchorRow;

            for (IndexT i = first; i <= last; i++)
            {
                if (this->rows[i].kind == RowEntity)
                    this->pendingSelection.Append(this->rows[i].entity);
            }
        }
        else
        {
            this->pendingSelection.Append(row.entity);
            this->anchorEntity = row.entity;
        }

        this->pendingSelectionAction = (additive && anchorRow == InvalidIndex) ? SelectionToggle : SelectionReplace;
        this->QueueAction(Util::Delegate<void()>::FromMethod<OutlinePanel, &OutlinePanel::ApplySelection>(this));
        return;
    }

    // The collection the panel has selected is the one new entities go into. It is the panel's own
    // state, but the
    // scene keeps a copy of it, so that part is queued
    this->selectedCollection = row.kind == RowCollection ? row.guid : Util::Guid();
    this->pendingActiveCollection = this->selectedCollection;
    this->QueueAction(Util::Delegate<void()>::FromMethod<OutlinePanel, &OutlinePanel::ApplyActiveCollection>(this));
}

//------------------------------------------------------------------------------
/**
    A row is selected when it carries one of the selected entities, or when it is the collection the
    panel has selected. The selection can hold more than one entity, so this is a membership test
    rather than one row index, and the selection is sorted so the test is a binary search.
*/
bool
OutlinePanel::IsSelectedRow(IndexT rowIndex) const
{
    const Row& row = this->rows[rowIndex];

    if (row.kind == RowEntity)
        return this->core->GetSelection()->IsSelected(row.entity);
    if (row.kind == RowCollection)
        return row.guid == this->selectedCollection;
    return false;
}

//------------------------------------------------------------------------------
/**
    True when an entity is part of the drag. The dragged entities are kept sorted, so this is a
    binary search.
*/
bool
OutlinePanel::IsDragged(Game::Entity entity) const
{
    return this->draggedEntities.BinarySearchIndex(entity) != InvalidIndex;
}

//------------------------------------------------------------------------------
/**
    True when the row is part of the drag. A collection drag carries one collection, because a
    collection is not part of the selection, and the root row is never part of one.
*/
bool
OutlinePanel::IsDraggedRow(const Row& row) const
{
    if (row.kind == RowEntity)
        return this->IsDragged(row.entity);
    if (row.kind == RowCollection)
        return row.guid == this->draggedCollection;
    return false;
}

//------------------------------------------------------------------------------
/**
    The entity a name refers to, or Invalid when nothing has that name.
*/
Game::Entity
OutlinePanel::EntityNamed(const Util::String& name) const
{
    const Scene* scene = this->core->GetScene();

    for (IndexT i = 0; i < scene->GetEditables().Size(); i++)
    {
        const Editable& editable = scene->GetEditables()[i];
        if (editable.guid.IsValid() && scene->HasInstance(editable.entity) && editable.name == name)
            return editable.entity;
    }
    return Game::Entity::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
IndexT
OutlinePanel::RowOfEntity(Game::Entity entity) const
{
    if (entity == Game::Entity::Invalid())
        return InvalidIndex;

    for (IndexT i = 0; i < (IndexT)this->rows.Size(); i++)
    {
        if (this->rows[i].kind == RowEntity && this->rows[i].entity == entity)
            return i;
    }
    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
    A selection that comes from outside the panel - the console, a command script, the clipboard or
    an undo - can put the selected entity behind a node that is closed, and then the panel has a
    selection it cannot draw. Opening the path to it is what makes it visible.

    It only runs when the selection changed since the last draw, because a node the user closed on
    top of a selection would otherwise be opened again every frame.
*/
void
OutlinePanel::RevealSelection()
{
    const Scene* scene = this->core->GetScene();
    const Util::Array<Game::Entity>& selection = this->core->GetSelection()->GetSelection();

    if (selection == this->lastSelection)
        return;

    this->lastSelection = selection;

    const IndexT numCollections = scene->GetCollections().Size();
    const IndexT numEditables = scene->GetEditables().Size();

    bool revealed = false;
    for (IndexT i = 0; i < selection.Size(); i++)
    {
        if (!scene->HasInstance(selection[i]))
            continue;

        revealed = true;

        if (this->mode == ModeCollections)
        {
            // The collection the entity is in, and every collection above it. The depth guard is
            // there because a
            // parent that points back is a cycle that does not end
            Util::Guid guid = scene->GetEditable(selection[i]).collection;
            IndexT depth = 0;
            while (depth++ <= numCollections)
            {
                const IndexT index = scene->FindCollection(guid);
                if (index == InvalidIndex)
                    break;

                const Collection& collection = scene->GetCollections()[index];
                if (!this->openCollections.Contains(collection.guid))
                    this->openCollections.Add(collection.guid, true);
                this->openCollections[collection.guid] = true;
                guid = collection.parent;
            }
        }
        else
        {
            // The parent chain above the entity, for the same reason
            Game::Entity parent = scene->GetParent(selection[i]);
            IndexT depth = 0;
            while (parent != Game::Entity::Invalid() && depth++ <= numEditables)
            {
                if (!this->openEntities.Contains(parent))
                    this->openEntities.Add(parent, true);
                this->openEntities[parent] = true;
                parent = scene->GetParent(parent);
            }
        }
    }

    if (!revealed)
        return;

    this->rootOpen = true;
    this->rowsDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
bool
OutlinePanel::MatchesFilter(const Util::String& name) const
{
    return FuzzyNameMatches(name, this->filter);
}

//------------------------------------------------------------------------------
/**
    The footer: how many entities there are, how many of them the filter leaves visible, or how many
    are selected.
*/
void
OutlinePanel::DrawFooter()
{
    const Scene* scene = this->core->GetScene();

    IndexT entities = 0;
    for (IndexT i = 0; i < scene->GetEditables().Size(); i++)
    {
        if (scene->GetEditables()[i].guid.IsValid() && scene->HasInstance(scene->GetEditables()[i].entity))
            entities++;
    }

    const Util::Array<Game::Entity>& selection = this->core->GetSelection()->GetSelection();

    ImGui::Separator();

    Util::String count;
    if (!selection.IsEmpty())
        count.Format("%d selected", (int)selection.Size());
    else if (this->filter.IsEmpty())
        count.Format("%d entities, %d collections", (int)entities, (int)scene->GetCollections().Size());
    else
        count.Format("%d of %d", (int)this->rows.Size(), (int)entities);

    ImGui::TextUnformatted(count.AsCharPtr());
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Game::Entity>
OutlinePanel::RowTargets(const Row& row) const
{
    Util::Array<Game::Entity> targets;

    if (row.kind != RowEntity)
        return targets;

    Ptr<SelectionContext> selection = this->core->GetSelection();
    if (selection->IsSelected(row.entity))
        targets = selection->GetSelection();
    else
        targets.Append(row.entity);

    return targets;
}

//------------------------------------------------------------------------------
/**
    The entities a collection holds. A collection is a field on an editable rather than a list the
    scene keeps, so this is the same scan the collection tree is built from, which is what makes the
    entities come in the order the tree draws them. Nested collections are rows of their own, so
    their entities are not part of this row's.
*/
Util::Array<Game::Entity>
OutlinePanel::CollectionEntities(const Row& row) const
{
    Util::Array<Game::Entity> entities;

    const Scene* scene = this->core->GetScene();
    for (IndexT i = 0; i < scene->GetEditables().Size(); i++)
    {
        const Editable& editable = scene->GetEditables()[i];
        if (editable.collection != row.guid)
            continue;

        if (!editable.guid.IsValid() || !scene->HasInstance(editable.entity))
            continue;

        entities.Append(editable.entity);
    }

    return entities;
}

//------------------------------------------------------------------------------
/**
    The create is the core's, which is the same one the 'create' command runs. It selects the new
    entity, and the path to that selection is opened by RevealSelection on the next draw, which is
    what makes a create behind a collapsed node look like something happened.
*/
void
OutlinePanel::ApplyCreate()
{
    this->core->CreateEntity("Entity", this->pendingCreateCollection);
}

//------------------------------------------------------------------------------
/**
*/
void
OutlinePanel::ApplyCreateCollection()
{
    Scene* scene = this->core->GetScene();

    Ptr<CreateCollectionCommand> command = CreateCollectionCommand::Create();
    command->scene = scene;
    command->record.name = this->pendingNewCollectionName;
    command->record.parent = this->pendingNewCollectionParent;

    this->core->GetCommandManager()->Execute(command);

    if (scene->FindCollection(command->record.guid) == InvalidIndex)
    {
        this->core->Log(LogMessageType::N_WARNING, "Collection not created");
        return;
    }

    if (this->pendingNewCollectionParent.IsValid())
    {
        if (!this->openCollections.Contains(this->pendingNewCollectionParent))
            this->openCollections.Add(this->pendingNewCollectionParent, true);
        this->openCollections[this->pendingNewCollectionParent] = true;
    }
    else
    {
        this->rootOpen = true;
    }

    this->selectedCollection = command->record.guid;
    scene->SetActiveCollection(command->record.guid);

    this->core->Log(LogMessageType::N_SYSTEM, Util::String("Created collection ") + this->pendingNewCollectionName);
}

//------------------------------------------------------------------------------
/**
*/
void
OutlinePanel::ApplyDelete()
{
    if (this->pendingDelete.IsEmpty())
        return;

    if (!this->core->DeleteEntities(this->pendingDelete))
        this->core->Log(LogMessageType::N_WARNING, "Nothing to delete");

    this->pendingDelete.Clear();
}

//------------------------------------------------------------------------------
/**
    Deleting a collection does not delete what is in it, it moves them to the parent. The collection
    the panel has selected is the one new entities go into, so when the deleted one is that, it has
    to move to the parent as well, otherwise the panel points at something that is not there
    anymore.
*/
void
OutlinePanel::ApplyDeleteCollection()
{
    Scene* scene = this->core->GetScene();
    const Util::Guid collection = this->pendingDeleteCollection;
    if (!collection.IsValid())
        return;

    const IndexT index = scene->FindCollection(collection);
    if (index == InvalidIndex)
    {
        this->core->Log(LogMessageType::N_WARNING, "Collection not found");
        return;
    }

    const Util::String name = scene->GetCollections()[index].name;
    const Util::Guid parent = scene->GetCollections()[index].parent;

    Ptr<DeleteCollectionCommand> command = DeleteCollectionCommand::Create();
    command->scene = scene;
    command->collection = collection;
    this->core->GetCommandManager()->Execute(command);

    if (this->selectedCollection == collection)
    {
        this->selectedCollection = parent;
        scene->SetActiveCollection(parent);
    }

    this->core->Log(LogMessageType::N_SYSTEM, Util::String("Deleted collection ") + name);
}

//------------------------------------------------------------------------------
/**
    The drop moves everything the drag carried. The whole move is one undo entry, because a drag
    that moves ten entities is one action.
*/
void
OutlinePanel::ApplyDrop()
{
    Scene* scene = this->core->GetScene();
    if (this->pendingDrop.IsEmpty())
        return;

    Ptr<EditCommandManager> manager = this->core->GetCommandManager();

    Ptr<MoveEntitiesToCollectionCommand> command = MoveEntitiesToCollectionCommand::Create();
    command->scene = scene;
    command->entities = this->pendingDrop;
    command->newCollection = this->pendingDropCollection;
    manager->Execute(command);
}

//------------------------------------------------------------------------------
/**
*/
void
OutlinePanel::ApplyFileDrop()
{
    Scene* scene = this->core->GetScene();
    if (this->pendingFilePath.IsEmpty() || scene == nullptr)
        return;

    Ptr<EditCommandManager> manager = this->core->GetCommandManager();

    // The whole drop is one undo entry, the way an entity drop is: the instantiation and the
    // placing
    // of its roots on the row. The undo unwinds the placing first, and the instantiation's own undo
    // then takes the entities and the collections the file carried out
    manager->BeginMacro("Instantiate");

    Ptr<InstantiatePrefabCommand> instantiate = InstantiatePrefabCommand::Create();
    instantiate->scene = scene;
    instantiate->path = this->pendingFilePath;
    instantiate->collection = this->pendingFileCollection;   // invalid at the root row means the active one
    manager->Execute(instantiate);

    // The placement follows the row. An entity row makes the row's entity the parent, and the roots
    // land at the parent's local origin. A collection row takes them to the world origin instead.
    // The children of the instantiation follow their parents and keep what the file has
    if (this->pendingFileReparent && this->pendingFileParent != Game::Entity::Invalid())
    {
        const Game::ComponentId transform = Game::GetComponentId<Game::HTransform>();
        for (IndexT i = 0; i < instantiate->created.Size(); i++)
        {
            const Game::Entity entity = instantiate->created[i];
            if (scene->GetParent(entity) != Game::Entity::Invalid())
                continue;

            Ptr<SetParentCommand> command = SetParentCommand::Create();
            command->scene = scene;
            command->child = entity;
            command->newParent = this->pendingFileParent;
            manager->Execute(command);

            // The parent command gives a child without one a local that keeps the world position
            // the
            // file had. The drop wants the parent's origin instead, so the position goes to zero on
            // the transform the parenting just made. The undo unwinds this first, and the
            // parenting's
            // own undo then takes the transform out again
            Game::HTransform local = scene->GetAuthoringWorld()->GetComponent<Game::HTransform>(entity);
            local.localPosition = Math::vec3(0.0f, 0.0f, 0.0f);

            Ptr<SetComponentCommand> zero = SetComponentCommand::Create();
            zero->scene = scene;
            zero->entity = entity;
            zero->component = transform;
            zero->newValue = Util::Blob(&local, sizeof(local));
            manager->Execute(zero);
        }
    }
    else if (!this->pendingFileReparent)
    {
        // The collection rows: the roots have no parent, so their position is the world one, and it
        // goes to the origin. A file that is placed under a collection starts from there rather
        // than
        // from where the file had it
        const Game::ComponentId position = Game::GetComponentId<Game::Position>();
        for (IndexT i = 0; i < instantiate->created.Size(); i++)
        {
            const Game::Entity entity = instantiate->created[i];
            if (scene->GetParent(entity) != Game::Entity::Invalid())
                continue;
            if (!scene->HasComponent(entity, position))
                continue;

            const Game::Position pos(Math::vec3(0.0f, 0.0f, 0.0f));

            Ptr<SetComponentCommand> command = SetComponentCommand::Create();
            command->scene = scene;
            command->entity = entity;
            command->component = position;
            command->newValue = Util::Blob(&pos, sizeof(pos));
            manager->Execute(command);
        }
    }

    manager->EndMacro();

    this->pendingFilePath.Clear();
}

//------------------------------------------------------------------------------
/**
    A drop on an entity row makes that entity the parent of everything the drag carries, and a drop
    on the root row of the hierarchy clears the parent. The ones that would make an entity its own
    ancestor are not in the list, so a drop that changes nothing does not become an entry.
*/
void
OutlinePanel::ApplyParent()
{
    Scene* scene = this->core->GetScene();
    if (this->pendingDrop.IsEmpty())
        return;

    Ptr<EditCommandManager> manager = this->core->GetCommandManager();

    manager->BeginMacro(this->pendingParent == Game::Entity::Invalid() ? "Clear parent" : "Set parent");
    for (IndexT i = 0; i < this->pendingDrop.Size(); i++)
    {
        Ptr<SetParentCommand> command = SetParentCommand::Create();
        command->scene = scene;
        command->child = this->pendingDrop[i];
        command->newParent = this->pendingParent;
        manager->Execute(command);
    }
    manager->EndMacro();
}

//------------------------------------------------------------------------------
/**
    Reparenting a collection through the command manager is what makes it undoable. An invalid
    parent makes the collection a root collection, which is what a drop on the root row is.
*/
void
OutlinePanel::ApplyReparentCollection()
{
    Scene* scene = this->core->GetScene();
    if (!this->pendingReparentCollection.IsValid())
        return;

    Ptr<SetCollectionParentCommand> command = SetCollectionParentCommand::Create();
    command->scene = scene;
    command->collection = this->pendingReparentCollection;
    command->newParent = this->pendingReparentParent;
    this->core->GetCommandManager()->Execute(command);
}

//------------------------------------------------------------------------------
/**
    The selection goes through the command manager, which is what makes it undoable. Adding is not
    toggling: an entity that is already in the selection stays in it, and the selection is a sorted
    set, so the union is built by inserting.
*/
void
OutlinePanel::ApplySelection()
{
    if (this->pendingSelection.IsEmpty())
        return;

    Ptr<SelectionContext> selection = this->core->GetSelection();

    if (this->pendingSelectionAction == SelectionAdd)
    {
        Util::Array<Game::Entity> merged = selection->GetSelection();
        for (IndexT i = 0; i < this->pendingSelection.Size(); i++)
            merged.InsertSorted(this->pendingSelection[i]);

        selection->SetSelection(merged);
    }
    else if (this->pendingSelectionAction == SelectionToggle)
        selection->Toggle(this->pendingSelection[0]);
    else
        selection->SetSelection(this->pendingSelection);
}

//------------------------------------------------------------------------------
/**
*/
void
OutlinePanel::ApplyActiveCollection()
{
    Scene* scene = this->core->GetScene();

    scene->SetActiveCollection(this->pendingActiveCollection);
}

//------------------------------------------------------------------------------
/**
    The unpack goes through the command manager, which is what makes it undoable. The command keeps
    the links it drops, so the undo attaches the entities to the instantiation again.
*/
void
OutlinePanel::ApplyUnpack()
{
    if (this->pendingUnpack == Game::Entity::Invalid())
        return;

    Ptr<UnpackPrefabCommand> command = UnpackPrefabCommand::Create();
    command->scene = this->core->GetScene();
    command->entity = this->pendingUnpack;
    this->core->GetCommandManager()->Execute(command);
}

//------------------------------------------------------------------------------
/**
*/
void
OutlinePanel::ApplyCopy()
{
    if (this->pendingCopy.IsEmpty())
        return;

    if (!this->core->GetClipboard()->Copy(this->pendingCopy))
        this->core->Log(LogMessageType::N_WARNING, "Nothing to copy");

    this->pendingCopy.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
OutlinePanel::ApplyDuplicate()
{
    if (this->pendingDuplicate.IsEmpty())
        return;

    if (!this->core->GetClipboard()->Duplicate(this->pendingDuplicate, this->pendingTargetCollection,
        this->pendingTargetParent))
        this->core->Log(LogMessageType::N_WARNING, "Nothing to duplicate");

    this->pendingDuplicate.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
OutlinePanel::ApplyPaste()
{
    Ptr<SceneDocument> doc = this->core->GetClipboard()->Read();
    if (!doc.isvalid())
    {
        this->core->Log(LogMessageType::N_WARNING, "Clipboard does not hold a scene document");
        return;
    }

    if (!this->core->GetClipboard()->Paste(doc, this->pendingTargetCollection, this->pendingTargetParent, "Paste"))
        this->core->Log(LogMessageType::N_WARNING, "Nothing pasted");
}

//------------------------------------------------------------------------------
/**
    What the rename field holds is queued rather than applied during the draw, and the field is
    closed, because a commit is the end of the edit. An empty name is not a name, and a name that is
    the same as the one the row already has is not a change, which is what makes Escape commit
    nothing: the field puts the buffer back to what it was opened with when the key is pressed.
*/
void
OutlinePanel::CommitRename(const Row& row)
{
    const Scene* scene = this->core->GetScene();
    const Util::String name(this->renameBuffer);

    if (!name.IsEmpty())
    {
        if (row.kind == RowEntity)
        {
            // The rename applies to the row's targets, which is the whole selection when the row is
            // part of it,
            // and more than one target is numbered
            this->QueueRename(this->RowTargets(row), name);
        }
        else
        {
            const IndexT index = scene->FindCollection(row.guid);
            if (index != InvalidIndex && name != scene->GetCollections()[index].name)
            {
                this->pendingRenameCollection = row.guid;
                this->pendingRenameName = name;
                this->QueueAction(Util::Delegate<void()>::FromMethod<OutlinePanel, &OutlinePanel::ApplyRenameCollection>(this));
            }
        }
    }

    // Only one field can be open, so both are closed here rather than only the one the commit came
    // from
    this->renamingEntity = Game::Entity::Invalid();
    this->renamingCollection = Util::Guid();
    this->renameBuffer[0] = 0;
}

//------------------------------------------------------------------------------
/**
    The names a rename gives are queued rather than applied where the rename came from, because a
    panel does not mutate the model while it draws. The base is what the rename field or the command
    line held: one entity gets it on its own, and more than one gets it with a number on each, in
    the order the entities were given. The ones that already have the name they would be given are
    not queued, which is what makes a rename that changes nothing not an entry.
*/
void
OutlinePanel::QueueRename(const Util::Array<Game::Entity>& entities, const Util::String& base)
{
    const Scene* scene = this->core->GetScene();

    this->pendingRenameEntities.Clear();
    this->pendingRenameNames.Clear();

    for (IndexT i = 0; i < entities.Size(); i++)
    {
        if (!scene->HasInstance(entities[i]))
            continue;

        Util::String name = base;
        if (entities.Size() > 1)
            name.Format("%s %d", base.AsCharPtr(), (int)i + 1);

        if (name == scene->GetEditable(entities[i]).name)
            continue;

        this->pendingRenameEntities.Append(entities[i]);
        this->pendingRenameNames.Append(name);
    }

    if (this->pendingRenameEntities.IsEmpty())
        return;

    this->pendingRenameName = base;
    this->QueueAction(Util::Delegate<void()>::FromMethod<OutlinePanel, &OutlinePanel::ApplyRename>(this));
}

//------------------------------------------------------------------------------
/**
    The name is applied through the command manager, which is what makes it undoable. The command
    takes the name the entity had on the first run, so a redo puts the same name back. The rows are
    built again from the version the scene bumps, which is what makes the row show the new name.
*/
void
OutlinePanel::ApplyRename()
{
    if (this->pendingRenameEntities.IsEmpty())
        return;

    Scene* scene = this->core->GetScene();
    Ptr<EditCommandManager> manager = this->core->GetCommandManager();

    // A rename of several entities is one entry, the same as a drag that moves several
    const bool many = this->pendingRenameEntities.Size() > 1;
    if (many)
        manager->BeginMacro("Rename entities");

    for (IndexT i = 0; i < this->pendingRenameEntities.Size(); i++)
    {
        Ptr<SetEntityNameCommand> command = SetEntityNameCommand::Create();
        command->scene = scene;
        command->entity = this->pendingRenameEntities[i];
        command->newName = this->pendingRenameNames[i];
        manager->Execute(command);
    }

    if (many)
        manager->EndMacro();

    if (many)
    {
        Util::String msg;
        msg.Format("Renamed %d entities to %s", (int)this->pendingRenameEntities.Size(), this->pendingRenameName.AsCharPtr());
        this->core->Log(LogMessageType::N_SYSTEM, msg);
    }
    else
    {
        this->core->Log(LogMessageType::N_SYSTEM, Util::String("Renamed ") + this->pendingRenameName);
    }

    this->pendingRenameEntities.Clear();
    this->pendingRenameNames.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
OutlinePanel::ApplyRenameCollection()
{
    Scene* scene = this->core->GetScene();

    Ptr<SetCollectionNameCommand> command = SetCollectionNameCommand::Create();
    command->scene = scene;
    command->collection = this->pendingRenameCollection;
    command->newName = this->pendingRenameName;
    this->core->GetCommandManager()->Execute(command);

    this->core->Log(LogMessageType::N_SYSTEM, Util::String("Renamed collection ") + this->pendingRenameName);
}

//------------------------------------------------------------------------------
/**
    More than one name can be given, which is the same as selecting several entities with
    Ctrl+click. Without an argument it clears the selection.
*/
void
OutlinePanel::CommandSelect(const Util::Array<Util::String>& args)
{
    Ptr<SelectionContext> selection = this->core->GetSelection();

    if (args.IsEmpty())
    {
        selection->Clear();
        return;
    }

    Util::Array<Game::Entity> entities;
    for (IndexT i = 0; i < args.Size(); i++)
    {
        const Game::Entity entity = this->EntityNamed(args[i]);
        if (entity == Game::Entity::Invalid())
        {
            this->core->Log(LogMessageType::N_WARNING, Util::String("No outline item with name ") + args[i]);
            continue;
        }
        entities.InsertSorted(entity);
    }

    if (entities.IsEmpty())
        return;

    // The selection context applies it through the command manager, which is what makes it undoable
    selection->SetSelection(entities);
}

//------------------------------------------------------------------------------
/**
*/
void
OutlinePanel::CommandCollection(const Util::Array<Util::String>& args)
{
    Scene* scene = this->core->GetScene();

    const Util::String name = args.IsEmpty() ? Util::String("Collection") : args[0];

    // A collection the scene already has is what the command makes active. An instance's
    // collections come
    // from its file, and there is no other way to reach one of them from the console, so a name
    // that is
    // already there is the one the command uses
    const Util::Array<Collection>& collections = scene->GetCollections();
    for (IndexT i = 0; i < collections.Size(); i++)
    {
        if (collections[i].name == name)
        {
            scene->SetActiveCollection(collections[i].guid);
            this->core->Log(LogMessageType::N_SYSTEM, Util::String("Active collection ") + name);
            return;
        }
    }

    Ptr<EditCommandManager> manager = this->core->GetCommandManager();

    Ptr<CreateCollectionCommand> command = CreateCollectionCommand::Create();
    command->scene = scene;
    command->record.name = name;
    command->record.parent = scene->GetActiveCollection();
    manager->Execute(command);

    if (scene->FindCollection(command->record.guid) == InvalidIndex)
    {
        this->core->Log(LogMessageType::N_WARNING, "Collection not created");
        return;
    }

    scene->SetActiveCollection(command->record.guid);
    this->core->Log(LogMessageType::N_SYSTEM, Util::String("Created collection ") + name);
}

//------------------------------------------------------------------------------
/**
*/
void
OutlinePanel::CommandParent(const Util::Array<Util::String>& args)
{
    Scene* scene = this->core->GetScene();
    if (args.IsEmpty())
    {
        this->core->Log(LogMessageType::N_SYSTEM, "Usage: parent <entity> [parent]");
        return;
    }

    const Game::Entity entity = this->EntityNamed(args[0]);
    if (entity == Game::Entity::Invalid())
    {
        this->core->Log(LogMessageType::N_WARNING, "No outline item with that name");
        return;
    }

    Game::Entity parent = Game::Entity::Invalid();
    if (args.Size() >= 2)
    {
        parent = this->EntityNamed(args[1]);
        if (parent == Game::Entity::Invalid())
        {
            this->core->Log(LogMessageType::N_WARNING, "No outline item with that name");
            return;
        }
    }

    Ptr<SetParentCommand> command = SetParentCommand::Create();
    command->scene = scene;
    command->child = entity;
    command->newParent = parent;
    this->core->GetCommandManager()->Execute(command);
}

//------------------------------------------------------------------------------
/**
    Rename the selected entities. Without a name the rename field is opened on the first selected
    entity, which is what the rename key does: the keybind runs this command, and the field is the
    part that gets the name typed into it. With a name the names are applied without a field,
    because the console has no field to open.

    More than one entity is numbered from the base, in the order the selection holds them in.
*/
void
OutlinePanel::CommandRename(const Util::Array<Util::String>& args)
{
    // F2 is the rename key whichever panel is in front, so a panel that renames something of its own,
    // like the asset browser, takes it before the selected entities are renamed
    if (args.IsEmpty() && this->core->OfferEditAction(EditAction::Rename))
        return;

    const Scene* scene = this->core->GetScene();
    const Util::Array<Game::Entity>& selection = this->core->GetSelection()->GetSelection();

    if (selection.IsEmpty())
    {
        this->core->Log(LogMessageType::N_WARNING, "Nothing selected to rename");
        return;
    }

    if (args.IsEmpty())
    {
        // The buffer is only filled when the field is opened, so running the command again while
        // editing does not
        // throw away what has been typed. The field is drawn on the row the entity is on, and
        // RevealSelection
        // opens the path to the selection before the rows are built, which is what gives a
        // selection made
        // outside the panel a row to draw the field on
        if (this->renamingEntity != selection[0])
        {
            scene->GetEditable(selection[0]).name.CopyToBuffer(this->renameBuffer, MaxNameLength);
            this->renamingEntity = selection[0];
            this->renamingCollection = Util::Guid();
        }
        this->renameFocusRequested = true;
        return;
    }

    // The name is the whole argument, which is what makes a name with a space in it work when it is
    // quoted
    Util::String name;
    for (IndexT i = 0; i < args.Size(); i++)
    {
        if (!name.IsEmpty())
            name += " ";
        name += args[i];
    }

    this->QueueRename(selection, name);
}

} // namespace Editor
