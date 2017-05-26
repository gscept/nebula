//------------------------------------------------------------------------------
//  visibilityserver.cc
//  (C) 2010 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "visibility/visibilitychecker.h"     
#include "visibility/visibilitycontainer.h"

namespace Visibility
{
using namespace Util;
using namespace Math;
using namespace Graphics;

#define NUM_JOBS_PERFRAME 256
//------------------------------------------------------------------------------
/**
*/
VisibilityChecker::VisibilityChecker():
    isOpen(false),
    lastFrameId(0),
    numQueriesThisFrame(0)
{ 
    this->visiblityQueries[0].SetSize(NUM_JOBS_PERFRAME);   
    this->visiblityQueries[1].SetSize(NUM_JOBS_PERFRAME);
}

//------------------------------------------------------------------------------
/**
*/
VisibilityChecker::~VisibilityChecker()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityChecker::RegisterEntity(const Ptr<Graphics::GraphicsEntity>& entity)
{
    n_assert(!this->registeredEntities.Contains(entity));

    // create one new entity context for this graphicsentity
    Ptr<VisibilityContext> entityVis = VisibilityContext::Create();
    entityVis->Setup(entity);
    this->registeredEntities.Add(entity, entityVis);

    // insert in each attached visibility system
    IndexT i;
    for (i = 0; i < this->visibilitySystems.Size(); ++i)
    {
    	this->visibilitySystems[i]->InsertVisibilityContext(entityVis);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityChecker::UnregisterEntity(const Ptr<Graphics::GraphicsEntity>& entity)
{
    n_assert(this->registeredEntities.Contains(entity));

    IndexT i;
    for (i = 0; i < this->visibilitySystems.Size(); ++i)
    {
        this->visibilitySystems[i]->RemoveVisibilityContext(this->registeredEntities[entity]);
    }

    this->registeredEntities.Erase(entity);
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityChecker::AttachVisibilitySystem(const Ptr<VisibilitySystemBase>& system)
{
    n_assert(InvalidIndex == this->visibilitySystems.FindIndex(system));    
    this->visibilitySystems.Append(system);    
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityChecker::RemoveVisibilitySystem(const Ptr<VisibilitySystemBase>& system)
{
    IndexT index = this->visibilitySystems.FindIndex(system);
    n_assert(InvalidIndex != index);
    this->visibilitySystems.EraseIndex(index);
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityChecker::WorldChanged(const Math::bbox& worldBox)
{
	n_assert(this->isOpen);    
	IndexT i;
	for (i = 0; i < this->visibilitySystems.Size(); ++i)
	{
		this->visibilitySystems[i]->OnWorldChanged(worldBox);    	
	}        
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityChecker::UpdateVisibilityContext(const Ptr<Graphics::GraphicsEntity>& entity)
{         
    n_assert(this->registeredEntities.Contains(entity.get()));

    const Ptr<VisibilityContext>& entityVis = this->registeredEntities[entity.get()];
    entityVis->UpdateBoundingBox(entity->GetGlobalBoundingBox());
    IndexT i;
    for (i = 0; i < this->visibilitySystems.Size(); ++i)
    {
        this->visibilitySystems[i]->UpdateVisibilityContext(entityVis);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityChecker::PerformVisibilityQuery(IndexT frameId, const Ptr<Graphics::GraphicsEntity>& observerEntity, uint entityMask)
{      
    IndexT bufferIndex = frameId % 2;
    //IndexT bufferIndex = frameId % 2;
    if (this->lastFrameId != frameId)
    {
        this->numQueriesThisFrame = 0;         
    }
    IndexT slot = this->numQueriesThisFrame; 
    n_assert(slot < NUM_JOBS_PERFRAME);
    if (!this->visiblityQueries[bufferIndex][slot].isvalid())
    {
        //  create and start new visibility job   
        this->visiblityQueries[bufferIndex][slot] = VisibilityQuery::Create();
        // attach all visibility systems which are used for this observer type
        IndexT visSystemIdx;
        for (visSystemIdx = 0; visSystemIdx < this->visibilitySystems.Size(); ++visSystemIdx)
        {
            uint systemObserverMask = this->visibilitySystems[visSystemIdx]->GetObserverBitMask();
            uint observerMask = 1<<observerEntity->GetType();
            if (0 != (observerMask & systemObserverMask))
            {
                this->visiblityQueries[bufferIndex][slot]->AttachVisibilitySystem(this->visibilitySystems[visSystemIdx]);	
            }               
        }   
        this->visiblityQueries[bufferIndex][slot]->SetObserver(observerEntity);
        this->visiblityQueries[bufferIndex][slot]->SetEntityMask(entityMask);
        this->visiblityQueries[bufferIndex][slot]->Run(frameId); 
    }

    this->lastFrameId = frameId;
    this->numQueriesThisFrame++; 

    // apply result of last frame
    this->ApplyLastVisibilityResults(bufferIndex, slot);
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityChecker::ApplyLastVisibilityResults(IndexT frameId, IndexT slot)
{   
    //IndexT bufferIndex = (frameId + 1) % 2;   
    IndexT bufferIndex = frameId % 2;

    n_assert(bufferIndex < 2)

    if (this->visiblityQueries[bufferIndex][slot].isvalid())
    {
        if (this->visiblityQueries[bufferIndex][slot]->IsFinished()
            // if visibility test would last longer than one frame (double buffering) we have to wait for the result
            || this->visiblityQueries[bufferIndex][slot]->WaitForFinished()) 
        {    
            // apply result from job        
            const Ptr<GraphicsEntity>& observer = this->visiblityQueries[bufferIndex][slot]->GetObserver();

			// check if object has been removed from stage in the mean time
			if(observer->IsAttachedToStage())
			{		
				GraphicsEntity::LinkType linkType = this->visiblityQueries[bufferIndex][slot]->GetObserverType();

				// clear old links for observer
				observer->ClearLinks(linkType);

				// link visible entities with observer
				const Util::FixedArray<Ptr<VisibilityContext> >& visEntities = this->visiblityQueries[bufferIndex][slot]->GetVisibleEntities();
				IndexT entityIdx;
				for (entityIdx = 0; entityIdx < visEntities.Size(); ++entityIdx)
				{
					if (visEntities[entityIdx].isvalid())
					{  
						const Ptr<GraphicsEntity>& gfxEntity = visEntities[entityIdx]->GetGfxEntity();
						if (gfxEntity.isvalid() 
							&& gfxEntity->IsValid() 
							&& gfxEntity->IsActive())
						{
							gfxEntity->AddLink(linkType, observer);
							observer->AddLink(linkType, gfxEntity);
							if (linkType == GraphicsEntity::CameraLink)
							{    
								gfxEntity->OnNotifyCullingVisible(observer, frameId);
							}
						} 
					}
				}
			}

            // clear job in array
            this->visiblityQueries[bufferIndex][slot] = 0; 
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityChecker::Open()
{
    n_assert(!this->isOpen);    
    IndexT i;
    for (i = 0; i < this->visibilitySystems.Size(); ++i)
    {
        this->visibilitySystems[i]->Open(i);    	
    }        
    this->isOpen = true;        
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityChecker::Close()
{
    n_assert(this->isOpen);
    IndexT i;
    for (i = 0; i < this->visibilitySystems.Size(); ++i)
    {
        this->visibilitySystems[i]->Close();    	
    }
    this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityChecker::AttachVisibilitySystems(const Util::Array<Ptr<VisibilitySystemBase> >& systems)
{
    this->visibilitySystems.AppendArray(systems);      
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityChecker::OnRenderDebug()
{
    IndexT i;
    for (i = 0; i < this->visibilitySystems.Size(); ++i)
    {
        this->visibilitySystems[i]->OnRenderDebug();                	
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityChecker::ClearVisibilityLinks(GraphicsEntity::LinkType linkType)
{
    // clear old links for all registered entities
    IndexT entityIdx;
    for (entityIdx = 0; entityIdx < this->registeredEntities.Size(); ++entityIdx)
    {
        this->registeredEntities.ValueAtIndex(entityIdx)->GetGfxEntity()->ClearLinks(linkType);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityChecker::AttachVisibilityContainer(const Ptr<VisibilityContainer>& container)
{
    // convert containers graphcis entities to contexts
    const Array<Ptr<GraphicsEntity> >& entities = container->GetGraphicsEntities();
    if (entities.Size() > 0)
    {      
        Array<Ptr<VisibilityContext> > contexts;
        IndexT i;
        for (i = 0; i < entities.Size(); ++i)
        {           
            // create one new entity context for this graphicsentity
            Ptr<VisibilityContext> entityVis;
            if (!this->registeredEntities.Contains(entities[i]))
            {
                entityVis = VisibilityContext::Create();
                entityVis->Setup(entities[i]);
                this->registeredEntities.Add(entities[i], entityVis);	
            }
            else
            {
                entityVis = this->registeredEntities[entities[i]];
            }
            contexts.Append(entityVis);
        }
        container->SetVisibilityContexts(contexts);  
    }

    IndexT i;
    for (i = 0; i < this->visibilitySystems.Size(); ++i)
    {
        this->visibilitySystems[i]->InsertVisibilityContainer(container);
    }       
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityChecker::BeginAttachVisibilityContainer()
{
    IndexT i;
    for (i = 0; i < this->visibilitySystems.Size(); ++i)
    {
        this->visibilitySystems[i]->BeginAttachVisibilityContainer();
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityChecker::EndAttachVisibilityContainer()
{
    IndexT i;
    for (i = 0; i < this->visibilitySystems.Size(); ++i)
    {
        this->visibilitySystems[i]->EndAttachVisibilityContainer();
    }
}
} // namespace Visibility
