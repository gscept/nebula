//------------------------------------------------------------------------------
//  @file inspectorpanel.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "core/cvar.h"
#include "foundation/stdneb.h"
#include "neweditor/panels/inspectorpanel.h"
#include "neweditor/commands/addcomponentcommand.h"
#include "neweditor/commands/deleteentitycommand.h"
#include "neweditor/commands/propertycommand.h"
#include "neweditor/commands/removecomponentcommand.h"
#include "neweditor/commands/revertcomponentcommand.h"
#include "neweditor/commands/revertrecordcommand.h"
#include "neweditor/commands/setcomponentcommand.h"
#include "neweditor/commands/setentitynamecommand.h"
#include "neweditor/commands/unpackprefabcommand.h"
#include "neweditor/base/drawsurface.h"
#include "neweditor/model/editcommandmanager.h"
#include "neweditor/api/shell.h"
#include "neweditor/api/editorfeature.h"
#include "neweditor/base/editorlog.h"
#include "neweditor/model/scene.h"
#include "neweditor/model/selectioncontext.h"
#include "neweditor/viewtools/toolcontext.h"
#include "neweditor/base/iconregistry.h"
#include "game/component.h"
#include "game/componentinspection.h"
#include "game/levelextension.h"
#include "memdb/attributeregistry.h"

