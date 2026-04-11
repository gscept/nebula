//------------------------------------------------------------------------------
//  urntest.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "urntest.h"
#include "io/urn.h"

namespace Test
{
__ImplementClass(Test::URNTest, 'URNT', Test::TestCase);

using namespace IO;

//------------------------------------------------------------------------------
/**
*/
void
URNTest::Run()
{
    // default-constructed URN is empty
    URN empty;
    VERIFY(empty.IsEmpty());
    VERIFY(!empty.IsValid());

    // constructors parse and split valid URNs
    URN parsed("urn:Example:asset/path?foo=bar#frag");
    VERIFY(parsed.IsValid());
    VERIFY(!parsed.IsEmpty());
    VERIFY(parsed.GetNamespace() == "example");
    VERIFY(parsed.GetSpecific() == "asset/path");
    VERIFY(parsed.GetQuery() == "foo=bar");
    VERIFY(parsed.GetFragment() == "frag");
    VERIFY(parsed.AsString() == "urn:example:asset/path?foo=bar#frag");

    // copy and equality compare all URN components
    URN copy(parsed);
    VERIFY(copy == parsed);
    copy.SetFragment("other");
    VERIFY(copy != parsed);

    // Set() reparses the whole string and should clear on invalid data
    URN viaSet;
    viaSet.Set("urn:nebula:test?debug=1#tail");
    VERIFY(viaSet.IsValid());
    VERIFY(viaSet.GetNamespace() == "nebula");
    VERIFY(viaSet.GetSpecific() == "test");
    VERIFY(viaSet.GetQuery() == "debug=1");
    VERIFY(viaSet.GetFragment() == "tail");

    viaSet.Set("invalid-urn");
    VERIFY(viaSet.IsEmpty());
    VERIFY(!viaSet.IsValid());

    // malformed but prefix-correct URNs should also fail parsing
    viaSet.Set("urn:");
    VERIFY(viaSet.IsEmpty());
    VERIFY(!viaSet.IsValid());

    viaSet.Set("urn:missingnss");
    VERIFY(viaSet.IsEmpty());
    VERIFY(!viaSet.IsValid());

    viaSet.Set("urn:nid:");
    VERIFY(viaSet.IsEmpty());
    VERIFY(!viaSet.IsValid());

    viaSet.Set("urn::nss");
    VERIFY(viaSet.IsEmpty());
    VERIFY(!viaSet.IsValid());

    // component setters should build a round-trippable URN string
    URN built;
    built.SetNamespace("movie");
    built.SetSpecific("episode/1");
    built.SetQuery("lang=en");
    built.SetFragment("scene");
    VERIFY(built.AsString() == "urn:movie:episode/1?lang=en#scene");

    // clear resets state and components
    built.Clear();
    VERIFY(built.IsEmpty());
    VERIFY(!built.IsValid());
    VERIFY(built.GetNamespace().IsEmpty());
    VERIFY(built.GetSpecific().IsEmpty());
    VERIFY(built.GetQuery().IsEmpty());
    VERIFY(built.GetFragment().IsEmpty());

    // user defined literal should construct a valid URN
    URN literal = "urn:book:isbn/9780131103627#cover"_urn;
    VERIFY(literal.IsValid());
    VERIFY(literal.GetNamespace() == "book");
    VERIFY(literal.GetSpecific() == "isbn/9780131103627");
    VERIFY(literal.GetFragment() == "cover");
}

} // namespace Test
