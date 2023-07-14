//------------------------------------------------------------------------------
//  uri.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/uri.h"
#include "io/assignregistry.h"
#include "io/fswrapper.h"
#include "core/config.h"


//------------------------------------------------------------------------------
/**
    Literal constructor form string, to use "foobar"_uri will automatically construct an IO::URI
*/
IO::URI
operator ""_uri(const char* c, std::size_t s)
{
    return IO::URI(c);
}

namespace IO
{
using namespace Util;

//------------------------------------------------------------------------------
/**
    Resolve assigns and split URI string into its components.

    @todo: this is too complicated...
*/
bool
URI::Split(const String& s)
{
    n_assert(s.IsValid());
    this->Clear();
    this->isEmpty = false;
    
    // resolve assigns
    String str;
    if (AssignRegistry::HasInstance())
    {
        str = AssignRegistry::Instance()->ResolveAssignsInString(s);
    }
    else
    {
        str = s;
    }

    // scheme is the first components and ends with a :
    IndexT schemeColonIndex = str.FindCharIndex(':');
    String potentialScheme;
    bool schemeIsDevice = false;
    if (InvalidIndex != schemeColonIndex)
    {
        potentialScheme = str.ExtractRange(0, schemeColonIndex);
        if (FSWrapper::IsDeviceName(potentialScheme))
        {
            // there is either no scheme given, or the "scheme"
            // is actually a device name
            // in both cases we fall back to the default scheme "file", and
            // just set the whole string as local path, there will be no
            // other components
            schemeIsDevice = true;
        }
    }
    if ((InvalidIndex == schemeColonIndex) || schemeIsDevice)
    {
        this->SetScheme(DEFAULT_IO_SCHEME);
        this->SetLocalPath(str);
        return true;
    }

    // check is a valid scheme was provided
    if (InvalidIndex != schemeColonIndex)
    {
        // a valid scheme is given
        this->SetScheme(potentialScheme);

        // after the scheme, and before the host, there must be a double slash
        if (!((str[schemeColonIndex + 1] == '/') && (str[schemeColonIndex + 2] == '/')))
        {
            return false;
        }
    }

    // extract UserInfo, Host and Port components
    IndexT hostStartIndex = schemeColonIndex + 3;
    IndexT hostEndIndex = str.FindCharIndex('/', hostStartIndex);
    String userInfoHostPort;
    String path;
    if (InvalidIndex == hostEndIndex)
    {
        userInfoHostPort = str.ExtractToEnd(hostStartIndex);
    }
    else
    {
        userInfoHostPort = str.ExtractRange(hostStartIndex, hostEndIndex - hostStartIndex);
        path = str.ExtractToEnd(hostEndIndex + 1);
    }

    // extract port number if exists
    IndexT portIndex = userInfoHostPort.FindCharIndex(':');
    IndexT atIndex = userInfoHostPort.FindCharIndex('@');
    if (InvalidIndex != portIndex)
    {
        if (InvalidIndex != atIndex)
        {
            n_assert(portIndex > atIndex);
        }
        this->SetPort(userInfoHostPort.ExtractToEnd(portIndex + 1));
        userInfoHostPort.TerminateAtIndex(portIndex);
    }
    if (InvalidIndex != atIndex)
    {
        this->SetHost(userInfoHostPort.ExtractToEnd(atIndex + 1));
        userInfoHostPort.TerminateAtIndex(atIndex);
        this->SetUserInfo(userInfoHostPort);
    }
    else
    {
        this->SetHost(userInfoHostPort);
    }

    // split path part into components
    if (path.IsValid())
    {
        IndexT fragmentIndex = path.FindCharIndex('#');
        IndexT queryIndex = path.FindCharIndex('?');
        if (InvalidIndex != queryIndex)
        {
            if (InvalidIndex != fragmentIndex)
            {
                n_assert(queryIndex > fragmentIndex);
            }
            this->SetQuery(path.ExtractToEnd(queryIndex + 1));
            path.TerminateAtIndex(queryIndex);
        }
        if (InvalidIndex != fragmentIndex)
        {
            this->SetFragment(path.ExtractToEnd(fragmentIndex + 1));
            path.TerminateAtIndex(fragmentIndex);
        }
        this->SetLocalPath(path);
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    This builds an URI string from its components.
*/
String
URI::Build() const
{
    n_assert(!this->IsEmpty());

    String str;
    str.Reserve(256);
    if (this->scheme.IsValid())
    {
        str.Append(this->scheme);
        str.Append("://");
    }
    if (this->userInfo.IsValid())
    {
        str.Append(this->userInfo);
        str.Append("@");
    }
    if (this->host.IsValid())
    {
        str.Append(this->host);
    }
    if (this->port.IsValid())
    {
        str.Append(":");
        str.Append(this->port);
    }
    if (this->localPath.IsValid())
    {
        str.Append("/");
        str.Append(this->localPath);
    }
    if (this->fragment.IsValid())
    {
        str.Append("#");
        str.Append(this->fragment);
    }
    if (this->query.IsValid())
    {
        str.Append("?");
        str.Append(this->query);
    }
    return str;
}

//------------------------------------------------------------------------------
/**
    This returns the "tail", which is the local path, the fragment and
    the query concatenated into one string.
*/
String
URI::GetTail() const
{   
    String str;
    str.Reserve(256);
    if (this->localPath.IsValid())
    {
        str.Append(this->localPath);
    }
    if (this->fragment.IsValid())
    {
        str.Append("#");
        str.Append(this->fragment);
    }
    if (this->query.IsValid())
    {
        str.Append("?");
        str.Append(this->query);
    }
    return str;
}

//------------------------------------------------------------------------------
/**
    Returns the host and local path in the form "//host/localpath".
    If no host has been set, only "/localpath" will be returned.
*/
String
URI::GetHostAndLocalPath() const
{
    String str;
    str.Reserve(this->host.Length() + this->localPath.Length() + 8);
    if (this->host.IsValid())
    {
        str.Append("//");
        str.Append(this->host);
        str.Append("/");
    }
    str.Append(this->localPath);
    return str;
}

//------------------------------------------------------------------------------
/**
    Appends an element to the local path. Automatically inserts
    a path delimiter "/".
*/
void
URI::AppendLocalPath(const String& pathComponent)
{
    n_assert(pathComponent.IsValid());
    this->localPath.Append("/");
    this->localPath.Append(pathComponent);
}

//------------------------------------------------------------------------------
/**
    This parses the query part of the URI (in the form
    param1=value&param2=value&param3=value ...) into a dictionary. Ill-formatted
    query fragments will be ignored.
*/
Dictionary<String,String>
URI::ParseQuery() const
{
    Dictionary<String,String> result;
    Array<String> keyValuePairs = this->query.Tokenize("&");
    IndexT i;
    for (i = 0; i < keyValuePairs.Size(); i++)
    {
        Array<String> keyValueTokens = keyValuePairs[i].Tokenize("=");
        if (keyValueTokens.Size() == 2)
        {
            result.Add(keyValueTokens[0], keyValueTokens[1]);
        }
    }
    return result;
}

} // namespace IO
