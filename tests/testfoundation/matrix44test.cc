//------------------------------------------------------------------------------
//  matrix44test.cc
//  (C) 2007 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "matrix44test.h"

namespace Test
{
__ImplementClass(Test::Matrix44Test, 'M4TS', Test::TestCase);

using namespace Math;

//------------------------------------------------------------------------------
/**
*/
void
Matrix44Test::Run()
{
    // construction
    mat4 m0(vec4(1.0f, 0.0f, 0.0f, 0.0f),
        vec4(0.0f, 1.0f, 0.0f, 0.0f),
        vec4(0.0f, 0.0f, 1.0f, 0.0f),
        vec4(0.0f, 0.0f, 0.0f, 1.0f));
    VERIFY(m0 == mat4());
    mat4 m1(m0);
    VERIFY(m1 == mat4());
    VERIFY(m0 == m1);
    mat4 m2(vec4(1.0f, 0.0f, 0.0f, 0.0f),
        vec4(0.0f, 1.0f, 0.0f, 0.0f),
        vec4(0.0f, 0.0f, 1.0f, 0.0f),
        vec4(1.0f, 2.0f, 3.0f, 1.0f));
    VERIFY(m0 != m2);
    m2 = m0;
    VERIFY(m0 == m2);

    // load and store
    NEBULA_ALIGN16 scalar f[16];
    m0.store(f);
    VERIFY(f[0]==1.0f && f[1]==0.0f && f[2]==0.0f && f[3]==0.0f &&
                 f[4]==0.0f && f[5]==1.0f && f[6]==0.0f && f[7]==0.0f &&
                 f[8]==0.0f && f[9]==0.0f && f[10]==1.0f && f[11]==0.0f &&
                 f[12]==0.0f && f[13]==0.0f && f[14]==0.0f && f[15]==1.0f);
    m1.load(f);
    VERIFY(m1 == mat4());
    m0.storeu(f);
    VERIFY(f[0]==1.0f && f[1]==0.0f && f[2]==0.0f && f[3]==0.0f &&
                 f[4]==0.0f && f[5]==1.0f && f[6]==0.0f && f[7]==0.0f &&
                 f[8]==0.0f && f[9]==0.0f && f[10]==1.0f && f[11]==0.0f &&
                 f[12]==0.0f && f[13]==0.0f && f[14]==0.0f && f[15]==1.0f);
    m1.loadu(f);
    VERIFY(m1 == mat4());
    m0.stream(f);
    VERIFY(f[0]==1.0f && f[1]==0.0f && f[2]==0.0f && f[3]==0.0f &&
                 f[4]==0.0f && f[5]==1.0f && f[6]==0.0f && f[7]==0.0f &&
                 f[8]==0.0f && f[9]==0.0f && f[10]==1.0f && f[11]==0.0f &&
                 f[12]==0.0f && f[13]==0.0f && f[14]==0.0f && f[15]==1.0f);
    m1.load(f);
    VERIFY(m1 == mat4());

    // component-wise access
    m0.set(vec4(2.0f, 0.0f, 0.0f, 0.0f),
        vec4(0.0f, 2.0f, 0.0f, 0.0f),
        vec4(0.0f, 0.0f, 2.0f, 0.0f),
        vec4(0.0f, 0.0f, 0.0f, 1.0f));
    vec4 value(2.0f, 0.0f, 0.0f, 0.0f);
    m1.row0 = value;
    value = vec4(0.0f, 2.0f, 0.0f, 0.0f);
    m1.row1 = value;
    value = vec4(0.0f, 0.0f, 2.0f, 0.0f);
    m1.row2 = value;
    value = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    m1.row3 = value;
    VERIFY(m0 == m1);
    VERIFY(m0.row0 == vec4(2.0f, 0.0f, 0.0f, 0.0f));
    VERIFY(m0.row1 == vec4(0.0f, 2.0f, 0.0f, 0.0f));
    VERIFY(m0.row2 == vec4(0.0f, 0.0f, 2.0f, 0.0f));
    VERIFY(m0.row3 == vec4(0.0f, 0.0f, 0.0f, 1.0f));

    // @todo: test remaining matrix44 functions
}

} // namespace Test