namespace Editor
{

__ImplementClass(Editor::InspectorPanel, 'INPL', Editor::EditorPanel);

// The name the add dropdown is opened with. The popup is a window of its own, so the filter and the
// focus are
// drawn inside it rather than on the panel
static const char* AddPopup = "add";


// The menu a component row's prefab marker opens. The marker is pushed onto the component's id, so
// one name is
// enough for every component in the panel
static const char* PrefabOverridePopup = "prefab_override";

// The width the filter costs inside the dropdown. The popup sizes itself to its content, so a
// filter narrower
// than the longest entry name would make the dropdown change width between frames
static const float FilterWidth = 200.0f;

// The width the rename field costs on the title row. A field that is as wide as the name it holds
// would move the
// row while the name is being typed
static const float RenameWidth = 160.0f;

Core::CVar* ed_show_entity_guid;

//------------------------------------------------------------------------------
/**
    The filter is fuzzy: the characters of the filter have to appear in the name in the same order,
    but not next to each other, so 'plr' matches 'Player'. The same rule as the outline's filter.
*/
static bool
FuzzyNameMatches(const char* name, const char* filter)
{
    if (filter[0] == '\0')
        return true;
    if (name[0] == '\0')
        return false;

    const char* nameCursor = name;
    const char* filterCursor = filter;
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
InspectorPanel::InspectorPanel() :
    addOpen(false),
    filterFocusRequested(false),
    highlightedEntry(InvalidIndex),
    pendingAddEntity(Game::Entity::Invalid()),
    pendingRemoveEntity(Game::Entity::Invalid()),
    pendingRemoveComponent(Game::ComponentId::Invalid()),
    pendingRevertEntity(Game::Entity::Invalid()),
    pendingRevertComponent(Game::ComponentId::Invalid()),
    pendingRevertInstance(InvalidIndex),
    pendingRevertRecord(),
    pendingFieldEntity(Game::Entity::Invalid()),
    pendingFieldComponent(Game::ComponentId::Invalid()),
    pendingField(InvalidIndex),
    pendingInstanceEntity(Game::Entity::Invalid()),
    renaming(false),
    renamingEntity(Game::Entity::Invalid()),
    renameFocusRequested(false),
    pendingRenameEntity(Game::Entity::Invalid()),
    pendingSelectParent(Game::Entity::Invalid())
{
    this->name = "inspector";
    this->filterBuffer[0] = 0;
    this->renameBuffer[0] = 0;

    // The cvar is created here rather than by the feature unit, because a game that does not have
    // this panel
    // should not be creating a cvar for it. The outline reads the same cvar and creates it as well,
    // which is
    // what makes either panel work on its own
    ed_show_entity_guid = Core::CVarCreate(Core::CVarType::CVar_Int, "ed_show_entity_guid", "0",
        "Show the entity guid next to the entity name");
}

//------------------------------------------------------------------------------
/**
*/
InspectorPanel::~InspectorPanel()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    The panel shows one entity. A selection of more than one is not something an inspector can show
    without becoming a table, so it says how many there are instead.
*/
void
InspectorPanel::Draw()
{
    // GetComponents is not const, because it reads the table through the database. The panel only
    // reads it
    Scene* scene = this->core->GetScene();

    // The rename field was opened on an entity of the scene it was drawn for, so a scene change
    // closes it
    if (this->renaming && !scene->HasInstance(this->renamingEntity))
    {
        this->renaming = false;
        this->renamingEntity = Game::Entity::Invalid();
        this->renameBuffer[0] = 0;
    }

    const Util::Array<Game::Entity>& selection = this->core->GetSelection()->GetSelection();

    float avail = ImGui::GetContentRegionAvail().x;
    float xStart = ImGui::GetCursorPosX();

    ImGui::Text("INSPECTOR");
    ImGui::SameLine();

    // The entity the panel is drawing, right aligned on the same row as the title, with the rename
    // button after it
    if (selection.Size() == 1 && scene->HasInstance(selection[0]))
    {
        this->DrawEntityName(selection[0]);
    }
    else if (selection.Size() > 1)
    {
        Util::String meta;
        meta.Format("%d entities", (int)selection.Size());

        ImGuiStyle& style = ImGui::GetStyle();
        float size = ImGui::CalcTextSize(meta.AsCharPtr()).x + style.FramePadding.x * 2.0f;
        float off = avail - size;
        if (off > 0.0f)
            ImGui::SetCursorPosX(xStart + off);

        ImGui::TextUnformatted(meta.AsCharPtr());
    }

    ImGui::Separator();

    if (selection.IsEmpty())
    {
        ImGui::TextUnformatted("Nothing selected");
        return;
    }

    if (selection.Size() > 1)
    {
        ImGui::TextUnformatted("The inspector shows one entity at a time");
        return;
    }

    const Game::Entity entity = selection[0];
    if (!scene->HasInstance(entity))
    {
        ImGui::TextUnformatted("The selection is not an authored entity");
        return;
    }

    const Editable& editable = scene->GetEditable(entity);

    this->DrawSection("General");

    ImGui::PushStyleVarY(ImGuiStyleVar_CellPadding, -4);
    if (Editor::DrawBeginTable("general"))
    {
        Util::String value;
        value.Format("0x%012llX", (unsigned long long)Ids::Id64(entity));
        this->DrawRow("Id", value);
        if (Core::CVarReadInt(ed_show_entity_guid) > 0)
        {
            this->DrawRow("Guid", editable.guid.AsString(), true);
        }
        this->DrawRow("Collection", this->CollectionName(editable.collection));
        this->DrawParent(entity);

        Editor::DrawEndTable();
    }
    ImGui::PopStyleVar();

    this->DrawAdd(entity);

    // An instance entity says what it is an instance of before its own values, because most of
    // what the panel shows below belongs to that file rather than to this scene
    if (scene->IsPrefabInstanceEntity(entity))
        this->DrawPrefab(entity);

    this->DrawTransform(entity);

    // GetComponents is not const, because it reads the table through the database. The panel only
    // reads it
    const Util::Array<Game::ComponentId>& components = scene->GetComponents(entity);

    for (IndexT i = 0; i < components.Size(); i++)
    {
        if (this->IsTransformComponent(components[i]))
            continue;

        if (components[i] == Game::GetComponentId<Game::Entity>())
            continue;

        if (EditorFeatureRegistry::IsRuntimeComponent(components[i]))
            continue;

        this->DrawComponent(entity, components[i]);
    }

    // The sections the features contribute. The panel has read everything it can read by now, so it
    // hands the
    // entity to each section and lets the feature that produced the data draw its own heading and
    // its own rows
    // through the surface. A section that draws nothing is a feature that has nothing to say about
    // this entity
    const Util::Array<EditorInspectorSection>& sections = EditorFeatureRegistry::GetSections();
    for (IndexT i = 0; i < sections.Size(); i++)
    {
        if (!sections[i].draw.IsValid())
            continue;

        sections[i].draw(entity);
    }

    // The edit is applied when the widget that changed the buffer is done with it, rather than on
    // the frame it
    // changed it, which is what makes a drag one undo entry. The command mirrors the value into the
    // sim world, so
    // the frame the drag ends is also the frame the preview stops being one
    if (!this->pendingEdits.IsEmpty() && !ImGui::IsAnyItemActive())
        this->QueueAction(Util::Delegate<void()>::FromMethod<InspectorPanel, &InspectorPanel::ApplyEdits>(this));
}

//------------------------------------------------------------------------------
/**
    The name of the entity the panel is drawing, right aligned on the row of the title, with the
    rename button after it.

    The button opens the field rather than renaming, because immediate mode has no element to get a
    commit from. The name is applied when Enter is pressed or when the field loses focus after an
    edit, and Escape closes the field without applying what was typed.
*/
void
InspectorPanel::DrawEntityName(Game::Entity entity)
{
    const Scene* scene = this->core->GetScene();
    const Editable& editable = scene->GetEditable(entity);

    if (this->renaming && this->renamingEntity != entity)
    {
        // The field was opened on another entity, so what its buffer holds is not something this
        // entity can be
        // given
        this->renaming = false;
        this->renameBuffer[0] = 0;
    }

    // The rename button is the glyph rather than the word. Its frame is pushed rather than set in
    // the style,
    // because the other buttons in the panel keep theirs. The hover and the active colors are left
    // alone, so the
    // glyph still reads as something clickable with nothing drawn on it at rest
    const char* glyph = IconGlyph("pencil-line");
    const char* label = glyph != nullptr ? glyph : "Rename";

    ImGuiStyle& style = ImGui::GetStyle();
    float buttonWidth = ImGui::CalcTextSize(label).x + style.FramePadding.x * 2.0f;
    float nameWidth = this->renaming ? RenameWidth : ImGui::CalcTextSize(editable.name.AsCharPtr()).x;

    float off = ImGui::GetContentRegionAvail().x - nameWidth - style.ItemSpacing.x - buttonWidth;
    if (off > 0.0f)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

    if (this->renaming)
    {
        if (this->renameFocusRequested)
        {
            ImGui::SetKeyboardFocusHere();
            this->renameFocusRequested = false;
        }

        float width = nameWidth;
        if (width > ImGui::GetContentRegionAvail().x)
            width = ImGui::GetContentRegionAvail().x;

        // The field is read after it is drawn, because the row has to end with an item rather than
        // with the cursor
        // move that right aligned it
        ImGui::PushItemWidth(width);
        const bool committed = ImGui::InputTextWithHint("##entity_name", "Name", this->renameBuffer, MaxNameLength, ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::PopItemWidth();

        // Escape is the field's own while it has focus: it puts the buffer back to what the field
        // was opened with
        // and deactivates it. That is the frame the field closes on, and the name it holds then is
        // the one the
        // entity already has, which is not a change
        if (committed || ImGui::IsItemDeactivated())
            this->CommitRename(entity);
    }
    else
    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(editable.name.AsCharPtr());
    }

    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

    if (IconButton(glyph, "Rename"))
    {
        // The buffer is only filled when the field is opened, so pressing it again while editing
        // does not throw
        // away what has been typed
        if (!this->renaming)
        {
            editable.name.CopyToBuffer(this->renameBuffer, MaxNameLength);
            this->renaming = true;
            this->renamingEntity = entity;
        }
        this->renameFocusRequested = true;
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

//------------------------------------------------------------------------------
/**
    One component as a section of its own.
*/
void
InspectorPanel::DrawComponent(Game::Entity entity, Game::ComponentId component)
{
    Game::ComponentInterface* cInterface =
        static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(component));
    if (cInterface == nullptr)
        return;

    this->DrawSection(cInterface->GetName(), entity, component);

    if (cInterface->GetNumFields() == 0)
    {
        ImGui::TextDisabled("flag");
        return;
    }

    if (Editor::DrawBeginTable(cInterface->GetName()))
    {
        this->DrawComponentRow(entity, component);

        Editor::DrawEndTable();
    }
}

//------------------------------------------------------------------------------
/**
    The transform as one section. Position, orientation and scale are three components, but every
    entity has them, so they are drawn together rather than as three sections. The rows are still
    the inspection system's, which is what keeps the widgets the same as they are everywhere else.
*/
void
InspectorPanel::DrawTransform(Game::Entity entity)
{
    const Game::ComponentId transform[3] = {
        Game::GetComponentId<Game::Position>(),
        Game::GetComponentId<Game::Orientation>(),
        Game::GetComponentId<Game::Scale>(),
    };

    this->DrawSection("Transform");

    if (Editor::DrawBeginTable("transform"))
    {
        for (int i = 0; i < 3; i++)
        {
            if (this->HasComponent(entity, transform[i]))
                this->DrawComponentRow(entity, transform[i]);
        }

        Editor::DrawEndTable();
    }
}

//------------------------------------------------------------------------------
/**
    The link one entity has to a scene file, and the two ways to be rid of it: unpack, which keeps
    every value the entity has and drops the link, and revert, which gives back what the level
    authored over the file.

    A component the instance removed against its chain has no section of its own to reset from, so
    the row that names it carries the button.
*/
void
InspectorPanel::DrawPrefab(Game::Entity entity)
{
    Scene* scene = this->core->GetScene();
    const PrefabInstance& instance = scene->GetPrefabInstance(entity);
    const Editable& editable = scene->GetEditable(entity);

    this->DrawSection("Prefab");

    ImGui::PushStyleVarY(ImGuiStyleVar_CellPadding, -4);
    if (Editor::DrawBeginTable("prefab"))
    {
        this->DrawRow("Source", instance.source, true);

        // The records above the immediate file. A single-hop instance has nothing to add here,
        // which
        // is what keeps an ordinary prefab looking the way it does today. The name is the one the
        // record has in the prefab file, not the name this level gave the entity
        Util::Array<ChainRecord> chain;
        scene->GetPrefabChain(entity, chain);
        for (IndexT i = 1; i < chain.Size(); i++)
        {
            Util::String hop;
            hop.Format("%s / %s", chain[i].source.AsCharPtr(), chain[i].record.AsCharPtr());
            this->DrawRow("Chain", hop, true);
        }

        for (IndexT i = 0; i < editable.prefabOverrides.Size(); i++)
        {
            const PrefabOverride& override = editable.prefabOverrides[i];
            if (!override.componentRemoved)
                continue;

            Game::ComponentInterface* interface =
                static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(override.component));
            const char* name = interface != nullptr ? interface->GetName() : "component";

            this->DrawRow("Removed", name);

            // The button is drawn into the cell the row's name is in, because the reset belongs to
            // the component the row names
            ImGui::SameLine();
            ImGui::PushID((int)override.component.id);
            if (IconButton(IconGlyph("rotate-ccw"), "Reset"))
            {
                this->pendingRevertEntity = entity;
                this->pendingRevertComponent = override.component;
                this->QueueAction(Util::Delegate<void()>::FromMethod<InspectorPanel, &InspectorPanel::ApplyRevertComponent>(this));
            }
            ImGui::PopID();
        }

        // The entity set belongs to the instance, so what the instance added and what it does not
        // have are
        // listed here rather than on a row per entity. A marker names a record of the file, and
        // there is no
        // entity for the panel to draw a row for
        if (!editable.prefabEntity.IsValid())
        {
            this->DrawRow("Added", editable.name);

            ImGui::SameLine();
            ImGui::PushID((int)entity.index);
            if (IconButton(IconGlyph("trash-2"), "Remove"))
            {
                this->pendingRevertEntity = entity;
                this->QueueAction(Util::Delegate<void()>::FromMethod<InspectorPanel, &InspectorPanel::ApplyRemoveAddition>(this));
            }
            ImGui::PopID();
        }

        const IndexT instanceIndex = editable.prefabInstance;
        for (IndexT i = 0; i < instance.removedRecords.Size(); i++)
        {
            const RemovedRecord& marker = instance.removedRecords[i];

            this->DrawRow("Removed", marker.name);

            ImGui::SameLine();
            ImGui::PushID((int)i);
            if (IconButton(IconGlyph("rotate-ccw"), "Reset"))
            {
                this->pendingRevertInstance = instanceIndex;
                this->pendingRevertRecord = marker.record;
                this->QueueAction(Util::Delegate<void()>::FromMethod<InspectorPanel, &InspectorPanel::ApplyRevertRecord>(this));
            }
            ImGui::PopID();
        }

        Editor::DrawEndTable();
    }
    ImGui::PopStyleVar();

    ImGui::Spacing();

    // What the level authored over the prefab goes back into the prefab file. The write is
    // not an undoable action: it changes a file rather than the scene
    if (IconButton(IconGlyph("upload"), "Apply All"))
    {
        this->pendingInstanceEntity = entity;
        this->QueueAction(Util::Delegate<void()>::FromMethod<InspectorPanel, &InspectorPanel::ApplyToPrefab>(this));
    }

    ImGui::SameLine();

    // Both actions are commands, which is what makes them undoable. Unpack is the whole
    // instantiation, and so is the revert: what the level authored anywhere in it goes back
    if (IconButton(IconGlyph("unlink"), "Unpack"))
    {
        this->pendingInstanceEntity = entity;
        this->QueueAction(Util::Delegate<void()>::FromMethod<InspectorPanel, &InspectorPanel::ApplyUnpack>(this));
    }

    ImGui::SameLine();

    if (IconButton(IconGlyph("rotate-ccw"), "Revert All"))
    {
        this->pendingInstanceEntity = entity;
        this->QueueAction(Util::Delegate<void()>::FromMethod<InspectorPanel, &InspectorPanel::ApplyRevertInstance>(this));
    }
}

//------------------------------------------------------------------------------
/**
    The marker that says an instance entity wrote over what its prefab chain gives, and the menu
    that gives a part of it back. The marker is drawn into the component's own row, because the
    override is a fact about that component rather than about the entity.

    A reset of one field writes only that field's bytes from the chain, and a reset of the component
    writes the whole component, which also puts back one the instance removed and takes away one it
    added.
*/
void
InspectorPanel::DrawPrefabMarker(Game::Entity entity, Game::ComponentId component)
{
    Scene* scene = this->core->GetScene();
    const PrefabOverride& state = scene->FindPrefabOverride(entity, component);
    if (state.component != component ||
        (state.overriddenFields.IsNull() && !state.componentAdded && !state.componentRemoved))
        return;

    Game::ComponentInterface* interface =
        static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(component));
    if (interface == nullptr)
        return;

    ImGui::SameLine();
    ImGui::PushID((int)component.id);

    const char* glyph = IconGlyph("circle-dot");
    const bool pressed = ImGui::Button(glyph != nullptr ? glyph : "override");
    if (pressed)
        ImGui::OpenPopup(PrefabOverridePopup);

    // The widget beside the marker shows the value the entity has, which says nothing about where
    // it came from. The tooltip is where the panel says which parts the level wrote itself
    {
        Util::String tip;
        for (IndexT field = 0; field < (IndexT)interface->GetNumFields(); field++)
        {
            if (!state.overriddenFields.IsSet((uint64_t)field))
                continue;

            if (!tip.IsEmpty())
                tip += ", ";
            tip += interface->GetFieldNames()[field];
        }
        if (state.componentAdded)
        {
            if (!tip.IsEmpty())
                tip += ", ";
            tip += "added by this scene";
        }

        ImGui::SetItemTooltip("%s", tip.IsEmpty() ? "overridden" : tip.AsCharPtr());
    }

    if (ImGui::BeginPopup(PrefabOverridePopup))
    {
        if (ImGui::MenuItem("Reset to Prefab"))
        {
            this->pendingRevertEntity = entity;
            this->pendingRevertComponent = component;
            this->QueueAction(Util::Delegate<void()>::FromMethod<InspectorPanel, &InspectorPanel::ApplyRevertComponent>(this));
        }

        for (IndexT field = 0; field < (IndexT)interface->GetNumFields(); field++)
        {
            if (!state.overriddenFields.IsSet((uint64_t)field))
                continue;

            Util::String item;
            item.Format("Reset %s to Prefab", interface->GetFieldNames()[field]);
            if (ImGui::MenuItem(item.AsCharPtr()))
            {
                this->pendingFieldEntity = entity;
                this->pendingFieldComponent = component;
                this->pendingField = field;
                this->QueueAction(Util::Delegate<void()>::FromMethod<InspectorPanel, &InspectorPanel::ApplyRevertField>(this));
            }
        }

        ImGui::EndPopup();
    }

    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
    The mark that says a field of this component comes from a file above the one the instance was
    made from. The marker says what this scene wrote, and this says what the chain gave, which is
    the other half of where a value came from. Plain text rather than a glyph, because the icon
    table has no asterisk and the font would have to carry one.
*/
void
InspectorPanel::DrawChainSource(Game::Entity entity, Game::ComponentId component)
{
    Scene* scene = this->core->GetScene();
    const PrefabInstance& instance = scene->GetPrefabInstance(entity);

    Game::ComponentInterface* interface =
        static_cast<Game::ComponentInterface*>(MemDb::AttributeRegistry::GetAttribute(component));
    if (interface == nullptr)
        return;

    Util::Array<ChainRecord> sources;
    scene->GetFieldSources(entity, component, sources);

    bool above = false;
    Util::String tip;
    for (IndexT field = 0; field < sources.Size(); field++)
    {
        // An empty entry is a field this scene wrote itself, and the immediate file is what the
        // Source row already shows, so neither is something the row has to say again
        if (sources[field].source.IsEmpty() || sources[field].source == instance.source)
            continue;

        above = true;
        if (!tip.IsEmpty())
            tip += "\n";
        tip += interface->GetFieldNames()[field];
        tip += " from ";
        tip += sources[field].source;
        tip += " / ";
        tip += sources[field].record;
    }
    if (!above)
        return;

    ImGui::SameLine();
    ImGui::PushID((int)component.id);
    ImGui::Button("*");
    ImGui::SetItemTooltip("%s", tip.AsCharPtr());
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
    The add component button and the dropdown it opens. The dropdown lists the components an entity
    can be given, and the ones it already has are drawn disabled with a checkmark, because giving an
    entity a component it has is not an action.

    The collider entries are drawn disabled without a checkmark when the entity already has a
    collider, because an entity is only allowed one and the physics manager asserts when it gets a
    second. Whether the entity has one is the same answer for every collider entry, so it is taken
    from the entity's table signature once rather than asked for component by component. The entries
    are grouped by the namespace the component is in, and the group is opened by a separator. The
    list is sorted by namespace and then name, because the registry is ordered by the attribute id
    and a namespace can be scattered through it. The separator is drawn on the row the group starts
    on, so a filter that leaves out the top of a group still opens it.

    The filter is the panel's rather than ImGui's, because immediate mode has no element to hold it
    between frames. It is cleared when the dropdown closes, so the dropdown starts empty every time
    it opens. Escape is one of those: it cancels the dropdown rather than picking. A pick with the
    keyboard keeps it, because the dropdown stays open and the same filter is what the next pick is
    made from.

    The popup is a window of its own, so the focus has to be asked for inside it, on the frame it
    opens, and the filter keeps it. That is why the highlight the keyboard moves is the panel's
    rather than ImGui's: there is no element to put focus on. Typing puts it on the top entry, the
    arrows and tab cycle it, and Enter adds the entry it is on and asks for the filter again,
    because Enter is also what deactivates a field. The mouse keeps ImGui's own highlight, and a
    click adds the row it is on and closes the dropdown.
*/
void
InspectorPanel::DrawAdd(Game::Entity entity)
{
    ImGui::Spacing();

    // The button is centered on the panel, because it is the thing that opens the dropdown rather
    // than a row of
    // the content. The width it costs is the label plus the frame padding on both sides
    ImGuiStyle& style = ImGui::GetStyle();
    float width = ImGui::CalcTextSize("Add").x + style.FramePadding.x * 2.0f;
    float off = (ImGui::GetContentRegionAvail().x - width) * 0.5f;
    if (off > 0.0f)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

    if (ImGui::Button("Add"))
    {
        ImGui::OpenPopup(AddPopup);
        this->filterFocusRequested = true;
    }

    if (!ImGui::BeginPopup(AddPopup))
    {
        // The frame BeginPopup does not open is the frame the popup was closed. When it was open
        // the frame before,
        // this is the frame it was cancelled, and the filter and the highlight go with it
        if (this->addOpen)
        {
            this->filterBuffer[0] = 0;
            this->highlightedEntry = InvalidIndex;
        }
        this->addOpen = false;
        return;
    }

    this->addOpen = true;

    if (this->filterFocusRequested)
    {
        ImGui::SetKeyboardFocusHere();
        this->filterFocusRequested = false;
    }

    ImGui::PushItemWidth(FilterWidth);
    const bool filterEdited = ImGui::InputTextWithHint("##add_filter", "Filter entries...", this->filterBuffer, MaxFilterLength);
    ImGui::PopItemWidth();

    // The entries the dropdown shows. The list is built again every frame, because the filter is
    // what decides
    // which of them are in it, and the highlight is an index into it
    const Util::FixedArray<MemDb::Attribute*>& attributes = MemDb::AttributeRegistry::GetAllAttributes();
    this->dropdownEntries.Clear();
    for (IndexT i = 0; i < attributes.Size(); i++)
    {
        // The array is indexed by the attribute id, so a slot can be one that was never registered
        MemDb::Attribute* attr = attributes[i];
        if (attr == nullptr || !attr->name.IsValid())
            continue;

        const Game::ComponentId component = Game::GetComponentId(attr->name);
        if (!this->IsAddableComponent(component))
            continue;

        Game::ComponentInterface* interface = static_cast<Game::ComponentInterface*>(attr);

        // attr->name is the registry key, which is the full name. The panel shows and
        // filters on the component name on its own, with the namespace as the group
        if (!FuzzyNameMatches(interface->GetName(), this->filterBuffer))
            continue;

        InspectorEntry entry;
        entry.component = component;
        entry.name = interface->GetName();
        const char* ns = interface->GetNamespace();
        entry.group = ns != nullptr ? ns : "";
        entry.isComponent = true;
        entry.present = this->HasComponent(entity, component);
        entry.blocked = this->core->IsBlocked(entity, component);
        this->dropdownEntries.Append(entry);
    }

    // What the editor has no type knowledge of is listed by the feature that produced it. The
    // entries come back
    // unfiltered and the panel filters them here, because the fuzzy match is one rule and the panel
    // should not
    // have two
    const Util::Array<EditorInspectorAddEntrySource>& sources = EditorFeatureRegistry::GetEntrySources();
    for (IndexT i = 0; i < sources.Size(); i++)
    {
        if (!sources[i].collect.IsValid())
            continue;

        Util::Array<InspectorEntry> contributed;
        sources[i].collect(entity, contributed);

        for (IndexT k = 0; k < contributed.Size(); k++)
        {
            if (!FuzzyNameMatches(contributed[k].name.AsCharPtr(), this->filterBuffer))
                continue;

            this->dropdownEntries.Append(contributed[k]);
        }
    }

    this->dropdownEntries.SortWithFunc(&InspectorPanel::EntryOrder);

    // Typing puts the highlight on the top entry that can be added, and a highlight that is not in
    // the list is not
    // a highlight
    if (filterEdited || this->highlightedEntry == InvalidIndex || this->highlightedEntry >= (IndexT)this->dropdownEntries.Size())
    {
        this->highlightedEntry = InvalidIndex;
        this->MoveHighlight(1);
    }

    // Escape cancels the dropdown. ImGui only closes a popup on Escape when keyboard navigation is
    // enabled, which
    // this editor does not enable, so the panel closes it itself. The route is global, so that the
    // key reaches the
    // popup whether or not its window is the focused one. The filter owns the left and the right
    // arrows, but
    // neither the up and the down nor tab, so those are the panel's while the input has focus
    if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteGlobal))
    {
        this->filterBuffer[0] = 0;
        this->highlightedEntry = InvalidIndex;
        ImGui::CloseCurrentPopup();
    }
    else if (!this->dropdownEntries.IsEmpty())
    {
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) || ImGui::IsKeyPressed(ImGuiKey_Tab))
            this->MoveHighlight(1);
        else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
            this->MoveHighlight(-1);
        else if (ImGui::IsKeyPressed(ImGuiKey_Enter) && this->highlightedEntry != InvalidIndex)
        {
            const InspectorEntry& entry = this->dropdownEntries[this->highlightedEntry];

            this->QueueAddEntry(entity, entry);

            this->filterFocusRequested = true;

            this->dropdownEntries[this->highlightedEntry].present = true;
            this->MoveHighlight(1);
        }
    }

    ImGui::Separator();

    const char* group = nullptr;

    for (IndexT i = 0; i < (IndexT)this->dropdownEntries.Size(); i++)
    {
        const InspectorEntry& entry = this->dropdownEntries[i];

        if (group == nullptr || Util::String::StrICmp(group, entry.group.AsCharPtr()) != 0)
        {
            group = entry.group.AsCharPtr();
            if (group[0] != '\0')
                ImGui::SeparatorText(group);
        }

        // The highlight is drawn before the row, because the row draws its background and then its
        // text over
        // whatever was there before it
        if (i == this->highlightedEntry)
        {
            ImVec2 min = ImGui::GetCursorScreenPos();
            ImVec2 max = ImVec2(min.x + ImGui::GetContentRegionAvail().x, min.y + ImGui::GetTextLineHeight());
            ImGui::GetWindowDrawList()->AddRectFilled(min, max, ImGui::GetColorU32(ImGuiCol_Header));
        }

        if (ImGui::MenuItem(entry.name.AsCharPtr(), nullptr, entry.present, !entry.present && !entry.blocked))
        {
            this->QueueAddEntry(entity, entry);

            this->filterBuffer[0] = 0;
            this->highlightedEntry = InvalidIndex;
            ImGui::CloseCurrentPopup();
        }
    }

    if (this->dropdownEntries.IsEmpty())
        ImGui::TextDisabled("No entry matches the filter");

    ImGui::EndPopup();
}

