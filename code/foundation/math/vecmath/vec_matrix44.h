#pragma once
//------------------------------------------------------------------------------
/**
    @class Math::matrix44

    A matrix44 class on top of VectorMath math functions.

    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "math/scalar.h"
#include "math/float4.h"
#include "math/plane.h"
#include "math/quaternion.h"

//------------------------------------------------------------------------------

#define mm_ror_ps(vec,i)	\
	(((i)%4) ? (_mm_shuffle_ps(vec,vec, _MM_SHUFFLE((unsigned char)(i+3)%4,(unsigned char)(i+2)%4,(unsigned char)(i+1)%4,(unsigned char)(i+0)%4))) : (vec))

namespace Math
{
class quaternion;
class plane;

typedef NEBULA3_ALIGN16 struct mm128_mat
{
	union
	{
		mm128_vec r[4];
		struct
		{
			float _11, _12, _13, _14;
			float _21, _22, _23, _24;
			float _31, _32, _33, _34;
			float _41, _42, _43, _44;
		};
		float m[4][4];
	};
} mm128_mat;

const mm128_ivec maskX = {-1,0,0,0};
const mm128_ivec maskY = {0,-1,0,0};
const mm128_ivec maskZ = {0,0,-1,0};
const mm128_ivec maskW = {0,0,0,-1};

// could not get the compiler to really pass it in registers for xbox, so
// this is a reference so far
typedef const matrix44& __Matrix44Arg;

class NEBULA3_ALIGN16 matrix44
{
public:
    /// default constructor, NOTE: does NOT setup components!
    matrix44();
    /// construct from components
    matrix44(float4 const &row0, float4 const &row1, float4 const &row2, float4 const &row3);
    /// copy constructor
    //matrix44(const matrix44& rhs);
    /// construct from vmMat4
    matrix44(const mm128_mat& rhs);

    /// assignment operator
    void operator=(const matrix44& rhs);
    /// assign vmMat4
    void operator=(const mm128_mat& rhs);
    /// equality operator
    bool operator==(const matrix44& rhs) const;
    /// inequality operator
    bool operator!=(const matrix44& rhs) const;

    /// load content from 16-byte-aligned memory
    void load(const scalar* ptr);
    /// load content from unaligned memory
    void loadu(const scalar* ptr);
    /// write content to 16-byte-aligned memory through the write cache
    void store(scalar* ptr) const;
    /// write content to unaligned memory through the write cache
    void storeu(scalar* ptr) const;
    /// stream content to 16-byte-aligned memory circumventing the write-cache
    void stream(scalar* ptr) const;

    /// set content
    void set(float4 const &row0, float4 const &row1, float4 const &row2, float4 const &row3);
    /// write access to x component
    void setrow0(float4 const &row0);
    /// write access to y component
    void setrow1(float4 const &row1);
    /// write access to z component
    void setrow2(float4 const &row2);
    /// write access to w component
    void setrow3(float4 const &row3);
    /// read-only access to x component
    const float4& getrow0() const;
    /// read-only access to y component
    const float4& getrow1() const;
    /// read-only access to z component
    const float4& getrow2() const;
    /// read-only access to w component
    const float4& getrow3() const;
	/// read-only access to row
	const float4& getrow(int row) const;
	/// read-write access to x component
	float4& row0() const;
	/// read-write access to y component
	float4& row1() const;
	/// read-write access to z component
	float4& row2() const;
	/// read-write access to w component
	float4& row3() const;
	/// read-write access to row
	float4& row(int i);

    /// write access to x component
    void set_xaxis(float4 const &x);
    /// write access to y component
    void set_yaxis(float4 const &y);
    /// write access to z component
    void set_zaxis(float4 const &z);
    /// write access to w component / pos component
    void set_position(float4 const &pos);
    /// read access to x component
    const float4& get_xaxis() const;
    /// read access to y component
    const float4& get_yaxis() const;
    /// read access to z component
    const float4& get_zaxis() const;
    /// read access to w component / pos component
    const float4& get_position() const;
	/// extracts scale components to target vector
	void get_scale(float4& scale) const;
    /// add a translation to pos_component
    void translate(float4 const &t);
    /// scale matrix
    void scale(float4 const &v);

    /// return true if matrix is identity
    bool isidentity() const;
    /// return determinant of matrix
    scalar determinant() const;
    /// decompose into scale, rotation and translation
    /// !!! Note:
    void decompose(float4& outScale, quaternion& outRotation, float4& outTranslation) const;

    /// build identity matrix
    static matrix44 identity();
    /// build matrix from affine transformation
    static matrix44 affinetransformation(scalar scaling, float4 const &rotationCenter, const quaternion& rotation, float4 const &translation);
    /// compute the inverse of a matrix
    static matrix44 inverse(const matrix44& m);
    /// build left handed lookat matrix
	static matrix44 lookatlh(const point& eye, const point& at, const vector& up);
    /// build right handed lookat matrix
	static matrix44 lookatrh(const point& eye, const point& at, const vector& up);
    /// multiply 2 matrices
    static matrix44 multiply(const matrix44& m0, const matrix44& m1);	
    /// build left handed orthogonal projection matrix
    static matrix44 ortholh(scalar w, scalar h, scalar zn, scalar zf);
    /// build right handed orthogonal projection matrix
    static matrix44 orthorh(scalar w, scalar h, scalar zn, scalar zf);
    /// build left-handed off-center orthogonal projection matrix
    static matrix44 orthooffcenterlh(scalar l, scalar r, scalar b, scalar t, scalar zn, scalar zf);
    /// build right-handed off-center orthogonal projection matrix
    static matrix44 orthooffcenterrh(scalar l, scalar r, scalar b, scalar t, scalar zn, scalar zf);
    /// build left-handed perspective projection matrix based on field-of-view
    static matrix44 perspfovlh(scalar fovy, scalar aspect, scalar zn, scalar zf);
    /// build right-handed perspective projection matrix based on field-of-view
    static matrix44 perspfovrh(scalar fovy, scalar aspect, scalar zn, scalar zf);
    /// build left-handed perspective projection matrix
    static matrix44 persplh(scalar w, scalar h, scalar zn, scalar zf);
    /// build right-handed perspective projection matrix
    static matrix44 persprh(scalar w, scalar h, scalar zn, scalar zf);
    /// build left-handed off-center perspective projection matrix
    static matrix44 perspoffcenterlh(scalar l, scalar r, scalar b, scalar t, scalar zn, scalar zf);
    /// build right-handed off-center perspective projection matrix
    static matrix44 perspoffcenterrh(scalar l, scalar r, scalar b, scalar t, scalar zn, scalar zf);
    /// build matrix that reflects coordinates about a plance
    static matrix44 reflect(const plane& p);
    /// build rotation matrix around arbitrary axis
    static matrix44 rotationaxis(float4 const &axis, scalar angle);
    /// build rotation matrix from quaternion
    static matrix44 rotationquaternion(const quaternion& q);
    /// build x-axis-rotation matrix
    static matrix44 rotationx(scalar angle);
    /// build y-axis-rotation matrix
    static matrix44 rotationy(scalar angle);
    /// build z-axis-rotation matrix
    static matrix44 rotationz(scalar angle);
    /// build rotation matrix from yaw, pitch and roll
    static matrix44 rotationyawpitchroll(scalar yaw, scalar pitch, scalar roll);
    /// build a scaling matrix from components
    static matrix44 scaling(scalar sx, scalar sy, scalar sz);
    /// build a scaling matrix from float4
    static matrix44 scaling(float4 const &s);
    /// build a transformation matrix
    static matrix44 transformation(float4 const &scalingCenter, const quaternion& scalingRotation, float4 const &scaling, float4 const &rotationCenter, const quaternion& rotation, float4 const &translation);
    /// build a translation matrix
    static matrix44 translation(scalar x, scalar y, scalar z);
    /// build a translation matrix from point
    static matrix44 translation(float4 const &t);
    /// return the transpose of a matrix
    static matrix44 transpose(const matrix44& m);
    /// transform 4d vector by matrix44, faster inline version than float4::transform
    static float4 transform(const float4 &v, const matrix44 &m);
    /// return a quaternion from rotational part of the 4x4 matrix
    static quaternion rotationmatrix(const matrix44& m);
    /// transform a plane with a matrix
    static plane transform(const plane& p, const matrix44& m);
    /// check if point lies inside matrix frustum
    static bool ispointinside(const float4& p, const matrix44& m);
    /// convert to any type
    template<typename T> T as() const;

//private:
    friend class float4;
    friend class plane;
    friend class quaternion;

    mm128_mat mat;
};

//------------------------------------------------------------------------------
/**
*/
__forceinline
matrix44::matrix44()
{
	this->mat.r[0] = _id_x;
	this->mat.r[1] = _id_y;
	this->mat.r[2] = _id_z;
	this->mat.r[3] = _id_w;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
matrix44::matrix44(float4 const &row0, float4 const &row1, float4 const &row2, float4 const &row3)
{
    this->mat.r[0].vec = row0.vec;
	this->mat.r[1].vec = row1.vec;
	this->mat.r[2].vec = row2.vec;
	this->mat.r[3].vec = row3.vec;
}


//------------------------------------------------------------------------------
/**
*/
__forceinline
matrix44::matrix44(const mm128_mat& rhs) :
    mat(rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::operator=(const matrix44& rhs)
{
    this->mat = rhs.mat;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::operator=(const mm128_mat& rhs)
{
    this->mat = rhs;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
matrix44::operator==(const matrix44& rhs) const
{
	return float4(this->mat.r[0]) == float4(rhs.mat.r[0]) &&
			float4(this->mat.r[1]) == float4(rhs.mat.r[1]) &&
			float4(this->mat.r[2]) == float4(rhs.mat.r[2]) &&
			float4(this->mat.r[3]) == float4(rhs.mat.r[3]);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
matrix44::operator!=(const matrix44& rhs) const
{
	return !(*this == rhs);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::load(const scalar* ptr)
{
	this->mat.r[0].vec = _mm_load_ps(ptr);
	this->mat.r[1].vec = _mm_load_ps(ptr + 4);
	this->mat.r[2].vec = _mm_load_ps(ptr + 8);
	this->mat.r[3].vec = _mm_load_ps(ptr + 12);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::loadu(const scalar* ptr)
{
	this->mat.r[0].vec = _mm_loadu_ps(ptr);
	this->mat.r[1].vec = _mm_loadu_ps(ptr + 4);
	this->mat.r[2].vec = _mm_loadu_ps(ptr + 8);
	this->mat.r[3].vec = _mm_loadu_ps(ptr + 12);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::store(scalar* ptr) const
{
	_mm_store_ps(ptr,this->mat.r[0].vec);
	_mm_store_ps((ptr + 4), this->mat.r[1].vec);
	_mm_store_ps((ptr + 8), this->mat.r[2].vec);
	_mm_store_ps((ptr + 12), this->mat.r[3].vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::storeu(scalar* ptr) const
{
	_mm_storeu_ps(ptr,this->mat.r[0].vec);
	_mm_storeu_ps((ptr + 4), this->mat.r[1].vec);
	_mm_storeu_ps((ptr + 8), this->mat.r[2].vec);
	_mm_storeu_ps((ptr + 12), this->mat.r[3].vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::stream(scalar* ptr) const
{
	this->storeu(ptr);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::set(float4 const &row0, float4 const &row1, float4 const &row2, float4 const &row3)
{
	this->mat.r[0].vec = row0.vec;
	this->mat.r[1].vec = row1.vec;
	this->mat.r[2].vec = row2.vec;
	this->mat.r[3].vec = row3.vec;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::setrow0(float4 const &r)
{
	this->mat.r[0].vec = r.vec;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline const float4&
matrix44::getrow0() const
{
	return *(float4*)&(this->mat.r[0]);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::setrow1(float4 const &r)
{
    this->mat.r[1] = r.vec;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline const float4&
matrix44::getrow1() const
{
    return *(float4*)&(this->mat.r[1]);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::setrow2(float4 const &r)
{
    this->mat.r[2] = r.vec;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline const float4&
matrix44::getrow2() const
{
    return *(float4*)&(this->mat.r[2]);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::setrow3(float4 const &r)
{
    this->mat.r[3] = r.vec;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline const float4&
matrix44::getrow3() const
{
    return *(float4*)&(this->mat.r[3]);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline const float4&
matrix44::getrow(int row) const
{
	return *(float4*)&(this->mat.r[row]);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4&
matrix44::row0() const
{
	return *(float4*)&(this->mat.r[0]);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4&
matrix44::row1() const
{
	return *(float4*)&(this->mat.r[1]);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4&
matrix44::row2() const
{
	return *(float4*)&(this->mat.r[2]);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4&
matrix44::row3() const
{
	return *(float4*)&(this->mat.r[3]);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4&
matrix44::row(int row)
{
	return *(float4*)&(this->mat.r[row]);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::set_xaxis(float4 const &x)
{
    this->mat.r[0] = x.vec;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::set_yaxis(float4 const &y)
{
    this->mat.r[1] = y.vec;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::set_zaxis(float4 const &z)
{
    this->mat.r[2] = z.vec;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::set_position(float4 const &pos)
{
    this->mat.r[3] = pos.vec;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline const float4&
matrix44::get_xaxis() const
{
    return *(float4*)&(this->mat.r[0]);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline const float4&
matrix44::get_yaxis() const
{
    return *(float4*)&(this->mat.r[1]);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline const float4&
matrix44::get_zaxis() const
{
    return *(float4*)&(this->mat.r[2]);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline const float4&
matrix44::get_position() const
{
    return *(float4*)&(this->mat.r[3]);
}


//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::get_scale(float4& v) const
{
	float4 xaxis = this->mat.r[0];
	float4 yaxis = this->mat.r[1];
	float4 zaxis = this->mat.r[2];
	scalar xScale = xaxis.length3();
	scalar yScale = yaxis.length3();
	scalar zScale = zaxis.length3();

	v.set_x(xScale);
	v.set_y(yScale);
	v.set_z(zScale);
	v.set_w(1.0f);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
void
matrix44::translate(float4 const &t)
{
    #if _DEBUG
    n_assert2(t.w() == 0, "w component not 0, use vector for translation not a point!");
    #endif
    this->mat.r[3] = (float4(this->mat.r[3]) + t).vec;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::scale(float4 const &s)
{
    // need to make sure that last column isn't erased
    float4 scl = s;
    scl.set_w(1.0f);

    this->mat.r[0] = float4::multiply(this->mat.r[0], scl).vec;
    this->mat.r[1] = float4::multiply(this->mat.r[1], scl).vec;
    this->mat.r[2] = float4::multiply(this->mat.r[2], scl).vec;
    this->mat.r[3] = float4::multiply(this->mat.r[3], scl).vec;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
matrix44::isidentity() const
{
    return float4(this->mat.r[0]) == _id_x &&
	float4(this->mat.r[1]) == _id_y &&
	float4(this->mat.r[2]) == _id_z &&
	float4(this->mat.r[3]) == _id_w;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
matrix44::determinant() const
{
    __m128 Va,Vb,Vc;
    __m128 r1,r2,r3,tt,tt2;
    __m128 sum,Det;

    __m128 _L1 = this->mat.r[0];
    __m128 _L2 = this->mat.r[1];
    __m128 _L3 = this->mat.r[2];
    __m128 _L4 = this->mat.r[3];
    // Calculating the minterms for the first line.

    // _mm_ror_ps is just a macro using _mm_shuffle_ps().
    tt = _L4; tt2 = mm_ror_ps(_L3,1);
    Vc = _mm_mul_ps(tt2, mm_ror_ps(tt,0));					// V3' dot V4
    Va = _mm_mul_ps(tt2, mm_ror_ps(tt,2));					// V3' dot V4"
    Vb = _mm_mul_ps(tt2, mm_ror_ps(tt,3));					// V3' dot V4^

    r1 = _mm_sub_ps(mm_ror_ps(Va,1), mm_ror_ps(Vc,2));		// V3" dot V4^ - V3^ dot V4"
    r2 = _mm_sub_ps(mm_ror_ps(Vb,2), mm_ror_ps(Vb,0));		// V3^ dot V4' - V3' dot V4^
    r3 = _mm_sub_ps(mm_ror_ps(Va,0), mm_ror_ps(Vc,1));		// V3' dot V4" - V3" dot V4'

    tt = _L2;
    Va = mm_ror_ps(tt,1);		sum = _mm_mul_ps(Va,r1);
    Vb = mm_ror_ps(tt,2);		sum = _mm_add_ps(sum,_mm_mul_ps(Vb,r2));
    Vc = mm_ror_ps(tt,3);		sum = _mm_add_ps(sum,_mm_mul_ps(Vc,r3));

    // Calculating the determinant.
    Det = _mm_mul_ps(sum,_L1);
    Det = _mm_add_ps(Det,_mm_movehl_ps(Det,Det));

    // Calculating the minterms of the second line (using previous results).
    tt = mm_ror_ps(_L1,1);		sum = _mm_mul_ps(tt,r1);
    tt = mm_ror_ps(tt,1);		sum = _mm_add_ps(sum,_mm_mul_ps(tt,r2));
    tt = mm_ror_ps(tt,1);		sum = _mm_add_ps(sum,_mm_mul_ps(tt,r3));

    // Testing the determinant.
    Det = _mm_sub_ss(Det,_mm_shuffle_ps(Det,Det,1));
    return float4::unpack_x(Det);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::identity()
{
	return matrix44();    
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::inverse(const matrix44& m)
{
    __m128 Va,Vb,Vc;
    __m128 r1,r2,r3,tt,tt2;
    __m128 sum,Det,RDet;
    __m128 trns0,trns1,trns2,trns3;

    const mm128_ivec pnpn = {0x00000000, static_cast<int>(0x80000000), 0x00000000, static_cast<int>(0x80000000)};
    const mm128_ivec npnp = {static_cast<int>(0x80000000), 0x00000000, static_cast<int>(0x80000000), 0x00000000};
    const mm128_vec zeroone =  {1.0f, 0.0f, 0.0f, 1.0f};

    __m128 _L1 = m.mat.r[0];
    __m128 _L2 = m.mat.r[1];
    __m128 _L3 = m.mat.r[2];
    __m128 _L4 = m.mat.r[3];
    // Calculating the minterms for the first line.

    // _mm_ror_ps is just a macro using _mm_shuffle_ps().
    tt = _L4; tt2 = mm_ror_ps(_L3,1);
    Vc = _mm_mul_ps(tt2, mm_ror_ps(tt,0));					// V3'dot V4
    Va = _mm_mul_ps(tt2, mm_ror_ps(tt,2));					// V3'dot V4"
    Vb = _mm_mul_ps(tt2, mm_ror_ps(tt,3));					// V3' dot V4^

    r1 = _mm_sub_ps(mm_ror_ps(Va,1), mm_ror_ps(Vc,2));		// V3" dot V4^ - V3^ dot V4"
    r2 = _mm_sub_ps(mm_ror_ps(Vb,2), mm_ror_ps(Vb,0));		// V3^ dot V4' - V3' dot V4^
    r3 = _mm_sub_ps(mm_ror_ps(Va,0), mm_ror_ps(Vc,1));		// V3' dot V4" - V3" dot V4'

    tt = _L2;
    Va = mm_ror_ps(tt,1);		sum = _mm_mul_ps(Va,r1);
    Vb = mm_ror_ps(tt,2);		sum = _mm_add_ps(sum,_mm_mul_ps(Vb,r2));
    Vc = mm_ror_ps(tt,3);		sum = _mm_add_ps(sum,_mm_mul_ps(Vc,r3));

    // Calculating the determinant.
    Det = _mm_mul_ps(sum,_L1);
    Det = _mm_add_ps(Det,_mm_movehl_ps(Det,Det));


    __m128 mtL1 = _mm_xor_ps(sum,pnpn);

    // Calculating the minterms of the second line (using previous results).
    tt = mm_ror_ps(_L1,1);		sum = _mm_mul_ps(tt,r1);
    tt = mm_ror_ps(tt,1);		sum = _mm_add_ps(sum,_mm_mul_ps(tt,r2));
    tt = mm_ror_ps(tt,1);		sum = _mm_add_ps(sum,_mm_mul_ps(tt,r3));
    __m128 mtL2 = _mm_xor_ps(sum,npnp);

    // Testing the determinant.
    Det = _mm_sub_ss(Det,_mm_shuffle_ps(Det,Det,1));

    // Calculating the minterms of the third line.
    tt = mm_ror_ps(_L1,1);
    Va = _mm_mul_ps(tt,Vb);									// V1' dot V2"
    Vb = _mm_mul_ps(tt,Vc);									// V1' dot V2^
    Vc = _mm_mul_ps(tt,_L2);								// V1' dot V2

    r1 = _mm_sub_ps(mm_ror_ps(Va,1), mm_ror_ps(Vc,2));		// V1" dot V2^ - V1^ dot V2"
    r2 = _mm_sub_ps(mm_ror_ps(Vb,2), mm_ror_ps(Vb,0));		// V1^ dot V2' - V1' dot V2^
    r3 = _mm_sub_ps(mm_ror_ps(Va,0), mm_ror_ps(Vc,1));		// V1' dot V2" - V1" dot V2'

    tt = mm_ror_ps(_L4,1);		sum = _mm_mul_ps(tt,r1);
    tt = mm_ror_ps(tt,1);		sum = _mm_add_ps(sum,_mm_mul_ps(tt,r2));
    tt = mm_ror_ps(tt,1);		sum = _mm_add_ps(sum,_mm_mul_ps(tt,r3));
    __m128 mtL3 = _mm_xor_ps(sum,pnpn);

    // Dividing is FASTER than rcp_nr! (Because rcp_nr causes many register-memory RWs).
    RDet = _mm_div_ss(zeroone, Det); // TODO: just 1.0f?
    RDet = _mm_shuffle_ps(RDet,RDet,0x00);

    // Devide the first 12 minterms with the determinant.
    mtL1 = _mm_mul_ps(mtL1, RDet);
    mtL2 = _mm_mul_ps(mtL2, RDet);
    mtL3 = _mm_mul_ps(mtL3, RDet);

    // Calculate the minterms of the forth line and devide by the determinant.
    tt = mm_ror_ps(_L3,1);		sum = _mm_mul_ps(tt,r1);
    tt = mm_ror_ps(tt,1);		sum = _mm_add_ps(sum,_mm_mul_ps(tt,r2));
    tt = mm_ror_ps(tt,1);		sum = _mm_add_ps(sum,_mm_mul_ps(tt,r3));
    __m128 mtL4 = _mm_xor_ps(sum,npnp);
    mtL4 = _mm_mul_ps(mtL4, RDet);

    // Now we just have to transpose the minterms matrix.
    trns0 = _mm_unpacklo_ps(mtL1,mtL2);
    trns1 = _mm_unpacklo_ps(mtL3,mtL4);
    trns2 = _mm_unpackhi_ps(mtL1,mtL2);
    trns3 = _mm_unpackhi_ps(mtL3,mtL4);
    _L1 = _mm_movelh_ps(trns0,trns1);
    _L2 = _mm_movehl_ps(trns1,trns0);
    _L3 = _mm_movelh_ps(trns2,trns3);
    _L4 = _mm_movehl_ps(trns3,trns2);

    return matrix44(float4(_L1), float4(_L2), float4(_L3), float4(_L4));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::lookatlh(const point& eye, const point& at, const vector& up)
{
#if NEBULA_DEBUG
    n_assert(up.length() > 0);
#endif
    // hmm the XM lookat functions are kinda pointless, because they
    // return a VIEW matrix, which is already inverse (so one would
    // need to reverse again!)
    const float4 zaxis = float4::normalize(at - eye);
    float4 normUp = float4::normalize(up);
    if (n_abs(float4::dot3(zaxis, normUp)) > 0.9999999f)
    {
        // need to choose a different up vector because up and lookat point
        // into same or opposite direction
        // just rotate y->x, x->z and z->y
        normUp = float4::permute(normUp, normUp, 1, 2, 0, 3);
    }
	const float4 xaxis = float4::normalize(float4::cross3(normUp, zaxis));
	const float4 yaxis = float4::normalize(float4::cross3(zaxis, xaxis));
    return matrix44(xaxis, yaxis, zaxis, eye);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::lookatrh(const point& eye, const point& at, const vector& up)
{
#if NEBULA_DEBUG
    n_assert(up.length() > 0);
#endif
    // hmm the XM lookat functions are kinda pointless, because they
    // return a VIEW matrix, which is already inverse (so one would
    // need to reverse again!)
    const float4 zaxis = float4::normalize(eye - at);
    float4 normUp = float4::normalize(up);
    if (n_abs(float4::dot3(zaxis, normUp)) > 0.9999999f)
    {
        // need to choose a different up vector because up and lookat point
        // into same or opposite direction
        // just rotate y->x, x->z and z->y
        normUp = float4::permute(normUp, normUp, 1, 2, 0, 3);
    }
	const float4 xaxis = float4::normalize(float4::cross3(normUp, zaxis));
	const float4 yaxis = float4::normalize(float4::cross3(zaxis, xaxis));
    return matrix44(xaxis, yaxis, zaxis, eye);
}

#ifdef N_USE_AVX
// dual linear combination using AVX instructions on YMM regs
static inline __m256 twolincomb_AVX_8(__m256 A01, const matrix44 &B)
{
	__m256 result;
	result = _mm256_mul_ps(_mm256_shuffle_ps(A01, A01, 0x00), _mm256_broadcast_ps(&B.mat.r[0].vec));
	result = _mm256_add_ps(result, _mm256_mul_ps(_mm256_shuffle_ps(A01, A01, 0x55), _mm256_broadcast_ps(&B.mat.r[1].vec)));
	result = _mm256_add_ps(result, _mm256_mul_ps(_mm256_shuffle_ps(A01, A01, 0xaa), _mm256_broadcast_ps(&B.mat.r[2].vec)));
	result = _mm256_add_ps(result, _mm256_mul_ps(_mm256_shuffle_ps(A01, A01, 0xff), _mm256_broadcast_ps(&B.mat.r[3].vec)));
	return result;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::multiply(const matrix44& m0, const matrix44& m1)
{
	matrix44 out;

	_mm256_zeroupper();
	__m256 A01 = _mm256_loadu_ps(&m0.mat.m[0][0]);
	__m256 A23 = _mm256_loadu_ps(&m0.mat.m[2][0]);

	__m256 out01x = twolincomb_AVX_8(A01, m1);
	__m256 out23x = twolincomb_AVX_8(A23, m1);

	_mm256_storeu_ps(&out.mat.m[0][0], out01x);
	_mm256_storeu_ps(&out.mat.m[2][0], out23x);
	return out;
}
#else
//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::multiply(const matrix44& m0, const matrix44& m1)
{
	matrix44 ret;

    float4 mw = m0.mat.r[0];

    // Splat the all components of the first row
    float4 mx = float4::splat_x(mw);
    float4 my = float4::splat_y(mw);
    float4 mz = float4::splat_z(mw);
    mw = float4::splat_w(mw);

    float4 m1x = m1.mat.r[0];
    float4 m1y = m1.mat.r[1];
    float4 m1z = m1.mat.r[2];
    float4 m1w = m1.mat.r[3];

    //multiply first row
    mx = float4::multiply(mx, m1x);
    my = float4::multiply(my, m1y);
    mz = float4::multiply(mz, m1z);
    mw = float4::multiply(mw, m1w);

    mx = mx + my;
    mz = mz + mw;
    ret.mat.r[0] = (mx + mz).vec;

    // rinse and repeat
    mw = m0.getrow1();

    mx = float4::splat_x(mw);
    my = float4::splat_y(mw);
    mz = float4::splat_z(mw);
    mw = float4::splat_w(mw);

 	mx = float4::multiply(mx, m1x);
 	my = float4::multiply(my, m1y);
 	mz = float4::multiply(mz, m1z);
 	mw = float4::multiply(mw, m1w);

    mx = mx + my;
    mz = mz + mw;
    ret.mat.r[1] = (mx + mz).vec;

    mw = m0.getrow2();

    mx = float4::splat_x(mw);
    my = float4::splat_y(mw);
    mz = float4::splat_z(mw);
    mw = float4::splat_w(mw);

    mx = float4::multiply(mx, m1.mat.r[0]);
    my = float4::multiply(my, m1.mat.r[1]);
    mz = float4::multiply(mz, m1.mat.r[2]);
    mw = float4::multiply(mw, m1.mat.r[3]);

    mx = mx + my;
    mz = mz + mw;
    ret.mat.r[2] = (mx + mz).vec;

    mw = m0.getrow3();

    mx = float4::splat_x(mw);
    my = float4::splat_y(mw);
    mz = float4::splat_z(mw);
    mw = float4::splat_w(mw);

	mx = float4::multiply(mx, m1x);
	my = float4::multiply(my, m1y);
	mz = float4::multiply(mz, m1z);
	mw = float4::multiply(mw, m1w);

    mx = mx + my;
    mz = mz + mw;
    ret.mat.r[3] = (mx + mz).vec;

    return ret;
}
#endif

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::ortholh(scalar w, scalar h, scalar zn, scalar zf)
{
	matrix44 m;
	scalar dist = 1.0f / (zf - zn);
#if __VULKAN__
	h *= -1;
	dist *= 0.5f;
#elif __OGL4__
	dist *= 0.5f;
#endif
	m.setrow0(float4(2.0f / w, 0.0f, 0.0f, 0.0f));
	m.setrow1(float4(0.0f, 2.0f / h, 0.0f, 0.0f));
	m.setrow2(float4(0.0f, 0.0f, dist, 0.0f));
	m.setrow3(float4(0.0f, 0.0, -dist * zn, 1.0f));
	return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::orthorh(scalar w, scalar h, scalar zn, scalar zf)
{
	matrix44 m;
	scalar dist = 1.0f / (zn - zf);
#if __VULKAN__
	h *= -1;
	dist *= 0.5f;
#elif __OGL4__
	dist *= 0.5f;
#endif
	m.setrow0(float4(2.0f / w, 0.0f, 0.0f, 0.0f));
	m.setrow1(float4(0.0f, 2.0f / h, 0.0f, 0.0f));
	m.setrow2(float4(0.0f, 0.0f, dist, 0.0f));
	m.setrow3(float4(0.0f, 0.0, dist * zn, 1.0f));
	return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::orthooffcenterlh(scalar l, scalar r, scalar b, scalar t, scalar zn, scalar zf)
{
	matrix44 m;
	scalar divwidth = 1.0f / (r - l);
	scalar divheight = 1.0f / (t - b);
	scalar dist = 1.0f / (zf - zn);
#if __VULKAN__
	divheight *= -1;
	dist *= 0.5f;
#elif __OGL4__
	dist *= 0.5f;
#endif
	m.setrow0(float4(2.0f * divwidth, 0.0f, 0.0f, 0.0f));
	m.setrow1(float4(0.0f, 2.0f * divheight, 0.0f, 0.0f));
	m.setrow2(float4(0.0f, 0.0f, dist, 0.0f));
	m.setrow3(float4(-(l+r) * divwidth, - (b+t) * divheight, -dist *  zn, 1.0f));
	return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::orthooffcenterrh(scalar l, scalar r, scalar b, scalar t, scalar zn, scalar zf)
{
	matrix44 m;
	scalar divwidth = 1.0f / (r - l);
	scalar divheight = 1.0f / (t - b);
	scalar dist = 1.0f / (zn - zf);
#if __VULKAN__
	divheight *= -1;
	dist *= 0.5f;
#elif __OGL4__
	dist *= 0.5f;
#endif
	m.setrow0(float4(2.0f * divwidth, 0.0f, 0.0f, 0.0f));
	m.setrow1(float4(0.0f, 2.0f * divheight, 0.0f, 0.0f));
	m.setrow2(float4(0.0f, 0.0f, dist, 0.0f));
	m.setrow3(float4(-(l+r) * divwidth, - (b+t) * divheight, dist *  zn, 1.0f));
	return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::perspfovlh(scalar fovy, scalar aspect, scalar zn, scalar zf)
{
    matrix44 m;
	scalar halfFov = 0.5f * fovy;
	scalar sinfov = n_sin(halfFov);
	scalar cosfov = n_cos(halfFov);

	scalar height = cosfov / sinfov;
	scalar width = height / aspect;

	scalar dist = zf / (zf - zn);
#if __VULKAN__
	height *= -1;
	dist *= 0.5f;
#elif __OGL4__
	dist *= 0.5f;
#endif
	m.setrow0(float4(width, 0.0f, 0.0f, 0.0f));
	m.setrow1(float4(0.0f, height, 0.0f, 0.0f));
	m.setrow2(float4(0.0f, 0.0f, dist, 1.0f));
	m.setrow3(float4(0.0f, 0.0f, -dist * zn, 0.0f));

	return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::perspfovrh(scalar fovy, scalar aspect, scalar zn, scalar zf)
{
	matrix44 m;
	scalar halfFov = 0.5f * fovy;
	scalar sinfov = n_sin(halfFov);
	scalar cosfov = n_cos(halfFov);

	scalar height = cosfov / sinfov;
	scalar width = height / aspect;

	scalar dist = zf / (zn - zf);
#if __VULKAN__
	height *= -1;
	dist *= 0.5f;
#elif __OGL4__
	dist *= 0.5f;
#endif
	m.setrow0(float4(width, 0.0f, 0.0f, 0.0f));
	m.setrow1(float4(0.0f, height, 0.0f, 0.0f));
	m.setrow2(float4(0.0f, 0.0f, dist, -1.0f));
	m.setrow3(float4(0.0f, 0.0f, dist * zn, 0.0f));

	return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::persplh(scalar w, scalar h, scalar zn, scalar zf)
{
	matrix44 m;
	scalar dist = zf / (zf - zn);	
#if __VULKAN__
	h *= -1;
	dist *= 0.5f;
#elif __OGL4__
	dist *= 0.5f;
#endif
	m.setrow0(float4(2.0f * zn  / w, 0.0f, 0.0f, 0.0f));
	m.setrow1(float4(0.0f, 2.0f * zn / h, 0.0f, 0.0f));
	m.setrow2(float4(0.0f, 0.0f, dist, 1.0f));
	m.setrow3(float4(0.0f, 0.0, -dist * zn, 0.0f));
	return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::persprh(scalar w, scalar h, scalar zn, scalar zf)
{
	matrix44 m;
	scalar dist = zf / (zn - zf);	
#if __VULKAN__
	h *= -1;
	dist *= 0.5f;
#elif __OGL4__
	dist *= 0.5f;
#endif
	m.setrow0(float4(2.0f * zn  / w, 0.0f, 0.0f, 0.0f));
	m.setrow1(float4(0.0f, 2.0f * zn / h, 0.0f, 0.0f));
	m.setrow2(float4(0.0f, 0.0f, dist, -1.0f));
	m.setrow3(float4(0.0f, 0.0, dist * zn, 0.0f));
	return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::perspoffcenterlh(scalar l, scalar r, scalar b, scalar t, scalar zn, scalar zf)
{
	matrix44 m;
	scalar divwidth = 1.0f / (r - l);
	scalar divheight = 1.0f / (t - b);
	scalar dist = zf / (zf - zn);
#if __VULKAN__
	divheight *= -1;
	dist *= 0.5f;
#elif __OGL4__
	dist *= 0.5f;
#endif
	m.setrow0(float4(2.0f * zn * divwidth, 0.0f, 0.0f, 0.0f));
	m.setrow1(float4(0.0f, 2.0f * zn * divheight, 0.0f, 0.0f));
	m.setrow2(float4(-(l+r) * divwidth, - (b+t) * divheight, dist, 1.0f));
	m.setrow3(float4(0.0f, 0.0f, -dist * zn, 0.0f));
	return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::perspoffcenterrh(scalar l, scalar r, scalar b, scalar t, scalar zn, scalar zf)
{
	matrix44 m;
	scalar divwidth = 1.0f / (r - l);
	scalar divheight = 1.0f / (t - b);
	scalar dist = zf / (zn - zf);
#if __VULKAN__
	divheight *= -1;
	dist *= 0.5f;
#elif __OGL4__
	dist *= 0.5f;
#endif
	m.setrow0(float4(2.0f * zn * divwidth, 0.0f, 0.0f, 0.0f));
	m.setrow1(float4(0.0f, 2.0f * zn * divheight, 0.0f, 0.0f));
	m.setrow2(float4((l+r) * divwidth, (b+t) * divheight, dist, -1.0f));
	m.setrow3(float4(0.0f, 0.0f, dist * zn, 0.0f));
	return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::rotationaxis(float4 const &axis, scalar angle)
{
    __m128 norm = float4::normalize3(axis).vec;

    scalar sangle = n_sin(angle);
    scalar cangle = n_cos(angle);

    __m128 m1_c = _mm_set_ps1(1.0f - cangle);
    __m128 c = _mm_set_ps1(cangle);
    __m128 s = _mm_set_ps1(sangle);

	__m128 nn1 = _mm_shuffle_ps(norm,norm,_MM_SHUFFLE(3,0,2,1));
	__m128 nn2 = _mm_shuffle_ps(norm,norm,_MM_SHUFFLE(3,1,0,2));

	__m128 v = _mm_mul_ps(nn1,m1_c);
	v = _mm_mul_ps(nn2,v);

	__m128 nn3 = _mm_mul_ps(norm, m1_c);
    nn3 = _mm_mul_ps(norm, nn3);
    nn3 = _mm_add_ps(nn3, c);

	__m128 nn4 = _mm_mul_ps(norm,s);
    nn4 = _mm_add_ps(nn4, v);
    __m128 nn5 = _mm_mul_ps(s, norm);
    nn5 = _mm_sub_ps(v,nn5);

	const mm128_ivec mask = {-1,-1,-1,0};
    v = _mm_and_ps(nn3,mask);

    __m128 v1 = _mm_shuffle_ps(nn4,nn5,_MM_SHUFFLE(2,1,2,0));
    v1 = _mm_shuffle_ps(v1,v1,_MM_SHUFFLE(0,3,2,1));

    __m128 v2 = _mm_shuffle_ps(nn4,nn5,_MM_SHUFFLE(0,0,1,1));
    v2 = _mm_shuffle_ps(v2,v2,_MM_SHUFFLE(2,0,2,0));


    nn5 = _mm_shuffle_ps(v,v1,_MM_SHUFFLE(1,0,3,0));
    nn5 = _mm_shuffle_ps(nn5,nn5,_MM_SHUFFLE(1,3,2,0));

	matrix44 m;	
	m.setrow0(nn5);
	
    nn5 = _mm_shuffle_ps(v,v1,_MM_SHUFFLE(3,2,3,1));
    nn5 = _mm_shuffle_ps(nn5,nn5,_MM_SHUFFLE(1,3,0,2));
    m.setrow1(nn5);

    v2 = _mm_shuffle_ps(v2,v,_MM_SHUFFLE(3,2,1,0));

    m.setrow2(v2);

    m.mat.r[3] = _id_w;
    return m;

    
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::rotationx(scalar angle)
{
	matrix44 m;

	scalar sangle = n_sin(angle);
	scalar cangle = n_cos(angle);

	m.mat.m[1][1] = cangle;
	m.mat.m[1][2] = sangle;

	m.mat.m[2][1] = -sangle;
	m.mat.m[2][2] = cangle;
	return m;	
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::rotationy(scalar angle)
{
	matrix44 m;

	scalar sangle = n_sin(angle);
	scalar cangle = n_cos(angle);

	m.mat.m[0][0] = cangle;
	m.mat.m[0][2] = -sangle;

	m.mat.m[2][0] = sangle;
	m.mat.m[2][2] = cangle;
	return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::rotationz(scalar angle)
{
	matrix44 m;

	scalar sangle = n_sin(angle);
	scalar cangle = n_cos(angle);

	m.mat.m[0][0] = cangle;
	m.mat.m[0][1] = sangle;

	m.mat.m[1][0] = -sangle;
	m.mat.m[1][1] = cangle;
	return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::rotationyawpitchroll(scalar yaw, scalar pitch, scalar roll)
{
	quaternion q = quaternion::rotationyawpitchroll(yaw,pitch,roll);
	return matrix44::rotationquaternion(q);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::scaling(scalar sx, scalar sy, scalar sz)
{
	matrix44 m;
	m.mat.r[0].vec = _mm_set_ps(0.0f, 0.0f, 0.0f, sx);
	m.mat.r[1].vec = _mm_set_ps(0.0f, 0.0f, sy, 0.0f);
	m.mat.r[2].vec = _mm_set_ps(0.0f, sz, 0.0f, 0.f);
	m.mat.r[3].vec = _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f);
	
	return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::scaling(float4 const &s)
{
	matrix44 m;
	m.mat.r[0].vec = _mm_and_ps(s.vec, maskX);
	m.mat.r[1].vec = _mm_and_ps(s.vec, maskY);
	m.mat.r[2].vec = _mm_and_ps(s.vec, maskZ);	

    return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::translation(scalar x, scalar y, scalar z)
{
	matrix44 m;
	m.mat.r[3].vec = _mm_set_ps(1.0f,z,y,x);
	return m;    
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::translation(float4 const &t)
{
	matrix44 m;
	m.mat.r[3].vec = t.vec;
	m.mat._44 = 1.0f;
    return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::transpose(const matrix44& m)
{
	__m128 xy1 = _mm_shuffle_ps(m.mat.r[0].vec,m.mat.r[1].vec,_MM_SHUFFLE(1,0,1,0));
	__m128 zw1 = _mm_shuffle_ps(m.mat.r[0].vec,m.mat.r[1].vec,_MM_SHUFFLE(3,2,3,2));
	__m128 xy2 = _mm_shuffle_ps(m.mat.r[2].vec,m.mat.r[3].vec,_MM_SHUFFLE(1,0,1,0));
	__m128 zw2 = _mm_shuffle_ps(m.mat.r[2].vec,m.mat.r[3].vec,_MM_SHUFFLE(3,2,3,2));

	matrix44 r;
	r.mat.r[0].vec = _mm_shuffle_ps(xy1,xy2,_MM_SHUFFLE(2,0,2,0));
	r.mat.r[1].vec = _mm_shuffle_ps(xy1,xy2,_MM_SHUFFLE(3,1,3,1));
	r.mat.r[2].vec = _mm_shuffle_ps(zw1,zw2,_MM_SHUFFLE(2,0,2,0));
	r.mat.r[3].vec = _mm_shuffle_ps(zw1,zw2,_MM_SHUFFLE(3,1,3,1));
    return r;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
float4
matrix44::transform(const float4 &v, const matrix44 &m)
{
	__m128 x = _mm_shuffle_ps(v.vec.vec, v.vec.vec, _MM_SHUFFLE(0, 0, 0, 0));
	__m128 y = _mm_shuffle_ps(v.vec.vec, v.vec.vec, _MM_SHUFFLE(1, 1, 1, 1));
	__m128 z = _mm_shuffle_ps(v.vec.vec, v.vec.vec, _MM_SHUFFLE(2, 2, 2, 2));
	__m128 w = _mm_shuffle_ps(v.vec.vec, v.vec.vec, _MM_SHUFFLE(3, 3, 3, 3));
	
	return _mm_add_ps(
		_mm_add_ps(_mm_mul_ps(x, m.mat.r[0].vec), _mm_mul_ps(y, m.mat.r[1].vec)), 
		_mm_add_ps(_mm_mul_ps(z, m.mat.r[2].vec), _mm_mul_ps(w, m.mat.r[3].vec))
		);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
quaternion
matrix44::rotationmatrix(const matrix44& m)
{
	return quaternion::rotationmatrix(m);	    
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
plane
matrix44::transform(const plane &p, const matrix44& m)
{
	return matrix44::transform(p.vec,m);    
}

} // namespace Math
//------------------------------------------------------------------------------

