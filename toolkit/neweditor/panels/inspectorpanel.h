#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::InspectorPanel

    The inspector - shows entities internal state, components and allows for modifying these.

    What the panel knows is the entity and the components the attribute registry knows. Everything
    it cannot read is drawn by the feature that produced it: the sections come from the core's
    registered sections, the entries the entity cannot be given come from the core's registered
    constraints, and the entries the editor has no type knowledge of come from the registered entry
    sources.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "neweditor/api/editorfeature.h"
#include "neweditor/core/editorpanel.h"
#include "game/componentid.h"
#include "game/entity.h"
#include "util/array.h"
#include "util/blob.h"
#include "util/dictionary.h"
#include "util/guid.h"
#include "util/string.h"

namespace Editor
{

class InspectorPanel final : public EditorPanel
{
    __DeclareClass(InspectorPanel)
public:
    InspectorPanel();
    virtual ~InspectorPanel();

    /// draw the selection
    void Draw() override;

private:
    /// a component value a widget changed during the draw
    struct PendingEdit
    {
        Game::Entity entity;
        Util::Blob value;
    };

    static bool EntryOrder(const InspectorEntry& lhs, const InspectorEntry& rhs);

    /// a section title. A component section gets a remove button on the same row, right aligned,
    /// because the row
    /// is the section and the button belongs to the component the section is
    void DrawSection(const char* title, Game::Entity entity = Game::Entity::Invalid(), Game::ComponentId component = Game::ComponentId::Invalid());
    /// one row: the key at half opacity and the value after it
    /// one row: the key at half opacity and the value after it. A copyable value is drawn as a
    /// link, and clicking
    /// it copies it and puts the value on the status bar
    void DrawRow(const char* key, const Util::String& value, bool copyable = false);
    /// the parent row. The name is a link that moves the selection to the parent, and while it is
    /// hovered the
    /// canvas shows the link between the two: the line and the hover outline on the parent
    void DrawParent(Game::Entity entity);
    /// the fields of one component, drawn by the inspection system into the panel's copy of the
    /// data
    void DrawComponent(Game::Entity entity, Game::ComponentId component);
    /// the prefab section of an instance entity: the file it links into, what it removed against
    /// it, and the two ways to stop being an instance of it
    void DrawPrefab(Game::Entity entity);
    /// the marker on a component row of an instance entity, and the menu that gives an override
    /// back
    void DrawPrefabMarker(Game::Entity entity, Game::ComponentId component);
    /// the mark on a component row when a field of it comes from a file above the one the instance
    /// was made from, with the tooltip that names the record it comes from
    void DrawChainSource(Game::Entity entity, Game::ComponentId component);
    /// the transform section: position, orientation and scale as one group
    void DrawTransform(Game::Entity entity);
    /// one component drawn as a row in the table that is currently open. The transform is previewed
    /// into the sim
    /// world while its widget is being dragged, so the canvas shows the change before the edit is
    /// applied
    void DrawComponentRow(Game::Entity entity, Game::ComponentId component);
    /// the add button and the dropdown it opens. The dropdown holds the components the attribute
    /// registry knows
    /// and whatever the registered entry sources add, as groups
    void DrawAdd(Game::Entity entity);
    /// the entity name on the title row, with the rename button after it
    void DrawEntityName(Game::Entity entity);
    /// move the highlight one entry in the dropdown. The entries the entity already has, and the
    /// ones it cannot be
    /// given, are stepped over, because neither is something it can pick
    void MoveHighlight(int direction);
    /// queue the entry a row or the keyboard picked. A component is something the panel can add
    /// itself; for
    /// anything else it only knows the name and the text the entry source produces for a default
    /// one
    void QueueAddEntry(Game::Entity entity, const InspectorEntry& entry);
    /// true when the entity has the component. The inspection system needs the data of a component
    /// the entity
    /// does not have, so this is what keeps a row from being drawn for one
    bool HasComponent(Game::Entity entity, Game::ComponentId component) const;
    /// true when the component is one of the three the transform section draws
    bool IsTransformComponent(Game::ComponentId component) const;
    /// true when the component is one an entity can be given. Game::Entity is not a component an
    /// entity is given,
    /// and the three the transform section draws are on every entity
    bool IsAddableComponent(Game::ComponentId component) const;

