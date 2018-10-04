#pragma once
//------------------------------------------------------------------------------
/**
    @class Math::vector
    
    A vector in homogenous space. A vector describes a direction and length
    in 3d space and always has a w component of 0.0.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "math/float4.h"

//------------------------------------------------------------------------------
namespace Math
{
class vector;

#if (defined(_XM_VMX128_INTRINSICS_) && !defined(_XM_NO_INTRINSICS_))
typedef const vector  __VectorArg;
#else
// win32 VC++ does not support passing aligned objects as value so far
// here is a bug-report at microsoft saying so:
// http://connect.microsoft.com/VisualStudio/feedback/ViewFeedback.aspx?FeedbackID=334581
typedef const vector& __VectorArg;
#endif

class NEBULA_ALIGN16 vector : public float4
{
public:
    /// default constructor
    vector();
    /// construct from components
    vector(scalar x, scalar y, scalar z);
	/// construct from single component
	vector(scalar v);
    /// construct from float4
    vector(const float4& rhs);
    /// construct from vmVector4
    vector(const __m128 &rhs);
    /// return the null vector
    static vector nullvec();
    /// return the standard up vector (0, 1, 0)
    static vector upvec();
    /// assignment operator
    void operator=(const vector& rhs);
    /// assign vmVector4
    void operator=(const __m128 &rhs);
    /// flip sign
    vector operator-() const;
    /// add vector inplace
    void operator+=(const vector& rhs);
    /// subtract vector inplace
    void operator-=(const vector& rhs);
    /// scale vector inplace
    void operator*=(scalar s);
    /// add 2 vectors
    vector operator+(const vector& rhs) const;
    /// subtract 2 vectors
    vector operator-(const vector& rhs) const;
    /// scale vector
    vector operator*(scalar s) const;
    /// equality operator
    bool operator==(const vector& rhs) const;
    /// inequality operator
    bool operator!=(const vector& rhs) const;
    /// set components
    void set(scalar x, scalar y, scalar z);

    friend class point;
};

//------------------------------------------------------------------------------
/**
*/
__forceinline
vector::vector() :
    float4(0.0f, 0.0f, 0.0f, 0.0f)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
vector::vector(scalar x, scalar y, scalar z) :
    float4(x, y, z, 0.0f)
{
    // empty
}
	
//------------------------------------------------------------------------------
/**
*/
__forceinline
vector::vector(scalar v) :
	float4(v, v, v, 0.0f)
{
		// empty
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
vector::vector(const float4& rhs) :
    float4(rhs)
{
    this->set_w(0.0f);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
vector::vector(const __m128 &rhs) :
    float4(rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vector
vector::nullvec()
{
    return vector(0.0f, 0.0f, 0.0f);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vector
vector::upvec()
{
    return vector(0.0f, 1.0f, 0.0f);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vector::operator=(const vector& rhs)
{
    this->vec = rhs.vec;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vector::operator=(const __m128 &rhs)
{
    this->vec.vec = rhs;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vector
vector::operator-() const
{
    static const  mm128_ivec negmask = {static_cast<int>(0x80000000),static_cast<int>(0x80000000),static_cast<int>(0x80000000),static_cast<int>(0x80000000)};
    return  _mm_xor_ps(negmask,this->vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vector::operator+=(const vector& rhs)
{
    float4::operator+=(rhs);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vector::operator-=(const vector& rhs)
{
    float4::operator-=(rhs);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vector::operator*=(scalar s)
{
    float4::operator*=(s);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vector
vector::operator+(const vector& rhs) const
{
    return float4::operator+(rhs);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vector
vector::operator-(const vector& rhs) const
{
    return float4::operator-(rhs);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vector
vector::operator*(scalar s) const
{
    return float4::operator*(s);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
vector::operator==(const vector& rhs) const
{
    return float4::operator==(rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
vector::operator!=(const vector& rhs) const
{
    return float4::operator!=(rhs.vec);
}    

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vector::set(scalar x, scalar y, scalar z)
{
    float4::set(x, y, z, 0.0f);
}

} // namespace Math
//------------------------------------------------------------------------------