//------------------------------------------------------------------------------
/**
    The kind is sorted first, so the components and what the editor cannot read come out as two
    groups rather than being interleaved by name. Within a kind the entries are ordered the way the
    component list always was
*/
bool
InspectorPanel::EntryOrder(const InspectorEntry& lhs, const InspectorEntry& rhs)
{
    if (lhs.isComponent != rhs.isComponent)
        return lhs.isComponent;

    const int byGroup = Util::String::StrICmp(lhs.group.AsCharPtr(), rhs.group.AsCharPtr());
    if (byGroup != 0)
        return byGroup < 0;

    return Util::String::StrICmp(lhs.name.AsCharPtr(), rhs.name.AsCharPtr()) < 0;
}

//------------------------------------------------------------------------------
/**
    The highlight only lands on an entry that can be added, so the ones the entity already has and
    the ones it cannot be given are stepped over. Present draws a checkmark and blocked does not,
    but neither is something the keyboard can pick. The wrap is what makes the list a cycle, and the
    guard is what keeps a list that is all disabled from cycling forever. A list that is all
    disabled has no highlight left, because there is nothing left to pick.
*/
void
InspectorPanel::MoveHighlight(int direction)
{
    const SizeT num = this->dropdownEntries.Size();
    if (num == 0)
        return;

    IndexT index = this->highlightedEntry == InvalidIndex ? 0 : this->highlightedEntry + direction;
    for (SizeT guard = 0; guard < num; guard++)
    {
        index = (IndexT)(((index % num) + num) % num);
        if (!this->dropdownEntries[index].present && !this->dropdownEntries[index].blocked)
        {
            this->highlightedEntry = index;
            return;
        }
        index += direction;
    }

    this->highlightedEntry = InvalidIndex;
}

