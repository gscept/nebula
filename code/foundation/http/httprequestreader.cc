//------------------------------------------------------------------------------
//  httprequestreader.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "http/httprequestreader.h"
#include "io/textreader.h"

namespace Http
{
__ImplementClass(Http::HttpRequestReader, 'HRQR', IO::StreamReader);

using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
HttpRequestReader::HttpRequestReader() :
    isValidHttpRequest(false),
    httpMethod(HttpMethod::InvalidHttpMethod)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
HttpRequestReader::ReadRequest()
{
    this->isValidHttpRequest = false;
        
    // attach a text reader to our stream and parse the request header
    Ptr<TextReader> textReader = TextReader::Create();
    textReader->SetStream(this->stream);
    if (textReader->Open())
    {
        // read the first line of the request
        // should be "METHOD PATH HTTP/1.1[CRLF]
        String headLine =  textReader->ReadLine();
        Array<String> headTokens = headLine.Tokenize(" ");
        if (headTokens.Size() != 3)
        {
            // malformed request header
            textReader->Close();
            return false;
        }

        // check if the HTTP version string looks alright
        if (!String::MatchPattern(headTokens[2], "HTTP/*"))
        {
            // malformed request header
            textReader->Close();
            return false;
        }

        // decode the HTTP method
        this->httpMethod = HttpMethod::FromString(headTokens[0]);

        // decode the remaining request header lines
        String host;
        bool endOfHeader = false;
        while (!textReader->Eof() && !endOfHeader)
        {
            String curLine = textReader->ReadLine();
            if (curLine.IsValid())
            {
                if (String::MatchPattern(curLine, "Host: *"))
                {
                    host = curLine;
                    host.SubstituteString("Host: ", "");
                }
            }
            else
            {
                endOfHeader = true;
            }
        }

        // build URI
        String uriString;
        uriString.Format("http://%s%s", host.AsCharPtr(), headTokens[1].AsCharPtr());
        this->requestURI = uriString;

        this->isValidHttpRequest = true;
        textReader->Close();
        return true;
    }
    return false;
}

} // namespace Http