#pragma once
//------------------------------------------------------------------------------
/**
    @class MGraphics::VisibilityBox

    A VisibilityBox implementes an transformed bounding box which groups
    graphics entities into a visibility group.

    (C) 2010 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/    
#include "visibility/visibilitycontainer.h"
#include "math/bbox.h"
#include "math/frustum.h"

//------------------------------------------------------------------------------
namespace Visibility
{
class VisibilityContext;

class VisibilityBox : public VisibilityContainer
{
    __DeclareClass(VisibilityBox);
public:
    /// constructor
    VisibilityBox();
    /// destructor
    virtual ~VisibilityBox();

    struct VisibilityBoxJobData
    {
        Math::matrix44 transform;
        Math::frustum boxFrustum;
        bool isVisible;
        bool isCameraBox;
        bool isProcessed; 
    };

    /// discard box, cleara neighbors
    void Discard();

    /// set the transformation, defines position, size and orientation of a unit cube
    void SetTransform(const Math::matrix44& m);
    /// get the transformation of the visibility box
    const Math::matrix44& GetTransform() const;
    /// attach a neighbour visibility box (note: dynamic removal of neighbours not allowed)
    void AttachNeighbour(IndexT neighbour);
    /// get neighbours of this visibility box
    const Util::Array<IndexT>& GetNeighbours() const;

    /// return true if a point is inside the visibility box
    bool IsPointInside(const Math::point& point) const;
    /// test if a bounding box is overlapping the visibility box
    bool IsBoundingBoxOverlapping(const Math::bbox& boundingBox) const;
    /// test if another visibility box is overlapping this box
    bool IsVisibilityBoxOverlapping(const Ptr<VisibilityBox>& otherBox) const;
    /// test visibility against view frustum
    bool IsFrustumOverlapping(const Math::frustum& viewFrustum) const;   
    /// get current visibility status
    bool IsVisible() const;
    /// return true if this is a camera box (the camera position is inside this box)
    bool IsCameraBox() const;
    /// return true if the box has already been processed in visibility check
    bool IsProcessed() const;
    /// get box data
    const VisibilityBox::VisibilityBoxJobData& GetVisibilityBoxJobData() const;

private:
    friend class VisibilityBoxSystem;
    /// set visibility status of box, called by VisibilityBoxServer
    void SetVisible(bool b);
    /// set the camera flag, if true, the camera position is inside this box
    void SetCameraBox(bool b);
    /// set processed flag, to prevent redundant visibility checks
    void SetProcessed(bool b);
    /// update the box frustum (for visibility and overlapping tests) from the current transform
    void UpdateBoxFrustumFromTransform();
    
    /// render a debug visualization
    void RenderDebug();

    VisibilityBoxJobData boxData;     
    Util::Array<IndexT> neighbours;
};
       
//------------------------------------------------------------------------------
/**
*/
inline const Math::matrix44&
VisibilityBox::GetTransform() const
{
    return this->boxData.transform;
}

//------------------------------------------------------------------------------
/**
*/
inline void
VisibilityBox::SetVisible(bool b)
{
    this->boxData.isVisible = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
VisibilityBox::IsVisible() const
{
    return this->boxData.isVisible;
}

//------------------------------------------------------------------------------
/**
*/
inline void
VisibilityBox::SetCameraBox(bool b)
{
    this->boxData.isCameraBox = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
VisibilityBox::IsCameraBox() const
{
    return this->boxData.isCameraBox;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<IndexT>&
VisibilityBox::GetNeighbours() const
{
    return this->neighbours;
}

//------------------------------------------------------------------------------
/**
*/
inline void
VisibilityBox::SetProcessed(bool b)
{
    this->boxData.isProcessed = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
VisibilityBox::IsProcessed() const
{
    return this->boxData.isProcessed;
}
} // namespace MGraphics
//------------------------------------------------------------------------------