//------------------------------------------------------------------------------
/**
    A component picked from the dropdown is queued rather than added during the draw, because the
    list the panel is drawing is the one it was built from.
*/
void
InspectorPanel::QueueAddEntry(Game::Entity entity, const InspectorEntry& entry)
{
    this->pendingAddEntity = entity;
    this->pendingAddEntry = entry;
    this->QueueAction(Util::Delegate<void()>::FromMethod<InspectorPanel, &InspectorPanel::ApplyAddEntry>(this));
}

//------------------------------------------------------------------------------
/**
    One component as a row in the table that is open. The inspection system draws the fields into
    the buffer it is given, so the panel gives it its own copy of the authored data rather than the
    data itself. What the copy holds after the draw is the value the widget wrote, and that is what
    gets queued.

    While a component is being edited, the copy is the pending edit rather than a fresh read of the
    scene, because the widget's value has to survive from frame to frame for as long as the drag
    runs.
*/
void
InspectorPanel::DrawComponentRow(Game::Entity entity, Game::ComponentId component)
{
    Scene* scene = this->core->GetScene();

    const SizeT typeSize = MemDb::AttributeRegistry::TypeSize(component);
    void* data = scene->GetComponentData(entity, component);
    if (data == nullptr || typeSize == 0)
        return;

    // A drag is not done until the button is released, so the value the widget is working on has to
    // survive from
    // frame to frame. Copying from the scene every frame would throw away what the widget wrote,
    // which is what
    // makes the value snap back to what the entity has
    const IndexT index = this->pendingEdits.FindIndex(component);
    if (index != InvalidIndex)
    {
        const Util::Blob& value = this->pendingEdits.ValueAtIndex(index).value;
        this->buffer.Set(value.GetPtr(), value.Size());
    }
    else
    {
        this->buffer.Set(data, typeSize);
    }

    ImGui::TableNextRow();

    bool commit = false;
    Game::ComponentInspection::DrawInspector(entity, component, this->buffer.GetPtr(), &commit);

    if (commit)
    {
        PendingEdit& edit = this->pendingEdits.Emplace(component);
        edit.entity = entity;
        edit.value.Set(this->buffer.GetPtr(), this->buffer.Size());

        if (Scene::IsTransformComponent(component))
            scene->PreviewComponent(entity, component, this->buffer.GetPtr());
    }

    // The marker is drawn into the cell the inspection system just drew, which is what puts it
    // after the widget rather than on a row of its own
    if (scene->IsPrefabInstanceEntity(entity))
    {
        this->DrawChainSource(entity, component);
        this->DrawPrefabMarker(entity, component);
    }
}

