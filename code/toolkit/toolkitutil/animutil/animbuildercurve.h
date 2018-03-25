#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::AnimBuilderCurve
    
    An animation curve object in the AnimBuilder class.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "coreanimation/curvetype.h"
#include "util/fixedarray.h"
#include "math/float4.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class AnimBuilderCurve
{
public:
    /// constructor
    AnimBuilderCurve();
    
    /// resize the key array
    void ResizeKeyArray(SizeT numKeys);

    /// set a key in the keys array
    void SetKey(IndexT i, const Math::float4& k);
    /// get a key at index
    const Math::float4& GetKey(IndexT i) const;
    /// get number of keys
    SizeT GetNumKeys() const;

	/// gets whether or not the curve is valid
	void FixInvalidKeys() const;

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
    void SetCurveType(CoreAnimation::CurveType::Code curveType);
    /// get the curve type
    CoreAnimation::CurveType::Code GetCurveType() const;

private:
    Util::FixedArray<Math::float4> keyArray;
    Math::float4 staticKey;
    IndexT firstKeyIndex;
    CoreAnimation::CurveType::Code curveType;
    bool isActive;
    bool isStatic;
};

//------------------------------------------------------------------------------
/**
*/
inline void
AnimBuilderCurve::SetKey(IndexT i, const Math::float4& k)
{
    this->keyArray[i] = k;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::float4&
AnimBuilderCurve::GetKey(IndexT i) const
{
    return this->keyArray[i];
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
AnimBuilderCurve::GetNumKeys() const
{
    return this->keyArray.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimBuilderCurve::SetActive(bool b)
{
    this->isActive = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
AnimBuilderCurve::IsActive() const
{
    return this->isActive;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimBuilderCurve::SetStatic(bool b)
{
    this->isStatic = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
AnimBuilderCurve::IsStatic() const
{
    return this->isStatic;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimBuilderCurve::SetStaticKey(const Math::float4& k)
{
    this->staticKey = k;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::float4&
AnimBuilderCurve::GetStaticKey() const
{
    return this->staticKey;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimBuilderCurve::SetFirstKeyIndex(IndexT i)
{
    this->firstKeyIndex = i;
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
AnimBuilderCurve::GetFirstKeyIndex() const
{
    return this->firstKeyIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimBuilderCurve::SetCurveType(CoreAnimation::CurveType::Code t)
{
    this->curveType = t;
}

//------------------------------------------------------------------------------
/**
*/
inline CoreAnimation::CurveType::Code
AnimBuilderCurve::GetCurveType() const
{
    return this->curveType;
}


//------------------------------------------------------------------------------
/**
*/
inline void 
AnimBuilderCurve::FixInvalidKeys() const
{
	switch (this->curveType)
	{
	case CoreAnimation::CurveType::Scale:
	case CoreAnimation::CurveType::Translation:
		{
			for (int keyIndex = 0; keyIndex < this->keyArray.Size(); keyIndex++)
			{
				if (this->keyArray[keyIndex].w() != 0)
				{
					this->keyArray[keyIndex].w() = 0;
				}
			}
		}
	}
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------
    