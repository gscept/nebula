#pragma once
//------------------------------------------------------------------------------
/**
    @class Visibility::ObserverContext
       
    (C) 2010 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "math/bbox.h"
#include "graphics/graphicsentity.h"
              
//------------------------------------------------------------------------------
namespace Visibility
{   
class ObserverContext : public Core::RefCounted
{
    __DeclareClass(ObserverContext);
public:    
    enum ObserverCullingType
    {
        ProjectionMatrix = 0,
        BoundingBox,
        SeeAll,

        InvalidObserverType
    };

    /// constructor
    ObserverContext();
    /// destructor
    virtual ~ObserverContext();     
    /// setup from graphics entity
    void Setup(const Ptr<Graphics::GraphicsEntity>& entity);
    /// compute clip status with bounding box
    Math::ClipStatus::Type ComputeClipStatus(const Math::bbox& boundingBox);
    /// get observer gfx entity
    const Ptr<Graphics::GraphicsEntity>& GetObserverEntity() const;
    /// get projection matrix
    Math::matrix44& GetProjectionMatrix();  
    /// get bbox  
    Math::bbox& GetBoundingBox();
    /// get position
    Math::point& GetPosition();
    /// get observer culling type
    ObserverCullingType GetType() const;
    /// get observer culling type
    ObserverCullingType* GetTypeRef();
              
private: 
    /// set matrix as observer view 
    void SetMatrix(const Math::matrix44& projView);
    /// set bbox as observer view 
    void SetBoundingBox(const Math::bbox& box);
    /// set see all flag as observer view 
    void SetSeeAll();
 
    Ptr<Graphics::GraphicsEntity> gfxEntity;
    ObserverCullingType type;    
    Math::bbox boundingBox;    
    Math::matrix44 projectionView;
    Math::point observerPos;
};

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Graphics::GraphicsEntity>& 
ObserverContext::GetObserverEntity() const
{
    return this->gfxEntity;
}
} // namespace Visibility
//------------------------------------------------------------------------------

