#pragma once
//------------------------------------------------------------------------------
/**
    @class Visibility::VisibilityBoxSystem

    The VisibilityBoxSystem allows to group entities in visibility boxes.
    If the camera is inside any box, only this box and all its neighbors (the overlapping boxes)
    are visible, and therfore its containing entities. 
    If he camera is outisde any box, all boxes are visible.
    The entities are automatically sorted into the surrounded boxes.
       
    (C) 2010 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "visibility/visibilitysystems/visibilitysystembase.h"
#include "visibility/visibilitysystems/visibilitybox.h"
#include "util/bitfield.h"
              
//------------------------------------------------------------------------------
namespace Visibility
{    
class VisibilityBoxSystem : public VisibilitySystemBase
{
    __DeclareClass(VisibilityBoxSystem);
public:
    /// constructor
    VisibilityBoxSystem();
    /// destructor
    virtual ~VisibilityBoxSystem();
                                                                       
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

    void SetDirty();
    /// insert visibility container, bunch of contextes with bunch infos
    virtual void InsertVisibilityContainer(const Ptr<VisibilityContainer>& container);
    /// end attach visibility container 
    virtual void EndAttachVisibilityContainer();  

    /// attach visibility job to port
    virtual Ptr<Jobs::Job> CreateVisibilityJob(IndexT frameId, const Ptr<Observer>& observer, Util::FixedArray<Ptr<VisibilityContext> >& outEntitiyArray, uint& entityMask);
             
    /// render debug visualizations
    virtual void OnRenderDebug(); 
    /// get observer type mask
    virtual uint GetObserverBitMask() const;
                     
private:   
    /// return all visibility boxes that overlap the bounding box, called by entity when it's bounding box changes
    SizeT FindOverlappingVisibilityBoxes(const Math::bbox& boundingBox, Util::Array<Ptr<VisibilityBox>>& outBoxes) const;

    /// update visibility status of VisibilityBoxes
    void UpdateVisibilityBoxes(const Ptr<Observer>& observer);
    /// update the neighbours of a given visibility box
    void UpdateNeighbours(const Ptr<VisibilityBox>& visBox); 
    /// clear all visibility boxes (cannot remove one by one)
    void DiscardVisibilityBoxes();   
    /// generate visibility links
    void CheckVisibility(const Ptr<Observer>& observer, Util::Array<Ptr<VisibilityContext> >& outEntitiyArray, uint entityMask);
    /// collect boxes for given visiblity contexts
    void CollectFlattenBoxMapping(IndexT bufferIndex);
                           
    Util::Array<Ptr<VisibilityBox> > visBoxes;            // currently attached visibility boxes        
    Util::Array<VisibilityBox::VisibilityBoxJobData> flatBoxData;
    Util::Array<IndexT> flatNeighborList;
    Util::Dictionary<Ptr<VisibilityContext>, Util::Array<Ptr<VisibilityBox> > > boxMapping;
    struct BoxJobWorkData
    {
        bool dirty;
        Util::Array<IndexT> flatBoxMapping;    
    };
    BoxJobWorkData workData[2];
    SizeT numVisibleBoxes;
    SizeT numCameraBoxes;
};
        
//------------------------------------------------------------------------------
/**
*/
inline uint 
VisibilityBoxSystem::GetObserverBitMask() const
{
    return (1 << ObserverMask::Observers);
}
} // namespace Visibility
//------------------------------------------------------------------------------

