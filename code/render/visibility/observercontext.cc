//------------------------------------------------------------------------------
//  entityvisibility.cc
//  (C) 2010 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "visibility/observercontext.h"
#include "graphics/spotlightentity.h"
#include "graphics/globallightentity.h"
#include "graphics/cameraentity.h"

namespace Visibility
{
__ImplementClass(Visibility::ObserverContext, 'OBCO', Core::RefCounted);

using namespace Math;

//------------------------------------------------------------------------------
/**
*/
ObserverContext::ObserverContext():
    type(InvalidObserverType)
{
}

//------------------------------------------------------------------------------
/**
*/
ObserverContext::~ObserverContext()
{
}

//------------------------------------------------------------------------------
/**
*/
void 
ObserverContext::SetMatrix(const Math::matrix44& projView)
{
    n_assert(this->type == InvalidObserverType);
    this->type = ProjectionMatrix;
    this->projectionView = projView;
}

//------------------------------------------------------------------------------
/**
*/
void 
ObserverContext::SetBoundingBox(const Math::bbox& box)
{
    n_assert(this->type == InvalidObserverType);
    this->type = BoundingBox;
    this->boundingBox = box;
}

//------------------------------------------------------------------------------
/**
*/
void 
ObserverContext::SetSeeAll()
{
    n_assert(this->type == InvalidObserverType);
    this->type = SeeAll;
}

//------------------------------------------------------------------------------
/**
*/
Math::ClipStatus::Type 
ObserverContext::ComputeClipStatus(const Math::bbox& boundingBox)
{
    switch (this->type)
    {
    case ProjectionMatrix:
        return boundingBox.clipstatus(this->projectionView);
    case BoundingBox:
        return this->boundingBox.clipstatus(boundingBox);  
    case SeeAll:
        return Math::ClipStatus::Inside;
    default:
        n_error("Invalid Observer Type");
    }
    return Math::ClipStatus::Invalid;
}

//------------------------------------------------------------------------------
/**
*/
void 
ObserverContext::Setup(const Ptr<Graphics::GraphicsEntity>& entity)
{
    this->gfxEntity = entity;
    this->observerPos = entity->GetTransform().get_position();
    if (entity->IsA(Graphics::GlobalLightEntity::RTTI))
    {
        this->SetSeeAll();
    }
    else if (entity->IsA(Graphics::CameraEntity::RTTI))
    {
        this->SetMatrix(entity.cast<Graphics::CameraEntity>()->GetViewProjTransform());
    }
    else
    {
        this->SetBoundingBox(entity->GetGlobalBoundingBox());
    }
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44& 
ObserverContext::GetProjectionMatrix()
{
    n_assert(this->type == ProjectionMatrix);
    return this->projectionView;
}

//------------------------------------------------------------------------------
/**
*/
Math::bbox& 
ObserverContext::GetBoundingBox()
{
    n_assert(this->type == BoundingBox);
    return this->boundingBox;  
}

//------------------------------------------------------------------------------
/**
*/
ObserverContext::ObserverCullingType 
ObserverContext::GetType() const
{
    return this->type;
}

//------------------------------------------------------------------------------
/**
*/
ObserverContext::ObserverCullingType* 
ObserverContext::GetTypeRef()
{
    return &this->type;
}

//------------------------------------------------------------------------------
/**
*/
Math::point& 
ObserverContext::GetPosition()
{
    return this->observerPos;
}
} // namespace Visibility
