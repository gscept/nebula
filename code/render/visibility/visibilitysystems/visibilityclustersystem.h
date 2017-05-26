#pragma once
//------------------------------------------------------------------------------
/**
    @class Visibility::VisibilityClusterSystem

    Culls the attached graphics entities if the viewer is inside a
    bounding box cluster. VisibilityClusters are created and manually configured
    by level designers inside the level editor.
       
    (C) 2010 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "visibility/visibilitysystems/visibilitysystembase.h"
#include "visibility/visibilitysystems/visibilitycluster.h"
#include "util/bitfield.h"
              
//------------------------------------------------------------------------------
namespace Visibility
{    
class VisibilityClusterSystem : public VisibilitySystemBase
{
    __DeclareClass(VisibilityClusterSystem);
public:
    /// constructor
    VisibilityClusterSystem();
    /// destructor
    virtual ~VisibilityClusterSystem();
                                                                       
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
    /// insert visibility container, bunch of contextes with bunch infos
    virtual void InsertVisibilityContainer(const Ptr<VisibilityContainer>& container);      
    /// attach visibility job to port
    virtual Ptr<Jobs::Job> CreateVisibilityJob(IndexT frameId, const Ptr<ObserverContext>& observer, Util::FixedArray<Ptr<VisibilityContext> >& outEntitiyArray, uint& entityMask);

     /// render debug visualizations
    virtual void OnRenderDebug(); 
    /// get observer type mask
    virtual uint GetObserverBitMask() const;
                     
private:   
    /// discard all visibility clusters
    void DiscardVisibilityClusters(); 
    /// get the camera cluster bit mask (bits of all clusters where the camera is inside)
    const Util::BitField<64>& GetVisibilityClusterBitMask() const; 
    /// generate visibility links
    void CheckVisibility(const Ptr<ObserverContext>& observer, Util::FixedArray<Ptr<VisibilityContext> >& outEntitiyArray, uint entityMask);
      
    /// update visibility status of VisibilityClusters
    void UpdateVisibilityClusters(const Math::point& cameraPos);
    /// increment accepted vischecks (called by VisibilityChecker)
    void IncrAcceptedVisChecks();
    /// increment rejected vischecks (called by VisibilityChecker)
    void IncrRejectedVisChecks();
    /// find cluster for visible context
    Util::Array<Ptr<VisibilityCluster> > FindClustersContainingPoint(const Math::point& pos);
    
    Util::Array<Ptr<VisibilityCluster> > visClusters;     // visibility clusters
    bool isOpen;    
    bool inBeginAttachCluster;
    Util::BitField<64> visibilityClusterBitMask;
    SizeT statsNumVisChecksAccepted;
    SizeT statsNumVisChecksRejected;
    Util::Dictionary<Ptr<VisibilityContext>, Util::BitField<64> > visContextBitmask;
};

//------------------------------------------------------------------------------
/**
*/
inline void
VisibilityClusterSystem::IncrAcceptedVisChecks()
{
    this->statsNumVisChecksAccepted++;
}

//------------------------------------------------------------------------------
/**
*/
inline void
VisibilityClusterSystem::IncrRejectedVisChecks()
{
    this->statsNumVisChecksRejected++;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::BitField<64>&
VisibilityClusterSystem::GetVisibilityClusterBitMask() const
{
    return this->visibilityClusterBitMask;
}     
//------------------------------------------------------------------------------
/**
*/
inline uint 
VisibilityClusterSystem::GetObserverBitMask() const
{
    return (1 << Graphics::GraphicsEntityType::Camera);
}
} // namespace Visibility
//------------------------------------------------------------------------------

