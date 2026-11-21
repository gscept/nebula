#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::OutlinePanel

    The scene outline, with two modes: collections and hierarchy. Both modes show the same authored
    entities, grouped two different ways.

    Collections shows them in the collection tree the scene keeps, hierarchy shows them in the
    parent chain of Game::HTransform.

    The tree state is kept per scene, so a tab you come back to shows the tree you left it with. The
    panel draws the active scene, so the state it is holding belongs to the scene it last drew and
    is put back before the next one is read.

    F2 is the rename key. It is registered as a keybind against the panel's rename command, which is
    what makes it rebindable, and the command renames the selected entities wherever the key comes
    from. A rename that applies to more than one entity numbers them: Foo 1, Foo 2.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/cvar.h"
#include "neweditor/core/editorpanel.h"
#include "game/entity.h"
#include "util/array.h"
#include "util/dictionary.h"
#include "util/guid.h"
#include "util/stack.h"
#include "util/string.h"

namespace Editor
{

class Scene;

class OutlinePanel final : public EditorPanel
{
    __DeclareClass(OutlinePanel)
public:
    OutlinePanel();
    virtual ~OutlinePanel();

    /// draw the tree
    void Draw() override;

protected:
    bool OnSetup() override;

private:
    enum Mode
    {
        ModeCollections,
        ModeHierarchy,
    };

    //------------------------------------------------------------------------------
    /**
        The tree state of one scene. The panel keeps one of these per open scene, which is what
        makes a tab you come back to show the tree you left it with.
    */
    struct SceneState
    {
        Util::Dictionary<Util::Guid, bool> openCollections;
        Util::Dictionary<Game::Entity, bool> openEntities;
        Util::String filter;
        /// the collection the panel has selected. The entity selection is on the scene, and it can
        /// hold more
        /// than one entity
        Util::Guid selectedCollection;
        /// the entity the last click landed on, which is the end a shift click ranges from
        Game::Entity anchorEntity = Game::Entity::Invalid();
        /// the selection the panel last drew. It is what tells the panel that the selection came
        /// from somewhere
        /// else, and that the path to it has to be opened
        Util::Array<Game::Entity> lastSelection;
        bool rootOpen = true;
    };

    /// put the state the panel is holding back under the scene it was drawn for, and read the state
    /// of the
    /// scene it is about to draw
    void SwitchState(const Scene* scene);
    /// drop the states of scenes that are no longer open
    void PruneStates();

    enum RowKind
    {
        RowRoot,
        RowCollection,
        RowEntity,
    };

    /// what a queued selection change does. Toggle is one entity, which is the way a Ctrl+click
    /// changes
    /// membership, and Add is the union with the entities a menu item named
    enum SelectionAction
    {
        SelectionReplace,
        SelectionToggle,
        SelectionAdd,
    };

    /// one node of the tree the current mode is drawing. Only the visible nodes are in the row
    /// array, so
    /// parentRow is the row that a node was drawn under, which is what makes a filter keep the path
    /// to a match
    struct Row
    {
        IndexT kind;
        int depth;
        IndexT parentRow;
        Game::Entity entity;
        /// the collection the row belongs to, which is what a drop on a collection row moves
        /// entities into and
        /// what the context menu creates an entity in
        Util::Guid guid;
        /// the identity of an entity row, which is what the guid column shows
        Util::Guid entityGuid;
        Util::String name;
        /// what the mode adds to the name: the collection in the hierarchy mode, the parent in the
        /// collections
        Util::String suffix;
        bool open;
        bool hasChildren;
        bool match;
    };

    /// an entry on the walk stack. The walk is iterative because a tree this deep is not worth the
    /// call stack
    struct PathEntry
    {
        IndexT index;
        int depth;
        IndexT row;
    };

    /// the rows the tree shows, in the order they are drawn
    void BuildRows();
    /// the node lists for the collection tree: collections under their parent, entities under their
    /// collection
    void BuildCollectionNodes();
    /// the node lists for the hierarchy tree, from the parent field of Game::HTransform
    void BuildHierarchyNodes();
    /// append one node's children to its parent's list, keeping the order they were created in
    void AppendNode(IndexT parent, IndexT node);
    /// walk the tree the current mode is drawing and append the visible rows
    void WalkTree(IndexT rootRow);
    /// drop the rows that do not match the filter, keeping the path to the ones that do
    void FilterRows();
    /// append one row and return its index
    IndexT AppendRow(IndexT kind, int depth, IndexT parentRow, Game::Entity entity, const Util::Guid& guid,
        const Util::Guid& entityGuid, const Util::String& name, const Util::String& suffix, bool open, bool hasChildren);

