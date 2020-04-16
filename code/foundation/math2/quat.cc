//------------------------------------------------------------------------------
//  xna_quaternion.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "math2/vec4.h"
#include "math2/mat4.h"
#include "system/byteorder.h"

namespace Math
{

//------------------------------------------------------------------------------
/**
*/
quat
rotationmatrix(const mat4& m)
{
	// FIXME write own implementation

	scalar trace = m._11 + m._22 + m._33;
	mat4 mm = transpose(m);
	scalar temp[4];

	vec4 r0 = mm.r[0];
	vec4 r1 = mm.r[1];
	vec4 r2 = mm.r[2];

	if (trace > 0.0f)
	{
		scalar s = n_sqrt(trace + 1.0f);
		temp[3] = (s * 0.5f);
		s = 0.5f / s;

		temp[0] = ((r2.y - r1.z) * s);
		temp[1] = ((r0.z - r2.x) * s);
		temp[2] = ((r1.x - r0.y) * s);
	}
	else
	{
		int i = r0.x < r1.y ?
			(r1.y < r2.z ? 2 : 1) :
			(r0.x < r2.z ? 2 : 0);
		int j = (i + 1) % 3;
		int k = (i + 2) % 3;

		scalar s = n_sqrt(mm.m[i][i] - mm.m[j][j] - mm.m[k][k] + 1.0f);
		temp[i] = s * 0.5f;
		s = 0.5f / s;

		temp[3] = (mm.m[k][j] - mm.m[j][k]) * s;
		temp[j] = (mm.m[j][i] + mm.m[i][j]) * s;
		temp[k] = (mm.m[k][i] + mm.m[i][k]) * s;
	}
	quat q(temp[0], temp[1], temp[2], temp[3]);
	return q;
}

//------------------------------------------------------------------------------
/**
*/
void
to_euler(const quat& q, float4& outangles)
{
	float q0 = q.x;
	float q1 = q.y;
	float q2 = q.z;
	float q3 = q.w;
	float x = atan2f(2.0f * (q0 * q1 + q2 * q3), 1.0f - 2.0f * (q1 * q1 + q2 * q2));
	float y = asinf(2.0f * (q0 * q2 - q3 * q1));
	float z = atanf((2.0f * (q0 * q3 + q1 * q2)) / (1.0f - 2.0f * (q2 * q2 + q3 * q3)));
	outangles.set(x, y, z, 0.0f);
}

} // namespace Math

