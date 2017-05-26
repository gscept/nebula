//------------------------------------------------------------------------------
//  testcase.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "testcase.h"

namespace Test
{
__ImplementClass(Test::TestCase, 'TSTC', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
TestCase::TestCase() :
    numVerified(0),
    numSucceeded(0),
    numFailed(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
TestCase::~TestCase()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Override this method in your derived class.
*/    
void
TestCase::Run()
{
    // empty
}

//------------------------------------------------------------------------------
/*
*/
void
TestCase::Verify(bool b)
{
    if (b)
    {
        n_printf("%s #%d: ok\n", this->GetClassName().AsCharPtr(), this->numVerified);
        this->numSucceeded++;
    }
    else
    {
        n_printf("%s #%d: FAILED\n", this->GetClassName().AsCharPtr(), this->numVerified);		
        this->numFailed++;
    }
    this->numVerified++;
}

}; // namespace Test
