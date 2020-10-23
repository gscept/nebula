//------------------------------------------------------------------------------
//  blobtest.cc
// (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "bitfieldtest.h"
#include "util/bitfield.h"

namespace Test
{
__ImplementClass(Test::BitFieldTest, 'BFDT' , Test::TestCase);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
BitFieldTest::Run()
{
    VERIFY(std::is_trivially_copyable<Util::BitField<32>>());
    VERIFY(std::is_trivially_destructible<Util::BitField<32>>());
    
    Util::BitField<1> b1;
    VERIFY(b1.IsNull());
    b1.SetBit<0>();
    VERIFY(b1.IsSet<0>());
    
    VERIFY(sizeof(Util::BitField<8>) == 1);
    VERIFY(sizeof(Util::BitField<16>) == 2);
    VERIFY(sizeof(Util::BitField<32>) == 4);
    VERIFY(sizeof(Util::BitField<64>) == 8);
    VERIFY(sizeof(Util::BitField<128>) == 16);
    VERIFY(sizeof(Util::BitField<129>) > 16);
    
    Util::BitField<8> b8_a = { 1, 2, 3, 7 };
    Util::BitField<8> b8_b = { 2, 3 };
    VERIFY(b8_a.IsSet(1));
    VERIFY(b8_a.IsSet(2));
    VERIFY(b8_a.IsSet(3));
    VERIFY(b8_a.IsSet(7));
    
    VERIFY(b8_a != b8_b);
    
    b8_a = b8_b;
    VERIFY(b8_a == b8_b);
    Util::BitField<8> b8_c(b8_a);
    VERIFY(b8_c == b8_a);

    Util::BitField<16> b16;
    Util::BitField<32> b32;
    Util::BitField<64> b64;
    Util::BitField<128> b128;
    Util::BitField<256> b256;
    Util::BitField<1050> b1050;

    VERIFY(true);
}

}; 