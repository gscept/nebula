#pragma once
//------------------------------------------------------------------------------
/**
    @class MGraphics::VisibilityCluster
  
    Culls the attached graphics entities if the viewer is inside a
    bounding box cluster. VisibilityClusters are created and manually configured
    by level designers inside the level editor.
    
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

class VisibilityCluster : public VisibilityContainer
{
    __DeclareClass(VisibilityCluster);
public:
    /// constructor
    VisibilityCluster();
    /// destructor
    virtual ~VisibilityCluster();

    /// set bounding box transforms
    void SetBoxTransforms(const Util::Array<Math::matrix44>& boxTransforms);
    /// setup the visibility cluster 
    void Setup();
    /// discard the visibility cluster
    void Discard();

    /// test if point is inside bounding boxes
    bool IsPointInside(const Math::point& point) const;
    /// set the "camera inside" flag
    void SetCameraInside(bool b);
    /// get the camera inside flag
    bool IsCameraInside() const;

    /// set the cluster's id bit
    void SetBitIndex(IndexT i);
    /// get the cluster's id bit
    IndexT GetBitIndex() const;

private:
    friend class VisibilityClusterSystem;
    
    /// render a debug visualization
    void RenderDebug();

    Math::bbox box;                          // overall bounding box
    Util::Array<Math::matrix44> boxTransform;      // box transforms
    Util::Array<Math::frustum> frustumCluster;     // fine-grained transformed-boundingbox frustums
    IndexT bitIndex;
    bool cameraInside;
};

//------------------------------------------------------------------------------
/**
*/
inline void
VisibilityCluster::SetCameraInside(bool b)
{
    this->cameraInside = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
VisibilityCluster::IsCameraInside() const
{
    return this->cameraInside;
}

//------------------------------------------------------------------------------
/**
*/
inline void
VisibilityCluster::SetBitIndex(IndexT i)
{
    this->bitIndex = i;
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
VisibilityCluster::GetBitIndex() const
{
    return this->bitIndex;
}

} // namespace MGraphics
//------------------------------------------------------------------------------
