//------------------------------------------------------------------------------
//  mat4test.cc
//  (C) 2009 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"

#include "mat4test.h"
#include "mathtestutil.h"
#include "stackalignment.h"
#include "testbase/stackdebug.h"

#include "math/vec3.h"
#include "math/vec4.h"
#include "math/mat4.h"
#include "timing/timer.h"

using namespace Math;

namespace Test
{
__ImplementClass(Test::Mat4Test, 'EM4T', Test::TestCase);

/*
volatile unsigned int *stack_before;
volatile unsigned int *stack_in;

__attribute((noinline)) 
void 
print_m(const mat4& mat)
{
    print(vec4(mat.getrow0().x(), mat.getrow1().x(), mat.getrow2().x(), mat.getrow3().x()));
    print(vec4(mat.getrow0().y(), mat.getrow1().y(), mat.getrow2().y(), mat.getrow3().y()));
    print(vec4(mat.getrow0().z(), mat.getrow1().z(), mat.getrow2().z(), mat.getrow3().z()));
    print(vec4(mat.getrow0().w(), mat.getrow1().w(), mat.getrow2().w(), mat.getrow3().w()));
}


__attribute((noinline))
void TestMatrixStackSize()
{
    mat4 testi[2];
    stack_in = g_stack_ptr;
    //print_m(testi);
    //print_m(testi[1]);
}
*/

static const scalar E = 0.00001;
static const vec4 E4(E, E, E, E);
static const vec3 E3(E, E, E);

bool matnearequal(mat4 lhs, mat4 rhs)
{
    return nearequal(lhs.r[0], rhs.r[0], E) && nearequal(lhs.r[1], rhs.r[1], E) && nearequal(lhs.r[2], rhs.r[2], E) && nearequal(lhs.r[3], rhs.r[3], E);
}

//------------------------------------------------------------------------------
/**
*/
void
Mat4Test::Run()
{
    STACK_CHECKPOINT("Test::Mat4Test::Run() begin");

    const vec4 pOneTwoThree(1.0, 2.0, 3.0, 1.0);
    const vec3 vOneTwoThree(1.0, 2.0, 3.0);
    const mat4 trans123(vec4(1.0f, 0.0f, 0.0f, 0.0f),
                        vec4(0.0f, 1.0f, 0.0f, 0.0f),
                        vec4(0.0f, 0.0f, 1.0f, 0.0f),
                        pOneTwoThree);
    const mat4 identity = mat4::identity;
    const mat4 rotOneX = rotationx(1.0f);
    const vec4 pZero(0.0, 0.0, 0.0, 1.0);
    const vec4 pOneX(1.0, 0.0, 0.0, 1.0);
    const vec4 pOneY(0.0, 1.0, 0.0, 1.0);
    vec4 result;

    // identity and construction
    mat4 m0(identity);
    VERIFY(m0 == identity);
    
    // multiply by identity
    m0 = identity * trans123;
    VERIFY(m0 == trans123);

    // point transform by matrix
    result = trans123 * pZero;
    VERIFY((result == pOneTwoThree));

    // multiplication and multiplication order, transform point by matrix
    const mat4 mRotOneX_Trans123 = trans123 * rotOneX;
    VERIFY(matnearequal(mRotOneX_Trans123,
                               mat4(vec4(1.0f,       0.0f,      0.0f, 0.0f),
                                    vec4(0.0f,  0.540302f, 0.841471f, 0.0f),
                                    vec4(0.0f, -0.841471f, 0.540302f, 0.0f),
                                    vec4(1.0f,       2.0f,      3.0f, 1.0f))));
    result = mRotOneX_Trans123 * pZero;
    VERIFY(nearequal(result, pOneTwoThree, E4));
    result = mRotOneX_Trans123 * pOneX;
    VERIFY(nearequal(result, vec4(2.0f, 2.0f, 3.0f, 1.0f), E4));
    result = mRotOneX_Trans123 * pOneY;
    VERIFY(nearequal(result, vec4(1.0f, 2.540302f, 3.841471f, 1.0f), E4));

    // translate
    m0 = identity;
    m0.translate(vOneTwoThree);
    VERIFY((m0 == trans123));
    m0 = mRotOneX_Trans123;
    m0 = translation(-0.1f, 4.5f, 2.1f);
    VERIFY(matnearequal(m0, mat4(vec4( 1.0f, 0.0f, 0.0f, 0.0f),
                                 vec4( 0.0f, 1.0f, 0.0f, 0.0f),
                                 vec4( 0.0f, 0.0f, 1.0f, 0.0f),
                                 vec4(-0.1f, 4.5f, 2.1f, 1.0f))));
    m0 = mRotOneX_Trans123;
    m0 = translation(vec3(-0.1f, 4.5f, 2.1f));
    VERIFY(matnearequal(m0, mat4(vec4( 1.0f, 0.0f, 0.0f, 0.0f),
                                 vec4( 0.0f, 1.0f, 0.0f, 0.0f),
                                 vec4( 0.0f, 0.0f, 1.0f, 0.0f),
                                 vec4(-0.1f, 4.5f, 2.1f, 1.0f))));

    // scale
    m0 = mRotOneX_Trans123;
    m0.scale(vec3(0.5f, 1.5f, -3.0f));
    VERIFY(matnearequal(m0, mat4(vec4(0.5f,       0.0f,       0.0f, 0.0f),
                                 vec4(0.0f,  0.810453f, -2.524413f, 0.0f),
                                 vec4(0.0f, -1.262206f, -1.620907f, 0.0f),
                                 vec4(0.5f,  3.000000f, -9.000000f, 1.0f))));
    m0 = scaling(0.1f, -2.0f, 13.0f);
    VERIFY(matnearequal(m0, mat4(vec4(0.1f,  0.0f,  0.0f, 0.0f),
                                 vec4(0.0f, -2.0f,  0.0f, 0.0f),
                                 vec4(0.0f,  0.0f, 13.0f, 0.0f),
                                 vec4(0.0f,  0.0f,  0.0f, 1.0f))));
    m0 = scaling(vec3(0.1f, -2.0f, 13.0f));
    VERIFY(matnearequal(m0, mat4(vec4(0.1f,  0.0f,  0.0f, 0.0f),
                                 vec4(0.0f, -2.0f,  0.0f, 0.0f),
                                 vec4(0.0f,  0.0f, 13.0f, 0.0f),
                                 vec4(0.0f,  0.0f,  0.0f, 1.0f))));

    // inverse
    m0 = inverse(mRotOneX_Trans123);
    VERIFY(matnearequal(m0, mat4(vec4( 1.0f,       0.0f,       0.0f, 0.0f),
                                 vec4( 0.0f,  0.540302f, -0.841471f, 0.0f),
                                 vec4( 0.0f,  0.841471f,  0.540302f, 0.0f),
                                 vec4(-1.0f, -3.605018f,  0.062035f, 1.0f))));
    // transpose
    m0 = transpose(mRotOneX_Trans123);
    VERIFY(matnearequal(m0, mat4(vec4( 1.0f,       0.0f,       0.0f, 1.0f),
                                 vec4( 0.0f,  0.540302f, -0.841471f, 2.0f),
                                 vec4( 0.0f,  0.841471f,  0.540302f, 3.0f),
                                 vec4( 0.0f,       0.0f,       0.0f, 1.0f))));
    // rotations
    const mat4 rotX = rotationx(2.0);
    VERIFY(matnearequal(rotX, mat4(vec4(1.000000f,  0.000000f,  0.000000f, 0.000000f),
                                   vec4(0.000000f, -0.416147f,  0.909297f, 0.000000f),
                                   vec4(0.000000f, -0.909297f, -0.416147f, 0.000000f),
                                   vec4(0.000000f,  0.000000f,  0.000000f, 1.000000f))));
    const mat4 rotY = rotationy(-1.7);
    VERIFY(matnearequal(rotY, mat4(vec4(-0.128845f, 0.000000f,  0.991665f, 0.000000f),
                                   vec4( 0.000000f, 1.000000f,  0.000000f, 0.000000f),
                                   vec4(-0.991665f, 0.000000f, -0.128845f, 0.000000f),
                                   vec4( 0.000000f, 0.000000f,  0.000000f, 1.000000f))));
    const mat4 rotZ = rotationz(3.1);
    VERIFY(matnearequal(rotZ, mat4(vec4(-0.999135f,  0.041581f, 0.000000f, 0.000000f),
                                   vec4(-0.041581f, -0.999135f, 0.000000f, 0.000000f),
                                   vec4( 0.000000f,  0.000000f, 1.000000f, 0.000000f),
                                   vec4( 0.000000f,  0.000000f, 0.000000f, 1.000000f))));
    const vec3 rotaxis = normalize(vec3(1.0f, 0.2f, 2.0f));
    const mat4 rot = rotationaxis(rotaxis, -2.53652f);
    VERIFY(matnearequal(rot, mat4(vec4(-0.460861f, -0.434427f,  0.773873f, 0.000000f),
                                  vec4( 0.579067f, -0.807997f, -0.108734f, 0.000000f),
                                  vec4( 0.672524f,  0.398013f,  0.623936f, 0.000000f),
                                  vec4( 0.000000f,  0.000000f,  0.000000f, 1.000000f))));
    // reflect
    const vec4 planeXZ(0.0, 1.0, 0.0, 0.0);
    const mat4 mReflectXZ = reflect(planeXZ);
    VERIFY(matnearequal(mReflectXZ,
                               mat4(vec4(1.0f,  0.0f, 0.0f, 0.0f ),
                                    vec4(0.0f, -1.0f, 0.0f, 0.0f ),
                                    vec4(0.0f,  0.0f, 1.0f, 0.0f ),
                                    vec4(0.0f,  0.0f, 0.0f, 1.0f ))));
    const vec4 pReflected = mReflectXZ * pOneTwoThree;
    VERIFY(nearequal(pReflected, vec4(1.0f, -2.0f, 3.0f, 1.0f), E4));

    // test 16-byte alignment of embedded members on the stack, if we use SSE/SSE2 on windows or
    // xbox or ps3

    // component-wise access
    mat4 m1;
    m0 = mat4(vec4(2.0f, 0.0f, 0.0f, 0.0f),
              vec4(0.0f, 2.0f, 0.0f, 0.0f),
              vec4(0.0f, 0.0f, 2.0f, 0.0f),
              vec4(0.0f, 0.0f, 0.0f, 1.0f));
    vec4 value(2.0f, 0.0f, 0.0f, 0.0f);
    m1.r[0] = value;
    value = vec4(0.0f, 2.0f, 0.0f, 0.0f);
    m1.r[1] = value;
    value = vec4(0.0f, 0.0f, 2.0f, 0.0f);
    m1.r[2] = value;
    value = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    m1.r[3] = value;
    VERIFY(m0 == m1);
    VERIFY(m0.r[0] == vec4(2.0f, 0.0f, 0.0f, 0.0f));
    VERIFY(m0.r[1] == vec4(0.0f, 2.0f, 0.0f, 0.0f));
    VERIFY(m0.r[2] == vec4(0.0f, 0.0f, 2.0f, 0.0f));
    VERIFY(m0.r[3] == vec4(0.0f, 0.0f, 0.0f, 1.0f));

    // determinant
    const scalar det = determinant(mRotOneX_Trans123);
    VERIFY(scalarequal(1.0, det));

    // decompose
    vec3 outScale;
    quat outRotation;
    vec3 outTranslation;
    const mat4 mScale_RotOneX_Trans123 = mRotOneX_Trans123 * scaling(0.5f, 2.0f, 3.0f);
    decompose(mScale_RotOneX_Trans123, outScale, outRotation, outTranslation);
    VERIFY(nearequal(outScale, vec3(0.5f, 2.0f, 3.0f), E3));
    VERIFY(nearequal(outRotation.vec, vec4(0.479426f, 0.0f, 0.0f, 0.877583f), E4));
    VERIFY(nearequal(outTranslation, vec3(1.0f, 2.0f, 3.0f), E3));

    // rotationquaternion
    mat4 m = rotationquat(outRotation);
    VERIFY(matnearequal(m, mat4(vec4(1.0f,       0.0f,      0.0f, 0.0f),
                                vec4(0.0f,  0.540302f, 0.841471f, 0.0f),
                                vec4(0.0f, -0.841471f, 0.540302f, 0.0f),
                                vec4(0.0f,       0.0f,      0.0f, 1.0f))));

    // affinetransformation
    const mat4 affine = affinetransformation(0.1f,
                                             vec3(0.5f, 3.0f, -1.7f), 
                                             outRotation, 
                                             vec3(-20.0f, 17.0f, 9.0f));

    VERIFY(matnearequal(affine, mat4(vec4(  0.1f,       0.0f,      0.0f, 0.0f),
                                     vec4(  0.0f,  0.054030f, 0.084147f, 0.0f),
                                     vec4(  0.0f, -0.084147f, 0.054030f, 0.0f),
                                     vec4(-20.0f, 16.948593f, 5.694101f, 1.0f))));

    // transformation
    const quat qRotOneX = rotationmatrix(rotOneX);

    const mat4 t = transformation(vec3(10.0f, -3.0f, 4.6f), 
                                  outRotation, 
                                  vec3(2.0f, -1.0f, 3.0f), 
                                  vec3(4.0f, 5.0f, -2.0f), 
                                  qRotOneX, 
                                  vec3(-33.0f, 10.0f, 15.0f));

    VERIFY(matnearequal(t, mat4(vec4(  2.0f,       0.0f,       0.0f, 0.0f),
                                vec4(  0.0f,  2.520287f,  0.559231f, 0.0f),
                                vec4(  0.0f, -1.123711f, -1.439683f, 0.0f),
                                vec4(-43.0f, 17.853806f, 18.134460f, 1.0f))));

    const vec3 eye(3.0f, 2.0f, 10.0f);
    const vec3 at(3.0f, 2.0f, 2.0f);
    const vec3 up(0.0f, 1.0f, 0.0f);
    // lookatlh
    mat4 tmp = lookatlh(eye, at, up);
    VERIFY(matnearequal(tmp, mat4(vec4(  -1.0f,  0.0f,  0.0f, 0.0f),
                                  vec4(  0.0f,  1.0f,  0.0f, 0.0f),
                                  vec4(  0.0f, 0.0f,  -1.0f, 0.0f),
                                  vec4(  3.0f, 2.0f, 10.0f, 1.0f))));
    // lookatrh
    tmp = lookatrh(eye, at, up);
    VERIFY(matnearequal(tmp, mat4(vec4(  1.0f,  0.0f,  0.0f, 0.0f),
                                  vec4(  0.0f,  1.0f,  0.0f, 0.0f),
                                  vec4(  0.0f,  0.0f,  1.0f, 0.0f),
                                  vec4(  3.0f, 2.0f, 10.0f, 1.0f))));
    // ortholh
    tmp = ortholh(1280.0f, 1024.0f, 0.1f, 100.0f);
    VERIFY(matnearequal(tmp, mat4(vec4( 0.001563f,      0.0f,       0.0f, 0.0f),
                                  vec4(      0.0f, 0.001953f,       0.0f, 0.0f),
                                  vec4(      0.0f,      0.0f,   0.02002f, 0.0f),
                                  vec4(      0.0f,      0.0f, -1.002002f, 1.0f))));

    // orthorh
    tmp = orthorh(1280.0f, 1024.0f, 0.1f, 100.0f);
    VERIFY(matnearequal(tmp, mat4(vec4( 0.001563f,      0.0f,       0.0f, 0.0f),
                                  vec4(      0.0f, 0.001953f,       0.0f, 0.0f),
                                  vec4(      0.0f,      0.0f,  -0.02002f, 0.0f),
                                  vec4(      0.0f,      0.0f, -1.002002f, 1.0f))));

    // orthooffcenterlh
    tmp = orthooffcenterlh(100.0f, 1380.0f, 1224.0f, 200.0f, 0.1f, 1000.0f);
    VERIFY(matnearequal(tmp, mat4(vec4(0.001562,   0.000000,  0.000000, 0.000000),
                                  vec4(0.000000,   0.001953,  0.000000, 0.000000),
                                  vec4(0.000000,   0.000000,  0.002000, 0.000000),
                                  vec4(-1.156250, -1.390625, -1.000200, 1.000000))));

    // orthooffcenterrh
    tmp = orthooffcenterrh(100.0f, 1380.0f, 1224.0f, 200.0f, 0.1f, 1000.0f);
    VERIFY(matnearequal(tmp, mat4(vec4(0.001562,   0.000000,  0.000000, 0.000000),
                                  vec4(0.000000,   0.001953,  0.000000, 0.000000),
                                  vec4(0.000000,   0.000000, -0.002000, 0.000000),
                                  vec4(-1.156250, -1.390625, -1.000200, 1.000000))));

    // perspfovlh
    tmp = perspfovlh(70.0f, 3.0f/4.0f, 0.1f, 50.0f);
    VERIFY(matnearequal(tmp, mat4(vec4( 2.814039f,     0.0f,      0.0f, 0.0f),
                                  vec4(      0.0f, 2.11053f,      0.0f, 0.0f),
                                  vec4(      0.0f,     0.0f, 1.002004f, 1.0f),
                                  vec4(      0.0f,     0.0f,  -0.1002f, 0.0f))));
    // perspfovrh
    tmp = perspfovrh(70.0f, 3.0f/4.0f, 0.1f, 50.0f);
    VERIFY(matnearequal(tmp, mat4(vec4( 2.814039f,     0.0f,       0.0f,  0.0f),
                                  vec4(      0.0f, 2.11053f,       0.0f,  0.0f),
                                  vec4(      0.0f,     0.0f, -1.002004f, -1.0f),
                                  vec4(      0.0f,     0.0f,  -0.1002f,   0.0f))));

    // persplh
    tmp = persplh(1280.0f, 1024.0f, 0.1f, 100.0f);
    VERIFY(matnearequal(tmp, mat4(vec4( 0.000156f,      0.0f,      0.0f, 0.0f),
                                  vec4(      0.0f, 0.000195f,      0.0f, 0.0f),
                                  vec4(      0.0f,      0.0f, 1.001001f, 1.0f),
                                  vec4(      0.0f,      0.0f,  -0.1001f, 0.0f))));

    // persprh
    tmp = persprh(1280.0f, 1024.0f, 0.1f, 100.0f);
    VERIFY(matnearequal(tmp, mat4(vec4( 0.000156f,      0.0f,       0.0f,  0.0f),
                                  vec4(      0.0f, 0.000195f,       0.0f,  0.0f),
                                  vec4(      0.0f,      0.0f, -1.001001f, -1.0f),
                                  vec4(      0.0f,      0.0f,   -0.1001f,  0.0f))));

    // perspoffcenterlh
    tmp = perspoffcenterlh(50.0f, 1330.0f, -150.0f, 874.0f, 1.0f, 1000.0f);
    VERIFY(matnearequal(tmp, mat4(vec4( 0.001563f,       0.0f,       0.0f, 0.0f),
                                  vec4(      0.0f,  0.001953f,       0.0f, 0.0f),
                                  vec4(-1.078125f, -0.707031f,  1.001001f, 1.0f),
                                  vec4(      0.0f,       0.0f, -1.001001f, 0.0f))));

    // perspoffcenterrh
    tmp = perspoffcenterrh(50.0f, 1330.0f, -150.0f, 874.0f, 1.0f, 1000.0f);
    VERIFY(matnearequal(tmp, mat4(vec4(0.001563f,      0.0f,       0.0f,  0.0f),
                                  vec4(     0.0f, 0.001953f,       0.0f,  0.0f),
                                  vec4(1.078125f, 0.707031f, -1.001001f, -1.0f),
                                  vec4(     0.0f,      0.0f, -1.001001f,  0.0f))));

    // rotationyawpitchroll
    tmp = rotationyawpitchroll(1.0f, -0.462f, 3.036f);
    VERIFY(matnearequal(tmp, mat4(vec4(-0.537292838, 0.0569459870, -0.841470957, 0.0f),
                                  vec4(0.278640598, -0.929708600, -0.240833968, 0.0f),
                                  vec4(-0.796037436, -0.363866359, 0.483658552, 0.0f),
                                  vec4(      0.0f,       0.0f,      0.0f, 1.0f))));

    tmp = rotationyawpitchroll(-7.0f, 3.0f, -2.0f);
    VERIFY(matnearequal(tmp, mat4(vec4(-0.313734174, -0.685521364, 0.656986594, 0.000000f),
                                  vec4(-0.861614943, 0.496286869, 0.106390685, 0.000000f),
                                  vec4(-0.398986936, -0.532691121, -0.746357560, 0.000000f),
                                  vec4( 0.000000f,  0.000000f,  0.000000f, 1.000000f))));
}

} // namespace Test
