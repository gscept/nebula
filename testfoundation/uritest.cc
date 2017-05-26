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
    this->Verify(!remoteFileUri.IsEmpty());
    this->Verify(remoteFileUri.Scheme() == "file");
    this->Verify(remoteFileUri.UserInfo().IsEmpty());
    this->Verify(remoteFileUri.Host() == "gambar");
    this->Verify(remoteFileUri.Port().IsEmpty());
    this->Verify(remoteFileUri.LocalPath() == "temp/bla.txt");
    this->Verify(remoteFileUri.Fragment().IsEmpty());
    this->Verify(remoteFileUri.Query().IsEmpty());
    this->Verify(remoteFileUri.AsString() == "file://gambar/temp/bla.txt");

    this->Verify(localFileUri.Scheme() == "file");
    this->Verify(localFileUri.UserInfo().IsEmpty());
    this->Verify(localFileUri.Host().IsEmpty());
    this->Verify(localFileUri.Port().IsEmpty());
    this->Verify(localFileUri.LocalPath() == "temp/blub.txt");
    this->Verify(localFileUri.Fragment().IsEmpty());
    this->Verify(localFileUri.Query().IsEmpty());

    this->Verify(simpleHttpUri.Scheme() == "http");
    this->Verify(simpleHttpUri.UserInfo().IsEmpty());
    this->Verify(simpleHttpUri.Host() == "www.radonlabs.de");
    this->Verify(simpleHttpUri.Port().IsEmpty());
    this->Verify(simpleHttpUri.LocalPath() == "index.html");
    this->Verify(simpleHttpUri.Fragment() == "main");
    this->Verify(simpleHttpUri.Query().IsEmpty());

    this->Verify(complexHttpUri.Scheme() == "http");
    this->Verify(complexHttpUri.UserInfo() == "floh");
    this->Verify(complexHttpUri.Host() == "www.radonlabs.de");
    this->Verify(complexHttpUri.Port() == "1088");
    this->Verify(complexHttpUri.LocalPath() == "query.php");
    this->Verify(complexHttpUri.Fragment() == "");
    this->Verify(complexHttpUri.Query() == "who=bla;where=blub");
}

}; // namespace Test
