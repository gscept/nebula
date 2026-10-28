//------------------------------------------------------------------------------
//  quat.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "math/vec4.h"
#include "math/mat4.h"
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
        scalar s = Math::sqrt(trace + 1.0f);
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

        scalar s = Math::sqrt(mm.m[i][i] - mm.m[j][j] - mm.m[k][k] + 1.0f);
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
quat
quatpitchyawroll(scalar x, scalar y, scalar z)
{
    float cx = cos(x * 0.5f);
    float sx = sin(x * 0.5f);
    float cy = cos(y * 0.5f);
    float sy = sin(y * 0.5f);
    float cz = cos(z * 0.5f);
    float sz = sin(z * 0.5f);
    
    return quat(
        sx * cy * cz - cx * sy * sz
        , cx * sy * cz + sx * cy * sz
        , cx * cy * sz - sx * sy * cz
        , cx * cy * cz + sx * sy * sz
    );
}

//------------------------------------------------------------------------------
/**
    The angles around the X, Y and Z axes that the quaternion represents. The inverse of from_euler().
*/
vec3
to_euler(const quat& q)
{
    float w = q.w;
    float x = q.x;
    float y = q.y;
    float z = q.z;

    // asinf() only answers for an argument inside [-1, 1], and a quaternion can round just outside of it
    float yArg = 2.0f * (w * y - z * x);
    if (yArg > 1.0f)
        yArg = 1.0f;
    else if (yArg < -1.0f)
        yArg = -1.0f;

    float xAngle = atan2f(2.0f * (w * x + y * z), 1.0f - 2.0f * (x * x + y * y));
    float yAngle = asinf(yArg);
    float zAngle = atan2f(2.0f * (w * z + x * y), 1.0f - 2.0f * (y * y + z * z));

    return vec3(xAngle, yAngle, zAngle);
}

//------------------------------------------------------------------------------
/**
    The quaternion for the angles around the X, Y and Z axes. The inverse of to_euler().
*/
quat
from_euler(const vec3& v)
{
    return quatpitchyawroll(v.x, v.y, v.z);
}

} // namespace Math

