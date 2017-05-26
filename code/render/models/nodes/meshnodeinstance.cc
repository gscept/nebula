//------------------------------------------------------------------------------
// meshnodeinstance.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "meshnodeinstance.h"
#include "transformnode.h"
#include "coregraphics/renderdevice.h"

using namespace CoreGraphics;
namespace Models
{

__ImplementClass(Models::MeshNodeInstance, 'MSNI', Models::StateNodeInstance);
//------------------------------------------------------------------------------
/**
*/
MeshNodeInstance::MeshNodeInstance()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
MeshNodeInstance::~MeshNodeInstance()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
MeshNodeInstance::OnVisibilityResolve(IndexT frameIndex, IndexT resolveIndex, float distToViewer)
{
	// check LOD distance and tell our model node that we are a visible instance
	const Ptr<TransformNode>& transformNode = this->modelNode.downcast<TransformNode>();
	if (transformNode->CheckLodDistance(distToViewer))
	{
		this->modelNode->AddVisibleNodeInstance(resolveIndex, this->surfaceInstance->GetCode(), this);
		StateNodeInstance::OnVisibilityResolve(frameIndex, resolveIndex, distToViewer);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
MeshNodeInstance::Render()
{
	StateNodeInstance::Render();
	RenderDevice::Instance()->Draw();
}

//------------------------------------------------------------------------------
/**
*/
void
MeshNodeInstance::RenderInstanced(SizeT numInstances)
{
	StateNodeInstance::Render();
	RenderDevice::Instance()->DrawIndexedInstanced(numInstances, 0);
}

} // namespace Models