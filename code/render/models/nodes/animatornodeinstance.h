#pragma once
//------------------------------------------------------------------------------
/**
    @class Animator::AnimatorInstance
    
    Legacy N2 crap!

    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/

#include "models/modelnodeinstance.h"
#include "timing/time.h"
#include "models/nodes/animlooptype.h"
#include "models/nodes/animkeyarray.h"
#include "models/nodes/animkey.h"
#include "math/vector.h"
#include "materials/materialvariableinstance.h"
#include "materials/surfaceconstantinstance.h"

namespace Models
{
class AnimatorNodeInstance : public Models::ModelNodeInstance
{
__DeclareClass(AnimatorNodeInstance);
public:
    /// Constructor
    AnimatorNodeInstance();
    /// Destructor
    virtual ~AnimatorNodeInstance();
    /// setup the model node instance
    virtual void Setup(const Ptr<ModelInstance>& inst, const Ptr<ModelNode>& node, const Ptr<ModelNodeInstance>& parentNodeInst);    
    /// called from ModelEntity::OnNotifyCullingVisible
    virtual void OnNotifyCullingVisible(IndexT frameIndex, Timing::Time time);

    virtual void OverwriteAnimationTime(Timing::Time time);
private:
    /// called by scene node objects which wish to be animated by this object
    void Animate(Timing::Time time);
    /// called when the node becomes visible with current time
    virtual void OnShow(Timing::Time time);

    struct AnimatedNode
    {
        Ptr<ModelNodeInstance> node;
        Ptr<Materials::SurfaceConstant> var;
    };

    Util::Array<Util::Array<AnimatedNode> > animSection;
    Timing::Time startTime;
    Timing::Time overWrittenAnimTime;
};
__RegisterClass(AnimatorNodeInstance);

//------------------------------------------------------------------------------
/**
*/
inline void
AnimatorNodeInstance::OverwriteAnimationTime(Timing::Time time)
{
    this->overWrittenAnimTime = this->startTime + time;
}
};