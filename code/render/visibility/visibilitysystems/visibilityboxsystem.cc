//------------------------------------------------------------------------------
//  VisibilityBoxSystem.cc
//  (C) 2010 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "visibility/visibilitysystems/visibilityboxsystem.h"
#include "coregraphics/shaperenderer.h"   
#include "threading/thread.h"      
#include "jobs/job.h"
#include "jobs/jobdatadesc.h"
#include "jobs/jobuniformdesc.h"

namespace Visibility
{
__ImplementClass(Visibility::VisibilityBoxSystem, 'VBOS', Visibility::VisibilitySystemBase);

using namespace CoreGraphics;
using namespace Util;
using namespace Math;
using namespace Threading;
using namespace Graphics;
using namespace Jobs;
// job function declaration
#if __PS3__
extern "C" {
    extern const char _binary_jqjob_render_visibilityboxsystemjobfunc_ps3_bin_start[];
    extern const char _binary_jqjob_render_visibilityboxsystemjobfunc_ps3_bin_size[];
}
#else
extern void VisibilityBoxSystemJobFunc(const JobFuncContext& ctx);
#endif
//------------------------------------------------------------------------------
/**
*/
VisibilityBoxSystem::VisibilityBoxSystem():
    numVisibleBoxes(0),
    numCameraBoxes(0) 
{
}

//------------------------------------------------------------------------------
/**
*/
VisibilityBoxSystem::~VisibilityBoxSystem()
{
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityBoxSystem::Open(IndexT orderIndex)
{    
    n_assert2(orderIndex > 0, "VisibilityBoxSystem have to be second or higher in list!");
    VisibilitySystemBase::Open(orderIndex);
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityBoxSystem::Close()
{                                
    this->DiscardVisibilityBoxes();
    VisibilitySystemBase::Close();        
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityBoxSystem::InsertVisibilityContext(const Ptr<VisibilityContext>& context)
{
    // find boxes which overlap the context bbox
    Util::Array<Ptr<VisibilityBox> > outBoxes;
    SizeT numBoxes = this->FindOverlappingVisibilityBoxes(context->GetBoundingBox(), outBoxes);
    if (numBoxes > 0)
    {
        n_assert(!this->boxMapping.Contains(context));
        this->boxMapping.Add(context, outBoxes); 
        this->SetDirty();
    }                                                   
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityBoxSystem::RemoveVisibilityContext(const Ptr<VisibilityContext>& context)
{
    // clear references
    if (this->boxMapping.Contains(context))
    {
        this->boxMapping.Erase(context);
        this->SetDirty();
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityBoxSystem::UpdateVisibilityContext(const Ptr<VisibilityContext>& context)
{    
    // clear old references
    if (this->boxMapping.Contains(context))
    {
        this->boxMapping.Erase(context);
    }
    // check for new visibility boxes 
    // find boxes which overlap the context bbox
    Util::Array<Ptr<VisibilityBox> > outBoxes;
    SizeT numBoxes = this->FindOverlappingVisibilityBoxes(context->GetBoundingBox(), outBoxes);
    if (numBoxes > 0)
    {
        this->boxMapping.Add(context, outBoxes);
        this->SetDirty();
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityBoxSystem::CheckVisibility(const Ptr<Observer>& observer, Util::Array<Ptr<VisibilityContext> >& outEntitiyArray, uint entityMask)
{        
    // update visibility boxes for this observer    
    this->UpdateVisibilityBoxes(observer);

    // now go thru visiblecontexts, checked before
    // cause this vis system comes after other systems, we work on the outEntitiyArray
    // and throw out any non visible contexts
    IndexT contextIdx;
    for (contextIdx = 0; contextIdx < outEntitiyArray.Size(); ++contextIdx)
    {
        if (this->boxMapping.Contains(outEntitiyArray[contextIdx]))
        {                    
            IndexT visBoxIndex;
            const Array<Ptr<VisibilityBox> >& boxes = this->boxMapping[outEntitiyArray[contextIdx]];
            for (visBoxIndex = 0; visBoxIndex < boxes.Size(); visBoxIndex++)
            {
                if (!boxes[visBoxIndex]->IsVisible())
                {
                    outEntitiyArray.EraseIndex(contextIdx);
                    break;
                }
            }            
        }
    }
}  

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityBoxSystem::InsertVisibilityContainer(const Ptr<VisibilityContainer>& container)
{
    n_assert(this->inAttachContainer);
    if (container->IsA(VisibilityBox::RTTI))
    {
        const Ptr<VisibilityBox>& visBox = container.cast<VisibilityBox>();        
        n_assert(this->isOpen);    
        this->visBoxes.Append(visBox);
        this->flatBoxData.Append(visBox->GetVisibilityBoxJobData());
    }                                     
}    

//------------------------------------------------------------------------------
/**
*/
void
VisibilityBoxSystem::DiscardVisibilityBoxes()
{
    IndexT i;
    for (i = 0; i < this->visBoxes.Size(); i++)
    {
        this->visBoxes[i]->Discard();
    }
    this->visBoxes.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityBoxSystem::UpdateNeighbours(const Ptr<VisibilityBox>& visBox)
{
    n_assert(visBox->GetNeighbours().Size() == 0);
    IndexT i;
    for (i = 0; i < this->visBoxes.Size(); i++)
    {        
        if (visBox != this->visBoxes[i])
        {
            if (visBox->IsVisibilityBoxOverlapping(this->visBoxes[i]))
            {
                visBox->AttachNeighbour(i);
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
    Update visibility status of visibility boxes.
*/
void
VisibilityBoxSystem::UpdateVisibilityBoxes(const Ptr<Observer>& observer)
{
    const point& cameraPos = observer->GetEntity()->transform->get_position();

    // reset stats member variables
    this->numVisibleBoxes = 0;
    this->numCameraBoxes = 0;

    // first clear the visibility of all visibility boxes
    IndexT i;
    for (i = 0; i < this->visBoxes.Size(); i++)
    {
        VisibilityBox* curBox = this->visBoxes[i];
        curBox->SetVisible(false);
        curBox->SetCameraBox(false);
        curBox->SetProcessed(false);
    }

    // first pass: find all first order boxes (which contain the camera position)
    for (i = 0; i < this->visBoxes.Size(); i++)
    {
        VisibilityBox* curBox = this->visBoxes[i];
        if (curBox->IsPointInside(cameraPos))
        {
            curBox->SetCameraBox(true);
            this->numCameraBoxes++;
        }
    }

    // if camera is not inside any box, need to make all boxes visible
    if (0 == this->numCameraBoxes)
    {
        for (i = 0; i < visBoxes.Size(); i++)
        {
            this->visBoxes[i]->SetVisible(true);
            this->numVisibleBoxes++;
        }
    }
    else
    {
        // second pass: set visibility flag on all camera boxes, and their
        // direct neighbours
        for (i = 0; i < visBoxes.Size(); i++)
        {
            VisibilityBox* curBox = this->visBoxes[i];
            if (curBox->IsCameraBox())
            {
                // all camera boxes are definitely visible
                curBox->SetVisible(true);
                curBox->SetProcessed(true);
                this->numVisibleBoxes++;

                // scan direct neighbours, set them to visible if they are
                // in the view frustum
                const Array<IndexT>& neighbours = curBox->GetNeighbours();
                IndexT neighbourIndex;
                for (neighbourIndex = 0; neighbourIndex < neighbours.Size(); neighbourIndex++)
                {
                    VisibilityBox* curNeighbour = this->visBoxes[neighbours[neighbourIndex]];

                    // skip already processed boxes
                    if (!curNeighbour->IsProcessed())
                    {
                        curNeighbour->SetProcessed(true);
                        bool visible = true;
                        if (observer->GetType() == ObserverType::Perspective)
                        {
                           frustum viewFrustum = frustum(observer->GetProjectionMatrix());
                           visible = curNeighbour->IsFrustumOverlapping(viewFrustum);
                        }
                        else if(observer->GetType() == ObserverType::BoundingBox)
                        {
                            frustum viewFrustum;
                            viewFrustum.set(observer->GetBoundingBox(), matrix44::identity());
                            visible = curNeighbour->IsFrustumOverlapping(viewFrustum);
                        }
                        if (visible)
                        {
                            curNeighbour->SetVisible(true);
                            this->numVisibleBoxes++;
                        }
                    }
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
This method finds all visibility boxes which overlap a bounding
box in global space. This method is called by the VisibilityChecker class
when their bounding box changes to find "their" visibility boxes.
*/
SizeT
VisibilityBoxSystem::FindOverlappingVisibilityBoxes(const bbox& boundingBox, Util::Array<Ptr<VisibilityBox> >& outBoxes) const
{
    n_assert(outBoxes.IsEmpty());
    IndexT i;
    for (i = 0; i < this->visBoxes.Size(); i++)
    {
        if (this->visBoxes[i]->IsBoundingBoxOverlapping(boundingBox))
        {
            outBoxes.Append(this->visBoxes[i]);
        }
    }
    return outBoxes.Size();
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityBoxSystem::OnRenderDebug()
{
    IndexT i;
    for (i = 0; i < this->visBoxes.Size(); ++i)
    {
        this->visBoxes[i]->SetCameraBox(this->flatBoxData[i].isCameraBox);
        this->visBoxes[i]->SetVisible(this->flatBoxData[i].isVisible);
        this->visBoxes[i]->RenderDebug();        
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityBoxSystem::EndAttachVisibilityContainer()
{
    // setup neighbours for all boxes
    IndexT i;
    for (i = 0; i < this->visBoxes.Size(); i++)
    {
        this->UpdateNeighbours(this->visBoxes[i]);
        // build neighbor list for thread job
        this->flatNeighborList.Append(this->visBoxes[i]->GetNeighbours().Size());
        this->flatNeighborList.AppendArray(this->visBoxes[i]->GetNeighbours());
    }
    VisibilitySystemBase::EndAttachVisibilityContainer();
}

//------------------------------------------------------------------------------
/**
    Every generated buffer in this function has to doublebuffered!
*/
Ptr<Jobs::Job> 
VisibilityBoxSystem::CreateVisibilityJob(IndexT frameId, const Ptr<ObserverContext>& observer, Util::FixedArray<Ptr<VisibilityContext> >& outEntitiyArray, uint& entityMask)
{               
    // collect all boxes for given entities in double buffer
    IndexT bufferIndex = frameId % 2;
    if (workData[bufferIndex].dirty)
    {
        this->CollectFlattenBoxMapping(bufferIndex);
        workData[bufferIndex].dirty = false;
    }
    
    // nothing to do if no entities are visible from rpevoius visibility systems
    if (this->workData[bufferIndex].flatBoxMapping.Size() == 0) 
    {
        return 0;
    }
    // only for cameras used, not lights
    n_assert(observer->GetType() == ObserverContext::ProjectionMatrix);
    // create new job           
    Ptr<Jobs::Job> visibilityJob = Jobs::Job::Create();
    // input data for job  
    // function
    JobFuncDesc jobFunction(VisibilityBoxSystemJobFunc);        
    // uniform data     

    JobUniformDesc uniformData(this->flatBoxData.Begin(), this->flatBoxData.Size(),  
                               this->flatNeighborList.Begin(), this->flatNeighborList.Size(), 0);  

    // update observer data    
    JobDataDesc inputData(&observer->GetPosition(), sizeof(Math::point), sizeof(Math::point), 
                          &entityMask, sizeof(uint), sizeof(uint), 
                          &observer->GetProjectionMatrix(), sizeof(matrix44), sizeof(matrix44),
                          this->workData[bufferIndex].flatBoxMapping.Begin(), this->workData[bufferIndex].flatBoxMapping.Size(), this->workData[bufferIndex].flatBoxMapping.Size());
    
    // update output data
    JobDataDesc outputData(outEntitiyArray.Begin(), outEntitiyArray.Size(), outEntitiyArray.Size());
    // setup job with data
    visibilityJob->Setup(uniformData, inputData, outputData, jobFunction);

    return visibilityJob;
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityBoxSystem::CollectFlattenBoxMapping(IndexT bufferIndex)
{
    this->workData[bufferIndex].flatBoxMapping.Clear();
    Util::Dictionary<Ptr<VisibilityContext>, Util::Array<IndexT> > flatSortedMapping;
    flatSortedMapping.BeginBulkAdd();
    IndexT i;
    for (i = 0; i < this->boxMapping.Size(); ++i)
    {                               
        Util::Array<IndexT> boxIndices;
        const Util::Array<Ptr<VisibilityBox> >& boxes = this->boxMapping.ValueAtIndex(i);
        boxIndices.Append(boxes.Size());
        IndexT boxIdx;
        for (boxIdx = 0; boxIdx < boxes.Size(); ++boxIdx)
        {
            IndexT boxIndexInArray = this->visBoxes.FindIndex(boxes[boxIdx]);
            n_assert(boxIndexInArray != InvalidIndex);
            boxIndices.Append(boxIndexInArray);                	
        }
        flatSortedMapping.Add(this->boxMapping.KeyAtIndex(i), boxIndices);    
    }
    flatSortedMapping.EndBulkAdd();
    IndexT dictIdx;
    for (dictIdx = 0; dictIdx < flatSortedMapping.Size(); ++dictIdx)
    {
        // works only on 32 bit systems!!!
        this->workData[bufferIndex].flatBoxMapping.Append((uint)flatSortedMapping.KeyAtIndex(dictIdx).get());
        this->workData[bufferIndex].flatBoxMapping.AppendArray(flatSortedMapping.ValueAtIndex(dictIdx));
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityBoxSystem::SetDirty()
{
    this->workData[0].dirty = true;
    this->workData[1].dirty = true;
}
} // namespace Visibility
