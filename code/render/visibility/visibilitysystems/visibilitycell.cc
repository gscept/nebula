//------------------------------------------------------------------------------
//  visibilitycell.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "visibility/visibilitysystems/visibilitycell.h"
#include "graphics/stage.h"
#include "graphics/graphicsserver.h"

namespace Visibility
{
__ImplementClass(Visibility::VisibilityCell, 'VICL', Core::RefCounted);

using namespace Math;
using namespace Util;
using namespace Graphics;

//------------------------------------------------------------------------------
/**
*/
VisibilityCell::VisibilityCell() :
    numEntitiesInHierarchyAllTypes(0)
{
    Memory::Clear(this->numEntitiesInHierarchyByType, sizeof(this->numEntitiesInHierarchyByType));
}

//------------------------------------------------------------------------------
/**
*/
VisibilityCell::~VisibilityCell()
{
    // make sure we've been properly cleaned up
    n_assert(!this->parentCell.isvalid());
    n_assert(this->childCells.IsEmpty());
    n_assert(this->entities.IsEmpty());
    IndexT i;
    for (i = 0; i < GraphicsEntityType::NumTypes; i++)
    {
        n_assert(this->entitiesByType[i].IsEmpty());
    }
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityCell::OnAttach()
{
    n_assert(this->entities.IsEmpty());

    this->numEntitiesInHierarchyAllTypes = 0;
    Memory::Clear(this->numEntitiesInHierarchyByType, sizeof(this->numEntitiesInHierarchyByType));
        
    // recurse into child cells
    IndexT i;
    for (i = 0; i < this->childCells.Size(); i++)
    {
        this->childCells[i]->OnAttach();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityCell::OnRemove()
{
    // first recurse to child cells
    IndexT i;
    for (i = 0; i < this->childCells.Size(); i++)
    {
        this->childCells[i]->OnRemove();
    }

    // cleanup
    this->parentCell = 0;
    this->numEntitiesInHierarchyAllTypes = 0;
    Memory::Clear(this->numEntitiesInHierarchyByType, sizeof(this->numEntitiesInHierarchyByType));
    this->childCells.Clear();
    this->entities.Clear();
    for (i = 0; i < GraphicsEntityType::NumTypes; i++)
    {
        this->entitiesByType[i].Clear();
    }
}

//------------------------------------------------------------------------------
/**
    NOTE: the cell hierarchy may only be built during the setup phase while
    the cell hierarchy haven't been added to the stage yet.
*/
void
VisibilityCell::AttachChildCell(const Ptr<VisibilityCell>& cell)
{
    n_assert(cell.isvalid())
    n_assert(!cell->GetParentCell().isvalid()); 

    cell->parentCell = this;
    this->childCells.Append(cell);
}

//------------------------------------------------------------------------------
/**
    Attach an context to this VisibilityCell. This will happen when a graphics context
    moves through the world, leaving and entering cells as necessary.
*/
void
VisibilityCell::AttachContext(const Ptr<VisibilityContext>& context)
{
    n_assert(context.isvalid());
    n_assert(context->GetGfxEntity()->GetType() < GraphicsEntityType::NumTypes);
      
    this->entities.Append(context);
    this->entitiesByType[context->GetGfxEntity()->GetType()].Append(context);

    this->UpdateNumEntitiesInHierarchy(context->GetGfxEntity()->GetType(), +1);
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityCell::RemoveContext(const Ptr<VisibilityContext>& context)
{
    n_assert(context.isvalid());
    n_assert(context->GetGfxEntity()->GetType() < GraphicsEntityType::NumTypes);
   
    IndexT entitiesIndex = this->entities.FindIndex(context);
    n_assert(InvalidIndex != entitiesIndex);
    this->entities.EraseIndex(entitiesIndex);
    
    IndexT entitiesByTypeIndex = this->entitiesByType[context->GetGfxEntity()->GetType()].FindIndex(context);
    n_assert(InvalidIndex != entitiesByTypeIndex);
    this->entitiesByType[context->GetGfxEntity()->GetType()].EraseIndex(entitiesByTypeIndex);
    
    this->UpdateNumEntitiesInHierarchy(context->GetGfxEntity()->GetType(), -1);
}

//------------------------------------------------------------------------------
/**
    Starting from this cell, try to find the smallest cell which completely
    contains the given entity:

    - starting from initial cell:
        - if the entity does not fit into the cell, move up the
          tree until the first cell is found which the entity completely fits into
        - if the entity fits into a cell, check each child cell if the 
          entity fits completely into the cell

    The entity will not be attached! If the entity does not fit into the 
    root cell, the root cell will be returned, not 0.
    
    @param  entity      pointer of entity to find new cell for
    @return             cell which completely encloses the entity (the root cell is an exception)
*/
Ptr<VisibilityCell> 
VisibilityCell::FindEntityContainmentCell(const Ptr<VisibilityContext>& entity)
{
    // get global bounding box of entity
    const bbox& entityBox = entity->GetBoundingBox();

    // find the first upward cell which completely contains the entity,
    // stop at tree root
    Ptr<VisibilityCell> curCell = this;
    while ((curCell->GetParentCell().isvalid()) && (!curCell->GetBoundingBox().contains(entityBox)))
    {
        curCell = curCell->GetParentCell();
    } 

    // find smallest downward cell which completely contains the entity
    IndexT cellIndex;
    SizeT numCells;
    do
    {
        const Array<Ptr<VisibilityCell> >& curChildren = curCell->GetChildCells();
        numCells = curChildren.Size();
        for (cellIndex = 0; cellIndex < numCells; cellIndex++)
        {
            const Ptr<VisibilityCell>& childCell = curChildren[cellIndex];
            if (childCell->GetBoundingBox().contains(entityBox))
            {
                curCell = childCell;
                break;
            }
        }
        // check for loop fallthrough: this means that the current cell either has
        // no children, or that none of the children completely contains the entity
    }
    while (cellIndex != numCells);
    return curCell;
}

//------------------------------------------------------------------------------
/**
    Insert a dynamic graphics entity into the cell tree. The entity
    will correctly be inserted into the smallest enclosing cell in the tree.
    The cell may not be currently attached to a cell, the refcount of the
    entity will be incremented.

    @param  entity      pointer to a graphics entity
*/
Ptr<VisibilityCell> 
VisibilityCell::InsertContext(const Ptr<VisibilityContext>& entity)
{
	Ptr<VisibilityCell> cell = this->FindEntityContainmentCell(entity);
    n_assert(cell.isvalid());
    cell->AttachContext(entity);

    return cell;
}

//------------------------------------------------------------------------------
/**
    Recursively update visibility links. This method is called by the
    top level method CollectVisibleContexts(). 
    
    NOTE: This is the core visibility detection method and must be FAST.
*/
void 
VisibilityCell::RecurseCollectVisibleContexts(const Ptr<ObserverContext>& observerContext, Util::Array<Ptr<VisibilityContext> >& visibilityContexts, uint entityTypeMask, Math::ClipStatus::Type clipStatus)
{
    n_assert(observerContext.isvalid());

    // break immediately if no context of wanted type in this cell or below
    if (this->GetNumEntitiesInHierarchyByTypeMask(entityTypeMask) == 0)
    {
        return;
    }

    // if clip status unknown or clipped, get clip status of this cell against observer context
    if ((ClipStatus::Invalid == clipStatus) || (ClipStatus::Clipped == clipStatus))
    {
        const bbox& cellBox = this->GetBoundingBox();
		clipStatus = observerContext->ComputeClipStatus(cellBox);
    }

    // proceed depending on clip status of cell against observer context
    if (ClipStatus::Outside == clipStatus)
    {
        // cell isn't visible by observer context
        return;
    }
    else if (ClipStatus::Inside == clipStatus)
    {
        // cell is fully visible by observer context, everything inside
        // the cell is fully visible as well
        IndexT observedType;
        for (observedType = 0; observedType < GraphicsEntityType::NumTypes; observedType++)
        {
            if (0 != (entityTypeMask & (1<<observedType)))
            {
                const Array<Ptr<VisibilityContext> >& observedEntities = this->entitiesByType[observedType];
				IndexT index;
                SizeT num = observedEntities.Size();
                for (index = 0; index < num; index++)
                {
                    const Ptr<VisibilityContext>& observedEntity = observedEntities[index];
                    
                    if (observedEntity->GetGfxEntity()->IsVisible())
                    {
                        visibilityContexts.Append(observedEntity);
                    }                       
                    // short cut: if generating light links, check if the observed
                    // context is actually visible by any camera
                    /*if ((GraphicsEntity::LightLink == linkType) && 
                        (observedEntity->GetLinks(GraphicsEntity::CameraLink).IsEmpty()))
                    {
                        continue;
                    }
                    else*/
                    /*if (observedEntity->IsValid() && observedEntity->IsVisible())
                    {

                    }*/
                }
			}
        }
    }
    else
    {
        // cell is partially visible, must check visibility of each contained context
        IndexT observedType;
        for (observedType = 0; observedType < GraphicsEntityType::NumTypes; observedType++)
        {
            if (0 != (entityTypeMask & (1<<observedType)))
            {
                const Array<Ptr<VisibilityContext> >& observedEntities = this->entitiesByType[observedType];
                IndexT index;
                SizeT num = observedEntities.Size();
                for (index = 0; index < num; index++)
                {
                    const Ptr<VisibilityContext>& observedEntity = observedEntities[index];
                    // short cut: if generating light links, check if the observed
                    // context is actually visible by any camera
                    /*if ((GraphicsEntity::LightLink == linkType) && 
                        (observedEntity->GetLinks(GraphicsEntity::CameraLink).IsEmpty()))
                    {
                        continue;
                    }
                    else*/                       
                    if (observerContext->ComputeClipStatus(observedEntity->GetBoundingBox()) != ClipStatus::Outside)
                    {    
                        if (observedEntity->GetGfxEntity()->IsVisible())
                        {
                            visibilityContexts.Append(observedEntity);
                        }
                    }                       
                }
            }
        }
    }

    // recurse into child cells (if this cell is fully or partially visible)
    IndexT childIndex;
    SizeT numChildren = this->childCells.Size();
    for (childIndex = 0; childIndex < numChildren; childIndex++)
    {
        this->childCells[childIndex]->RecurseCollectVisibleContexts(observerContext, visibilityContexts, entityTypeMask, clipStatus);
    }
}

//------------------------------------------------------------------------------
/**
    Frontend method for updating visibility links. This method
    simply calls RecurseCollectVisibleContexts() which recurses into child
    cells if necessary.
*/
void 
VisibilityCell::CollectVisibleContexts(const Ptr<ObserverContext>& observerEntity, Util::Array<Ptr<VisibilityContext> >& visibilityContexts, uint entityTypeMask)
{
    this->RecurseCollectVisibleContexts(observerEntity, visibilityContexts, entityTypeMask, ClipStatus::Invalid);
}

//------------------------------------------------------------------------------
/**
    Update the number of entities in hierarchy. Must be called when
    entities are added or removed from this cell.
*/
void
VisibilityCell::UpdateNumEntitiesInHierarchy(GraphicsEntityType::Code type, int num)
{
    n_assert((type >= 0) && (type < GraphicsEntityType::NumTypes));

    this->numEntitiesInHierarchyAllTypes += num;
    this->numEntitiesInHierarchyByType[type] += num;
    n_assert(this->numEntitiesInHierarchyAllTypes >= 0);
    VisibilityCell* p = this->parentCell.get_unsafe();
    if (p) do
    {
        p->numEntitiesInHierarchyAllTypes += num;
        p->numEntitiesInHierarchyByType[type] += num;
        n_assert(p->numEntitiesInHierarchyAllTypes >= 0);
    }
    while (0 != (p = p->parentCell.get_unsafe()));
}

} // namespace Visibility
