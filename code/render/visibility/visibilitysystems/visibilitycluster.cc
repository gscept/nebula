//------------------------------------------------------------------------------
//  visibilitycluster.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "visibility/visibilitysystems/visibilitycluster.h"
#include "visibility/visibilitysystems/visibilityclustersystem.h"  
#include "visibility/visibilitycontext.h"
#include "coregraphics/shaperenderer.h"
#include "threading/thread.h"

namespace Visibility
{
    __ImplementClass(Visibility::VisibilityCluster, 'VCLU', Visibility::VisibilityContainer);

using namespace Math;
using namespace Util;
using namespace CoreGraphics;
using namespace Threading;
//------------------------------------------------------------------------------
/**
*/
VisibilityCluster::VisibilityCluster() :
    cameraInside(false),
    bitIndex(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
VisibilityCluster::~VisibilityCluster()
{
    this->Discard();    
}

//------------------------------------------------------------------------------
/**
    Setup the visibility cluster. The cluster volume is described by
    a set of transformed bounding boxes, the associated graphics entities
    must be provided as well.
*/
void 
VisibilityCluster::Setup()
{
    n_assert(this->visibilityContexts.Size() > 0);

    // setup the bounding frustums and the overall bounding box
    bbox unitCubeBox(point(0.0f, 0.0f, 0.0f), vector(0.5f, 0.5f, 0.5f));
    this->frustumCluster.Reserve(this->boxTransform.Size());
    this->box.begin_extend();
    IndexT i;
    for (i = 0; i < this->boxTransform.Size(); i++)
    {
        const matrix44& curTransform = this->boxTransform[i];

        // add bounding frustum
        frustum f;
        f.set(unitCubeBox, curTransform);
        this->frustumCluster.Append(f);

        // extend global bounding box
        bbox transformedBox;
        transformedBox = unitCubeBox;
        transformedBox.transform(curTransform);
        this->box.extend(transformedBox);
    }
    this->box.end_extend();
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityCluster::Discard()
{
    this->frustumCluster.Clear();
}

//------------------------------------------------------------------------------
/**
*/
bool
VisibilityCluster::IsPointInside(const point& p) const
{
    n_printf("IsPointInside: p: %s\n", String::FromFloat4(p).AsCharPtr());
    if (this->box.contains(p))
    {
        IndexT i;
        for (i = 0; i < this->frustumCluster.Size(); i++)
        {
            if (this->frustumCluster[i].inside(p))
            {
                return true;
            }
        }
    }
    // fallthrough: point not inside cluster
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityCluster::RenderDebug()
{        
    float4 color;
    IndexT i;
    for (i = 0; i < this->boxTransform.Size(); i++)
    {
        if (this->IsCameraInside())
        {
            color.set(1.0f, 1.0f, 0.0f, 0.5f);
        }
        else
        {
            color.set(0.0f, 1.0f, 1.0f, 0.5f);
        }
        ShapeRenderer::Instance()->AddShape(RenderShape(Thread::GetMyThreadId(), RenderShape::Box, RenderShape::CheckDepth, this->boxTransform[i], color));            
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityCluster::SetBoxTransforms(const Util::Array<Math::matrix44>& transforms)
{
    this->boxTransform = transforms;
}
} // namespace Visibility