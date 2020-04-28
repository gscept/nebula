#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::TransformDeviceBase
    
    Manages global transform matrices and their combinations. Input transforms 
    are the view transform (transforms from world to view space),
    the projection transform (describes the projection from view space
    into projection space (pre-div-z)) and the current model matrix
    (transforms from model to world space). From these input transforms,
    the TransformDevice computes all useful combinations and
    inverted versions.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/singleton.h"
#include "math/mat4.h"

namespace CoreGraphics
{
class Shader;
}

//------------------------------------------------------------------------------
namespace Base
{
class TransformDeviceBase : public Core::RefCounted
{
    __DeclareClass(TransformDeviceBase);
    __DeclareSingleton(TransformDeviceBase);
public:
    /// constructor
    TransformDeviceBase();
    /// destructor
    virtual ~TransformDeviceBase();
    
    /// open the transform device
    bool Open();
    /// close the transform device
    void Close();
    /// return true if device is open
    bool IsOpen() const;
    /// apply view dependent settings
    void ApplyViewSettings();
    /// apply any model transform needed, implementation is platform dependent
    void ApplyModelTransforms(const Ptr<CoreGraphics::Shader>& shdInst);
    
    /// set projection transform
    void SetProjTransform(const Math::mat4& m);
    /// get current projection matrix
    const Math::mat4& GetProjTransform();
    /// get inverted projection transform
    const Math::mat4& GetInvProjTransform();
    /// set view transform
    void SetViewTransform(const Math::mat4& m);
    /// get view transform
    const Math::mat4& GetViewTransform();
    /// get current inverted view transform
    const Math::mat4& GetInvViewTransform();
    /// get current view-projection transform
    const Math::mat4& GetViewProjTransform();
    /// set model transform
    void SetModelTransform(const Math::mat4& m);
    /// get current model transform
    const Math::mat4& GetModelTransform();
    /// get current inverted model transform
    const Math::mat4& GetInvModelTransform();
    /// get current model-view matrix
    const Math::mat4& GetModelViewTransform();
    /// get current inverted model-view-transform
    const Math::mat4& GetInvModelViewTransform();
    /// get current model-view-projection transform
    const Math::mat4& GetModelViewProjTransform();

	/// set object id
	void SetObjectId(const uint id);
	/// get object id
	uint GetObjectId() const;
    /// set focal length
    void SetFocalLength(const Math::vec2& len);
    /// get focal length
    const Math::vec2& GetFocalLength() const;
	/// set depth planes
	void SetNearFarPlane(const Math::vec2& planes);
	/// get depth planes
	const Math::vec2& GetNearFarPlane() const;


private:
    enum TransformType
    {
        View = 0,
        InvView,
        Proj,
        InvProj,
        ViewProj,
        Model,
        InvModel,
        ModelView,
        InvModelView,
        ModelViewProj,

        NumTransformTypes,
    };

    /// set the dirty flag
    void SetDirtyFlag(TransformType type);
    /// clear the dirty flag
    void ClearDirtyFlag(TransformType type);
    /// return true if transform is dirty
    bool IsDirty(TransformType type) const;
    /// update the inverted projection transform
    void UpdateInvProjTransform();
    /// update the inverted view-projection transform
    void UpdateInvViewTransform();
    /// update the view-projection transform
    void UpdateViewProjTransform();
    /// update inverted model transform
    void UpdateInvModelTransform();
    /// update model-view transform
    void UpdateModelViewTransform();
    /// update inverted model-view transform
    void UpdateInvModelViewTransform();
    /// update model-view-proj transform
    void UpdateModelViewProjTransform();

    bool isOpen;
    uint dirtyFlags;                                // or'ed (1<<TransformType) dirty flags
    Util::FixedArray<Math::mat4> transforms;    // index is transform type
	uint id;										// id of current object
    Math::vec2 focalLength;
	Math::vec2 nearFarPlane;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
TransformDeviceBase::IsOpen() const
{
    return this->isOpen;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TransformDeviceBase::SetDirtyFlag(TransformType type)
{
    this->dirtyFlags |= (1<<type);
}

//------------------------------------------------------------------------------
/**
*/
inline void
TransformDeviceBase::ClearDirtyFlag(TransformType type)
{
    this->dirtyFlags &= ~(1<<type);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
TransformDeviceBase::IsDirty(TransformType type) const
{
    return (0 != (this->dirtyFlags & (1<<type)));
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::mat4&
TransformDeviceBase::GetProjTransform()
{
    return this->transforms[Proj];
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::mat4&
TransformDeviceBase::GetInvProjTransform()
{
    if (this->IsDirty(InvProj))
    {
        this->UpdateInvProjTransform();
    }
    return this->transforms[InvProj];
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::mat4&
TransformDeviceBase::GetViewTransform()
{
    return this->transforms[View];
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::mat4&
TransformDeviceBase::GetInvViewTransform()
{
    if (this->IsDirty(InvView))
    {
        this->UpdateInvViewTransform();
    }
    return this->transforms[InvView];
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::mat4&
TransformDeviceBase::GetViewProjTransform()
{
    if (this->IsDirty(ViewProj))
    {
        this->UpdateViewProjTransform();
    }
    return this->transforms[ViewProj];
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::mat4&
TransformDeviceBase::GetModelTransform()
{
    return this->transforms[Model];
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::mat4&
TransformDeviceBase::GetInvModelTransform()
{
    if (this->IsDirty(InvModel))
    {
        this->UpdateInvModelTransform();
    }
    return this->transforms[InvModel];
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::mat4&
TransformDeviceBase::GetModelViewTransform()
{
    if (this->IsDirty(ModelView))
    {
        this->UpdateModelViewTransform();
    }
    return this->transforms[ModelView];
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::mat4&
TransformDeviceBase::GetInvModelViewTransform()
{
    if (this->IsDirty(InvModelView))
    {
        this->UpdateInvModelViewTransform();
    }
    return this->transforms[InvModelView];
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::mat4&
TransformDeviceBase::GetModelViewProjTransform()
{
    if (this->IsDirty(ModelViewProj))
    {
        this->UpdateModelViewProjTransform();
    }
    return this->transforms[ModelViewProj];
}

//------------------------------------------------------------------------------
/**
*/
inline void
TransformDeviceBase::SetObjectId(const uint id)
{
	this->id = id;
}

//------------------------------------------------------------------------------
/**
*/
inline uint
TransformDeviceBase::GetObjectId() const
{
	return this->id;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
TransformDeviceBase::SetFocalLength(const Math::vec2& len)
{
    this->focalLength = len;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::vec2& 
TransformDeviceBase::GetFocalLength() const
{
    return this->focalLength;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
TransformDeviceBase::SetNearFarPlane(const Math::vec2& planes)
{
	this->nearFarPlane = planes;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::vec2&
TransformDeviceBase::GetNearFarPlane() const
{
	return this->nearFarPlane;
}

} // namespace Base
//------------------------------------------------------------------------------

    