//------------------------------------------------------------------------------
//  mediatypetest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "mediatypetest.h"
#include "io/mediatype.h"

namespace Test
{
__ImplementClass(Test::MediaTypeTest, 'MMTT', Test::TestCase);

using namespace IO;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
MediaTypeTest::Run()
{
    MediaType m0("text/plain");
    MediaType m1("text", "html");
    MediaType m2 = m0;

    this->Verify(m0.GetType() == "text");
    this->Verify(m0.GetSubType() == "plain");
    this->Verify(m0.AsString() == "text/plain");

    this->Verify(m1.GetType() == "text");
    this->Verify(m1.GetSubType() == "html");
    this->Verify(m1.AsString() == "text/html");

    this->Verify(m2.GetType() == "text");
    this->Verify(m2.GetSubType() == "plain");
    this->Verify(m2.AsString() == "text/plain");

    this->Verify(m0 == m2);
    this->Verify(m0 != m1);
}

}