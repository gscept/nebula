//------------------------------------------------------------------------------
//  matrix44.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "math/matrix44.h"
#include "math/plane.h"
#include "math/quaternion.h"

namespace Math
{

//------------------------------------------------------------------------------
/**
	based on this http://www.opengl.org/discussion_boards/showthread.php/169605-reflection-matrix-how-to-derive
*/
matrix44
matrix44::reflect(const plane& p)
{
	float4 norm = plane::normalize(p).vec;

	const float4 two(-2.0f,-2.0f,-2.0f,0.0f);

	// s = -2 * n
	float4 s = float4::multiply(norm,two);

	float4 x = float4::splat_x(norm);
	float4 y = float4::splat_y(norm);
	float4 z = float4::splat_z(norm);
	float4 w = float4::splat_w(norm);

	matrix44 m;
	// identity - 2 * nxn
	m.setrow0(float4::multiplyadd(x,s,_id_x));
	m.setrow1(float4::multiplyadd(y,s,_id_y));
	m.setrow2(float4::multiplyadd(z,s,_id_z));
	m.setrow3(float4::multiplyadd(w,s,_id_w));

	return m;
}

//------------------------------------------------------------------------------
/**
*/
void
matrix44::decompose(float4& outScale, quaternion& outRotation, float4& outTranslation) const
{
	// Copy the matrix first - we'll use this to break down each component
	matrix44 mCopy(this->mat);

	// Start by extracting the translation (and/or any projection) from the given matrix
	outTranslation = mCopy.get_position();
	outTranslation.set_w(0.0f);
	mCopy.set_position(_id_w);
	
	// Extract the rotation component - this is done using polar decompostion, where
	// we successively average the matrix with its inverse transpose until there is
	// no/a very small difference between successive averages
	scalar norm;
	int count = 0;
	matrix44 rotation = mCopy;
	do {
		matrix44 nextRotation;
		matrix44 currInvTranspose = matrix44::inverse(matrix44::transpose(rotation));

		// Go through every component in the matrices and find the next matrix		
		nextRotation.setrow0((rotation.getrow0() + currInvTranspose.getrow0()) * 0.5f);
		nextRotation.setrow1((rotation.getrow1() + currInvTranspose.getrow1()) * 0.5f);
		nextRotation.setrow2((rotation.getrow2() + currInvTranspose.getrow2()) * 0.5f);
		nextRotation.setrow3((rotation.getrow3() + currInvTranspose.getrow3()) * 0.5f);

		norm = 0.0f;
		norm = n_max(norm,float4::dot3((rotation.getrow0() - nextRotation.getrow0()).abs(),_plus1));
		norm = n_max(norm,float4::dot3((rotation.getrow1() - nextRotation.getrow1()).abs(),_plus1));
		norm = n_max(norm,float4::dot3((rotation.getrow2() - nextRotation.getrow2()).abs(),_plus1));		
		
		rotation = nextRotation;
	} while (count < 100 && norm > 0.00001f);

	outRotation = matrix44::rotationmatrix(rotation);

	// The scale is simply the removal of the rotation from the non-translated matrix
	matrix44 scaleMatrix = matrix44::multiply(mCopy,matrix44::inverse(rotation));
	scaleMatrix.get_scale(outScale);
	outScale.set_w(0.0f);

	// Calculate the normalized rotation matrix and take its determinant to determine whether
	// it had a negative scale or not...
	float4 r0 = float4::normalize(mCopy.getrow0());
	float4 r1 = float4::normalize(mCopy.getrow1());
	float4 r2 = float4::normalize(mCopy.getrow2());
	matrix44 nr(r0,r1,r2,_id_w);
	
	// Special consideration: if there's a single negative scale 
	// (all other combinations of negative scales will
	// be part of the rotation matrix), the determinant of the 
	// normalized rotation matrix will be < 0. 
	// If this is the case we apply an arbitrary negative to one 
	// of the component of the scale.
	scalar determinant = nr.determinant();
	if (determinant < 0.0) 
	{
		outScale.set_x(outScale.x() * -1.0f);		
	}
}

//------------------------------------------------------------------------------
/**
*/
matrix44
matrix44::affinetransformation(scalar scaling, float4 const &rotationCenter, const quaternion& rotation, float4 const &translation)
{
	// M = MScaling * Inverse(MRotationOrigin) * MRotation * MRotationOrigin * MTranslation;
	float4 scalev;
	scalev.vec.vec = _mm_set_ps1(scaling);
	matrix44 scale = matrix44::scaling(scalev);
	matrix44 rot = matrix44::rotationquaternion(rotation);
	float4 rotc = rotationCenter;
	rotc.set_w(0.0f);
	float4 trans = translation;
	trans.set_w(0.0f);

	matrix44 m = scale;
	m.setrow3(m.getrow3() - rotc);
	m = matrix44::multiply(m,rot);
	m.setrow3(m.getrow3() + rotc + trans);
	return m;
}

//------------------------------------------------------------------------------
/**
    TODO: rewrite using SSE
*/
matrix44
matrix44::rotationquaternion(const quaternion& q)
{	
    matrix44 ret;
    float d = q.lengthsq();
    n_assert(d != 0.0f);
    float s = 2.0f / d;
    float xs = q.x() * s, ys = q.y() * s, zs = q.z() * s;
    float wx = q.w() * xs, wy = q.w() * ys, wz = q.w() * zs;
    float xx = q.x() * xs, xy = q.x() * ys, xz = q.x() * zs;
    float yy = q.y() * ys, yz = q.y() * zs, zz = q.z() * zs;       

    ret.row0().set(1.0f - (yy + zz), xy + wz, xz - wy, 0.0f);
    ret.row1().set(xy - wz, 1.0f - (xx + zz), yz + wx, 0.0f);
    ret.row2().set(xz + wy, yz - wx, 1.0f - (xx + yy), 0.0f);
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
matrix44
matrix44::transformation(float4 const &scalingCenter, const quaternion& scalingRotation, float4 const &scaling, float4 const &rotationCenter, const quaternion& rotation, float4 const &translation)
{
	float4 scalc = scalingCenter;
	scalc.set_w(0.0f);
	float4 nscalc = - scalc;
	float4 rotc = rotationCenter;
	rotc.set_w(0.0f);
	float4 trans = translation;
	trans.set_w(0.0f);

	matrix44 mscaletrans = matrix44::translation(nscalc);
	matrix44 mscalerotate = matrix44::rotationquaternion(scalingRotation);
	matrix44 mscalerotateinv = matrix44::transpose(mscalerotate);
	matrix44 mrotate = matrix44::rotationquaternion(rotation);
	matrix44 mscale = matrix44::scaling(scaling);

	matrix44 m = matrix44::multiply(mscaletrans, mscalerotateinv);
	m = matrix44::multiply(m, mscale);
	m = matrix44::multiply(m, mscalerotate);
	m.setrow3(m.getrow3() + scalc - rotc);
	m = matrix44::multiply(m, mrotate);
	m.setrow3(m.getrow3() + rotc + trans);
	return m;
}

//------------------------------------------------------------------------------
/**
*/
bool
matrix44::ispointinside(const float4& p, const matrix44& m)
{
    float4 p1 = matrix44::transform(p, m);
    // vectorized compare operation
    return !(float4::less4_any(float4(p1.x(), p1.w(), p1.y(), p1.w()),
             float4(-p1.w(), p1.x(), -p1.w(), p1.y()))
            ||
            float4::less4_any(float4(p1.z(), p1.w(), 0, 0),
            float4(-p1.w(), p1.z(), 0, 0)));
}
} // namespace Math
