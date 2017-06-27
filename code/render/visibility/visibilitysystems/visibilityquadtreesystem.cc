//------------------------------------------------------------------------------
//  visibilityserver.cc
//  (C) 2010 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "visibility/visibilitysystems/visibilityquadtreesystem.h"
#include "coregraphics/shaperenderer.h"
#include "threading/thread.h"
#include "jobs/jobdatadesc.h"
#include "jobs/jobuniformdesc.h"

namespace Visibility
{
    __ImplementClass(Visibility::VisibilityQuadtreeSystem, 'VIQT', Visibility::VisibilitySystemBase);

using namespace CoreGraphics;
using namespace Jobs;
using namespace Util;
using namespace Math;
using namespace Threading;
using namespace Graphics;

// job function declaration
#if __PS3__
extern "C" {
    extern const char _binary_jqjob_render_visibilityquadtreejobfunc_ps3_bin_start[];
    extern const char _binary_jqjob_render_visibilityquadtreejobfunc_ps3_bin_size[];
}
#else
extern void VisibilityQuadtreeJobFunc(const JobFuncContext& ctx);
#endif
//------------------------------------------------------------------------------
/**
*/
VisibilityQuadtreeSystem::VisibilityQuadtreeSystem():
    quadTreeDepth(0)
{
}

//------------------------------------------------------------------------------
/**
*/
VisibilityQuadtreeSystem::~VisibilityQuadtreeSystem()
{
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityQuadtreeSystem::Open(IndexT orderIndex)
{    
    // this visibility system have to come first, before any other
    n_assert2(orderIndex == 0, "VisibilityQuadtree have to come first, before any other!");
    this->quadTree.Setup(this->quadTreeBox, this->quadTreeDepth);
    this->numCellsBuilt = 0;
    this->rootCell = this->CreateQuadTreeCell(0, 0, 0, 0);

    VisibilitySystemBase::Open(orderIndex);
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityQuadtreeSystem::Close()
{           
    this->contextCellMapping.Clear();
    this->rootCell->OnRemove();
    this->rootCell = nullptr;
    VisibilitySystemBase::Close();        
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityQuadtreeSystem::InsertVisibilityContext(const Ptr<VisibilityContext>& context)
{
    const Ptr<VisibilityCell>& cell = this->rootCell->InsertContext(context);
    this->contextCellMapping.Add(context, cell);
    this->SetDirty();
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityQuadtreeSystem::RemoveVisibilityContext(const Ptr<VisibilityContext>& context)
{
    this->contextCellMapping[context]->RemoveContext(context);    
    this->contextCellMapping.Erase(context);
    this->SetDirty();
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityQuadtreeSystem::UpdateVisibilityContext(const Ptr<VisibilityContext>& context)
{       
    const Ptr<VisibilityCell>& oldCell = this->contextCellMapping[context];
    const Ptr<VisibilityCell>& newCell = oldCell->FindEntityContainmentCell(context);
    if (oldCell != newCell)
    {                                      
        oldCell->RemoveContext(context);
        newCell->AttachContext(context);
        this->contextCellMapping.Erase(context);   
        this->contextCellMapping.Add(context, newCell);
        this->SetDirty();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityQuadtreeSystem::OnWorldChanged(const Math::bbox& box)
{
	this->quadTreeBox = box;
	this->quadTree.Setup(this->quadTreeBox, this->quadTreeDepth);
	this->ResizeVisibilityCells(this->rootCell, 0, 0, 0);
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityQuadtreeSystem::ResizeVisibilityCells(const Ptr<VisibilityCell>& cell, uchar curLevel, ushort curCol, ushort curRow)
{
	int nodeIndex = this->quadTree.GetNodeIndex(curLevel, curCol, curRow);
	const QuadTree<CellInfo>::Node& node = this->quadTree.GetNodeByIndex(nodeIndex);
	cell->SetBoundingBox(node.GetBoundingBox());

	// create child cells
	uchar childLevel = curLevel + 1;
	if (childLevel < this->quadTree.GetDepth())
	{
		const Util::Array<Ptr<VisibilityCell>>& cells = cell->GetChildCells();
		ushort i;
		for (i = 0; i < cells.Size(); i++)
		{
			ushort childCol = 2 * curCol + (i & 1);
			ushort childRow = 2 * curRow + ((i & 2) >> 1);
			this->ResizeVisibilityCells(cells[i], childLevel, childCol, childRow);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
Ptr<VisibilityCell> 
VisibilityQuadtreeSystem::CreateQuadTreeCell(VisibilityCell* parentCell, uchar curLevel, ushort curCol, ushort curRow)
{
    // create a new cell
    Ptr<VisibilityCell> cell = VisibilityCell::Create();
    int nodeIndex = this->quadTree.GetNodeIndex(curLevel, curCol, curRow);
    const QuadTree<CellInfo>::Node& node = this->quadTree.GetNodeByIndex(nodeIndex);
    cell->SetBoundingBox(node.GetBoundingBox());        
    this->numCellsBuilt++;

    // create child cells
    uchar childLevel = curLevel + 1;
    if (childLevel < this->quadTree.GetDepth())
    {        
        ushort i;
        for (i = 0; i < 4; i++)
        {
            ushort childCol = 2 * curCol + (i & 1);
            ushort childRow = 2 * curRow + ((i & 2) >> 1);
            Ptr<VisibilityCell> childCell = this->CreateQuadTreeCell(cell, childLevel, childCol, childRow);
            cell->AttachChildCell(childCell);
        }        
    }
    return cell;
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityQuadtreeSystem::OnRenderDebug()
{
    // render boxes for cells
    float alpha = 0.2f / this->quadTreeDepth;
    float4 color(1, 1, 1, alpha);
    this->RenderCell(this->rootCell, color);
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityQuadtreeSystem::RenderCell(const Ptr<VisibilityCell>& cell, const Math::float4& color)
{
    // render    
    Math::float4 curColor(color);   
    if (cell->GetEntityContexts().Size() > 0)
    {
        int u = 0;
        int v = 0;
        int y = 255;
        IndexT i;
        for (i = 0; i < cell->GetEntityContexts().Size(); ++i)
        {
        	switch (cell->GetEntityContexts()[i]->GetGfxEntity()->GetType())
            {
            case ObserverMask::Graphics:
                u = 255;                
                break;
            case ObserverMask::Observers:
                v = 255;
                break;
            case GraphicsEntityType::Light:                
                u -= 128;
                v -= 128;
                break;
            }
        }
        // convert yuv to rgb
        curColor.x() = (float)(n_min((9535 * (y - 16) + 13074 * (v - 128)) >> 13, 255)) / 255.0f;
        curColor.y() = (float)(n_min((9535 * (y - 16) - 6660 * (v - 128) - 3203 * (u - 128)) >> 13, 255))/ 255.0f;
        curColor.z() = (float)(n_min((9535 * (y - 16) + 16531 * (u - 128)) >> 13, 255))/ 255.0f; 
        curColor.w() *= 2.0f;
    }
    ShapeRenderer::Instance()->AddShape(RenderShape(Thread::GetMyThreadId(), RenderShape::Box, RenderShape::CheckDepth, cell->GetBoundingBox().to_matrix44(), curColor));    
    ShapeRenderer::Instance()->AddWireFrameBox(cell->GetBoundingBox(), curColor, Thread::GetMyThreadId());
    
    
    IndexT i;
    for (i = 0; i < cell->GetChildCells().Size(); ++i)
    {
        this->RenderCell(cell->GetChildCells()[i], color);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityQuadtreeSystem::PrepareTreeData(IndexT bufferIndex)
{   
    // allocate memory for entity infos
    SizeT numEntitiesInTree = this->rootCell->GetNumEntitiesInHierarchy();
    this->entityInfos.SetSize(numEntitiesInTree);
    IndexT curEntityInfoIdx = 0;
    this->CollectContextPtr(this->rootCell, &this->quadTree.NodeByIndex(0), curEntityInfoIdx);

    workData[bufferIndex].quadtreeSize = this->quadTree.GetNumNodesInTree() * sizeof(QuadTree<CellInfo>::Node); 
    workData[bufferIndex].entityInfosSize = this->entityInfos.Size() * sizeof(EntityInfo);
    // copy tree, could change on next visibility query, so we need own mem copy for this job    
    workData[bufferIndex].workBuffer = Memory::Alloc(Memory::ScratchHeap, workData[bufferIndex].quadtreeSize + workData[bufferIndex].entityInfosSize);
    Memory::Copy(&this->quadTree.NodeByIndex(0), workData[bufferIndex].workBuffer, workData[bufferIndex].quadtreeSize);
    Memory::Copy(this->entityInfos.Begin(), (uchar*)workData[bufferIndex].workBuffer + workData[bufferIndex].quadtreeSize, workData[bufferIndex].entityInfosSize);
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityQuadtreeSystem::CollectContextPtr(const Ptr<VisibilityCell>& cell, Util::QuadTree<CellInfo>::Node* node, IndexT& curEntityIndex)
{
    SizeT numEntitiesInCell = cell->GetEntityContexts().Size();
    n_assert(numEntitiesInCell < 65535);    
             
    // save number of entites in cell and start index of entity infos
    CellInfo& info = const_cast<CellInfo&>(node->GetElement());
    info.numEntitiesInHierarchy = cell->GetNumEntitiesInHierarchy();
    info.entityInfoStartIndex = curEntityIndex;
    info.numEntitiesInCell = numEntitiesInCell;

    // save additional infos per entity in linear array
    IndexT i;
    for (i = 0; i < numEntitiesInCell; ++i)
    {
        this->entityInfos[curEntityIndex].entityPtr = cell->GetEntityContexts()[i].get();
        this->entityInfos[curEntityIndex].entityBox = cell->GetEntityContexts()[i]->GetBoundingBox();
        this->entityInfos[curEntityIndex].entityType = cell->GetEntityContexts()[i]->GetType();        
        curEntityIndex++;
    }      

    IndexT childIdx;
    for (childIdx = 0; childIdx < cell->GetChildCells().Size(); ++childIdx)
    {
        this->CollectContextPtr(cell->GetChildCells()[childIdx], node->GetChildAt(childIdx), curEntityIndex);
    }       
}

//------------------------------------------------------------------------------
/**
*/
Ptr<Jobs::Job> 
VisibilityQuadtreeSystem::CreateVisibilityJob(IndexT frameId, const Ptr<Observer>& observer, Util::FixedArray<Ptr<VisibilityContext> >& outEntityArray, uint& entityMask)
{   
    IndexT bufferIndex = frameId % 2;
    // first check if tree data is dirty
    if (workData[bufferIndex].bufferDirty)
    {
        this->PrepareTreeData(bufferIndex);      
        workData[bufferIndex].bufferDirty = false;
    } 
    // create new job           
    Ptr<Jobs::Job> visibilityJob = Jobs::Job::Create();
    // input data for job  
    // function
    JobFuncDesc jobFunction(VisibilityQuadtreeJobFunc);        
    // uniform data     
    
    JobUniformDesc uniformData(workData[bufferIndex].workBuffer, workData[bufferIndex].quadtreeSize, (uchar*)workData[bufferIndex].workBuffer + workData[bufferIndex].quadtreeSize, workData[bufferIndex].entityInfosSize, 0);  
    uint dummy;
    // update observer data
	JobDataDesc inputData({
		std::make_tuple(observer->GetTypeRef(), sizeof(void*), sizeof(void*)),
		std::make_tuple(&entityMask, sizeof(void*), sizeof(void*)),
		std::make_tuple(&dummy, 1, 1)
	});
    switch (observer->GetType())
    {
    case ObserverType::BoundingBox:      
        inputData.Update(2, &observer->GetBoundingBox(), sizeof(bbox), sizeof(bbox));
        break;
	case ObserverType::Orthographic:
    case ObserverType::Perspective:
        inputData.Update(2, &observer->GetProjectionMatrix(), sizeof(matrix44), sizeof(matrix44));
        break;
    case ObserverType::All:
        // nothing to do sees all
        break;
    }

    // update output data
    JobDataDesc outputData(outEntityArray.Begin(), outEntityArray.Size() * sizeof(Ptr<VisibilityContext>), outEntityArray.Size() * sizeof(Ptr<VisibilityContext>));

    // setup job with data
    visibilityJob->Setup(uniformData, inputData, outputData, jobFunction);
    
    return visibilityJob;
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityQuadtreeSystem::SetDirty()
{
    this->workData[0].bufferDirty = true;
    this->workData[1].bufferDirty = true;
}
} // namespace Visibility
