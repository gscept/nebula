#ifndef TEST_TESTCASE_H
#define TEST_TESTCASE_H
//------------------------------------------------------------------------------
/**
    @class Test::TestCase
    
    Base class for a test case.
    
    (C) 2006 Radon Labs GmbH
*/
#include "core/refcounted.h"

//------------------------------------------------------------------------------
namespace Test
{
class TestCase : public Core::RefCounted
{
    __DeclareClass(TestCase);
public:
    /// constructor
    TestCase();
    /// destructor
    virtual ~TestCase();
    /// run the test
    virtual void Run();
    /// verify a statement
    void Verify(bool b);
    /// return number of succeeded verifies
    int GetNumSucceeded() const;
    /// return number of failed verifies
    int GetNumFailed() const;
    /// return overall number of verifies
    int GetNumVerified() const;
private:
    int numVerified;
    int numSucceeded;
    int numFailed;    
};

//------------------------------------------------------------------------------
/*
*/
inline
int
TestCase::GetNumSucceeded() const
{
    return this->numSucceeded;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
TestCase::GetNumFailed() const
{
    return this->numFailed;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
TestCase::GetNumVerified() const
{
    return this->numVerified;
}

};
//------------------------------------------------------------------------------
#endif    