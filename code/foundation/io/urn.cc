//------------------------------------------------------------------------------
//  urn.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "io/urn.h"
#include "io/assignregistry.h"
#include "io/fswrapper.h"
#include "core/config.h"


//------------------------------------------------------------------------------
/**
    Literal constructor form string, to use "foobar"_urn will automatically construct an IO::URN
*/
IO::URN
operator ""_urn(const char* c, std::size_t s)
{
    return IO::URN(c);
}

namespace IO
{
using namespace Util;

//------------------------------------------------------------------------------
/**
    Resolve assigns and split URN string into its components.

*/
bool
URN::Split(const String& s)
{
    n_assert(s.IsValid());
    this->Clear();
    this->isEmpty = false;

    // resolve assigns first
    String str;
    if (AssignRegistry::HasInstance())
    {
        str = AssignRegistry::Instance()->ResolveAssignsInString(s);
    }
    else
    {
        str = s;
    }

    // URN must start with "urn:"
    if (str.Length() < 5)
    {
        this->Clear();
        return false;
    }
    String prefix = str.ExtractRange(0, 4);
    prefix.ToLower();
    if (prefix != "urn:")
    {
        this->Clear();
        return false;
    }

    // split off fragment and query from the URN body
    String body = str.ExtractToEnd(4);
    IndexT fragmentIndex = body.FindCharIndex('#');
    if (InvalidIndex != fragmentIndex)
    {
        this->SetFragment(body.ExtractToEnd(fragmentIndex + 1));
        body.TerminateAtIndex(fragmentIndex);
    }

    IndexT queryIndex = body.FindCharIndex('?');
    if (InvalidIndex != queryIndex)
    {
        this->SetQuery(body.ExtractToEnd(queryIndex + 1));
        body.TerminateAtIndex(queryIndex);
    }

    // remaining body must be <nid>:<nss>
    IndexT nidSeparatorIndex = body.FindCharIndex(':');
    if (InvalidIndex == nidSeparatorIndex)
    {
        this->Clear();
        return false;
    }

    String nid = body.ExtractRange(0, nidSeparatorIndex);
    String nss = body.ExtractToEnd(nidSeparatorIndex + 1);
    if (!nid.IsValid() || !nss.IsValid())
    {
        this->Clear();
        return false;
    }

    nid.ToLower();
    this->SetNamespace(nid);
    this->SetSpecific(nss);
    return true;
}

//------------------------------------------------------------------------------
/**
    This builds an URI string from its components.
*/
String
URN::Build() const
{
    n_assert(!this->IsEmpty());
    n_assert(this->nid.IsValid());
    n_assert(this->nss.IsValid());

    String str;
    str.Reserve(256);
    str.Append("urn:");
    str.Append(this->nid);
    str.Append(":");
    str.Append(this->nss);
    if (this->query.IsValid())
    {
        str.Append("?");
        str.Append(this->query);
    }
    if (this->fragment.IsValid())
    {
        str.Append("#");
        str.Append(this->fragment);
    }
    return str;
}

} // namespace IO
