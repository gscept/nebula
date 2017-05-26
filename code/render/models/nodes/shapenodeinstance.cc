//------------------------------------------------------------------------------
//  materialshapenodeinstance.cc
//  (C) 2011-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "models/nodes/shapenodeinstance.h"
#include "coregraphics/renderdevice.h"
#include "models/nodes/transformnode.h"

namespace Models
{
__ImplementClass(Models::ShapeNodeInstance, 'SPNI', Models::StateNodeInstance);


using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
ShapeNodeInstance::ShapeNodeInstance()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ShapeNodeInstance::~ShapeNodeInstance()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
ShapeNodeInstance::OnVisibilityResolve(IndexT frameIndex, IndexT resolveIndex, float distanceToViewer)
{
    // check LOD distance and tell our model node that we are a visible instance
    const Ptr<TransformNode>& transformNode = this->modelNode.downcast<TransformNode>();
	if (transformNode->CheckLodDistance(distanceToViewer))
    {
        this->modelNode->AddVisibleNodeInstance(resolveIndex, this->surfaceInstance->GetCode(), this);
        StateNodeInstance::OnVisibilityResolve(frameIndex, resolveIndex, distanceToViewer);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ShapeNodeInstance::Render()
{
    StateNodeInstance::Render();
    RenderDevice::Instance()->Draw();
}    

//------------------------------------------------------------------------------
/**
*/
void 
ShapeNodeInstance::RenderInstanced(SizeT numInstances)
{
    StateNodeInstance::Render();
    RenderDevice::Instance()->DrawIndexedInstanced(numInstances, 0);
}
} // namespace Models
