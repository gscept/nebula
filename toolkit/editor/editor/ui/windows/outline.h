#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::Outline

    Shows an outline of the world and its entities.

    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "editor/ui/window.h"
#include "editor/editor.h"

namespace Presentation
{

class Outline : public BaseWindow
{
    __DeclareClass(Outline)
public:
    Outline();
    ~Outline();

    void Run(SaveMode save) override;

private:
    enum class Mode
    {
        Collections,
        Hierarchy
    };
    struct HierarchyTreeNode
    {
        Editor::Entity entity;
        Editor::Entity parent;
    };
    struct HierarchyTree
    {
        struct ParentStackEntry
        {
            IndexT parentIndex;
            bool isOpen;
        };

        Util::Array<HierarchyTreeNode> nodes;
        Util::Stack<ParentStackEntry> parentStack;
    };

    void DrawCollection(const Util::Guid& collection, const Util::Array<Editor::Entity>& entities, Util::String const& nameFilter);
    void DrawCollectionEntities(const Util::Guid& collection, const Util::Array<Editor::Entity>& entities, Util::String const& nameFilter);
    void DrawHierarchyEntity(HierarchyTree& tree, IndexT index, Util::String const& nameFilter);
    void DrawHierarchyPane(Util::Array<Editor::Entity> const& entities, Util::String const& nameFilter);
    bool DrawEntitySelectable(Editor::Entity entity, bool hasChildren, bool parentOpen, Util::String const& nameFilter);
    void AcceptDrop(const Util::Guid& collection);
    void AcceptHierarchyRootDrop();
    void BeginRename(const Util::Guid& collection);

    Mode mode = Mode::Collections;
    Util::Guid selectedCollection;
    bool collectionSelected = false;
    Util::Guid renameCollection;
    bool openRenamePopup = false;
    char renameBuffer[128] = {};
};
__RegisterClass(Outline)

} // namespace Presentation
