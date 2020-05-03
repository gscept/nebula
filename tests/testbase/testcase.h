#ifndef TEST_TESTCASE_H
#define TEST_TESTCASE_H
//------------------------------------------------------------------------------
/**
    @class Test::TestCase
    
    Base class for a test case.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
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
    void Verify(bool b, const char * test, const char * file, int line);
    /// return number of succeeded verifies
    int GetNumSucceeded() const;
    /// return number of failed verifies
    int GetNumFailed() const;
    /// return overall number of verifies
    int GetNumVerified() const;


    struct FailedTest
    {
        Util::String compare;
        Util::String file;
        int line;
    };

    /// get errors
    const Util::Array<FailedTest> & GetFailed() const;
private:
    int numVerified;
    int numSucceeded;
    int numFailed;  

    Util::Array< FailedTest> failed;
};

#define VERIFY(test) Verify(test, #test, __FILE__, __LINE__)
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

//------------------------------------------------------------------------------
/**
*/
inline
const Util::Array<TestCase::FailedTest> &
TestCase::GetFailed() const
{
    return this->failed;
}

};
//------------------------------------------------------------------------------
#endif    