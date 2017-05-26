#pragma once
//------------------------------------------------------------------------------
/**
    @class Visibility::VisibilityQuadtree

    Simple quadtree for culling. 
    Entities are automatically sorted into quadtree.
       
    (C) 2010 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "visibility/visibilitysystems/visibilitysystembase.h"
#include "visibility/visibilitysystems/visibilitycell.h"
#include "util/quadtree.h"
#include "jobs/job.h"
              
//------------------------------------------------------------------------------
namespace Visibility
{    
class VisibilityQuadtree : public VisibilitySystemBase
{
    __DeclareClass(VisibilityQuadtree);
public:      
    struct CellInfo 
    {
        SizeT numEntitiesInHierarchy;
        IndexT entityInfoStartIndex;
        IndexT numEntitiesInCell;
    };

    struct EntityInfo
    {
        Math::bbox entityBox;        
        VisibilityContext* entityPtr;
        uint entityType;
    };
    /// constructor
    VisibilityQuadtree();
    /// destructor
    virtual ~VisibilityQuadtree();
                                                                       
    /// set quad tree depth and boudning box
    void SetQuadTreeSettings(uchar depth, const Math::bbox& worldBBox);
    /// open the graphics server
    virtual void Open(IndexT orderIndex);
    /// close the graphics server
    virtual void Close();

    /// insert entity visibility
    virtual void InsertVisibilityContext(const Ptr<VisibilityContext>& entityVis);
    /// remove entity visibility
    virtual void RemoveVisibilityContext(const Ptr<VisibilityContext>& entityVis);
    /// update entity visibility
    virtual void UpdateVisibilityContext(const Ptr<VisibilityContext>& entityVis);

	/// resizes quadtree
	virtual void OnWorldChanged(const Math::bbox& box);
	/// update cell sizes
	virtual void ResizeVisibilityCells(const Ptr<VisibilityCell>& cell, uchar curLevel, ushort curCol, ushort curRow);
                
    /// attach visibility job to port
    virtual Ptr<Jobs::Job> CreateVisibilityJob(IndexT frameId, const Ptr<ObserverContext>& observer, Util::FixedArray<Ptr<VisibilityContext> >& outEntitiyArray, uint& entityMask);
    /// render debug visualizations
    virtual void OnRenderDebug();
    /// get observer type mask
    virtual uint GetObserverBitMask() const;

private:
    /// create a quad tree and its children, recursively
    Ptr<VisibilityCell> CreateQuadTreeCell(VisibilityCell* parentCell, uchar curLevel, ushort curCol, ushort curRow);
    /// render quadtree cell
    void RenderCell(const Ptr<VisibilityCell>& cell, const Math::float4& color);
    /// prepare job input data from quadtree
    void PrepareTreeData(IndexT bufferIndex); 
    /// collect context ptr
    void CollectContextPtr(const Ptr<VisibilityCell>& cell, Util::QuadTree<CellInfo>::Node* node, IndexT& curEntityIndex);
    /// mark tree structure as dirty
    void SetDirty();

    int numCellsBuilt;
    uchar quadTreeDepth;
    Math::bbox quadTreeBox;
    Util::QuadTree<CellInfo> quadTree;
    Util::FixedArray<EntityInfo> entityInfos;
    Ptr<VisibilityCell> rootCell;
    Util::Dictionary<Ptr<VisibilityContext>, Ptr<VisibilityCell> > contextCellMapping;    
    struct JobWorkData
    {
        void* workBuffer;
        bool bufferDirty;  
        SizeT quadtreeSize;
        SizeT entityInfosSize;
    };
    JobWorkData workData[2];
};

//------------------------------------------------------------------------------
/**
*/
inline void
VisibilityQuadtree::SetQuadTreeSettings(uchar depth, const Math::bbox& worldBox)
{
    this->quadTreeDepth = depth;
    this->quadTreeBox = worldBox;
}
//------------------------------------------------------------------------------
/**
*/
inline uint 
VisibilityQuadtree::GetObserverBitMask() const
{
    return (1 << Graphics::GraphicsEntityType::Camera) | (1 << Graphics::GraphicsEntityType::Light);
}
} // namespace Visibility
//------------------------------------------------------------------------------

