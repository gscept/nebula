#pragma once
//------------------------------------------------------------------------------
/**
    @class Models::TransformNodeInstance

    Holds and applies per-node-instance transformation.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "models/modelnodeinstance.h"
#include "math/vector.h"
#include "math/point.h"
#include "math/quaternion.h"
#include "math/transform44.h"

//------------------------------------------------------------------------------
namespace Models
{
class TransformNodeInstance : public ModelNodeInstance
{
    __DeclareClass(TransformNodeInstance);
public:
    /// constructor
    TransformNodeInstance();
    /// destructor
    virtual ~TransformNodeInstance();

    /// called from ModelEntity::OnRenderBefore
    virtual void OnRenderBefore(IndexT frameIndex, Timing::Time time);
    /// apply per-instance state prior to rendering
	virtual void ApplyState(IndexT frameIndex, const IndexT& pass);

    /// set position
    void SetPosition(const Math::point& p);
    /// get position
    const Math::point& GetPosition() const;
    /// set rotate quaternion
    void SetRotate(const Math::quaternion& r);
    /// get rotate quaternion
    const Math::quaternion& GetRotate() const;
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
    /// set optional offset matrix
    void SetOffsetMatrix(const Math::matrix44& m);
    /// get optional offset matrix
    const Math::matrix44& GetOffsetMatrix() const;
    /// is transformnode in viewspace
    bool IsInViewSpace() const;
    /// set transformnode in viewspace
    void SetInViewSpace(bool b);
    /// get LockedToViewer	
    bool GetLockedToViewer() const;
    /// set LockedToViewer
    void SetLockedToViewer(bool val);

    /// get resulting local transform matrix in local parent space
    const Math::matrix44& GetLocalTransform();
    /// get model space transform (valid after Update())
    const Math::matrix44& GetModelTransform() const;  

protected:
    /// called when attached to ModelInstance
    virtual void Setup(const Ptr<ModelInstance>& inst, const Ptr<ModelNode>& node, const Ptr<ModelNodeInstance>& parentNodeInst);
    /// called when removed from ModelInstance
    virtual void Discard();
    /// render node specific debug shape
    virtual void RenderDebug();    

    Ptr<TransformNodeInstance> parentTransformNodeInstance;
    Math::transform44 tform;
    Math::matrix44 modelTransform;
    bool isInViewSpace;
    bool lockedToViewer;
};

//------------------------------------------------------------------------------
/**
*/
inline void
TransformNodeInstance::SetPosition(const Math::point& p)
{
    this->tform.setposition(p);
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::point&
TransformNodeInstance::GetPosition() const
{
    return this->tform.getposition();
}

//------------------------------------------------------------------------------
/**
*/
inline void
TransformNodeInstance::SetRotate(const Math::quaternion& r)
{
    this->tform.setrotate(r);
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::quaternion&
TransformNodeInstance::GetRotate() const
{
    return this->tform.getrotate();
}

//------------------------------------------------------------------------------
/**
*/
inline void
TransformNodeInstance::SetScale(const Math::vector& s)
{
    this->tform.setscale(s);
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::vector&
TransformNodeInstance::GetScale() const
{
    return this->tform.getscale();
}

//------------------------------------------------------------------------------
/**
*/
inline void
TransformNodeInstance::SetRotatePivot(const Math::point& p)
{
    this->tform.setrotatepivot(p);
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::point&
TransformNodeInstance::GetRotatePivot() const
{
    return this->tform.getrotatepivot();
}

//------------------------------------------------------------------------------
/**
*/
inline void
TransformNodeInstance::SetScalePivot(const Math::point& p)
{
    this->tform.setscalepivot(p);
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::point&
TransformNodeInstance::GetScalePivot() const
{
    return this->tform.getscalepivot();
}

//------------------------------------------------------------------------------
/**
*/
inline void
TransformNodeInstance::SetOffsetMatrix(const Math::matrix44& m)
{
    this->tform.setoffset(m);
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::matrix44&
TransformNodeInstance::GetOffsetMatrix() const
{
    return this->tform.getoffset();
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::matrix44&
TransformNodeInstance::GetLocalTransform()
{
    return this->tform.getmatrix();
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::matrix44&
TransformNodeInstance::GetModelTransform() const
{
    return this->modelTransform;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
TransformNodeInstance::IsInViewSpace() const
{
    return this->isInViewSpace;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
TransformNodeInstance::SetInViewSpace(bool b)
{
    this->isInViewSpace = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
TransformNodeInstance::GetLockedToViewer() const
{
    return this->lockedToViewer;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
TransformNodeInstance::SetLockedToViewer(bool val)
{
    this->lockedToViewer = val;
}
} // namespace Models
//------------------------------------------------------------------------------
