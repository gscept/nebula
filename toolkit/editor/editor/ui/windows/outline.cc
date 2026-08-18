//------------------------------------------------------------------------------
//  outline.cc
//  (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "outline.h"
#include "editor/commandmanager.h"
#include "editor/tools/selectioncontext.h"
#include "editor/cmds.h"
#include "basegamefeature/components/basegamefeature.h"
#include "core/cvar.h"

namespace Presentation
{
__ImplementClass(Presentation::Outline, 'OtWn', Presentation::BaseWindow);

//------------------------------------------------------------------------------
/**
*/
static bool
HasSelectedAncestor(Editor::Entity entity, const Util::Array<Editor::Entity>& selection)
{
    Game::World* editorWorld = Editor::state.editorWorld;
    SizeT depth = 0;
    while (editorWorld->HasComponent<Game::HTransform>(entity) && depth++ <= Editor::state.editables.Size())
    {
        entity = editorWorld->GetComponent<Game::HTransform>(entity).parent;
        if (entity == Game::Entity::Invalid() || !editorWorld->IsValid(entity) || !editorWorld->HasInstance(entity))
        {
            return false;
        }
        if (selection.BinarySearchIndex(entity) != InvalidIndex)
        {
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
FuzzyNameMatches(Util::String const& name, Util::String const& filter)
{
    if (filter.IsEmpty())
    {
        return true;
    }
    if (name.IsEmpty())
    {
        return false;
    }

    const char* nameCursor = name.AsCharPtr();
    const char* filterCursor = filter.AsCharPtr();
    while (*nameCursor != '\0' && *filterCursor != '\0')
    {
        if (Util::String::ToLower(*nameCursor) == Util::String::ToLower(*filterCursor))
        {
            filterCursor++;
        }
        nameCursor++;
    }
    return *filterCursor == '\0';
}

//------------------------------------------------------------------------------
/**
*/
Outline::Outline()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Outline::~Outline()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
Outline::BeginRename(const Util::Guid& collection)
{
    IndexT const index = Editor::FindCollection(collection);
    if (index == InvalidIndex)
    {
        return;
    }

    this->renameCollection = collection;
    Editor::state.collections[index].name.CopyToBuffer(this->renameBuffer, sizeof(this->renameBuffer));
    this->openRenamePopup = true;
}

//------------------------------------------------------------------------------
/**
*/
void
Outline::AcceptDrop(const Util::Guid& collection)
{
    if (!ImGui::BeginDragDropTarget())
    {
        return;
    }

    if (ImGuiPayload const* payload = ImGui::AcceptDragDropPayload("entity"))
    {
        n_assert(payload->DataSize == sizeof(Editor::Entity));
        Editor::Entity const entity = *(Editor::Entity const*)payload->Data;
        Util::Array<Editor::Entity> entities;
        if (Tools::SelectionContext::Selection().BinarySearchIndex(entity) != InvalidIndex)
        {
            entities = Tools::SelectionContext::Selection();
        }
        else
        {
            entities.Append(entity);
        }
        Edit::MoveEntitiesToCollection(entities, collection);
    }

    if (ImGuiPayload const* payload = ImGui::AcceptDragDropPayload("collection"))
    {
        n_assert(payload->DataSize == sizeof(Util::Guid));
        Util::Guid const draggedCollection = *(Util::Guid const*)payload->Data;
        Edit::SetCollectionParent(draggedCollection, collection);
    }

    ImGui::EndDragDropTarget();
}

//--------------------------------------------------------------------------
/**
*/
bool
Outline::DrawEntitySelectable(Editor::Entity entity, bool hasChildren, bool parentOpen, Util::String const& nameFilter)
{
    Editor::Editable& editable = Editor::state.editables[entity.index];
    bool const selected = Tools::SelectionContext::Selection().BinarySearchIndex(entity) != InvalidIndex;
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth;
    if (!hasChildren)
    {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    else
    {
        flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    }
    if (selected)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    Util::String label = editable.name;
    if (this->mode == Mode::Hierarchy)
    {
        if (editable.collection.IsValid())
        {
            IndexT const collectionIndex = Editor::FindCollection(editable.collection);
            if (collectionIndex != InvalidIndex)
            {
                label.Append("  [");
                label.Append(Editor::state.collections[collectionIndex].name);
                label.Append("]");
            }
        }
        else
        {
            label.Append("  [Scene]");
        }
    }
    else if (this->mode == Mode::Collections)
    {
        if (Editor::state.editorWorld->HasComponent<Game::HTransform>(entity))
        {
            Game::Entity const parent = Editor::state.editorWorld->GetComponent<Game::HTransform>(entity).parent;
            if (Editor::state.editorWorld->IsValid(parent) && parent.index < Editor::state.editables.Size())
            {
                label.Append("  [parent: ");
                label.Append(Editor::state.editables[parent.index].name);
                label.Append("]");
            }
        }
    }

    Util::String const entityGuid = editable.guid.AsString();
    ImGui::PushID(entityGuid.AsCharPtr());
    if (!nameFilter.IsEmpty())
    {
        ImGui::SetNextItemOpen(true, ImGuiCond_Always);
    }
    
    bool isOpen = parentOpen && ImGui::TreeNodeEx("##hierarchyEntity", flags, label.AsCharPtr());
    if (parentOpen)
    {
        if (ImGui::IsItemHovered())
        {
            Tools::SelectionContext::SetHovered(entity);
        }
        if (ImGui::IsItemClicked(0))
        {
            Util::Array<Editor::Entity> selection;
            if (ImGui::GetIO().KeyCtrl)
            {
                selection = Tools::SelectionContext::Selection();
                IndexT const selectionIndex = selection.BinarySearchIndex(entity);
                if (selectionIndex == InvalidIndex)
                {
                    selection.InsertSorted(entity);
                }
                else
                {
                    selection.EraseIndex(selectionIndex);
                }
            }
            else if (!selected)
            {
                selection.Append(entity);
            }
            else
            {
                selection = Tools::SelectionContext::Selection();
            }
            Edit::SetSelection(selection);
            Editor::state.activeCollection = editable.collection;
            this->collectionSelected = false;
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (ImGuiPayload const* payload = ImGui::AcceptDragDropPayload("entity"))
            {
                n_assert(payload->DataSize == sizeof(Editor::Entity));
                Editor::Entity const dragged = *(Editor::Entity const*)payload->Data;
                Util::Array<Editor::Entity> selectedEntities;
                if (Tools::SelectionContext::Selection().BinarySearchIndex(dragged) != InvalidIndex)
                {
                    selectedEntities = Tools::SelectionContext::Selection();
                }
                else
                {
                    selectedEntities.Append(dragged);
                }
                Edit::CommandManager::BeginMacro("Set parent", false);
                for (Editor::Entity child : selectedEntities)
                {
                    if (child != entity && !HasSelectedAncestor(child, selectedEntities))
                    {
                        Edit::SetParent(child, entity);
                    }
                }
                Edit::CommandManager::EndMacro();
            }
            ImGui::EndDragDropTarget();
        }

        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload("entity", &entity, sizeof(entity));
            ImGui::TextUnformatted(editable.name.AsCharPtr());
            ImGui::EndDragDropSource();
        }

        bool deleted = false;
        if (ImGui::BeginPopupContextItem())
        {
            if (!selected)
            {
                Edit::SetSelection({entity});
            }
            if (Editor::state.editorWorld->HasComponent<Game::HTransform>(entity))
            {
                Game::HTransform transform = Editor::state.editorWorld->GetComponent<Game::HTransform>(entity);
                if (Editor::state.editorWorld->IsValid(transform.parent) &&
                    Editor::state.editorWorld->HasInstance(transform.parent) &&
                    ImGui::MenuItem("Select Parent"))
                {
                    Edit::SetSelection({transform.parent});
                }
                if (ImGui::MenuItem("Clear Parent"))
                {
                    Edit::ClearParent(transform.parent);
                }
            }
            if (hasChildren && ImGui::MenuItem("Select Hierarchy"))
            {
                n_printf("Not implemented!");
                // TODO: This needs to be looked over.
                
                //Util::Array<Editor::Entity> hierarchySelection;
                //hierarchySelection.Append(node->entity);
                //IndexT childOffset = 0;
                //while (index + childOffset < tree.nodes.Size())
                //{
                //    auto const& child = tree.nodes[index + childOffset];
                //    if (child.parent == node->entity)
                //    {
                //        hierarchySelection.Append(child.entity);
                //    }
                //    else
                //    {
                //        break;
                //    }
                //    childOffset++;
                //}
                //hierarchySelection.Sort();
                //Edit::SetSelection(hierarchySelection);
            }
            if (ImGui::MenuItem("Duplicate"))
            {
                Util::Array<Editor::Entity> selection = selected ? Tools::SelectionContext::Selection() : Util::Array<Editor::Entity>{entity};

                Edit::CommandManager::BeginMacro("Duplicate entities", false);
                Util::Array<Editor::Entity> duplicates = Util::Array<Editor::Entity>(selection.Size(), 8);
                for (Editor::Entity selectedEntity : selection)
                {
                    Editor::Entity duplicate = Edit::DuplicateEntity(selectedEntity);
                    Edit::MoveEntitiesToCollection({duplicate}, Editor::state.editables[selectedEntity.index].collection);
                    duplicates.Append(duplicate);
                }
                Edit::SetSelection(duplicates);
                Edit::CommandManager::EndMacro();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete"))
            {
                Util::Array<Editor::Entity> selection = selected ? 
                    Tools::SelectionContext::Selection() : Util::Array<Editor::Entity>{entity};

                Edit::CommandManager::BeginMacro("Delete entities", false);
                Edit::SetSelection({});
                for (Editor::Entity selectedEntity : selection)
                {
                    Edit::DeleteEntity(selectedEntity);
                }
                Edit::CommandManager::EndMacro();
                deleted = true;
            }
            ImGui::EndPopup();
        }

        static Core::CVar* debugWorlds = Core::CVarGet("cl_debug_worlds");
        if (Core::CVarReadInt(debugWorlds) > 0)
        {
            ImGui::SameLine();
            ImGui::TextDisabled("%s", editable.guid.AsString().AsCharPtr());
        }
    }
    
    ImGui::PopID();

    return isOpen;
}


//------------------------------------------------------------------------------
/**
*/
void
Outline::DrawHierarchyEntity(
    HierarchyTree& tree,
    IndexT index,
    Util::String const& nameFilter
)
{
    HierarchyTreeNode* node = &tree.nodes[index];
    HierarchyTreeNode* nextNode = tree.nodes.Size() == index + 1 ? nullptr : &tree.nodes[index + 1];
    HierarchyTreeNode* prevNode = index == 0 ? nullptr : &tree.nodes[index - 1];

    bool isChild = node->parent != Editor::Entity::Invalid();
    bool isLastChild = true;
    bool hasChildren = false;
    if (nextNode != nullptr)
    {
        hasChildren = nextNode->parent == node->entity;
        isLastChild = nextNode->parent != node->parent && !hasChildren;
    }

    while (!tree.parentStack.IsEmpty() && 
        tree.nodes[tree.parentStack.Peek().parentIndex].entity != node->parent &&
        tree.nodes[tree.parentStack.Peek().parentIndex].entity != node->entity
    )
    {
        auto entry = tree.parentStack.Pop();
        if (entry.isOpen)
        {
            ImGui::TreePop();
        }
    }

    bool parentOpen = !tree.parentStack.IsEmpty() ? tree.parentStack.Peek().isOpen : true;
    bool isOpen = DrawEntitySelectable(node->entity, hasChildren, parentOpen, nameFilter);

    if (hasChildren)
    {
        tree.parentStack.Push({index, isOpen});
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Outline::AcceptHierarchyRootDrop()
{
    if (!ImGui::BeginDragDropTarget())
    {
        return;
    }
    if (ImGuiPayload const* payload = ImGui::AcceptDragDropPayload("entity"))
    {
        n_assert(payload->DataSize == sizeof(Editor::Entity));
        Editor::Entity const dragged = *(Editor::Entity const*)payload->Data;
        Util::Array<Editor::Entity> selectedEntities;
        if (Tools::SelectionContext::Selection().BinarySearchIndex(dragged) != InvalidIndex)
        {
            selectedEntities = Tools::SelectionContext::Selection();
        }
        else
        {
            selectedEntities.Append(dragged);
        }
        Edit::CommandManager::BeginMacro("Clear parent", false);
        for (Editor::Entity entity : selectedEntities)
        {
            if (!HasSelectedAncestor(entity, selectedEntities))
            {
                Edit::ClearParent(entity);
            }
        }
        Edit::CommandManager::EndMacro();
    }
    ImGui::EndDragDropTarget();
}

//------------------------------------------------------------------------------
/**
*/
void
Outline::DrawCollectionEntities(const Util::Guid& collection, const Util::Array<Editor::Entity>& entities, Util::String const& nameFilter)
{
    Util::Array<Editor::Entity> collectionEntities;
    for (Editor::Entity entity : entities)
    {
        Editor::Editable const& editable = Editor::state.editables[entity.index];
        if (editable.collection == collection && FuzzyNameMatches(editable.name, nameFilter))
        {
            collectionEntities.Append(entity);
        }
    }

    collectionEntities.SortWithFunc([](Editor::Entity const& lhs, Editor::Entity const& rhs)
    {
        Editor::Editable const& lhsEditable = Editor::state.editables[lhs.index];
        Editor::Editable const& rhsEditable = Editor::state.editables[rhs.index];
        if (lhsEditable.collectionOrder == rhsEditable.collectionOrder)
        {
            return lhs < rhs;
        }
        return lhsEditable.collectionOrder < rhsEditable.collectionOrder;
    });

    for (Editor::Entity entity : collectionEntities)
    {
        if (this->DrawEntitySelectable(entity, false, true, nameFilter))
        {
            //ImGui::TreePop();
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Outline::DrawCollection(const Util::Guid& collectionGuid, const Util::Array<Editor::Entity>& entities, Util::String const& nameFilter)
{
    IndexT const index = Editor::FindCollection(collectionGuid);
    if (index == InvalidIndex)
    {
        return;
    }

    Editor::Collection const collection = Editor::state.collections[index];

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                               ImGuiTreeNodeFlags_OpenOnDoubleClick |
                               ImGuiTreeNodeFlags_SpanFullWidth |
                               ImGuiTreeNodeFlags_DefaultOpen;

    if (this->collectionSelected && this->selectedCollection == collectionGuid)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    Util::String const collectionGuidString = collectionGuid.AsString();
    ImGui::PushID(collectionGuidString.AsCharPtr());

    bool const open = ImGui::TreeNodeEx("##collection", flags, "%s", collection.name.AsCharPtr());

    if (ImGui::IsItemClicked(0))
    {
        this->selectedCollection = collectionGuid;
        Editor::state.activeCollection = collectionGuid;
        this->collectionSelected = true;
    }

    this->AcceptDrop(collectionGuid);

    if (ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("collection", &collectionGuid, sizeof(collectionGuid));
        ImGui::TextUnformatted(collection.name.AsCharPtr());
        ImGui::EndDragDropSource();
    }

    bool collapseChildren = false;

    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("New Collection"))
        {
            Util::Guid const newCollection = Edit::CreateCollection("Collection", collectionGuid);
            this->selectedCollection = newCollection;
            Editor::state.activeCollection = newCollection;
            this->collectionSelected = newCollection.IsValid();
        }

        if (ImGui::MenuItem("Rename"))
        {
            this->BeginRename(collectionGuid);
        }

        if (ImGui::MenuItem("Collapse Children"))
        {
            collapseChildren = true;
        }

        if (ImGui::MenuItem("Delete"))
        {
            if (Edit::DeleteCollection(collectionGuid))
            {
                this->selectedCollection = collection.parent;
                Editor::state.activeCollection = collection.parent;
                this->collectionSelected = true;
            }
        }

        ImGui::EndPopup();
    }

    if (open)
    {
        Util::Array<Util::Guid> children;

        for (IndexT collectionIndex = 0; collectionIndex < Editor::state.collections.Size(); collectionIndex++)
        {
            if (Editor::state.collections[collectionIndex].parent == collectionGuid)
            {
                children.Append(Editor::state.collections[collectionIndex].guid);
            }
        }

        children.SortWithFunc([](Util::Guid const& lhs, Util::Guid const& rhs)
        {
            Editor::Collection const& lhsCollection = Editor::state.collections[Editor::FindCollection(lhs)];
            Editor::Collection const& rhsCollection = Editor::state.collections[Editor::FindCollection(rhs)];

            if (lhsCollection.order == rhsCollection.order)
            {
                return lhsCollection.guid < rhsCollection.guid;
            }

            return lhsCollection.order < rhsCollection.order;
        });

        for (Util::Guid const& child : children)
        {
            if (collapseChildren)
            {
                ImGui::SetNextItemOpen(false);
            }
            this->DrawCollection(child, entities, nameFilter);
        }

        this->DrawCollectionEntities(collectionGuid, entities, nameFilter);
        ImGui::TreePop();
    }
    ImGui::PopID();
}

//--------------------------------------------------------------------------
/**
*/
void
Outline::DrawHierarchyPane(Util::Array<Editor::Entity> const& entities, Util::String const& nameFilter)
{
    Game::World* world = Editor::state.editorWorld;

    HierarchyTree tree;

    Util::Queue<Editor::Entity> entityQueue;
    for (Editor::Entity entity : entities)
    {
        entityQueue.Enqueue(entity);
    }
    
    // build hierarchy
    while(!entityQueue.IsEmpty())
    {
        Editor::Entity entity = entityQueue.Dequeue();

        HierarchyTreeNode node;
        node.entity = entity;
        node.parent = Editor::Entity::Invalid();

        if (!world->HasComponent<Game::HTransform>(entity))
        {
            tree.nodes.Append(node);
        }
        else
        {
            Game::HTransform t = world->GetComponent<Game::HTransform>(entity);
            if (t.parent == Editor::Entity::Invalid() || !world->IsValid(t.parent))
            {
                tree.nodes.Append(node);
                continue;
            }
            
            node.parent = t.parent;
            // find parent and insert right after it
            int i;
            for (i = 0; i < tree.nodes.Size(); i++)
            {
                if (tree.nodes[i].entity == node.parent)
                {
                    tree.nodes.Insert(i + 1, node);
                    i = -1; // found parent
                    break;
                }
            }

            if (i == tree.nodes.Size())
            {
                // No valid parent found. This might mean that it's later in the queue.
                // Requeue the entity, and keep going.
                entityQueue.Enqueue(entity);
                continue;
            }
        }
    }

    // draw hierarchy    
    for (int i = 0; i < tree.nodes.Size(); i++)
    {
        DrawHierarchyEntity(tree, i, nameFilter);
    }

    while (!tree.parentStack.IsEmpty())
    {
        auto entry = tree.parentStack.Pop();
        if (entry.isOpen)
        {
            ImGui::TreePop();
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Outline::Run(SaveMode save)
{
    static char nameFilterBuffer[256];

    if (this->collectionSelected &&
        this->selectedCollection.IsValid() &&
        Editor::FindCollection(this->selectedCollection) == InvalidIndex)
    {
        this->selectedCollection = Util::Guid();
        this->collectionSelected = false;
    }

    if (ImGui::BeginTabBar("OutlineMode"))
    {
        if (ImGui::BeginTabItem("Collections"))
        {
            this->mode = Mode::Collections;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Hierarchy"))
        {
            this->mode = Mode::Hierarchy;
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    if (ImGui::Button("New Entity"))
    {
        Edit::CreateEntity();
    }

    ImGui::SameLine();

    if (this->mode == Mode::Collections)
    {
        if (ImGui::Button("+ Collection"))
        {
            Util::Guid const parent = this->collectionSelected ? this->selectedCollection : Util::Guid();
            Util::Guid const collection = Edit::CreateCollection("Collection", parent);

            this->selectedCollection = collection;
            Editor::state.activeCollection = collection;
            this->collectionSelected = collection.IsValid();
        }
        ImGui::SameLine();
    }

    ImGui::PushItemWidth(180);
    ImGui::InputText("Search", nameFilterBuffer, 256);
    Util::String nameFilter = nameFilterBuffer;
    ImGui::PopItemWidth();

    ImGui::Separator();

    Util::Array<Editor::Entity> entities;
    Game::Filter filter = Game::FilterBuilder().Including<Game::Entity>().Build();
    Game::Dataset data = Editor::state.editorWorld->Query(filter);
    for (int v = 0; v < data.numViews; v++)
    {
        Game::Dataset::View const& view = data.views[v];
        Editor::Entity const* const viewEntities = (Editor::Entity*)view.buffers[0];
        for (IndexT i = 0; i < view.numInstances; i++)
        {
            if (view.validInstances.IsSet(i))
            {
                entities.Append(viewEntities[i]);
            }
        }
    }
    Game::DestroyFilter(filter);

    if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
    {
        if (this->mode == Mode::Collections)
        {
            ImGuiTreeNodeFlags rootFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow |
                                           ImGuiTreeNodeFlags_SpanFullWidth;
            if (this->collectionSelected && !this->selectedCollection.IsValid())
            {
                rootFlags |= ImGuiTreeNodeFlags_Selected;
            }

            bool const rootOpen = ImGui::TreeNodeEx("Scene", rootFlags);

            if (ImGui::IsItemClicked(0))
            {
                this->selectedCollection = Util::Guid();
                Editor::state.activeCollection = Util::Guid();
                this->collectionSelected = true;
            }

            this->AcceptDrop(Util::Guid());

            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("New Collection"))
                {
                    Util::Guid const collection = Edit::CreateCollection("Collection");
                    this->selectedCollection = collection;
                    Editor::state.activeCollection = collection;
                    this->collectionSelected = collection.IsValid();
                    this->BeginRename(collection);
                }
                ImGui::EndPopup();
            }

            if (rootOpen)
            {
                Util::Array<Util::Guid> rootCollections;
                for (IndexT i = 0; i < Editor::state.collections.Size(); i++)
                {
                    if (!Editor::state.collections[i].parent.IsValid())
                    {
                        rootCollections.Append(Editor::state.collections[i].guid);
                    }
                }

                rootCollections.SortWithFunc([](Util::Guid const& lhs, Util::Guid const& rhs)
                {
                    Editor::Collection const& lhsCollection = Editor::state.collections[Editor::FindCollection(lhs)];
                    Editor::Collection const& rhsCollection = Editor::state.collections[Editor::FindCollection(rhs)];
                    if (lhsCollection.order == rhsCollection.order)
                    {
                        return lhsCollection.guid < rhsCollection.guid;
                    }
                    return lhsCollection.order < rhsCollection.order;
                });
                
                for (Util::Guid const& collection : rootCollections)
                {
                    this->DrawCollection(collection, entities, nameFilter);
                }

                this->DrawCollectionEntities(Util::Guid(), entities, nameFilter);
                ImGui::TreePop();
            }
        }
        else
        {
            ImGuiTreeNodeFlags rootFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow |
                                           ImGuiTreeNodeFlags_SpanFullWidth;
            bool const rootOpen = ImGui::TreeNodeEx("Hierarchy", rootFlags);
            this->AcceptHierarchyRootDrop();
            if (rootOpen)
            {
                this->DrawHierarchyPane(entities, nameFilter);
                ImGui::TreePop();
            }
        }
    }
    ImGui::EndChild();

    bool const openingRenamePopup = this->openRenamePopup;
    if (openingRenamePopup)
    {
        ImGui::OpenPopup("Rename Collection");
        this->openRenamePopup = false;
    }

    if (ImGui::BeginPopupModal("Rename Collection", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (openingRenamePopup)
        {
            ImGui::SetKeyboardFocusHere();
        }

        bool const accept = ImGui::InputText(
            "Name",
            this->renameBuffer,
            sizeof(this->renameBuffer),
            ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll
        );

        if ((accept || ImGui::Button("Rename")) && this->renameBuffer[0] != 0)
        {
            Edit::SetCollectionName(this->renameCollection, this->renameBuffer);
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

} // namespace Presentation
