//------------------------------------------------------------------------------
//  scalartest.cc
//  (C) 2009 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "scalartest.h"
#include "math/scalar.h"
#include "testbase/stackdebug.h"

using namespace Math;

static const scalar TEST_EPSILON = 0.000001;

#define RUN_TEST_PARAM1(func, param, expected)\
{\
    scalar result = (scalar)func(param);\
    VERIFY(nearly_equal(result, (expected), TEST_EPSILON));\
}

#define RUN_TEST_PARAM2( func, param0, param1, expected )\
{\
    scalar result = (scalar)func((param0), (param1));\
    VERIFY(nearly_equal(result, (expected), TEST_EPSILON));\
}

using namespace Test;

__ImplementClass(Test::ScalarTest, 'SCTS', Test::TestCase);

//------------------------------------------------------------------------------
/**
    checks if 2 scalars are *nearly* equal
*/
static bool nearly_equal(scalar a, scalar b, scalar epsilon)
{
    return Math::abs(a - b) < epsilon;
}

//------------------------------------------------------------------------------
/**
    test for math/ps3/ps3scalar.h
*/
void ScalarTest::Run()
{
    STACK_CHECKPOINT("Test::ScalarTest::Run()");

    static const int LOOPS = 100;
    int index;
    scalar test;

    // scalar   Math::rand();
    for(index = 0; index < LOOPS; ++index)
    {
        test = Math::rand();
        VERIFY(test >= scalar(0.0));
        VERIFY(test <= scalar(1.0));
    }

    // scalar   Math::rand(scalar min, scalar max);
    for(index = 0; index < LOOPS; ++index)
    {
        scalar min = Math::rand() * scalar(index);
        scalar max = Math::rand() * scalar(index);
        if(min > max) 
        {
            scalar tmp = max;
            max = min;
            min = tmp;
        }
        test = Math::rand(min, max);
        VERIFY(test >= min);
        VERIFY(test <= max);
    }

    // Math::sin
    RUN_TEST_PARAM1(Math::sin, 0.0, 0.0);
    RUN_TEST_PARAM1(Math::sin, 1.0, 0.8414709848078965066525023216303);
    RUN_TEST_PARAM1(Math::sin, N_PI_HALF, 1.0);
    RUN_TEST_PARAM1(Math::sin, N_PI, 0.0);
    // Math::cos
    RUN_TEST_PARAM1(Math::cos, 0.0, 1.0);
    RUN_TEST_PARAM1(Math::cos, 1.0, 0.54030230586813971740093660744298);
    RUN_TEST_PARAM1(Math::cos, N_PI_HALF, 0.0);
    RUN_TEST_PARAM1(Math::cos, N_PI, -1.0);
    // Math::tan
    RUN_TEST_PARAM1(Math::tan, -N_PI, 0.0);
    RUN_TEST_PARAM1(Math::tan, 0.0, 0.0);
    RUN_TEST_PARAM1(Math::tan, 1.0, 1.5574077246549022305069748074584);
    RUN_TEST_PARAM1(Math::tan, N_PI, 0.0);
    // Math::asin
    RUN_TEST_PARAM1(Math::asin, 0.0, 0.0);
    RUN_TEST_PARAM1(Math::asin, 0.8414709848078965066525023216303, 1.0);
    RUN_TEST_PARAM1(Math::asin, 1.0, N_PI_HALF);
    // Math::acos
    RUN_TEST_PARAM1(Math::acos, 1.0, 0.0);
    RUN_TEST_PARAM1(Math::acos, 0.54030230586813971740093660744298, 1.0);
    RUN_TEST_PARAM1(Math::acos, -1.0, N_PI);
    RUN_TEST_PARAM1(Math::acos, 0.0, N_PI_HALF);
    // Math::atan
    RUN_TEST_PARAM1(Math::atan, 0.0, 0.0);
    RUN_TEST_PARAM1(Math::atan, 1.5574077246549022305069748074584, 1.0);
    RUN_TEST_PARAM1(Math::atan, N_PI, 1.2626272556789116834443220836057);
    // Math::sqrt
    RUN_TEST_PARAM1(Math::sqrt, 0.0, 0.0);
    RUN_TEST_PARAM1(Math::sqrt, 1.0, 1.0);
    RUN_TEST_PARAM1(Math::sqrt, 2.0, 1.4142135623730950488016887242097);
    RUN_TEST_PARAM1(Math::sqrt, 23.0, 4.7958315233127195415974380641627);
    // Math::fchop
    RUN_TEST_PARAM1(Math::fchop, 0.0, 0.0);
    RUN_TEST_PARAM1(Math::fchop, 0.001, 0.0);
    RUN_TEST_PARAM1(Math::fchop, 0.999, 0.0);
    RUN_TEST_PARAM1(Math::fchop, 1.01, 1.0);
    // Math::modangle
    static const scalar REVOLUTION = N_PI * 2.0;
    RUN_TEST_PARAM1(Math::modangle,  REVOLUTION, 0.0);
    RUN_TEST_PARAM1(Math::modangle, -REVOLUTION, 0.0);
    RUN_TEST_PARAM1(Math::modangle, -REVOLUTION - 0.1f, -0.1f);
    RUN_TEST_PARAM1(Math::modangle, -REVOLUTION + 0.1f, 0.1f);
    RUN_TEST_PARAM1(Math::modangle,  REVOLUTION + 0.1f, 0.1f);
    RUN_TEST_PARAM1(Math::modangle,  N_PI, -N_PI);

    RUN_TEST_PARAM1(Math::modangle,  -N_PI, -N_PI);
    RUN_TEST_PARAM1(Math::modangle,  -N_PI - 0.1f, N_PI - 0.1f);
    RUN_TEST_PARAM1(Math::modangle,  N_PI + 0.1f, -N_PI + 0.1f);
    // n_log2
    RUN_TEST_PARAM1(Math::log2,  1.0, 0.0);
    RUN_TEST_PARAM1(Math::log2,  2.0, 1.0);
    RUN_TEST_PARAM1(Math::log2, 100.0, 6.6438561897747276615228717066083);
    // n_exp
    RUN_TEST_PARAM1(Math::exp,  0.0, 1.0);
    RUN_TEST_PARAM1(Math::exp,  0.5, 1.6487212707001281468486507878142);
    RUN_TEST_PARAM1(Math::exp,  1.0, 2.7182818284590452353602874713527);
    // n_frnd
    RUN_TEST_PARAM1(Math::frnd,  -3.0, -3.0);
    RUN_TEST_PARAM1(Math::frnd,  -3.1, -3.0);
    RUN_TEST_PARAM1(Math::frnd,  -2.9, -3.0);
    RUN_TEST_PARAM1(Math::frnd,  -2.1, -2.0);
    RUN_TEST_PARAM1(Math::frnd,   0.5,  1.0);
    RUN_TEST_PARAM1(Math::frnd,   0.0,  0.0);
    // n_pow
    RUN_TEST_PARAM2(Math::pow, -1.0, -1.0, -1.0);
    RUN_TEST_PARAM2(Math::pow, -2.0,  2.0,  4.0);
    // n_fmod
    RUN_TEST_PARAM2(Math::fmod, 2.0, 4.0, 2.0);
    RUN_TEST_PARAM2(Math::fmod, 3.0, 4.0, 3.0);
}
