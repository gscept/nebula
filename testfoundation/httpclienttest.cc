//------------------------------------------------------------------------------
//  httpclienttest.cc
//  (C) 2009 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "httpclienttest.h"
#include "http/httpclient.h"
#include "io/memorystream.h"
#include "io/ioserver.h"

namespace Test
{
__ImplementClass(Test::HttpClientTest, 'htct', Test::TestCase);

using namespace Util;
using namespace IO;
using namespace Http;

//------------------------------------------------------------------------------
/**
*/
void
HttpClientTest::Run()
{
    Ptr<IoServer> ioServer = IoServer::Create();
    Ptr<HttpClient> httpClient = HttpClient::Create();

    // build a few URIs we're interested in
    URI serverUri("http://old.gscept.com");
    URI imgUri("http://old.gscept.com/themes/site_themes/gscept/gimage/logo.png");

    bool connected = httpClient->Connect(serverUri);
    this->Verify(connected);
    this->Verify(httpClient->IsConnected());
    if (connected)
    {
        HttpStatus::Code status;
        Ptr<Stream> contentStream = MemoryStream::Create();
        status = httpClient->SendRequest(HttpMethod::Get, imgUri, contentStream);
        this->Verify(HttpStatus::OK == status);
        this->Verify(contentStream->GetSize() == 5646);

        httpClient->Disconnect();
        this->Verify(!httpClient->IsConnected());
    }
}

} // namespace Test