    /// the model changes that were queued during the draw
    void ApplyEdits();
    /// the entry picked from the dropdown. A component goes through the add command; anything else
    /// through the
    /// property command, built with an empty before text, which is what the command reads as an
    /// add, and the
    /// text the entry source produces for a default instance
    void ApplyAddEntry();
    /// the component a remove button was pressed on
    void ApplyRemoveComponent();
    /// the component a Reset to Prefab was pressed on
    void ApplyRevertComponent();
    /// one field of a component, put back to what the chain gives by writing the chain's bytes
    /// into the value the entity has
    void ApplyRevertField();
    /// every override an instance entity authored, given back to its prefab chain
    void ApplyRevertInstance();
    /// a record the instance marked removed, put back as the entity the record names
    void ApplyRevertRecord();
    /// an entity the instance added, taken off the instance. It has no record, so this is the
    /// delete
    void ApplyRemoveAddition();
    /// the instantiation an Unpack was pressed on
    void ApplyUnpack();
    /// what the instance authored, written back into its prefab file
    void ApplyToPrefab();
    /// queue what the rename field committed and close the field
    void CommitRename(Game::Entity entity);
    /// the name the rename field committed
    void ApplyRename();
    /// the entity a parent link was clicked on
    void ApplySelectParent();

    /// the collection an entity is authored in, or 'Scene' when it is under the root
    Util::String CollectionName(const Util::Guid& guid) const;
    /// the name an entity is known by, or '-' when it is not an authored entity
    Util::String EntityName(Game::Entity entity) const;

    /// the copy the fields are drawn into. One buffer is reused for every component in the frame
    Util::Blob buffer;

    /// the components with a value waiting to be applied
    Util::Dictionary<Game::ComponentId, PendingEdit> pendingEdits;

    /// the filter in the add dropdown. Immediate mode needs a buffer that survives between frames
    static const IndexT MaxFilterLength = 256;
    char filterBuffer[MaxFilterLength];

    /// the dropdown state. The popup is a window of its own, so the panel has to remember that it
    /// was open: the
    /// frame BeginPopup does not open is the frame the popup was closed, and that is when the
    /// filter and the
    /// highlight are cleared
    bool addOpen;
    bool filterFocusRequested;

    /// the rename field. Immediate mode needs a buffer that survives between frames, and the field
    /// has to say
    /// which entity it was opened on, because a selection change means the buffer is not something
    /// the newly
    /// selected entity can be given
    static const IndexT MaxNameLength = 128;
    char renameBuffer[MaxNameLength];
    bool renaming;
    Game::Entity renamingEntity;
    bool renameFocusRequested;

    /// the name the rename field committed
    Game::Entity pendingRenameEntity;
    Util::String pendingRenameName;

    /// the entries the dropdown shows. The list is built again every frame, because the filter is
    /// what decides
    /// which of them are in it, and the highlight is an index into it
    Util::Array<InspectorEntry> dropdownEntries;

    /// the entry the keyboard is on. Mouse hover is ImGui's own highlight, so this is only the
    /// keyboard's
    IndexT highlightedEntry;

    /// the entry picked from the dropdown, kept because the panel cannot read what it is and only
    /// the entry
    /// source can say what a new one holds
    Game::Entity pendingAddEntity;
    InspectorEntry pendingAddEntry;

    /// the component a remove button was pressed on
    Game::Entity pendingRemoveEntity;
    Game::ComponentId pendingRemoveComponent;

    /// the component a Reset to Prefab was pressed on
    Game::Entity pendingRevertEntity;
    Game::ComponentId pendingRevertComponent;

    /// the record of an instance a Reset was pressed on
    IndexT pendingRevertInstance;
    Util::Guid pendingRevertRecord;

    /// the field of a component a menu put back to what the chain gives
    Game::Entity pendingFieldEntity;
    Game::ComponentId pendingFieldComponent;
    IndexT pendingField;

    /// the entity an unpack or a whole-entity revert was pressed on
    Game::Entity pendingInstanceEntity;

    /// the entity a parent link was clicked on
    Game::Entity pendingSelectParent;
};

} // namespace Editor