//------------------------------------------------------------------------------
/**
    The values the widgets wrote are applied through the command manager, which is what makes them
    undoable. The command takes the value as it was at the time of the draw, so the buffer can be
    reused by the next component.
*/
void
InspectorPanel::ApplyEdits()
{
    Ptr<EditCommandManager> manager = this->core->GetCommandManager();

    for (IndexT i = 0; i < this->pendingEdits.Size(); i++)
    {
        const Game::ComponentId component = this->pendingEdits.KeyAtIndex(i);
        const PendingEdit& edit = this->pendingEdits.ValueAtIndex(i);
        if (!edit.value.IsValid())
            continue;

        Ptr<SetComponentCommand> command = SetComponentCommand::Create();
        command->scene = this->core->GetScene();
        command->entity = edit.entity;
        command->component = component;
        command->newValue = edit.value;
        manager->Execute(command);
    }

    this->pendingEdits.Clear();
}

//------------------------------------------------------------------------------
/**
    The entry picked from the dropdown, added through the command manager, which is what makes it
    undoable.

    A component is something the panel can add itself, and the value is the default one because
    there is nothing on the entity to copy from yet.

    Anything else is something the panel cannot read, so it goes through the same command the
    recorder uses. The command holds the entity's text, so the before text is what the entity has
    now and the after text is that with the default the entry source produces put over it. The undo
    of this is the same command running its before side, which is what takes the entry off again.

    The keys the default text carries are the ones the instance authored, because an entry the
    instance added is its own, and the marker has to say so. Without the marker the next save writes
    nothing for it, and a linked entity that writes nothing for it loses it.

    This runs after the frame, so the guid map has to be the scene's own again: the draw's context
    is only valid for the draw that set it.
*/
void
InspectorPanel::ApplyAddEntry()
{
    Scene* scene = this->core->GetScene();

    if (this->pendingAddEntry.isComponent)
    {
        Ptr<AddComponentCommand> command = AddComponentCommand::Create();
        command->scene = scene;
        command->entity = this->pendingAddEntity;
        command->component = this->pendingAddEntry.component;
        this->core->GetCommandManager()->Execute(command);
        return;
    }

    const Game::LevelExtension* ext = Game::LevelExtensionRegistry::GetExtension(this->pendingAddEntry.extension);
    if (ext == nullptr || ext->capture == nullptr || ext->merge == nullptr)
        return;

    Util::HashTable<Util::Guid, Game::Entity> map;
    scene->BuildSceneMap(map);
    scene->SetRefContext(map);

    Util::String defaultText;
    if (this->pendingAddEntry.defaultText.IsValid())
        this->pendingAddEntry.defaultText(this->pendingAddEntry.name.AsCharPtr(), defaultText);
    if (defaultText.IsEmpty())
        return;

    Ptr<PropertyCommand> command = PropertyCommand::Create();
    command->scene = scene;
    command->entity = this->pendingAddEntity;
    command->extension = this->pendingAddEntry.extension;
    command->path = this->pendingAddEntry.name;

    ext->capture(this->pendingAddEntity, command->oldText);
    ext->merge(command->oldText.AsCharPtr(), defaultText.AsCharPtr(), command->newText);

    // Only a linked entity has a marker. An entity that is its own carries every field, so there is
    // nothing to
    // say about which part of it it authored
    if (scene->IsPrefabInstanceEntity(this->pendingAddEntity))
    {
        // The marker is written whole by the command, so the keys the entity authored before this
        // add have to
        // be carried over. Starting from empty would make the add write a marker with only the new
        // entry's
        // keys, and the fields authored before it would start following the prefab
        command->oldFields = scene->FindExtensionOverride(this->pendingAddEntity, this->pendingAddEntry.extension).fields;
        command->newFields = command->oldFields;

        Util::Array<Util::String> added;
        if (ext->keys != nullptr)
            ext->keys(defaultText.AsCharPtr(), added);

        for (IndexT i = 0; i < added.Size(); i++)
        {
            bool already = false;
            for (IndexT f = 0; f < command->newFields.Size(); f++)
            {
                if (command->newFields[f] == added[i])
                {
                    already = true;
                    break;
                }
            }
            if (!already)
                command->newFields.Append(added[i]);
        }
    }

    this->core->GetCommandManager()->Execute(command);
}

