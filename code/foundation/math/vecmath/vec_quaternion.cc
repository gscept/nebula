//------------------------------------------------------------------------------
//  xna_quaternion.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "math/float4.h"
#include "math/matrix44.h"
#include "system/byteorder.h"

namespace Math
{

//------------------------------------------------------------------------------
/**
*/
quaternion
quaternion::rotationmatrix(const matrix44& m)
{
	// FIXME write own implementation

	scalar trace = m.mat._11 + m.mat._22 + m.mat._33;
	matrix44 mm = matrix44::transpose(m);
	scalar temp[4];

	if (trace > 0.0f)
	{
		scalar s = n_sqrt(trace + 1.0f);
		temp[3]=(s * 0.5f);
		s = 0.5f / s;

		temp[0]=((mm.getrow2().y() - mm.getrow1().z()) * s);
		temp[1]=((mm.getrow0().z() - mm.getrow2().x()) * s);
		temp[2]=((mm.getrow1().x() - mm.getrow0().y()) * s);
	} 
	else 
	{
		int i = mm.getrow0().x() < mm.getrow1().y() ? 
			(mm.getrow1().y() < mm.getrow2().z() ? 2 : 1) :
			(mm.getrow0().x() < mm.getrow2().z() ? 2 : 0); 
		int j = (i + 1) % 3;  
		int k = (i + 2) % 3;

		scalar s = n_sqrt(mm.mat.m[i][i] - mm.mat.m[j][j] - mm.mat.m[k][k] + 1.0f);
		temp[i] = s * 0.5f;
		s = 0.5f / s;

		temp[3] = (mm.mat.m[k][j] - mm.mat.m[j][k]) * s;
		temp[j] = (mm.mat.m[j][i] + mm.mat.m[i][j]) * s;
		temp[k] = (mm.mat.m[k][i] + mm.mat.m[i][k]) * s;
	}
	quaternion q(temp[0],temp[1],temp[2],temp[3]);
	return q;
}


//------------------------------------------------------------------------------
/**
*/
void
quaternion::to_euler(const quaternion& q, float4& outangles)
{
	float q0 = q.x();
	float q1 = q.y();
	float q2 = q.z();
	float q3 = q.w();
	float x = atan2f(2.0f * (q0 * q1 + q2*q3), 1.0f - 2.0f * (q1*q1 + q2*q2));
	float y = asinf(2.0f * (q0*q2 - q3*q1));
	float z = atanf((2.0f * (q0*q3 + q1*q2))/( 1.0f - 2.0f *(q2*q2 + q3*q3)));
	outangles.set(x, y, z, 0.0f);
}

}

