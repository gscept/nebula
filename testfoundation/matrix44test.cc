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
    matrix44 m0(float4(1.0f, 0.0f, 0.0f, 0.0f),
                float4(0.0f, 1.0f, 0.0f, 0.0f),
                float4(0.0f, 0.0f, 1.0f, 0.0f),
                float4(0.0f, 0.0f, 0.0f, 1.0f));
    this->Verify(m0.isidentity());
    matrix44 m1(m0);
    this->Verify(m0.isidentity());
    this->Verify(m0 == m1);
    matrix44 m2(float4(1.0f, 0.0f, 0.0f, 0.0f),
                float4(0.0f, 1.0f, 0.0f, 0.0f),
                float4(0.0f, 0.0f, 1.0f, 0.0f),
                float4(1.0f, 2.0f, 3.0f, 1.0f));
    this->Verify(m0 != m2);
    m2 = m0;
    this->Verify(m0 == m2);

    // load and store
    NEBULA3_ALIGN16 scalar f[16];
    m0.store(f);
    this->Verify(f[0]==1.0f && f[1]==0.0f && f[2]==0.0f && f[3]==0.0f &&
                 f[4]==0.0f && f[5]==1.0f && f[6]==0.0f && f[7]==0.0f &&
                 f[8]==0.0f && f[9]==0.0f && f[10]==1.0f && f[11]==0.0f &&
                 f[12]==0.0f && f[13]==0.0f && f[14]==0.0f && f[15]==1.0f);
    m1.load(f);
    this->Verify(m1.isidentity());
    m0.storeu(f);
    this->Verify(f[0]==1.0f && f[1]==0.0f && f[2]==0.0f && f[3]==0.0f &&
                 f[4]==0.0f && f[5]==1.0f && f[6]==0.0f && f[7]==0.0f &&
                 f[8]==0.0f && f[9]==0.0f && f[10]==1.0f && f[11]==0.0f &&
                 f[12]==0.0f && f[13]==0.0f && f[14]==0.0f && f[15]==1.0f);
    m1.loadu(f);
    this->Verify(m1.isidentity());
    m0.stream(f);
    this->Verify(f[0]==1.0f && f[1]==0.0f && f[2]==0.0f && f[3]==0.0f &&
                 f[4]==0.0f && f[5]==1.0f && f[6]==0.0f && f[7]==0.0f &&
                 f[8]==0.0f && f[9]==0.0f && f[10]==1.0f && f[11]==0.0f &&
                 f[12]==0.0f && f[13]==0.0f && f[14]==0.0f && f[15]==1.0f);
    m1.load(f);
    this->Verify(m1.isidentity());

    // component-wise access
    m0.set(float4(2.0f, 0.0f, 0.0f, 0.0f),
           float4(0.0f, 2.0f, 0.0f, 0.0f),
           float4(0.0f, 0.0f, 2.0f, 0.0f),
           float4(0.0f, 0.0f, 0.0f, 1.0f));
    float4 value(2.0f, 0.0f, 0.0f, 0.0f);
    m1.setrow0(value);
    value = float4(0.0f, 2.0f, 0.0f, 0.0f);
    m1.setrow1(value);
    value = float4(0.0f, 0.0f, 2.0f, 0.0f);
    m1.setrow2(value);
    value = float4(0.0f, 0.0f, 0.0f, 1.0f);
    m1.setrow3(value);
    this->Verify(m0 == m1);
    this->Verify(m0.getrow0() == float4(2.0f, 0.0f, 0.0f, 0.0f));
    this->Verify(m0.getrow1() == float4(0.0f, 2.0f, 0.0f, 0.0f));
    this->Verify(m0.getrow2() == float4(0.0f, 0.0f, 2.0f, 0.0f));
    this->Verify(m0.getrow3() == float4(0.0f, 0.0f, 0.0f, 1.0f));

    // @todo: test remaining matrix44 functions
}

} // namespace Test