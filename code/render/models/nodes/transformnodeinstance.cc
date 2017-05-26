//------------------------------------------------------------------------------
//  transformnodeinstance.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "models/modelinstance.h"
#include "models/nodes/transformnodeinstance.h"
#include "models/nodes/transformnode.h"
#include "coregraphics/transformdevice.h"

// debug rendering
#include "threading/thread.h"
#include "coregraphics/shaperenderer.h"

namespace Models
{
__ImplementClass(Models::TransformNodeInstance, 'TFNI', Models::ModelNodeInstance);

using namespace Math;
using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
TransformNodeInstance::TransformNodeInstance():
    isInViewSpace(false),
    lockedToViewer(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
TransformNodeInstance::~TransformNodeInstance()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
TransformNodeInstance::Setup(const Ptr<ModelInstance>& inst, 
                             const Ptr<ModelNode>& node, 
                             const Ptr<ModelNodeInstance>& parentNodeInst)
{
    n_assert(inst.isvalid());
    n_assert(node.isvalid() && node->IsA(TransformNode::RTTI));
    n_assert(!this->parentTransformNodeInstance.isvalid());
    const Ptr<TransformNode>& transformNode = node.downcast<TransformNode>();

    // instantiate transform node attributes
    this->SetPosition(transformNode->GetPosition());
    this->SetRotate(transformNode->GetRotation());
    this->SetScale(transformNode->GetScale());
    this->SetRotatePivot(transformNode->GetRotatePivot());
    this->SetScalePivot(transformNode->GetScalePivot());
    this->SetInViewSpace(transformNode->IsInViewSpace());
    this->SetLockedToViewer(transformNode->GetLockedToViewer());

    // check if parent is a transform node, if yes, store a pointer here
    // so we don't need to do this check again inside Update()
    if (parentNodeInst.isvalid() && parentNodeInst->IsA(TransformNodeInstance::RTTI))
    {
        this->parentTransformNodeInstance = parentNodeInst.downcast<TransformNodeInstance>();
    }

    // hand to parent class
    ModelNodeInstance::Setup(inst, node, parentNodeInst);
}

//------------------------------------------------------------------------------
/**
*/
void
TransformNodeInstance::Discard()
{
    // need to clear smart pointers to prevent ref leaks
    this->parentTransformNodeInstance = 0;
    ModelNodeInstance::Discard();
}

//------------------------------------------------------------------------------
/**
    The update method should first invoke any animators which change 
    per-instance attributes (this is done in the parent class). Then the
    local space transforms must be flattened into model space.

    NOTE: this method must be called late in the frame to give other
    systems a chance to modify the transform matrix (for instance the
    character attachment system).
*/
void
TransformNodeInstance::OnRenderBefore(IndexT frameIndex, Timing::Time time)
{
    // call parent class
    ModelNodeInstance::OnRenderBefore(frameIndex, time);

    // flatten transforms
    // @todo: do some sort of identity matrix and dirty handling
	if (this->parentTransformNodeInstance.isvalid())
	{
		const matrix44& parentTransform = this->parentTransformNodeInstance->GetModelTransform();
		const matrix44& localTransform = this->GetLocalTransform();
		this->modelTransform = matrix44::multiply(localTransform, parentTransform);    
	}
	else
	{
		// we have no parent
		this->modelTransform = matrix44::multiply(this->GetLocalTransform(), this->modelInstance->GetTransform());
	}

	if (this->isInViewSpace)
	{
		// need to undo view space transform
		matrix44 invViewSpace = TransformDevice::Instance()->GetInvViewTransform();

		// save original position
		float4 pos = this->modelTransform.get_position();

		// set position to 0 so that we rotate around origin
		this->modelTransform.set_position(point(0, 0, 0));
		this->modelTransform = matrix44::multiply(this->modelTransform, invViewSpace);

		// reset position
		this->modelTransform.set_position(pos);
	}
	if (this->lockedToViewer)
	{
		// need to undo view space translation                
		this->modelTransform.set_position(TransformDevice::Instance()->GetInvViewTransform().get_position());
	}

}

//------------------------------------------------------------------------------
/**
    Set our model matrix (computed in the Update() method) 
    as current model matrix in the TransformDevice.
*/
void
TransformNodeInstance::ApplyState(IndexT frameIndex, const IndexT& pass)
{    
    TransformDevice::Instance()->SetModelTransform(this->modelTransform);
}

//------------------------------------------------------------------------------
/**
    Render a debug visualization of the node.
*/
void
TransformNodeInstance::RenderDebug()
{
    //bbox box = this->modelNode->GetBoundingBox();
    //box.transform(this->modelTransform);
    //Shape shape(Threading::Thread::GetMyThreadId(), Shape::Box, box.to_matrix44(), float4(0.0f, 0.0f, 1.0f, 0.1f));
    //ShapeRenderer::Instance()->AddShape(shape);
}
} // namespace Models