//------------------------------------------------------------------------------
/**
    The component a remove button was pressed on is removed through the command manager, which is
    what makes it undoable. The command reads the value before the component is gone, so the undo
    puts it back with the value it had.
*/
void
InspectorPanel::ApplyRemoveComponent()
{
    Ptr<EditCommandManager> manager = this->core->GetCommandManager();

    Ptr<RemoveComponentCommand> command = RemoveComponentCommand::Create();
    command->scene = this->core->GetScene();
    command->entity = this->pendingRemoveEntity;
    command->component = this->pendingRemoveComponent;
    manager->Execute(command);
}

//------------------------------------------------------------------------------
/**
    The component a Reset to Prefab was pressed on goes back to what the chain gives, through the
    command that remembers the value and the override state the entity had.
*/
void
InspectorPanel::ApplyRevertComponent()
{
    Ptr<RevertComponentCommand> command = RevertComponentCommand::Create();
    command->scene = this->core->GetScene();
    command->entity = this->pendingRevertEntity;
    command->component = this->pendingRevertComponent;
    this->core->GetCommandManager()->Execute(command);
}

//------------------------------------------------------------------------------
/**
    A record the instance marked removed goes back through the command that keeps the entity id,
    which is what makes the undo put the same entity back.
*/
void
InspectorPanel::ApplyRevertRecord()
{
    Ptr<RevertRecordCommand> command = RevertRecordCommand::Create();
    command->scene = this->core->GetScene();
    command->instanceIndex = this->pendingRevertInstance;
    command->record = this->pendingRevertRecord;
    this->core->GetCommandManager()->Execute(command);
}

//------------------------------------------------------------------------------
/**
    An entity the instance added has no record, so taking it off the instance is the same as
    deleting it, and the delete is what the command manager makes undoable.
*/
void
InspectorPanel::ApplyRemoveAddition()
{
    Util::Array<Game::Entity> entities;
    entities.Append(this->pendingRevertEntity);
    this->core->DeleteEntities(entities);
}

//------------------------------------------------------------------------------
/**
    One field of a component goes back by writing the chain's bytes for that field into the value
    the entity has and leaving the rest of the value alone. The scene recomputes what is overridden
    from the value that arrives, so the field drops out of the override state by itself, and the
    command is the same one a widget's edit goes through.
*/
void
InspectorPanel::ApplyRevertField()
{
    Scene* scene = this->core->GetScene();
    if (!scene->HasComponent(this->pendingFieldEntity, this->pendingFieldComponent))
        return;

    Util::Blob base;
    if (!scene->GetPrefabBaseValue(this->pendingFieldEntity, this->pendingFieldComponent, base))
        return;

    Game::ComponentInterface* interface = static_cast<Game::ComponentInterface*>(
        MemDb::AttributeRegistry::GetAttribute(this->pendingFieldComponent));
    if (interface == nullptr || this->pendingField < 0 || this->pendingField >= (IndexT)interface->GetNumFields())
        return;

    const void* current = scene->GetComponentData(this->pendingFieldEntity, this->pendingFieldComponent);
    const SizeT typeSize = MemDb::AttributeRegistry::TypeSize(this->pendingFieldComponent);
    if (current == nullptr || typeSize == 0)
        return;

    Util::Blob value(current, typeSize);

    // The field runs to the next one, and the last one to the end of the component, which is the
    // field together with the padding that belongs to it
    const SizeT offset = (SizeT)interface->GetFieldByteOffsets()[this->pendingField];
    const SizeT size = this->pendingField + 1 < (IndexT)interface->GetNumFields()
        ? (SizeT)interface->GetFieldByteOffsets()[this->pendingField + 1] - offset
        : typeSize - offset;

    Memory::Copy((const byte*)base.GetPtr() + offset, (byte*)value.GetPtr() + offset, size);

    Ptr<SetComponentCommand> command = SetComponentCommand::Create();
    command->scene = scene;
    command->entity = this->pendingFieldEntity;
    command->component = this->pendingFieldComponent;
    command->newValue = value;
    this->core->GetCommandManager()->Execute(command);
}

