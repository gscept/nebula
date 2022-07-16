//------------------------------------------------------------------------------
//  mat4.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "math/sse.h"
#include "math/mat4.h"
#include "math/quat.h"

namespace Math
{

const mat4 mat4::identity = mat4(_id_x, _id_y, _id_z, _id_w);

//------------------------------------------------------------------------------
/**
    based on this http://www.opengl.org/discussion_boards/showthread.php/169605-reflection-matrix-how-to-derive
*/
mat4
reflect(const vec4& p)
{
    vec4 norm = normalize(p);

    const vec4 two(-2.0f,-2.0f,-2.0f,0.0f);

    // s = -2 * n
    vec4 s = norm * two;

    vec4 x = splat_x(norm);
    vec4 y = splat_y(norm);
    vec4 z = splat_z(norm);
    vec4 w = splat_w(norm);

    mat4 m;
    // identity - 2 * nxn
    m.r[0] = fmadd(x.vec,s.vec,_id_x);
    m.r[1] = fmadd(y.vec,s.vec,_id_y);
    m.r[2] = fmadd(z.vec,s.vec,_id_z);
    m.r[3] = fmadd(w.vec,s.vec,_id_w);

    return m;
}

//------------------------------------------------------------------------------
/**
*/
void
decompose(const mat4& mat, vec3& outScale, quat& outRotation, vec3& outTranslation)
{
    // Copy the matrix first - we'll use this to break down each component
    mat4 mCopy(mat);

    // Start by extracting the translation (and/or any projection) from the given matrix
    outTranslation = xyz(mCopy.position);
    mCopy.position = _id_w;
    
    // Extract the rotation component - this is done using polar decompostion, where
    // we successively average the matrix with its inverse transpose until there is
    // no/a very small difference between successive averages
    scalar norm;
    int count = 0;
    mat4 rotation = mCopy;
    do 
    {
        mat4 nextRotation;
        mat4 currInvTranspose = inverse(transpose(rotation));

        // Go through every component in the matrices and find the next matrix      
        nextRotation.r[0] = ((rotation.r[0] + currInvTranspose.r[0]) * vec4(0.5f));
        nextRotation.r[1] = ((rotation.r[1] + currInvTranspose.r[1]) * vec4(0.5f));
        nextRotation.r[2] = ((rotation.r[2] + currInvTranspose.r[2]) * vec4(0.5f));
        nextRotation.r[3] = ((rotation.r[3] + currInvTranspose.r[3]) * vec4(0.5f));


        norm = 0.0f; // dot3((rotation - rotation).abs(), _plus1).x
        norm = Math::max(norm, dot3(abs(rotation.r[0] - nextRotation.r[0]), vec4(_plus1)));
        norm = Math::max(norm, dot3(abs(rotation.r[1] - nextRotation.r[1]), vec4(_plus1)));
        norm = Math::max(norm, dot3(abs(rotation.r[2] - nextRotation.r[2]), vec4(_plus1)));
        
        rotation = nextRotation;
    } while (count < 100 && norm > 0.00001f);

    outRotation = rotationmatrix(rotation);

    // The scale is simply the removal of the rotation from the non-translated matrix
    mat4 scaleMatrix = inverse(rotation) * mCopy;
    vec4 tempScale;
    scaleMatrix.get_scale(tempScale);
    outScale = xyz(tempScale);

    // Calculate the normalized rotation matrix and take its determinant to determine whether
    // it had a negative scale or not...
    vec4 r0 = normalize(vec4(mCopy.r[0]));
    vec4 r1 = normalize(vec4(mCopy.r[1]));
    vec4 r2 = normalize(vec4(mCopy.r[2]));
    mat4 nr(r0,r1,r2,_id_w);
    
    // Special consideration: if there's a single negative scale 
    // (all other combinations of negative scales will
    // be part of the rotation matrix), the determinant of the 
    // normalized rotation matrix will be < 0. 
    // If this is the case we apply an arbitrary negative to one 
    // of the component of the scale.
    scalar det = determinant(nr);
    if (det < 0.0)
    {
        outScale.x = outScale.x * -1.0f;
    }
}

//------------------------------------------------------------------------------
/**
*/
mat4
affinetransformation(scalar scale, const vec3& rotationCenter, const quat& rotation, const vec3& translation)
{
    // M = MScaling * Inverse(MRotationOrigin) * MRotation * MRotationOrigin * MTranslation;
    vec3 scalev(scale);
    mat4 scalem = scaling(scalev);
    mat4 rot = rotationquat(rotation);
    vec4 rotc = vec4(rotationCenter, 0.0f);
    vec4 trans = vec4(translation, 0.0f);

    mat4 m = scalem;
    m.r[3] = _mm_sub_ps(m.r[3].vec, rotc.vec);
    m = rot * m;
    m.r[3] = _mm_add_ps(_mm_add_ps(m.r[3].vec, rotc.vec), trans.vec);
    return m;
}

//------------------------------------------------------------------------------
/**
    TODO: rewrite using SSE
*/
mat4
rotationquat(const quat& q)
{   
    mat4 ret = mat4::identity;
    float d = lengthsq(q);
    n_assert(d != 0.0f);
    float s = 2.0f / d;
    float xs = q.x * s, ys = q.y * s, zs = q.z * s;
    float wx = q.w * xs, wy = q.w * ys, wz = q.w * zs;
    float xx = q.x * xs, xy = q.x * ys, xz = q.x * zs;
    float yy = q.y * ys, yz = q.y * zs, zz = q.z * zs;

    ret.r[0] = vec4(1.0f - (yy + zz), xy + wz, xz - wy, 0.0f);
    ret.r[1] = vec4(xy - wz, 1.0f - (xx + zz), yz + wx, 0.0f);
    ret.r[2] = vec4(xz + wy, yz - wx, 1.0f - (xx + yy), 0.0f);
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
mat4
transformation(const vec3& scalingCenter, const quat& scalingRotation, const vec3& scale, const vec3& rotationCenter, const quat& rotation, const vec3& trans)
{
    vec3 nscalc = -scalingCenter;
    vec4 rotc = vec4(rotationCenter, 0.0f);
    vec4 translate = vec4(trans, 0.0f);

    mat4 mscaletrans = translation(nscalc);
    mat4 mscalerotate = rotationquat(scalingRotation);
    mat4 mscalerotateinv = transpose(mscalerotate);
    mat4 mrotate = rotationquat(rotation);
    mat4 mscale = scaling(scale);

    mat4 m = mscalerotateinv * mscaletrans;
    m = mscale * m;
    m = mscalerotate * m;
    m.r[3] = m.r[3] + vec4(scalingCenter, 0.0f) - rotc;
    m = mrotate * m;
    m.r[3] = m.r[3] + rotc + translate;
    return m;
}

//------------------------------------------------------------------------------
/**
*/
bool
ispointinside(const vec4& p, const mat4& m)
{
    vec4 p1 = m * p;
    // vectorized compare operation
    return !(
            less_any(vec4(p1.x, p1.w, p1.y, p1.w), vec4(-p1.w, p1.x, -p1.w, p1.y))
        ||  less_any(vec4(p1.z, p1.w, 0, 0), vec4(-p1.w, p1.z, 0, 0)));
}
} // namespace Math
