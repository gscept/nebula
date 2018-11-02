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

    VERIFY(m0.GetType() == "text");
    VERIFY(m0.GetSubType() == "plain");
    VERIFY(m0.AsString() == "text/plain");

    VERIFY(m1.GetType() == "text");
    VERIFY(m1.GetSubType() == "html");
    VERIFY(m1.AsString() == "text/html");

    VERIFY(m2.GetType() == "text");
    VERIFY(m2.GetSubType() == "plain");
    VERIFY(m2.AsString() == "text/plain");

    VERIFY(m0 == m2);
    VERIFY(m0 != m1);
}

}