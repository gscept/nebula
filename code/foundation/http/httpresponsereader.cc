//------------------------------------------------------------------------------
//  httpresponsereader.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "http/httpresponsereader.h"
#include "io/textreader.h"
#include "io/memorystream.h"

namespace Http
{
__ImplementClass(Http::HttpResponseReader, 'HTRR', IO::StreamReader);

using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
HttpResponseReader::HttpResponseReader() :
    isValidHttpResponse(false),
    httpStatus(HttpStatus::InvalidHttpStatus),
    contentLength(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
HttpResponseReader::ReadResponse()
{
    this->isValidHttpResponse = false;
    this->httpStatus = HttpStatus::InvalidHttpStatus;
    this->contentLength = 0;
    this->contentType.Clear();
    bool retval = false;
        
    // attach a text reader to our stream and parse the response header
    Ptr<TextReader> textReader = TextReader::Create();
    textReader->SetStream(this->stream);
    if (textReader->Open())
    {
        // read first line, should be "HTTP/x.x STATUSCODE STATUSSTRING"
        String headLine = textReader->ReadLine();
        Array<String> headTokens = headLine.Tokenize(" ");
        if (headTokens.Size() < 3)
        {
            // malformed response header
            return false;
        }

        // check if the HTTP version string looks alright
        if (!String::MatchPattern(headTokens[0], "HTTP/*"))
        {
            // malformed request header
            return false;
        }

        // decode the status code
        this->httpStatus = HttpStatus::FromString(headTokens[1]);
        if (HttpStatus::OK == this->httpStatus)
        {
            // decode the remaining request header lines
            bool endOfHeader = false;
            while (!textReader->Eof() && !endOfHeader)
            {
                String curLine = textReader->ReadLine();
                Array<String> tokens;
                curLine.TrimRight("\r");
                if (curLine.IsValid())
                {
                    if (String::MatchPattern(curLine, "Content-Length: *"))
                    {
                        // read content length
                        tokens = curLine.Tokenize(" :");
                        n_assert(tokens.Size() == 2);
                        this->contentLength = tokens[1].AsInt();
                    }
                    else if (String::MatchPattern(curLine, "Content-Type: *"))
                    {
                        // read content MIME type
                        tokens = curLine.Tokenize(" :");
                        n_assert(tokens.Size() == 2);
                        this->contentType.Set(tokens[1]);
                    }
                }
                else
                {
                    endOfHeader = true;
                }
            }
            retval = true;
        }
        this->isValidHttpResponse = true;
        textReader->Close();
    }
    return retval;
}

} // namespace Http