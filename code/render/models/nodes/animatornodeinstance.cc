//------------------------------------------------------------------------------
//  animatornodeinstance.cc
//  (C) 2008 RadonLabs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "models/nodes/animatornodeinstance.h"
#include "models/modelinstance.h"
#include "models/modelnode.h"
#include "models/modelnodeinstance.h"
#include "models/nodes/statenodeinstance.h"
#include "models/nodes/statenode.h"
#include "models/nodes/transformnodeinstance.h"
#include "models/nodes/animatornode.h"
#include "coregraphics/shaderstate.h"
#include "coregraphics/shadervariable.h"
#include "coregraphics/shadervariableinstance.h"
#include "coreanimation/animresource.h"
#include "coreanimation/sampletype.h"
#include "coreanimation/animsamplebuffer.h"
#include "coreanimation/animutil.h"
#include "graphics/modelentity.h"

namespace Models
{
__ImplementClass(Models::AnimatorNodeInstance, 'AMNI', Models::ModelNodeInstance);

using namespace Math;
using namespace CoreAnimation;

//------------------------------------------------------------------------------
/**
*/
AnimatorNodeInstance::AnimatorNodeInstance() : 
	startTime(-1.0),
	overWrittenAnimTime(-1.0)
{
    //Empty
}

//------------------------------------------------------------------------------
/**
*/
AnimatorNodeInstance::~AnimatorNodeInstance()
{
    //Empty
}

//------------------------------------------------------------------------------
/**
*/
void 
AnimatorNodeInstance::Setup(const Ptr<ModelInstance>& inst, const Ptr<ModelNode>& node, const Ptr<ModelNodeInstance>& parentNodeInst)
{
    SizeT numAnimSections = node.cast<AnimatorNode>()->GetNumAnimSections();
    IndexT animSecIdx;
    for (animSecIdx = 0; animSecIdx < numAnimSections; animSecIdx++)
    {
        // Get all animationPaths from animatornode
        Util::Array<Util::String> animPaths = node.cast<AnimatorNode>()->GetAllAnimatedNodesPaths(animSecIdx);        
        Util::Array<AnimatedNode> section;
        // get the nodes to the paths
        IndexT i;
        for (i = 0; i < animPaths.Size(); i++)
        {
            Ptr<ModelNodeInstance> modelNodeInst = inst->LookupNodeInstance(animPaths[i]);
            n_assert(modelNodeInst.isvalid());            
            AnimatedNode newNode;
            newNode.node = modelNodeInst;
            AnimatorNode::NodeType animNodeType = node.cast<AnimatorNode>()->GetAnimationNodeType(animSecIdx);
            const CoreGraphics::ShaderVariable::Name& varName = node.cast<AnimatorNode>()->GetShaderVariableName(animSecIdx);
            if (modelNodeInst->IsA(StateNodeInstance::RTTI))
            {
                Ptr<StateNodeInstance> stateNodeInstance = modelNodeInst.cast<StateNodeInstance>();
				Ptr<StateNode> stateNode = modelNodeInst->GetModelNode().cast<StateNode>();
                newNode.var = stateNodeInstance->GetSurfaceInstance()->GetConstant(varName);
            }
            if ((animNodeType == AnimatorNode::IntAnimator) ||
                (animNodeType == AnimatorNode::FloatAnimator) ||
                (animNodeType == AnimatorNode::Float4Animator))
            {
                n_assert(newNode.var.isvalid());
            }
            section.Append(newNode);
        }
        this->animSection.Append(section);
    }
    ModelNodeInstance::Setup(inst, node, parentNodeInst);
}

//------------------------------------------------------------------------------
/**
*/
void 
AnimatorNodeInstance::OnShow(Timing::Time time)
{
    ModelNodeInstance::OnShow(time);

    // reset time
    this->startTime = time;

    // call animate to set right state, OnShow could be done after the modelnodeinstance update!
    this->Animate(time);    
}

//------------------------------------------------------------------------------
/**
*/
void 
AnimatorNodeInstance::OnNotifyCullingVisible(IndexT frameIndex, Timing::Time time)
{
    ModelNodeInstance::OnNotifyCullingVisible(frameIndex, time);

    // if overwrite time is set, use it for animation sampling
    if (this->overWrittenAnimTime >= 0)
    {
        this->Animate(this->overWrittenAnimTime);
    }
    else
    {
        this->Animate(time);    
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AnimatorNodeInstance::Animate(Timing::Time time)
{
    time -= this->startTime;
    Timing::Tick ticks = Timing::SecondsToTicks(time);
    SizeT numAnimSections = this->modelNode.cast<AnimatorNode>()->GetNumAnimSections();
    IndexT animSecIdx;
    for (animSecIdx = 0; animSecIdx < numAnimSections; animSecIdx++)
    {
        const Util::Array<AnimatedNode>& curAnimSection = this->animSection[animSecIdx];
        IndexT i;
        for (i = 0; i < curAnimSection.Size(); i++)
        {
            const AnimatedNode& curNode = curAnimSection[i];
            Ptr<StateNodeInstance> stateNode = curNode.node.downcast<StateNodeInstance>();
            Ptr<TransformNodeInstance> transformNode = curNode.node.downcast<TransformNodeInstance>();
            n_assert(curNode.node->IsA(TransformNodeInstance::RTTI));
            AnimatorNode::NodeType animNodeType = this->modelNode.cast<AnimatorNode>()->GetAnimationNodeType(animSecIdx);
            
            if (animNodeType == AnimatorNode::Float4Animator)
            {
                // get sampled key
                static AnimKey<Math::float4> key;
                AnimKeyArray<AnimKey<Math::float4> >& float4KeyArray = this->modelNode.cast<AnimatorNode>()->GetFloat4KeyArray(animSecIdx);
                AnimLoopType::Type& loopType = this->modelNode.cast<AnimatorNode>()->GetLoopType(animSecIdx);
                if (float4KeyArray.Sample((float)time, loopType, key))
                {
                    curNode.var->SetValue(key.GetValue());
                }
            }
            else if (animNodeType == AnimatorNode::FloatAnimator)
            {
                // get sampled key
                static AnimKey<float> key;
                AnimKeyArray<AnimKey<float> >& floatKeyArray = this->modelNode.cast<AnimatorNode>()->GetFloatKeyArray(animSecIdx);
                AnimLoopType::Type& loopType = this->modelNode.cast<AnimatorNode>()->GetLoopType(animSecIdx);
                if (floatKeyArray.Sample((float)time, loopType, key))
                {
                    curNode.var->SetValue(key.GetValue());
                    //n_printf("Float aniamation of node %s: time: %f value: %f\n", curNode.node->GetName().Value(), time, key.GetValue());                    
                }
            }
            else if (animNodeType == AnimatorNode::IntAnimator)
            {
                // get sampled key
                static AnimKey<int> key;
                AnimKeyArray<AnimKey<int> >& intKeyArray = this->modelNode.cast<AnimatorNode>()->GetIntKeyArray(animSecIdx);
                AnimLoopType::Type& loopType = this->modelNode.cast<AnimatorNode>()->GetLoopType(animSecIdx);
                if (intKeyArray.Sample((float)time, loopType, key))
                {
                    curNode.var->SetValue(key.GetValue());
                }
            }
            else if (animNodeType == AnimatorNode::TransformAnimator)
            {
                // sample key arrays and manipulate target object
                static AnimKey<Math::vector> key;
                AnimLoopType::Type& loopType = this->modelNode.cast<AnimatorNode>()->GetLoopType(animSecIdx);
                AnimKeyArray<AnimKey<Math::vector> >& posArray = this->modelNode.cast<AnimatorNode>()->GetPosArray(animSecIdx);
                AnimKeyArray<AnimKey<Math::vector> >& eulerArray = this->modelNode.cast<AnimatorNode>()->GetEulerArray(animSecIdx);
                AnimKeyArray<AnimKey<Math::vector> >& scaleArray = this->modelNode.cast<AnimatorNode>()->GetScaleArray(animSecIdx);

                if (posArray.Sample((float)time, loopType, key))
                {
                    transformNode->SetPosition(key.GetValue());
                }
                if (eulerArray.Sample((float)time, loopType, key))
                {
                    Math::quaternion quaternion = Math::quaternion::rotationyawpitchroll(key.GetValue().y(), key.GetValue().x(), key.GetValue().z());
                    transformNode->SetRotate(quaternion);
                }
                if (scaleArray.Sample((float)time, loopType, key))
                {
                    transformNode->SetScale(key.GetValue());
                }
            }
            else if (animNodeType == AnimatorNode::TransformCurveAnimator)
            {
                const Ptr<ManagedAnimResource> & managedAnimResource = this->GetModelNode().cast<AnimatorNode>()->GetManagedAnimResource(animSecIdx);
                const Ptr<AnimResource>& animResource = managedAnimResource->GetAnimResource();
                n_assert(animResource.isvalid());

                IndexT clipIndex = this->GetModelNode().cast<AnimatorNode>()->GetAnimationGroup(animSecIdx);

                Ptr<AnimSampleBuffer> result = AnimSampleBuffer::Create();
                result->Setup(animResource);
                
                // need to substract start time of the clip from the sample time
                Timing::Tick sampleTime = ticks - animResource->GetClipByIndex(clipIndex).GetStartTime();
                AnimUtil::Sample(animResource, clipIndex, SampleType::Linear, sampleTime, 1.0f, result);

                // write result to transform node instance
                n_assert(result->GetNumSamples() == 3);
                Math::float4* keys = result->GetSamplesPointer();
                transformNode->SetRotate(Math::quaternion(keys[1]));
                transformNode->SetPosition(keys[0]);
                transformNode->SetScale(keys[2]);
            }
            else if (animNodeType == AnimatorNode::UvAnimator)
            {
                // sample key arrays and manipulate target object
                static AnimKey<Math::vector> key;
                AnimLoopType::Type& loopType = this->modelNode.cast<AnimatorNode>()->GetLoopType(animSecIdx);
                AnimKeyArray<AnimKey<Math::vector> >& posArray = this->modelNode.cast<AnimatorNode>()->GetPosArray(animSecIdx);
                AnimKeyArray<AnimKey<Math::vector> >& eulerArray = this->modelNode.cast<AnimatorNode>()->GetEulerArray(animSecIdx);
                AnimKeyArray<AnimKey<Math::vector> >& scaleArray = this->modelNode.cast<AnimatorNode>()->GetScaleArray(animSecIdx);

                matrix44 uvTransform = matrix44::identity();
                if (posArray.Sample((float)time, loopType, key))
                {
                    uvTransform.set_position(point(key.GetValue()));
                }
                if (eulerArray.Sample((float)time, loopType, key))
                {
                    matrix44 rotateM = matrix44::rotationyawpitchroll(0, 0, key.GetValue().x());
                    uvTransform = matrix44::multiply(uvTransform, rotateM);
                }
                if (scaleArray.Sample((float)time, loopType, key))
                {
                    uvTransform.scale(key.GetValue());
                }
                curNode.var->SetValue(uvTransform);
            }
        }
    }
}

};
