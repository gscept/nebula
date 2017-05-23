#pragma once
#ifndef MATH_SCE_MATRIX44_H
#define MATH_SCE_MATRIX44_H
//------------------------------------------------------------------------------
/**
    @class Math::matrix44
    
    A matrix44 class on top of SCE math functions.
    
    (C) 2007 Radon Labs GmbH
*/
#include "core/types.h"
#include "math/sce/sce_scalar.h"
#include "math/sce/sce_float4.h"

//------------------------------------------------------------------------------
namespace Math
{
class quaternion;
class plane;

class matrix44
{
public:
    /// default constructor, NOTE: does NOT setup components!
    matrix44();
    /// construct from components
    matrix44(const float4& row0, const float4& row1, const float4& row2, const float4& row3);
    /// copy constructor
    matrix44(const matrix44& rhs);
    
    /// assignment operator
    void operator=(const matrix44& rhs);
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
    void set(const float4& row0, const float4& row1, const float4& row2, const float4& row3);
    /// write access to x component
    void setrow0(const float4& row0);
    /// write access to y component
    void setrow1(const float4& row1);
    /// write access to z component
    void setrow2(const float4& row2);
    /// write access to w component
    void setrow3(const float4& row3);
    /// read-only access to x component
    float4 getrow0() const;
    /// read-only access to y component
    float4 getrow1() const;
    /// read-only access to z component
    float4 getrow2() const;
    /// read-only access to w component
    float4 getrow3() const;

    /// write access to x component
    void set_xaxis(const float4& x);
    /// write access to y component
    void set_yaxis(const float4& y);
    /// write access to z component
    void set_zaxis(const float4& z);
    /// write access to w component / pos component
    void set_position(const float4& pos);
    /// read access to x component
    float4 get_xaxis() const;
    /// read access to y component
    float4 get_yaxis() const;
    /// read access to z component
    float4 get_zaxis() const;
    /// read access to w component / pos component
    float4 get_position() const;
    /// add a translation to pos_component
    void translate(const float4& t);
    /// scale matrix
    void scale(const float4& v);

    /// return true if matrix is identity
    bool isidentity() const;
    /// return determinant of matrix
    scalar determinant() const;
    /// decompose into scale, rotation and translation
    void decompose(float4& outScale, quaternion& outRotation, float4& outTranslation) const;