//------------------------------------------------------------------------------
/**
    Every override in the instantiation goes back. The components are collected before any of them
    is reverted, because a revert writes the override state of the entity it works on, and the list
    is read from that same state. The reverts run as one macro, which is what makes one undo put the
    whole instantiation back.

    The managed properties are part of the same macro. A property is reverted through the same
    command a field change runs, with the text the chain gives for every key the entity carries and
    the marker emptied. A property the chain has nothing for is one the instance authored on its
    own, and it goes with the marker.
*/
void
InspectorPanel::ApplyRevertInstance()
{
    Scene* scene = this->core->GetScene();

    Util::Array<Game::Entity> entities;
    scene->GetInstanceEntities(this->pendingInstanceEntity, entities);

    Util::Array<Game::Entity> revertEntities;
    Util::Array<Game::ComponentId> revertComponents;

    // The managed properties. A marker entry is one per extension, and the property a key names is
    // the part
    // before the first dot. The entries are collected whole rather than read while the commands
    // run, because a
    // command writes the marker the list is read from
    Util::Array<Game::Entity> propertyEntities;
    Util::Array<PrefabOverride> propertyOverrides;

    for (IndexT i = 0; i < entities.Size(); i++)
    {
        const Editable& editable = scene->GetEditable(entities[i]);
        for (IndexT o = 0; o < editable.prefabOverrides.Size(); o++)
        {
            const PrefabOverride& override = editable.prefabOverrides[o];

            if (override.extension.IsValid())
            {
                if (!override.fields.IsEmpty())
                {
                    propertyEntities.Append(entities[i]);
                    propertyOverrides.Append(override);
                }
                continue;
            }

            if (override.overriddenFields.IsNull() && !override.componentAdded && !override.componentRemoved)
                continue;

            revertEntities.Append(entities[i]);
            revertComponents.Append(override.component);
        }
    }

    if (revertComponents.IsEmpty() && propertyEntities.IsEmpty())
        return;

    Ptr<EditCommandManager> manager = this->core->GetCommandManager();
    manager->BeginMacro("Revert prefab instance");

    for (IndexT i = 0; i < revertComponents.Size(); i++)
    {
        Ptr<RevertComponentCommand> command = RevertComponentCommand::Create();
        command->scene = scene;
        command->entity = revertEntities[i];
        command->component = revertComponents[i];
        manager->Execute(command);
    }

#if NEBULA_DOTNET
    // One command per entity, because a command applies the entity's whole text. The marker is
    // written whole, so
    // the command that empties it is the one an undo puts back
    //
    // This runs after the frame, so the guid map has to be the scene's own again: the draw's
    // context is only valid for the draw that set it
    Util::HashTable<Util::Guid, Game::Entity> map;
    scene->BuildSceneMap(map);
    scene->SetRefContext(map);

    for (IndexT i = 0; i < propertyEntities.Size(); i++)
    {
        const Game::Entity entity = propertyEntities[i];
        const PrefabOverride& override = propertyOverrides[i];

        const Game::LevelExtension* extension = Game::LevelExtensionRegistry::GetExtension(override.extension);
        if (extension == nullptr || extension->resolve == nullptr || extension->keys == nullptr)
            continue;

        Util::String chainText;
        Util::Array<Util::String> chainKeys;
        scene->GetChainExtension(entity, override.extension, chainText, chainKeys);

        Util::String oldText;
        if (extension->capture != nullptr)
            extension->capture(entity, oldText);

        // Every key the entity carries, not only the ones the marker has. A field the instance did
        // not author is
        // still a field the entity has, and an exact apply takes off whatever the composed text
        // does not name
        Util::Array<Util::String> allKeys;
        extension->keys(oldText.AsCharPtr(), allKeys);

        // A property the chain does not have is one the instance authored on its own, and there is
        // nothing to
        // revert it to. It drops out of the text, which is what makes the exact apply take it off,
        // the same way a
        // component the instance added is taken off
        Util::Array<Util::String> fromChain;
        for (IndexT f = 0; f < allKeys.Size(); f++)
        {
            const IndexT dot = allKeys[f].FindCharIndex('.');
            if (dot == InvalidIndex)
                continue;

            Util::String name;
            name.Set(allKeys[f].AsCharPtr(), (SizeT)dot);

            bool chainHas = false;
            for (IndexT k = 0; k < chainKeys.Size(); k++)
            {
                const IndexT kdot = chainKeys[k].FindCharIndex('.');
                if (kdot == InvalidIndex)
                    continue;

                Util::String kname;
                kname.Set(chainKeys[k].AsCharPtr(), (SizeT)kdot);
                if (kname == name)
                {
                    chainHas = true;
                    break;
                }
            }
            if (chainHas)
                fromChain.Append(allKeys[f]);
        }

        Ptr<PropertyCommand> command = PropertyCommand::Create();
        command->scene = scene;
        command->entity = entity;
        command->extension = override.extension;
        command->oldText = oldText;
        extension->resolve(chainText.AsCharPtr(), fromChain, command->newText);
        command->oldFields = override.fields;
        command->isRevert = true;
        manager->Execute(command);
    }
#endif

    manager->EndMacro();
}

//------------------------------------------------------------------------------
/**
    The instantiation an Unpack was pressed on becomes ordinary authored entities of this scene,
    through the command that keeps the links so the undo can put them back.
*/
void
InspectorPanel::ApplyUnpack()
{
    Ptr<UnpackPrefabCommand> command = UnpackPrefabCommand::Create();
    command->scene = this->core->GetScene();
    command->entity = this->pendingInstanceEntity;
    this->core->GetCommandManager()->Execute(command);
}

//------------------------------------------------------------------------------
/**
    What the instance authored goes into its own prefab file, and the tabs that instance the same
    file follow it. No command wraps this: the write lands in a file rather than in the scene, which
    is not what the scene's undo stack answers for.
*/
void
InspectorPanel::ApplyToPrefab()
{
    Scene* scene = this->core->GetScene();
    if (!scene->IsPrefabInstanceEntity(this->pendingInstanceEntity))
        return;

    const Util::String source = scene->GetPrefabInstance(this->pendingInstanceEntity).source;
    if (!scene->ApplyInstanceToPrefab(this->pendingInstanceEntity))
    {
        this->core->Log(LogMessageType::N_WARNING, "could not write " + source);
        return;
    }

    this->core->PrefabFileWritten(source, scene);
    this->core->Log(LogMessageType::N_SYSTEM,
        "Applied " + scene->GetEditable(this->pendingInstanceEntity).name + " to " + source);
}

