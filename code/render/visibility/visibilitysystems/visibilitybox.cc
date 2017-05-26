//------------------------------------------------------------------------------
//  visibilityBox.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "visibility/visibilitysystems/visibilitybox.h"
#include "visibility/visibilitysystems/visibilityboxsystem.h"  
#include "visibility/visibilitycontext.h"
#include "coregraphics/shaperenderer.h"
#include "threading/thread.h"

namespace Visibility
{
__ImplementClass(Visibility::VisibilityBox, 'VBOX', Visibility::VisibilityContainer);

using namespace Math;
using namespace Util;
using namespace CoreGraphics;
using namespace Threading;
//------------------------------------------------------------------------------
/**
*/
VisibilityBox::VisibilityBox()    
{                             
    boxData.isProcessed = false;
    boxData.isCameraBox = false;
    boxData.isVisible   = false;
    this->UpdateBoxFrustumFromTransform();
}

//------------------------------------------------------------------------------
/**
*/
VisibilityBox::~VisibilityBox()
{  
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityBox::AttachNeighbour(IndexT newNeighbour)
{
    this->neighbours.Append(newNeighbour);
}

//------------------------------------------------------------------------------
/**
Update the visibility box's transform. This will also re-compute the
boxFrustum members.
*/
void
VisibilityBox::SetTransform(const matrix44& m)
{
    this->boxData.transform = m;
    this->UpdateBoxFrustumFromTransform();
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityBox::UpdateBoxFrustumFromTransform()
{
    bbox unitCubeBox(point(0.0f, 0.0f, 0.0f), vector(0.5f, 0.5f, 0.5f));
    this->boxData.boxFrustum.set(unitCubeBox, this->boxData.transform);
}

//------------------------------------------------------------------------------
/**
*/
bool
VisibilityBox::IsPointInside(const point& p) const
{
    return this->boxData.boxFrustum.inside(p);
}

//------------------------------------------------------------------------------
/**
Test if a global-space bounding box is overlapping the visibility box.
*/
bool
VisibilityBox::IsBoundingBoxOverlapping(const bbox& boundingBox) const
{
    ClipStatus::Type clipResult = this->boxData.boxFrustum.clipstatus(boundingBox);
    return (ClipStatus::Outside != clipResult);
}

//------------------------------------------------------------------------------
/**
Test if another visibility box is overlapping this box.
*/
bool
VisibilityBox::IsVisibilityBoxOverlapping(const Ptr<VisibilityBox>& other) const
{
    // a unit cube box...
    const static bbox unitCubeBox(point(0.0f, 0.0f, 0.0f), vector(0.5f, 0.5f, 0.5f));
    ClipStatus::Type clipResult = this->boxData.boxFrustum.clipstatus(unitCubeBox, other->GetTransform());
    return (ClipStatus::Outside != clipResult);
}

//------------------------------------------------------------------------------
/**
    Test if this visibility box is overlapping the provided view frustum.
    This is used as an additional visibility test to check whether a 
    direct neighbor visibility box is even visible.
*/
bool
VisibilityBox::IsFrustumOverlapping(const frustum& viewFrustum) const
{
    // do a transformed unit-cube with our own bounding box on the
    // provided view frustum
    const static bbox unitCubeBox(point(0.0f, 0.0f, 0.0f), vector(0.5f, 0.5f, 0.5f));
    ClipStatus::Type clipResult = viewFrustum.clipstatus(unitCubeBox, this->GetTransform());
    return (ClipStatus::Outside != clipResult);
}

//------------------------------------------------------------------------------
/**
Render debug visualization of the visibility box.
*/
void
VisibilityBox::RenderDebug()
{
    float4 color;
    if (this->IsCameraBox())
    {
        // camera is inside this box
        color.set(0.0f, 1.0f, 0.0f, 0.5f);
    }
    else if (this->IsVisible())
    {
        // this box is a visible (neighbour)
        color.set(0.0f, 0.0f, 1.0f, 0.5f);
    }
    else
    {
        // this box is invisible (-> should never show up on screen)
        color.set(1.0f, 0.0f, 0.0f, 0.5f);
    }
    bbox boundingBox(this->boxData.transform);
	ShapeRenderer::Instance()->AddShape(RenderShape(Thread::GetMyThreadId(), RenderShape::Box, RenderShape::CheckDepth, boundingBox.to_matrix44(), color));            
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityBox::Discard()
{
    this->neighbours.Clear();
}

//------------------------------------------------------------------------------
/**
*/
const VisibilityBox::VisibilityBoxJobData& 
VisibilityBox::GetVisibilityBoxJobData() const
{
    return this->boxData;
}
} // namespace Visibility