    /// build identity matrix
    static matrix44 identity();
    /// build matrix from affine transformation
    static matrix44 affinetransformation(scalar scaling, const float4& rotationCenter, const quaternion& rotation, const float4& translation);
    /// compute the inverse of a matrix
    static matrix44 inverse(const matrix44& m);
    /// build left handed lookat matrix
    static matrix44 lookatlh(const float4& eye, const float4& at, const float4& up);
    /// build right handed lookat matrix
    static matrix44 lookatrh(const float4& eye, const float4& at, const float4& up);
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
    static matrix44 rotationaxis(const float4& axis, scalar angle);
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
    static matrix44 scaling(const float4& s);
    /// build a transformation matrix
    static matrix44 transformation(const float4& scalingCenter, const quaternion& scalingRotation, const float4& scaling, const float4& rotationCenter, const quaternion& rotation, const float4& translation);
    /// build a translation matrix from scalars
    static matrix44 translation(scalar x, scalar y, scalar z);
    /// build a translation matrix from point
    static matrix44 translation(const float4& t);
    /// return the transpose of a matrix
    static matrix44 transpose(const matrix44& m);
    /// transform 4d vector by matrix44
    static float4 transform(const float4& v, const matrix44& m);
    /// transform plane with matrix
    static plane transform(const plane& p, const matrix44& m);

private:
    float4 r0;
    float4 r1;
    float4 r2;
    float4 r3;
};

//------------------------------------------------------------------------------
/**
*/
__forceinline
matrix44::matrix44()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
matrix44::matrix44(const float4& row0, const float4& row1, const float4& row2, const float4& row3) :
    r0(row0),
    r1(row1),
    r2(row2),
    r3(row3)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
matrix44::matrix44(const matrix44& rhs) :
    r0(rhs.r0),
    r1(rhs.r1),
    r2(rhs.r2),
    r3(rhs.r3)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::operator=(const matrix44& rhs)
{
    this->r0 = rhs.r0;
    this->r1 = rhs.r1;
    this->r2 = rhs.r2;
    this->r3 = rhs.r3;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
matrix44::operator==(const matrix44& rhs) const
{
    return (this->r0 == rhs.r0) && (this->r1 == rhs.r1) && (this->r2 == rhs.r2) && (this->r3 == rhs.r3);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
matrix44::operator!=(const matrix44& rhs) const
{
    return (this->r0 != rhs.r0) || (this->r1 != rhs.r1) || (this->r2 != rhs.r2) || (this->r3 != rhs.r3);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::load(const scalar* ptr)
{
    this->r0.load(ptr);
    this->r1.load(ptr + 4);
    this->r2.load(ptr + 8);
    this->r3.load(ptr + 12);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::loadu(const scalar* ptr)
{
    this->r0.loadu(ptr);
    this->r1.loadu(ptr + 4);
    this->r2.loadu(ptr + 8);
    this->r3.loadu(ptr + 12);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::store(scalar* ptr) const
{
    this->r0.store(ptr);
    this->r1.store(ptr + 4);
    this->r2.store(ptr + 8);
    this->r3.store(ptr + 12);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::storeu(scalar* ptr) const
{
    this->r0.storeu(ptr);
    this->r1.storeu(ptr + 4);
    this->r2.storeu(ptr + 8);
    this->r3.storeu(ptr + 12);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::stream(scalar* ptr) const
{
    this->r0.stream(ptr);
    this->r1.stream(ptr + 4);
    this->r2.stream(ptr + 8);
    this->r3.stream(ptr + 12);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::set(const float4& row0, const float4& row1, const float4& row2, const float4& row3)
{
    this->r0 = row0;
    this->r1 = row1;
    this->r2 = row2;
    this->r3 = row3;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::setrow0(const float4& row0)
{
    this->r0 = row0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
matrix44::getrow0() const
{
    return this->r0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::setrow1(const float4& row1)
{
    this->r1 = row1;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
matrix44::getrow1() const
{
    return this->r1;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::setrow2(const float4& row2)
{
    this->r2 = row2;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
matrix44::getrow2() const
{
    return this->r2;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::setrow3(const float4& row3)
{
    this->r3 = row3;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
matrix44::getrow3() const
{
    return this->r3;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::set_xaxis(const float4& x)
{
    this->r0 = x;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::set_yaxis(const float4& y)
{
    this->r1 = y;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::set_zaxis(const float4& z)
{
    this->r2 = z;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::set_position(const float4& pos)
{
    this->r3 = pos;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
matrix44::get_xaxis() const
{
    return this->r0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
matrix44::get_yaxis() const
{
    return this->r1;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
matrix44::get_zaxis() const
{
    return this->r2;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
matrix44::get_position() const
{
    return this->r3;
}
//------------------------------------------------------------------------------
/**
*/
__forceinline
void 
matrix44::translate(const float4& t)
{
    n_assert2(t.w() == 0, "w component not 0, use vector for translation not a point!");
    this->r3 += t;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
matrix44::scale(const float4& s) 
{    
    n_assert2(s.w() == 1, "w component should be 1 for matrix44::scale()!");

    this->r0 = float4::multiply(this->r0, s);
    this->r1 = float4::multiply(this->r1, s);
    this->r2 = float4::multiply(this->r2, s);
    this->r3 = float4::multiply(this->r3, s);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
matrix44::isidentity() const
{
    // return (TRUE == D3DXMatrixIsIdentity((CONST D3DXMATRIX*)this));
    return false;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
matrix44::determinant() const
{
    // return D3DXMatrixDeterminant((CONST D3DXMATRIX*)this);
    return 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::identity()
{
    matrix44 res;
    // D3DXMatrixIdentity((D3DXMATRIX*)&res);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::inverse(const matrix44& m)
{
    matrix44 res;
    // D3DXMatrixInverse((D3DXMATRIX*)&res, NULL, (CONST D3DXMATRIX*)&m);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::lookatlh(const float4& eye, const float4& at, const float4& up)
{
    // hmm the D3DX lookat functions are kinda pointless, because they
    // return a VIEW matrix, which is already inverse (so one would
    // need to reverse again!)
    float4 zaxis = float4::normalize(at - eye);
    float4 xaxis = float4::normalize(float4::cross3(up, zaxis));
    float4 yaxis = float4::cross3(zaxis, xaxis);
    return matrix44(xaxis, yaxis, zaxis, eye);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::lookatrh(const float4& eye, const float4& at, const float4& up)
{
    // hmm the D3DX lookat functions are kinda pointless, because they
    // return a VIEW matrix, which is already inverse (so one would
    // need to reverse again!)
    float4 zaxis = float4::normalize(eye - at);
    float4 xaxis = float4::normalize(float4::cross3(up, zaxis));
    float4 yaxis = float4::cross3(zaxis, xaxis);
    return matrix44(xaxis, yaxis, zaxis, eye);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::multiply(const matrix44& m0, const matrix44& m1)
{
    matrix44 res;
    // D3DXMatrixMultiply((D3DXMATRIX*)&res, (CONST D3DXMATRIX*)&m0, (CONST D3DXMATRIX*)&m1);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::ortholh(scalar w, scalar h, scalar zn, scalar zf)
{
    matrix44 res;
    // D3DXMatrixOrthoLH((D3DXMATRIX*)&res, w, h, zn, zf);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::orthorh(scalar w, scalar h, scalar zn, scalar zf)
{
    matrix44 res;
    // D3DXMatrixOrthoRH((D3DXMATRIX*)&res, w, h, zn, zf);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::orthooffcenterlh(scalar l, scalar r, scalar b, scalar t, scalar zn, scalar zf)
{
    matrix44 res;
    // D3DXMatrixOrthoOffCenterLH((D3DXMATRIX*)&res, l, r, b, t, zn, zf);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::orthooffcenterrh(scalar l, scalar r, scalar b, scalar t, scalar zn, scalar zf)
{
    matrix44 res;
    // D3DXMatrixOrthoOffCenterRH((D3DXMATRIX*)&res, l, r, b, t, zn, zf);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::perspfovlh(scalar fovy, scalar aspect, scalar zn, scalar zf)
{
    matrix44 res;
    // D3DXMatrixPerspectiveFovLH((D3DXMATRIX*)&res, fovy, aspect, zn, zf);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::perspfovrh(scalar fovy, scalar aspect, scalar zn, scalar zf)
{
    matrix44 res;
    // D3DXMatrixPerspectiveFovRH((D3DXMATRIX*)&res, fovy, aspect, zn, zf);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::persplh(scalar w, scalar h, scalar zn, scalar zf)
{
    matrix44 res;
    // D3DXMatrixPerspectiveLH((D3DXMATRIX*)&res, w, h, zn, zf);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::persprh(scalar w, scalar h, scalar zn, scalar zf)
{
    matrix44 res;
    // D3DXMatrixPerspectiveRH((D3DXMATRIX*)&res, w, h, zn, zf);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::perspoffcenterlh(scalar l, scalar r, scalar b, scalar t, scalar zn, scalar zf)
{
    matrix44 res;
    // D3DXMatrixPerspectiveOffCenterLH((D3DXMATRIX*)&res, l, r, b, t, zn, zf);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::perspoffcenterrh(scalar l, scalar r, scalar b, scalar t, scalar zn, scalar zf)
{
    matrix44 res;
    // D3DXMatrixPerspectiveOffCenterRH((D3DXMATRIX*)&res, l, r, b, t, zn, zf);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::rotationaxis(const float4& axis, scalar angle)
{
    matrix44 res;
    // D3DXMatrixRotationAxis((D3DXMATRIX*)&res, (CONST D3DXVECTOR3*)&axis, angle);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::rotationx(scalar angle)
{
    matrix44 res;
    // D3DXMatrixRotationX((D3DXMATRIX*)&res, angle);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::rotationy(scalar angle)
{
    matrix44 res;
    // D3DXMatrixRotationY((D3DXMATRIX*)&res, angle);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::rotationz(scalar angle)
{
    matrix44 res;
    // D3DXMatrixRotationZ((D3DXMATRIX*)&res, angle);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::rotationyawpitchroll(scalar yaw, scalar pitch, scalar roll)
{
    matrix44 res;
    // D3DXMatrixRotationYawPitchRoll((D3DXMATRIX*)&res, yaw, pitch, roll);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::scaling(scalar sx, scalar sy, scalar sz)
{
    matrix44 res;
    // D3DXMatrixScaling((D3DXMATRIX*)&res, sx, sy, sz);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::scaling(const float4& s)
{
    matrix44 res;
    // D3DXMatrixScaling((D3DXMATRIX*)&res, s.X, s.Y, s.Z);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::translation(scalar x, scalar y, scalar z)
{
    matrix44 res;
    // D3DXMatrixTranslation((D3DXMATRIX*)&res, x, y, z);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::translation(const float4& t)
{
    matrix44 res;
    // D3DXMatrixTranslation((D3DXMATRIX*)&res, t.X, t.Y, t.Z);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline matrix44
matrix44::transpose(const matrix44& m)
{
    matrix44 res;
    // D3DXMatrixTranspose((D3DXMATRIX*)&res, (CONST D3DXMATRIX*)&m);
    return res;
}




} // namespace Math
//------------------------------------------------------------------------------
#endif
