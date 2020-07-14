//------------------------------------------------------------------------------
//  uritest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "uritest.h"
#include "core/ptr.h"
#include "io/uri.h"
#include "io/ioserver.h"

namespace Test
{
__ImplementClass(Test::URITest, 'URIT', Test::TestCase);

using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
void
URITest::Run()
{
    // we need a file server
    Ptr<IoServer> ioServer = IoServer::Create();

    // build some URIs
    URI remoteFileUri("file://gambar/temp/bla.txt");
    URI localFileUri("file:///temp/blub.txt");
    URI simpleHttpUri("http://www.radonlabs.de/index.html#main");
    URI complexHttpUri("http://floh@www.radonlabs.de:1088/query.php?who=bla;where=blub");

    // verify URIs for correct splitting
    VERIFY(!remoteFileUri.IsEmpty());
    VERIFY(remoteFileUri.Scheme() == "file");
    VERIFY(remoteFileUri.UserInfo().IsEmpty());
    VERIFY(remoteFileUri.Host() == "gambar");
    VERIFY(remoteFileUri.Port().IsEmpty());
    VERIFY(remoteFileUri.LocalPath() == "temp/bla.txt");
    VERIFY(remoteFileUri.Fragment().IsEmpty());
    VERIFY(remoteFileUri.Query().IsEmpty());
    VERIFY(remoteFileUri.AsString() == "file://gambar/temp/bla.txt");

    VERIFY(localFileUri.Scheme() == "file");
    VERIFY(localFileUri.UserInfo().IsEmpty());
    VERIFY(localFileUri.Host().IsEmpty());
    VERIFY(localFileUri.Port().IsEmpty());
    VERIFY(localFileUri.LocalPath() == "temp/blub.txt");
    VERIFY(localFileUri.Fragment().IsEmpty());
    VERIFY(localFileUri.Query().IsEmpty());

    VERIFY(simpleHttpUri.Scheme() == "http");
    VERIFY(simpleHttpUri.UserInfo().IsEmpty());
    VERIFY(simpleHttpUri.Host() == "www.radonlabs.de");
    VERIFY(simpleHttpUri.Port().IsEmpty());
    VERIFY(simpleHttpUri.LocalPath() == "index.html");
    VERIFY(simpleHttpUri.Fragment() == "main");
    VERIFY(simpleHttpUri.Query().IsEmpty());

    VERIFY(complexHttpUri.Scheme() == "http");
    VERIFY(complexHttpUri.UserInfo() == "floh");
    VERIFY(complexHttpUri.Host() == "www.radonlabs.de");
    VERIFY(complexHttpUri.Port() == "1088");
    VERIFY(complexHttpUri.LocalPath() == "query.php");
    VERIFY(complexHttpUri.Fragment() == "");
    VERIFY(complexHttpUri.Query() == "who=bla;where=blub");
}

}; // namespace Test
