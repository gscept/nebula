#pragma once
//------------------------------------------------------------------------------
/**
    @class Models::TransformNode
    
    Defines a transformation in a transform hierarchy.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "models/modelnode.h"
#include "math/point.h"
#include "math/vector.h"
#include "math/quaternion.h"

//------------------------------------------------------------------------------
namespace Models
{
class TransformNode : public ModelNode
{
    __DeclareClass(TransformNode);
public:
    /// constructor
    TransformNode();
    /// destructor
    virtual ~TransformNode();

    /// create a model node instance
    virtual Ptr<ModelNodeInstance> CreateNodeInstance() const;
    /// parse data tag (called by loader code)
    virtual bool ParseDataTag(const Util::FourCC& fourCC, const Ptr<IO::BinaryReader>& reader);
    /// get overall state of contained resources (Initial, Loaded, Pending, Failed, Cancelled)
    virtual Resources::Resource::State GetResourceState() const;

    /// set position
    void SetPosition(const Math::point& p);
    /// get position
    const Math::point& GetPosition() const;
    /// set rotate quaternion
    void SetRotation(const Math::quaternion& r);
    /// get rotate quaternion
    const Math::quaternion& GetRotation() const;
    /// set scale
    void SetScale(const Math::vector& s);
    /// get scale
    const Math::vector& GetScale() const;
    /// set rotate pivot
    void SetRotatePivot(const Math::point& p);
    /// get rotate pivot
    const Math::point& GetRotatePivot() const;
    /// set scale pivot
    void SetScalePivot(const Math::point& p);
    /// get scale pivot
    const Math::point& GetScalePivot() const;
    /// is transformnode in viewspace
    bool IsInViewSpace() const;
    /// set transformnode in viewspace
    void SetInViewSpace(bool b);
    /// get MinDistance	
    float GetMinDistance() const;
    /// set MinDistance
    void SetMinDistance(float val);
    /// get MaxDistance	
    float GetMaxDistance() const;
    /// set MaxDistance
    void SetMaxDistance(float val);  
    /// are lod distances used
    bool LodDistancesUsed() const;
    /// called when attached to model node
    virtual void OnAttachToModel(const Ptr<Model>& model);
    /// get LockedToViewer	
    bool GetLockedToViewer() const;
    /// set LockedToViewer
    void SetLockedToViewer(bool val);

    /// helper method to check whether the distance is within lod distances
    bool CheckLodDistance(float distToViewer) const;

protected:
    Math::point position;
    Math::quaternion rotate;
    Math::vector scale;
    Math::point rotatePivot;
    Math::point scalePivot;
    bool isInViewSpace;
    float minDistance;
    float maxDistance;
    bool useLodDistances;
    bool lockedToViewer;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
TransformNode::CheckLodDistance(float distToViewer) const
{
    if (this->useLodDistances)
    {
        if ((distToViewer >= this->minDistance) && (distToViewer < this->maxDistance))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        // lod distances not set, always visible
        return true;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void
TransformNode::SetPosition(const Math::point& p)
{
    this->position = p;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::point&
TransformNode::GetPosition() const
{
    return this->position;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TransformNode::SetRotation(const Math::quaternion& r)
{
    this->rotate = r;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::quaternion&
TransformNode::GetRotation() const
{
    return this->rotate;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TransformNode::SetScale(const Math::vector& s)
{
    this->scale = s;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::vector&
TransformNode::GetScale() const
{
    return this->scale;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TransformNode::SetRotatePivot(const Math::point& p)
{
    this->rotatePivot = p;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::point&
TransformNode::GetRotatePivot() const
{
    return this->rotatePivot;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TransformNode::SetScalePivot(const Math::point& p)
{
    this->scalePivot = p;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::point&
TransformNode::GetScalePivot() const
{
    return this->scalePivot;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
TransformNode::IsInViewSpace() const
{
    return this->isInViewSpace;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
TransformNode::SetInViewSpace(bool b)
{
    this->isInViewSpace = b;
}

//------------------------------------------------------------------------------
/**
*/
inline float 
TransformNode::GetMinDistance() const
{
    return minDistance;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
TransformNode::SetMinDistance(float val)
{
    this->minDistance  = val;
    this->useLodDistances = true;
}

//------------------------------------------------------------------------------
/**
*/
inline float 
TransformNode::GetMaxDistance() const
{
    return this->maxDistance;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
TransformNode::SetMaxDistance(float val)
{
    this->maxDistance = val;
    this->useLodDistances = true;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
TransformNode::LodDistancesUsed() const
{
    return this->useLodDistances;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
TransformNode::GetLockedToViewer() const
{
    return this->lockedToViewer;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
TransformNode::SetLockedToViewer(bool val)
{
    this->lockedToViewer = val;
}
} // namespace Models
//------------------------------------------------------------------------------
    