    /// the row the tree shows at a column
    void DrawRow(IndexT rowIndex);
    /// open or close a node
    void ToggleOpen(const Row& row);
    /// select a row. Ctrl adds to the selection instead of replacing it, and Shift selects the rows
    /// between
    /// the anchor and this one
    void SelectRow(IndexT rowIndex, bool additive, bool ranged);
    /// true when the row carries a selected entity, or is the selected collection
    bool IsSelectedRow(IndexT rowIndex) const;
    /// true when an entity is part of the drag
    bool IsDragged(Game::Entity entity) const;
    /// true when the row is part of the drag
    bool IsDraggedRow(const Row& row) const;
    /// the entity a name refers to, or Invalid when nothing has that name
    Game::Entity EntityNamed(const Util::String& name) const;
    /// the row an entity is drawn on, or InvalidIndex when it is not. Only the visible rows are in
    /// the array, so
    /// an entity behind a collapsed node or filtered out has no row
    IndexT RowOfEntity(Game::Entity entity) const;
    /// open the nodes on the path to the selected entities, so that a selection made outside the
    /// panel is drawn
    void RevealSelection();
    /// true when a name matches the filter. The filter is fuzzy, so 'plr' matches 'Player'
    bool MatchesFilter(const Util::String& name) const;
    /// the word the icon column shows for a row
    static const char* RowKindName(IndexT kind);
    /// the footer text: how many entities there are, how many of them the filter leaves visible, or
    /// how many are selected
    void DrawFooter();

    /// the model changes. These run after the frame has drawn
    void ApplyCreate();
    void ApplyCreateCollection();
    void ApplyDelete();
    void ApplyDeleteCollection();
    void ApplyDrop();
    void ApplyParent();
    /// run the queued file drop from the file browser: instantiate the file and put its roots on
    /// the row
    void ApplyFileDrop();
    void ApplyReparentCollection();
    void ApplySelection();
    /// the active collection is the one new entities go into, so it is part of the model even
    /// though the panel
    /// keeps its own copy of it
    void ApplyActiveCollection();
    /// the instantiation the context menu unpacked. The entities keep their values and become
    /// ordinary entities of this scene
    void ApplyUnpack();
    /// the clipboard actions. The copy writes the row's targets as a scene document, and the
    /// duplicate
    /// and the paste put entities into the row's collection, under the row when the row is an
    /// entity
    void ApplyCopy();
    void ApplyDuplicate();
    void ApplyPaste();
    /// queue what the rename field committed and close the field. The field is the row while it is
    /// open, so the
    /// row tells it whether the name goes to an entity or to a collection
    void CommitRename(const Row& row);
    /// queue the names a rename gives: the base on its own for one entity, and the base with a
    /// number on each of
    /// the ones it would change, so a rename that changes nothing is not an entry
    void QueueRename(const Util::Array<Game::Entity>& entities, const Util::String& base);
    void ApplyRename();
    void ApplyRenameCollection();

    /// the entities a copy, a duplicate, a delete or a rename applies to: the row when it is not
    /// part of the
    /// selection, and the whole selection when it is
    Util::Array<Game::Entity> RowTargets(const Row& row) const;

    /// the entities a collection row holds, in the order they were created in. A nested collection
    /// is a row of its
    /// own and its entities are on that row, so they are not part of this one's
    Util::Array<Game::Entity> CollectionEntities(const Row& row) const;

    /// commands
    void CommandSelect(const Util::Array<Util::String>& args);
    void CommandCollection(const Util::Array<Util::String>& args);
    void CommandParent(const Util::Array<Util::String>& args);
    void CommandRename(const Util::Array<Util::String>& args);

    Mode mode;

    /// the filter. Immediate mode needs a buffer that survives between frames
    static const IndexT MaxFilterLength = 256;
    char filterBuffer[MaxFilterLength];
    Util::String filter;

    /// the guid column. The cvar is read every frame, and the value it had last time is what tells
    /// the panel that
    /// the rows have to be drawn again
    Core::CVar* showGuidCVar;
    bool showGuid;

    /// the rows the tree shows, in the order they are drawn
    Util::Array<Row> rows;

    /// the open state. It is kept here rather than in ImGui, because a collapsed node has no row
    Util::Dictionary<Util::Guid, bool> openCollections;
    Util::Dictionary<Game::Entity, bool> openEntities;
    bool rootOpen;

    /// the collection the outline has selected. The entity selection is on the core, and it can
    /// hold more than one
    /// entity
    Util::Guid selectedCollection;

    /// the entity the last click landed on, which is the end a shift click ranges from. It is kept
    /// as an entity
    /// rather than a row index, because a rebuild moves the rows around between the two clicks
    Game::Entity anchorEntity;

