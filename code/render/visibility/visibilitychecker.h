#pragma once
//------------------------------------------------------------------------------
/**
    @class Visibility::VisibilityChecker

    The VisibilityChecker manages all visibility systems.
    Its the interface for all access to the visibility system.
    If any entity is attached to a stage it will be registered by the checker and added 
    to all attached visibility systems.

    If the stage wants to check the visibility it just calls PerformVisibilityQuery
    which starts a new visibility check with the given observer entity and applies 
    the result of the last frame check.
       
    (C) 2010 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "graphics/graphicsentity.h"
#include "visibility/visibilitycontext.h"
#include "visibility/visibilitysystems/visibilitysystembase.h"
#include "visibility/visibilityquery.h"
#include "visibility/visibilitycontext.h"
              
//------------------------------------------------------------------------------
namespace Visibility
{    
class VisibilityContainer;

class VisibilityChecker 
{
public:        
    /// constructor
    VisibilityChecker();
    /// destructor
    virtual ~VisibilityChecker();

    /// open
    void Open();
    /// close
    void Close();

    /// attach visibility system
    void AttachVisibilitySystem(const Ptr<VisibilitySystemBase>& system);
    /// attach visibility system as array
    void AttachVisibilitySystems(const Util::Array<Ptr<VisibilitySystemBase> >& systems);
    /// remove visibility system
    void RemoveVisibilitySystem(const Ptr<VisibilitySystemBase>& system);

	/// notifies visibility systems that the world bounds has changed
	void WorldChanged(const Math::bbox& worldBox);

    /// register entity
    void RegisterEntity(const Ptr<Graphics::GraphicsEntity>& entity);
    /// unregister entity
    void UnregisterEntity(const Ptr<Graphics::GraphicsEntity>& entity);

    /// begin attach visibility container 
    void BeginAttachVisibilityContainer();
    /// attach visibility container, alternative method for registering a bunch of entities 
    void AttachVisibilityContainer(const Ptr<VisibilityContainer>& container);
    /// end attach visibility container 
    void EndAttachVisibilityContainer();

    /// update entity visibility context on a transform change
    void UpdateVisibilityContext(const Ptr<Graphics::GraphicsEntity>& entity);
    
    /// check visibility with given view projection transform, will build links in entities
    void PerformVisibilityQuery(IndexT frameId, const Ptr<Graphics::GraphicsEntity>& observerEntity, uint entityMask);
          
    /// clear visibility links
    void ClearVisibilityLinks(Graphics::GraphicsEntity::LinkType linkType);

    /// on render debug
    void OnRenderDebug();

private:       
    /// apply visibility results of last visibility request 
    void ApplyLastVisibilityResults(IndexT frameId, IndexT slot);

    bool isOpen;
    IndexT lastFrameId;
    SizeT numQueriesThisFrame;
    Util::Array<Ptr<VisibilitySystemBase> > visibilitySystems;    
    Util::FixedArray<Ptr<VisibilityQuery> > visiblityQueries[2];
    Util::Dictionary<Ptr<Graphics::GraphicsEntity>, Ptr<VisibilityContext> > registeredEntities;
    Ptr<VisibilityContext> observerContext;
};

} // namespace Visibility
//------------------------------------------------------------------------------

