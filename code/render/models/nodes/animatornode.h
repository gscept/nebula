#pragma once
//------------------------------------------------------------------------------
/**
	@class Models::AnimatorNode

    Legacy N2 crap!

    (C) 2008 RadonLabs GmbH
	(C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "models/modelnode.h"
#include "models/nodes/animkeyarray.h"
#include "models/nodes/animlooptype.h"
#include "coregraphics/shadervariable.h"
#include "coreanimation/managedanimresource.h"
#include "models/modelnodeinstance.h"
#include "util/variant.h"

//------------------------------------------------------------------------------
namespace Models
{
class AnimatorNode : public Models::ModelNode
{
    __DeclareClass(AnimatorNode);
public:
    enum NodeType
    {
        IntAnimator = 0,
        FloatAnimator,
        Float4Animator,
        TransformAnimator,
        TransformCurveAnimator,
        UvAnimator,

        InvalidNodeType
    };

    /// constructor
    AnimatorNode();
    /// destructor
    virtual ~AnimatorNode();
    /// create animator instance
    virtual Ptr<Models::ModelNodeInstance> CreateNodeInstance() const;
    /// get the loop type
    AnimLoopType::Type& GetLoopType(IndexT i) const;

    /// get the name of the shader parameter to manipulate
    const CoreGraphics::ShaderVariable::Name& GetShaderVariableName(IndexT i) const;

    /// load resources
    virtual void LoadResources(bool sync);
    /// unload resources
    virtual void UnloadResources();
    /// get overall state of contained resources (Initial, Loaded, Pending, Failed, Cancelled)
    virtual Resources::Resource::State GetResourceState() const;
 
    /// get the animation resource name
    const Util::String& GetAnimation(IndexT i) const;

    /// get the animation group to use
    int GetAnimationGroup(IndexT i);
    /// return animresource
    Ptr<CoreAnimation::ManagedAnimResource> & GetManagedAnimResource(IndexT i);

    /// get key at
    //void GetKeyAt(int index, float& time, Math::float4& key) const;
    /// get key array
    AnimKeyArray<AnimKey<Math::float4> >& GetFloat4KeyArray(IndexT i);

    /// get key at
    //void GetKeyAt(int index, float& time, float& key) const;
    /// get key array
    AnimKeyArray<AnimKey<float> >& GetFloatKeyArray(IndexT i);

    /// get key array
    AnimKeyArray<AnimKey<int> >& GetIntKeyArray(IndexT i);

    /// Get Posistion Array from AnimSection
    AnimKeyArray<AnimKey<Math::vector> > & GetPosArray(IndexT i);
    /// Get Euler Array from AnimSection
    AnimKeyArray<AnimKey<Math::vector> > & GetEulerArray(IndexT i);
    /// Get Scale Array from AnimSection
    AnimKeyArray<AnimKey<Math::vector> > & GetScaleArray(IndexT i);
    /// Get the Layer from AnimSection
    Util::Array<int>& GetLayerArray(IndexT i);

    /// get animNodeType
    NodeType GetAnimationNodeType(IndexT i);

    /// return all paths in animSection array
    Util::Array<Util::String> GetAllAnimatedNodesPaths(IndexT i);

    /// parse data tag (called by loader code)
    virtual bool ParseDataTag(const Util::FourCC& fourCC, const Ptr<IO::BinaryReader>& reader);

    /// get number of animation sections
    SizeT GetNumAnimSections() const;

private:
    /// load animation resource
    bool LoadAnimation();
    /// unload animation resource
    void UnloadAnimation();

    struct AnimSection
    {
        int animationGroup;
        Util::String animationName;
        Util::Array<Util::String> animatedNodesPath;
        NodeType animationNodeType;

        AnimKeyArray<AnimKey<Math::vector> > posArray;
        AnimKeyArray<AnimKey<Math::vector> > eulerArray;
        AnimKeyArray<AnimKey<Math::vector> > scaleArray;
        Util::Array<int> layer;


        AnimKeyArray<AnimKey<Math::float4> > float4KeyArray;
        AnimKeyArray<AnimKey<float> > floatKeyArray;
        AnimKeyArray<AnimKey<int> > intKeyArray;

        AnimLoopType::Type loopType;
        CoreGraphics::ShaderVariable::Name shaderVarName;
        Ptr<CoreAnimation::ManagedAnimResource> managedAnimResource;
    };

    Util::Array<AnimSection> animSection;
};
__RegisterClass(AnimatorNode);


//------------------------------------------------------------------------------
/**
    Get the loop type for this animation.
*/
inline
AnimLoopType::Type&
AnimatorNode::GetLoopType(IndexT i) const
{
    return this->animSection[i].loopType;
}

//-----------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::ShaderVariable::Name& 
AnimatorNode::GetShaderVariableName(IndexT i) const
{
    return this->animSection[i].shaderVarName;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
AnimatorNode::GetAnimationGroup(IndexT i)
{
    return this->animSection[i].animationGroup;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::String&
AnimatorNode::GetAnimation(IndexT i) const
{
    return this->animSection[i].animationName;
}

//------------------------------------------------------------------------------
/**
*/
inline
Ptr<CoreAnimation::ManagedAnimResource> &
AnimatorNode::GetManagedAnimResource(IndexT i)
{
    return this->animSection[i].managedAnimResource;
}

//-----------------------------------------------------------------------------
/**
*/
inline AnimKeyArray<AnimKey<Math::float4> >&
AnimatorNode::GetFloat4KeyArray(IndexT i)
{
    return this->animSection[i].float4KeyArray;
}

//-----------------------------------------------------------------------------
/**
*/
inline AnimKeyArray<AnimKey<float> >&
AnimatorNode::GetFloatKeyArray(IndexT i)
{
    return this->animSection[i].floatKeyArray;
}

//-----------------------------------------------------------------------------
/**
*/
inline AnimKeyArray<AnimKey<int> >&
AnimatorNode::GetIntKeyArray(IndexT i)
{
    return this->animSection[i].intKeyArray;
}

//-----------------------------------------------------------------------------
/**
*/
inline AnimatorNode::NodeType
AnimatorNode::GetAnimationNodeType(IndexT i)
{
    return this->animSection[i].animationNodeType;
}

//-----------------------------------------------------------------------------
/**
*/
inline Util::Array<Util::String>
AnimatorNode::GetAllAnimatedNodesPaths(IndexT i)
{
    return this->animSection[i].animatedNodesPath;
}

//-----------------------------------------------------------------------------
/**
*/
inline AnimKeyArray<AnimKey<Math::vector> > & 
AnimatorNode::GetPosArray(IndexT i)
{
    return this->animSection[i].posArray;
}

//-----------------------------------------------------------------------------
/**
*/
inline AnimKeyArray<AnimKey<Math::vector> > & 
AnimatorNode::GetEulerArray(IndexT i)
{
    return this->animSection[i].eulerArray;
}

//-----------------------------------------------------------------------------
/**
*/
inline AnimKeyArray<AnimKey<Math::vector> > & 
AnimatorNode::GetScaleArray(IndexT i)
{
    return this->animSection[i].scaleArray;
}

//-----------------------------------------------------------------------------
/**
*/
inline SizeT 
AnimatorNode::GetNumAnimSections() const
{
    return this->animSection.Size();
}

//-----------------------------------------------------------------------------
/**
*/
inline Util::Array<int>& 
AnimatorNode::GetLayerArray(IndexT i)
{
    return this->animSection[i].layer;
}

} // namespace Models
//------------------------------------------------------------------------------