    /// the selection the panel last drew, which is what tells the panel that the selection came
    /// from somewhere
    /// else, and that the path to it has to be opened
    Util::Array<Game::Entity> lastSelection;

    /// the tree state of every open scene
    Util::Dictionary<Util::Guid, SceneState> sceneStates;
    /// the scene the state the panel is holding belongs to
    Util::Guid stateScene;

    /// the drag state. The entities are the truth rather than row indexes, because a rebuild moves
    /// the rows
    /// around while a drag is still running
    Util::Array<Game::Entity> draggedEntities;
    /// a drag carries one collection, because a collection is not part of the selection
    Util::Guid draggedCollection;
    IndexT dropTargetRow;

    /// the row the context menu is open on. The popup is a window of its own, so the row it applies
    /// to has to be
    /// remembered rather than read from where the click landed
    IndexT contextRow;
    bool contextPopupRequested;

    /// the row a left press landed on. Immediate mode only knows a drag exists after the mouse has
    /// moved off the
    /// row, so a press is only a click when the release comes without a drag, and the selection is
    /// applied on the
    /// release. The modifiers are the ones the press had, because a drag that takes the selection
    /// with it is not
    /// a selection
    IndexT pressedRow;
    bool pressedAdditive;
    bool pressedRanged;

    /// the rename field. It is opened from the context menu or from the rename command and drawn on
    /// the row it
    /// was opened on, and the row is found by the entity or by the guid rather than by an index,
    /// because a
    /// rebuild moves the rows around while the field is open. Only one field can be open at a time,
    /// so the buffer
    /// is shared. Immediate mode needs a buffer that survives between frames
    static const IndexT MaxNameLength = 128;
    char renameBuffer[MaxNameLength];
    Game::Entity renamingEntity;
    Util::Guid renamingCollection;
    bool renameFocusRequested;

    /// the new collection prompt. It is opened from the button and drawn on the row under the
    /// filter, and the
    /// buffer is what survives between frames. The parent is the active collection on the frame the
    /// button was
    /// pressed, which is the one the collection is created under
    char newCollectionBuffer[MaxNameLength];
    bool newCollectionOpen;
    bool newCollectionFocusRequested;
    Util::Guid newCollectionParent;

    /// the model changes that were queued during the draw
    Util::Guid pendingCreateCollection;
    Util::Array<Game::Entity> pendingDelete;
    Util::Guid pendingDeleteCollection;
    Util::Array<Game::Entity> pendingDrop;
    Util::Guid pendingDropCollection;
    /// the entity a drop on an entity row makes the parent of pendingDrop. Invalid clears the
    /// parent
    Game::Entity pendingParent;
    /// a file dragged in from the file browser and where its row places the entities it turns into.
    /// The row decides the way it does for dropped entities: the entity to parent under, or the
    /// collection to put the entities in
    Util::String pendingFilePath;
    Game::Entity pendingFileParent;
    Util::Guid pendingFileCollection;
    bool pendingFileReparent;
    Util::Guid pendingReparentCollection;
    Util::Guid pendingReparentParent;
    Util::Array<Game::Entity> pendingSelection;
    SelectionAction pendingSelectionAction;
    Util::Guid pendingActiveCollection;
    /// the instantiation an Unpack was pressed on
    Game::Entity pendingUnpack;
    /// the entities the clipboard actions were pressed on, and where they go. The collection is the
    /// row's and the parent is the row when the row is an entity, which is what makes a paste or a
    /// duplicate on a row put the entities under that row
    Util::Array<Game::Entity> pendingCopy;
    Util::Array<Game::Entity> pendingDuplicate;
    Util::Guid pendingTargetCollection;
    Game::Entity pendingTargetParent;
    /// the entities a rename queued and the names they are given, in the same order. The name is
    /// the base the
    /// field or the command line held, which is what the log says
    Util::Array<Game::Entity> pendingRenameEntities;
    Util::Array<Util::String> pendingRenameNames;
    Util::Guid pendingRenameCollection;
    Util::String pendingRenameName;
    /// the name the new collection prompt committed, and the parent it was opened with
    Util::String pendingNewCollectionName;
    Util::Guid pendingNewCollectionParent;

    /// the child lists. A node is a collection index below numCollections and an editable index
    /// above it, so the
    /// same three arrays serve both modes. The last slot is the root bucket
    Util::Array<IndexT> nodeHead;
    Util::Array<IndexT> nodeLast;
    Util::Array<IndexT> nodeNext;
    IndexT numCollections;
    IndexT rootBucket;

    Util::Stack<PathEntry> path;

    /// the scene version the rows were last built from
    uint64_t treeVersion;

    /// the rows have to be built again before the next draw
    bool rowsDirty;
};

} // namespace Editor
