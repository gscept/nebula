//------------------------------------------------------------------------------
//  VisibilityClusterSystem.cc
//  (C) 2010 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "visibility/visibilitysystems/visibilityclustersystem.h"
#include "coregraphics/shaperenderer.h"
#include "threading/thread.h"

namespace Visibility
{
__ImplementClass(Visibility::VisibilityClusterSystem, 'VCLS', Visibility::VisibilitySystemBase);

using namespace CoreGraphics;
using namespace Util;
using namespace Math;
using namespace Threading;
using namespace Graphics;

//------------------------------------------------------------------------------
/**
*/
VisibilityClusterSystem::VisibilityClusterSystem():
    inBeginAttachCluster(false),
    statsNumVisChecksAccepted(0),
    statsNumVisChecksRejected(0)
{
}

//------------------------------------------------------------------------------
/**
*/
VisibilityClusterSystem::~VisibilityClusterSystem()
{
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityClusterSystem::Open(IndexT orderIndex)
{    
    n_assert2(orderIndex > 0, "VisibilityClusterSystem have to be second or higher in list!");
    VisibilitySystemBase::Open(orderIndex);
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityClusterSystem::Close()
{                                
    this->DiscardVisibilityClusters();
    VisibilitySystemBase::Close();        
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityClusterSystem::InsertVisibilityContext(const Ptr<VisibilityContext>& context)
{
    // do nothing
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityClusterSystem::RemoveVisibilityContext(const Ptr<VisibilityContext>& context)
{
    // remove entity from cluster
    // could be a object that isn't in any cluster, like dynamic objects
    if (this->visContextBitmask.Contains(context))
    {
        this->visContextBitmask.Erase(context);                    
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityClusterSystem::UpdateVisibilityContext(const Ptr<VisibilityContext>& context)
{    
    // only for static objects, nothing to do on transform change    
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityClusterSystem::CheckVisibility(const Ptr<Observer>& observer, Util::FixedArray<Ptr<VisibilityContext> >& outEntitiyArray, uint entityMask)
{        
    // update visibility for this observer
    const point& cameraPos = observer->GetObserverEntity()->GetTransform().get_position();
    this->UpdateVisibilityClusters(cameraPos);

    // now go thru visiblecontexts, checked before
    // cause this vis system comes after other systems, we work on the outEntitiyArray
    // and throw out any non visible contexts
    IndexT i;
    for (i = 0; i < outEntitiyArray.Size(); ++i)
    {
        // visible context must not be in any cluster
        if (!this->visContextBitmask.Contains(outEntitiyArray[i]))
        {
            continue;
        }

        // we are invisible, if our cluster bit mask is identical with
        // the visibility bit mask from the visibility server
        bool visible = true;
        const BitField<64>& visContextBitmask = this->visContextBitmask[outEntitiyArray[i]];
        if (!visContextBitmask.IsNull())
        {
            const BitField<64>& cameraClusterMask = this->GetVisibilityClusterBitMask();
            if (!cameraClusterMask.IsNull())
            {
                BitField<64> andMask = BitField<64>::And(cameraClusterMask, visContextBitmask);
                if (cameraClusterMask == andMask)
                {
                    visible = false;
                }
            }
        }
        if (visible)
        {
            this->IncrAcceptedVisChecks();
        }
        else
        {
            // cull vis context
            outEntitiyArray[i] = 0;
            --i;
            this->IncrRejectedVisChecks();
        }            
    }
}  

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityClusterSystem::InsertVisibilityContainer(const Ptr<VisibilityContainer>& container)
{
    n_assert(this->inAttachContainer);
    if (container->IsA(VisibilityCluster::RTTI))
    { 
        const Ptr<VisibilityCluster>& visCluster = container.cast<VisibilityCluster>();        
        n_assert(this->isOpen);
        if (this->visClusters.Size() >= 64)
        {
            n_error("VisibilityClusterSystem: too many visibility clusters in level (max=64)!");
        }
        visCluster->SetBitIndex(this->visClusters.Size());
        visCluster->Setup();
        // save link from entities to its cluster as bit mask
        const Util::Array<Ptr<VisibilityContext> >& visContexts = visCluster->GetVisibilityContexts();
        IndexT i;
        for (i = 0; i < visContexts.Size(); ++i)
        {
            if (this->visContextBitmask.Contains(visContexts[i]))
            {
                this->visContextBitmask[visContexts[i]].SetBit(visCluster->GetBitIndex());
            }
            else
            {
                Util::BitField<64> newClusterBitMask;
                newClusterBitMask.SetBit(visCluster->GetBitIndex());
                this->visContextBitmask.Add(visContexts[i], newClusterBitMask);    	
            }        
        }
        this->visClusters.Append(visCluster);  
    }
}    

//------------------------------------------------------------------------------
/**
*/
void
VisibilityClusterSystem::DiscardVisibilityClusters()
{
    IndexT i;
    for (i = 0; i < this->visClusters.Size(); i++)
    {
        this->visClusters[i]->Discard();
    }
    this->visClusters.Clear();
}

//------------------------------------------------------------------------------
/**
Update visibility status of visibility clusters. The result of this
is the bit mask of clusters which have the camera inside.
*/
void
VisibilityClusterSystem::UpdateVisibilityClusters(const point& cameraPos)
{
    this->visibilityClusterBitMask.Clear();
    IndexT i;
    for (i = 0; i < this->visClusters.Size(); i++)
    {
        const Ptr<VisibilityCluster>& visCluster = this->visClusters[i];
        if (visCluster->IsPointInside(cameraPos))
        {
            visCluster->SetCameraInside(true);
            this->visibilityClusterBitMask.SetBit(visCluster->GetBitIndex());
        }
        else
        {
            visCluster->SetCameraInside(false);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Ptr<VisibilityCluster> > 
VisibilityClusterSystem::FindClustersContainingPoint(const Math::point& pos)
{
    Util::Array<Ptr<VisibilityCluster> > result;
    IndexT i;
    for (i = 0; i < this->visClusters.Size(); ++i)
    {
        if (visClusters[i]->IsPointInside(pos))    
        {
            result.Append(visClusters[i]);            
        }
    }
    return result;
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityClusterSystem::OnRenderDebug()
{
    IndexT i;
    for (i = 0; i < this->visClusters.Size(); ++i)
    {
        this->visClusters[i]->RenderDebug();        
    }
}

//------------------------------------------------------------------------------
/**
*/
Ptr<Jobs::Job> 
VisibilityClusterSystem::CreateVisibilityJob(IndexT frameId, const Ptr<Observer>& observer, Util::FixedArray<Ptr<VisibilityContext> >& outEntitiyArray, uint& entityMask)
{
    // no multi threading yet
    this->CheckVisibility(observer, outEntitiyArray, entityMask);

    Ptr<Jobs::Job> visibilityJob;
    return visibilityJob;
}
} // namespace Visibility
