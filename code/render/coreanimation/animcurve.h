#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreAnimation::AnimCurve
    
    An animation curve describes a set of animation keys in an AnimKeyBuffer.
    AnimCurves are always part of an AnimClip object, and share properties
    with all other AnimCurves in their AnimClip object. An AnimCurve may
    be collapsed into a single key, so that AnimCurves where all keys
    are identical don't take up any space in the animation key buffer.
    For performance reasons, AnimCurve's are not as flexible as their
    Maya counterparts, for instance it is not possible to set 
    the pre- and post-infinity types per curve, but only per clip.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "coreanimation/curvetype.h"
#include "coreanimation/infinitytype.h"
#include "math/float4.h"

//------------------------------------------------------------------------------
namespace CoreAnimation
{
class AnimCurve
{
public:
    /// constructor
    AnimCurve();
    /// activate/deactivate the curve, only active curves will be evaluated
    void SetActive(bool b);
    /// return true if the animation curve is active
    bool IsActive() const;
    /// set/clear the static flag
    void SetStatic(bool b);
    /// return true if the curve is static
    bool IsStatic() const;
    /// set the static key of the curve
    void SetStaticKey(const Math::float4& staticKey);
    /// get the static key of the curve
    const Math::float4& GetStaticKey() const;
    /// set index of the first key in the AnimKeyBuffer
    void SetFirstKeyIndex(IndexT index);
    /// get index of the first key in the AnimKeyBuffer
    IndexT GetFirstKeyIndex() const;
    /// set the curve type
    void SetCurveType(CurveType::Code curveType);
    /// get the curve type
    CurveType::Code GetCurveType() const;

private:
    Math::float4 staticKey;
    IndexT firstKeyIndex;
    CurveType::Code curveType;
    bool isActive;
    bool isStatic;
};

//------------------------------------------------------------------------------
/**
*/
inline
AnimCurve::AnimCurve() :
    staticKey(0.0f, 0.0f, 0.0f, 0.0f),
    firstKeyIndex(0),
    curveType(CurveType::Float4),
    isActive(true),
    isStatic(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimCurve::SetActive(bool b)
{
    this->isActive = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
AnimCurve::IsActive() const
{
    return this->isActive;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimCurve::SetStatic(bool b)
{
    this->isStatic = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
AnimCurve::IsStatic() const
{
    return this->isStatic;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimCurve::SetStaticKey(const Math::float4& key)
{
    this->staticKey = key;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::float4&
AnimCurve::GetStaticKey() const
{
    return this->staticKey;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimCurve::SetFirstKeyIndex(IndexT i)
{
    this->firstKeyIndex = i;
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
AnimCurve::GetFirstKeyIndex() const
{
    return this->firstKeyIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimCurve::SetCurveType(CurveType::Code t)
{
    this->curveType = t;
}

//------------------------------------------------------------------------------
/**
*/
inline CurveType::Code
AnimCurve::GetCurveType() const
{
    return this->curveType;
}

} // namespace AnimCurve
//------------------------------------------------------------------------------
    