//------------------------------------------------------------------------------
/**
    What the rename field holds is queued rather than applied during the draw, and the field is
    closed, because a commit is the end of the edit. An empty name is not a name, and a name that is
    the same as the one the entity has is not a change, which is what makes Escape commit nothing:
    the field puts the buffer back to what it was opened with when the key is pressed.
*/
void
InspectorPanel::CommitRename(Game::Entity entity)
{
    const Scene* scene = this->core->GetScene();
    const Util::String name(this->renameBuffer);

    if (!name.IsEmpty() && name != scene->GetEditable(entity).name)
    {
        this->pendingRenameEntity = entity;
        this->pendingRenameName = name;
        this->QueueAction(Util::Delegate<void()>::FromMethod<InspectorPanel, &InspectorPanel::ApplyRename>(this));
    }

    this->renaming = false;
    this->renamingEntity = Game::Entity::Invalid();
    this->renameBuffer[0] = 0;
}

//------------------------------------------------------------------------------
/**
    The name is applied through the command manager, which is what makes it undoable. The command
    takes the name the entity had on the first run, so a redo puts the same name back.
*/
void
InspectorPanel::ApplyRename()
{
    Ptr<EditCommandManager> manager = this->core->GetCommandManager();

    Ptr<SetEntityNameCommand> command = SetEntityNameCommand::Create();
    command->scene = this->core->GetScene();
    command->entity = this->pendingRenameEntity;
    command->newName = this->pendingRenameName;
    manager->Execute(command);
}

//------------------------------------------------------------------------------
/**
*/
void
InspectorPanel::ApplySelectParent()
{
    if (this->pendingSelectParent == Game::Entity::Invalid())
        return;

    Util::Array<Game::Entity> selection;
    selection.Append(this->pendingSelectParent);
    this->core->GetSelection()->SetSelection(selection);
}

//------------------------------------------------------------------------------
/**
    A section title. A component section gets a remove button on the same row, right aligned,
    because the row is the section and the button belongs to the component the section is. The
    general and the transform sections are not a component, so they do not get one, and neither do
    the fixed column components, because they cannot come off an entity. The row itself is the
    surface's, so the panel only decides whether the button is offered and what a click on it means.
*/
void
InspectorPanel::DrawSection(const char* title, Game::Entity entity, Game::ComponentId component)
{
    // The owner and the three transform components are the fixed columns of every entity table, so
    // they
    // cannot come off an entity and the section does not offer it
    const Game::ComponentId ownerId = Game::GetComponentId<Game::Entity>();
    const bool removable = component != Game::ComponentId::Invalid() && component != ownerId &&
        !this->core->GetScene()->IsTransformComponent(component);

    // The name the buttons are pushed under has to tell the sections apart, and here it is the
    // component name,
    // which is the title as well. The marker is not drawn, because the panel's own marker is on the
    // component's
    // rows rather than on its section
    const int clicked = Editor::DrawSection(title, removable ? title : nullptr, false);

    if (clicked == 1)
    {
        this->pendingRemoveEntity = entity;
        this->pendingRemoveComponent = component;
        this->QueueAction(Util::Delegate<void()>::FromMethod<InspectorPanel, &InspectorPanel::ApplyRemoveComponent>(this));
    }
}

//------------------------------------------------------------------------------
/**
    The key is drawn at half opacity, because it is the label of the row and not its content. A
    copyable row is a link, and what a link does is the panel's rather than the row's: the row only
    says it was clicked.
*/
void
InspectorPanel::DrawRow(const char* key, const Util::String& value, bool copyable)
{
    if (!copyable)
    {
        Editor::DrawRowText(key, value.AsCharPtr(), nullptr);
        return;
    }

    if (Editor::DrawRowLink(key, value.AsCharPtr(), nullptr))
    {
        ImGui::SetClipboardText(value.AsCharPtr());

        Util::String msg;
        msg.Format("Copied %s to clipboard", value.AsCharPtr());
        this->SetStatus(msg, 3.0f);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
InspectorPanel::DrawParent(Game::Entity entity)
{
    Scene* scene = this->core->GetScene();
    const Game::Entity parent = scene->GetParent(entity);
    const Util::String name = this->EntityName(parent);

    // A parent outside this level is a name rather than a link, because there is nothing to select
    if (parent == Game::Entity::Invalid() || !scene->HasInstance(parent))
    {
        Editor::DrawRowText("Parent", name.AsCharPtr(), nullptr);
        return;
    }

    const bool clicked = Editor::DrawRowLink("Parent", name.AsCharPtr(), nullptr);

    if (ImGui::IsItemHovered())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        this->core->GetTools()->DrawHighlight(parent, entity);
    }

    if (clicked)
    {
        this->pendingSelectParent = parent;
        this->QueueAction(Util::Delegate<void()>::FromMethod<InspectorPanel, &InspectorPanel::ApplySelectParent>(this));
    }
}

//------------------------------------------------------------------------------
/**
    The three components the Transform section draws. They are what every entity has, so a component
    that is one of these does not get a section of its own.
*/
bool
InspectorPanel::IsTransformComponent(Game::ComponentId component) const
{
    return component == Game::GetComponentId<Game::Position>() ||
        component == Game::GetComponentId<Game::Orientation>() ||
        component == Game::GetComponentId<Game::Scale>();
}

//------------------------------------------------------------------------------
/**
    The components an entity can be given. Game::Entity is not one an entity is given, and the three
    the transform section draws are on every entity. A runtime component is one code attaches, so an
    entity cannot be given one either.
*/
bool
InspectorPanel::IsAddableComponent(Game::ComponentId component) const
{
    if (component == Game::GetComponentId<Game::Entity>())
        return false;

    if (EditorFeatureRegistry::IsRuntimeComponent(component))
        return false;

    return !this->IsTransformComponent(component);
}

//------------------------------------------------------------------------------
/**
    The components an entity has are the attributes of its table, so a component it does not have is
    not in the array, and its data is not something the inspection system can be given.
*/
bool
InspectorPanel::HasComponent(Game::Entity entity, Game::ComponentId component) const
{
    Scene* scene = this->core->GetScene();

    const Util::Array<Game::ComponentId>& components = scene->GetComponents(entity);
    for (IndexT i = 0; i < components.Size(); i++)
    {
        if (components[i] == component)
            return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
Util::String
InspectorPanel::CollectionName(const Util::Guid& guid) const
{
    const Scene* scene = this->core->GetScene();

    const IndexT index = scene->FindCollection(guid);
    if (index == InvalidIndex)
        return "Scene";
    return scene->GetCollections()[index].name;
}

//------------------------------------------------------------------------------
/**
*/
Util::String
InspectorPanel::EntityName(Game::Entity entity) const
{
    const Scene* scene = this->core->GetScene();
    if (entity == Game::Entity::Invalid() || !scene->HasInstance(entity))
        return "-";

    return scene->GetEditable(entity).name;
}

} // namespace Editor
