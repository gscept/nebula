#pragma once
//------------------------------------------------------------------------------
/**
    @class Visibility::VisibilitySystemBase
       
    (C) 2010 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "visibility/observer.h"
#include "visibility/visibilitycontext.h"
#include "jobs/jobport.h"
              
//------------------------------------------------------------------------------
namespace Visibility
{    
class VisibilityContainer;
class VisibilitySystemBase : public Core::RefCounted
{
    __DeclareClass(VisibilitySystemBase);
public:
    /// constructor
    VisibilitySystemBase();
    /// destructor
    virtual ~VisibilitySystemBase();
    
    /// open the graphics server
    virtual void Open(IndexT orderIndex);
    /// close the graphics server
    virtual void Close();
    /// return true if graphics server is open
    bool IsOpen() const;

    /// insert entity visibility
    virtual void InsertVisibilityContext(const Ptr<VisibilityContext>& entityVis);
    /// remove entity visibility
    virtual void RemoveVisibilityContext(const Ptr<VisibilityContext>& entityVis);
    /// update entity visibility
    virtual void UpdateVisibilityContext(const Ptr<VisibilityContext>& entityVis);

	/// updates visibility system to accommodate for the new world bounds
	virtual void OnWorldChanged(const Math::bbox& box);

    /// begin attach visibility container 
    virtual void BeginAttachVisibilityContainer();
    /// attach visibility container, alternative method for registering a bunch of entities 
    virtual void InsertVisibilityContainer(const Ptr<VisibilityContainer>& container);
    /// end attach visibility container 
    virtual void EndAttachVisibilityContainer();

    /// attach visibility job to port
    virtual Ptr<Jobs::Job> CreateVisibilityJob(IndexT frameId, const Graphics::GraphicsEntityId observer, Util::FixedArray<Ptr<VisibilityContext> >& outEntitiyArray, uint& entityMask);
    /// render debug visualizations
    virtual void OnRenderDebug();    
    /// get observer type mask
    virtual uint GetObserverBitMask() const;

protected:  
    bool isOpen;
    bool inAttachContainer;    
};

//------------------------------------------------------------------------------
/**
*/
inline bool
VisibilitySystemBase::IsOpen() const
{
    return this->isOpen;
}

//------------------------------------------------------------------------------
/**
*/
inline uint 
VisibilitySystemBase::GetObserverBitMask() const
{
    return 0;
}
} // namespace Visibility
//------------------------------------------------------------------------------

