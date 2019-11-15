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
    URI serverUri("http://github.com");
    
    bool connected = httpClient->Connect(serverUri);
    VERIFY(connected);
    VERIFY(httpClient->IsConnected());
    if (connected)
    {
        /*
        URI imgUri("http://old.gscept.com/themes/site_themes/gscept/gimage/logo.png"); 
        HttpStatus::Code status;
        Ptr<Stream> contentStream = MemoryStream::Create();
        status = httpClient->SendRequest(HttpMethod::Get, imgUri, contentStream);
        VERIFY(HttpStatus::OK == status);
        VERIFY(contentStream->GetSize() == 5646);
        */

        httpClient->Disconnect();
        VERIFY(!httpClient->IsConnected());
    }
}

} // namespace Test
