//------------------------------------------------------------------------------
//  charevalskeletonjob.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "jobs/stdjob.h"
#include "math/float4.h"
#include "math/matrix44.h"
#include "math/bbox.h"
#include "visibility/observercontext.h"
#include "visibility/visibilitysystems/visibilityquadtreesystem.h"
#include "util/quadtree.h"

namespace Visibility
{
using namespace Math;
using namespace Util;
using namespace Graphics;

//------------------------------------------------------------------------------
/**
*/
void  
CheckCell(QuadTree<VisibilityQuadtreeSystem::CellInfo>::Node* node,
          VisibilityQuadtreeSystem::EntityInfo* entityInfos,
          SizeT numEntityInfos,
          bbox* observerBox, 
          matrix44* observerFrustum, 
          Ptr<VisibilityContext>* visEntities,
          SizeT* numEntitiesVisible,
          uint entityTypeMask,
          ClipStatus::Type clipStatus)
{       
    SizeT numEntitiesInHierarchy = node->GetElement().numEntitiesInHierarchy;
    if (numEntitiesInHierarchy == 0) 
    {
        return;
    }
    // if clip status unknown or clipped, get clip status of this cell against observer context
    if ((ClipStatus::Invalid == clipStatus) || (ClipStatus::Clipped == clipStatus))
    {
        if (observerBox != 0)
        {
            clipStatus = observerBox->clipstatus(node->GetBoundingBox());
        }
        else if (observerFrustum != 0)
        {               
            clipStatus = node->GetBoundingBox().clipstatus(*observerFrustum);
        }       
        else
        {
            clipStatus = ClipStatus::Inside;
        }        
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
        SizeT numEntitiesInCell = node->GetElement().numEntitiesInCell;
        IndexT entityInfoIdx = node->GetElement().entityInfoStartIndex;
        IndexT i;
        for (i = 0; i < numEntitiesInCell; ++i)
        {
            if (0 != ((1<<entityInfos[entityInfoIdx].entityType) & entityTypeMask))
            {
                visEntities[*numEntitiesVisible] = entityInfos[entityInfoIdx].entityPtr;                	                
                (*numEntitiesVisible)++;
            }             
            n_assert(entityInfoIdx < numEntityInfos);       
            entityInfoIdx++;
        }    
    }
    else
    {
        // cell is partially visible, must check visibility of each contained context        
        SizeT numEntitiesInCell = node->GetElement().numEntitiesInCell;
        IndexT entityInfoIdx = node->GetElement().entityInfoStartIndex;
        IndexT index;                
        for (index = 0; index < numEntitiesInCell; index++)
        {
            const bbox& entityBox = entityInfos[entityInfoIdx].entityPtr->GetBoundingBox(); 
            ClipStatus::Type entityClipStatus;
            if (observerBox != 0)
            {
                entityClipStatus = observerBox->clipstatus(entityBox);
            }
            else if (observerFrustum != 0)
            {                           
                entityClipStatus = entityBox.clipstatus(*observerFrustum);
            }
            else
            {
                entityClipStatus = ClipStatus::Inside;
            }
            if (entityClipStatus != ClipStatus::Outside)
            {   
                if (0 != ((1<<entityInfos[entityInfoIdx].entityType) & entityTypeMask) && !entityInfos[entityInfoIdx].entityPtr->GetGfxEntity()->IsAlwaysVisible())
                {
                    visEntities[*numEntitiesVisible] = entityInfos[entityInfoIdx].entityPtr; 
                    (*numEntitiesVisible)++;
                }                           
            }
            // increment info index    
            n_assert(entityInfoIdx < numEntityInfos);
            entityInfoIdx++;      
        }
    }
    
    // recurse into child cells (if this cell is fully or partially visible)
    IndexT childIndex;
    // quadtree has always 0 or 4 childrenz
    const SizeT numChildren = node->GetChildAt(0) != 0 ? 4 : 0;
    for (childIndex = 0; childIndex < numChildren; childIndex++)
    {
        // jump to child cell        
        CheckCell(node->GetChildAt(childIndex), entityInfos, numEntityInfos, observerBox, observerFrustum, visEntities, numEntitiesVisible, entityTypeMask, clipStatus);        
    }
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityQuadtreeJobFunc(const JobFuncContext& ctx)
{
    QuadTree<VisibilityQuadtreeSystem::CellInfo>::Node* rootNode = (QuadTree<VisibilityQuadtreeSystem::CellInfo>::Node*)ctx.uniforms[0];    
    VisibilityQuadtreeSystem::EntityInfo* entityInfos = (VisibilityQuadtreeSystem::EntityInfo*)ctx.uniforms[1];
    SizeT numEntityInfos = ctx.uniformSizes[1] / sizeof(VisibilityQuadtreeSystem::EntityInfo);
    // FIXME: array elements ptr points to extra memory address
    Ptr<VisibilityContext>* output = (Ptr<VisibilityContext>*)ctx.outputs[0];
    SizeT numEntitiesVisible = 0;
    ObserverContext::ObserverCullingType type = (ObserverContext::ObserverCullingType)*(uint*)ctx.inputs[0];
    uint entityTypeMask = *(uint*)ctx.inputs[1];
    uint curCellIndex = 0;
    uint curEntitiesIndex = 0;

    //type = ObserverContext::SeeAll;
    switch (type)
    {
    case ObserverContext::BoundingBox:
        {
            bbox* observerBox = (bbox*)ctx.inputs[2];
            // go thru all boxes and compute clipspace
            CheckCell(rootNode, entityInfos, numEntityInfos, observerBox, 0, output, &numEntitiesVisible, entityTypeMask, ClipStatus::Invalid);
        }           
        break;
    case ObserverContext::ProjectionMatrix:
        {
            matrix44* observerFrustum = (matrix44*)ctx.inputs[2]; 
            CheckCell(rootNode, entityInfos, numEntityInfos, 0, observerFrustum, output, &numEntitiesVisible, entityTypeMask, ClipStatus::Invalid);
        }           
        break;
    case ObserverContext::SeeAll:
        // just copy all entities to output
        {    
            CheckCell(rootNode, entityInfos, numEntityInfos, 0, 0, output, &numEntitiesVisible, entityTypeMask, ClipStatus::Invalid); 
        }
        break;
    }
}

} // namespace Visibility
__ImplementSpursJob(Visibility::VisibilityQuadtreeJobFunc);

