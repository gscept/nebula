#ifndef TEST_VARIANTTEST_H
#define TEST_VARIANTTEST_H
//------------------------------------------------------------------------------
/**
    @class Test::VariantTest
  
    Test the Util::Variant class.
    
    (C) 2006 Radon Labs GmbH
*/    
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class VariantTest : public TestCase
{
    __DeclareClass(VariantTest);
public:
    /// run the test
    virtual void Run();
};

} // namespace Test
//------------------------------------------------------------------------------
#endif
