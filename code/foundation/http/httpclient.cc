//------------------------------------------------------------------------------
//  httpclient.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"

// HttpClient is not implemented on the Wii
#if __NEBULA3_HTTP_FILESYSTEM__
#include "http/httpclient.h"
#include "http/httprequestwriter.h"
#include "http/httpresponsereader.h"

namespace Http
{
__ImplementClass(Http::HttpClient, 'HTCL', Core::RefCounted);

using namespace Util;
using namespace IO;
using namespace Net;

//------------------------------------------------------------------------------
/**
*/
HttpClient::HttpClient() :
    userAgent("Mozilla")    // NOTE: web browser are picky about user agent strings, so use something common
{
    this->tcpClient = TcpClient::Create();
    this->tcpClient->SetBlocking(true);
}

//------------------------------------------------------------------------------
/**
*/
HttpClient::~HttpClient()
{
    if (this->IsConnected())
    {
        this->Disconnect();
    }
    this->tcpClient = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
bool
HttpClient::Connect(const URI& uri)
{
    n_assert(!this->IsConnected());
    n_assert(this->tcpClient->IsBlocking());

    // not connected yet, setup connection through HTTP server
    IpAddress ipAddress(uri);
    if (ipAddress.GetPort() == 0)
    {
        ipAddress.SetPort(80);
    }
    this->tcpClient->SetServerAddress(ipAddress);
    TcpClient::Result result = this->tcpClient->Connect();
    n_assert(result != TcpClient::Connecting);
    return (TcpClient::Success == result);
}

//------------------------------------------------------------------------------
/**
*/
void
HttpClient::Disconnect()
{
    if (this->IsConnected())
    {
        this->tcpClient->Disconnect();
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
HttpClient::IsConnected() const
{
    return this->tcpClient->IsConnected();
}

//------------------------------------------------------------------------------
/**
*/
HttpStatus::Code 
HttpClient::SendRequest(HttpMethod::Code requestMethod, const URI& uri, const Ptr<Stream>& responseContentStream)
{
    n_assert(this->IsConnected());

    // write a proper HTTP request to the send stream
    Ptr<HttpRequestWriter> httpRequestWriter = HttpRequestWriter::Create();
    httpRequestWriter->SetMethod(requestMethod);
    httpRequestWriter->SetURI(uri);
    httpRequestWriter->SetUserAgent(this->userAgent);
    httpRequestWriter->SetStream(this->tcpClient->GetSendStream());
    if (httpRequestWriter->Open())
    {
        httpRequestWriter->WriteRequestHeader();
        httpRequestWriter->Close();
    }
    else
    {
        return HttpStatus::ServiceUnavailable;
    }

    // send off the http request, and wait for the response
    if (!this->tcpClient->Send())
    {
        // send failed
        return HttpStatus::ServiceUnavailable;
    }

    // wait for the response (we're working in blocking mode)
    if (!this->tcpClient->Recv())
    {
        // receive failed
        return HttpStatus::ServiceUnavailable;
    }

    // decode the HTTP response from the receive stream
    Ptr<HttpResponseReader> httpResponseReader = HttpResponseReader::Create();
    httpResponseReader->SetStream(this->tcpClient->GetRecvStream());
    if (httpResponseReader->Open())
    {
        httpResponseReader->ReadResponse();
        if (httpResponseReader->IsValidHttpResponse())
        {
            if (HttpStatus::OK == httpResponseReader->GetStatusCode())
            {
                SizeT contentLength = httpResponseReader->GetContentLength();
                if (contentLength > 0)
                {
                    uchar* buf = (uchar*) Memory::Alloc(Memory::ScratchHeap, contentLength);
                    SizeT bufPos = 0;
                    SizeT bytesToRead = contentLength;
                    bool done = false;
                    while (!done)
                    {
                        const Ptr<Stream>& recvStream = this->tcpClient->GetRecvStream();
                        SizeT bytesRead = recvStream->Read(&(buf[bufPos]), bytesToRead);
                        bytesToRead -= bytesRead;
                        bufPos += bytesRead;
                        if (bytesToRead > 0)
                        {
                            recvStream->Close();
                            this->tcpClient->Recv();
                            recvStream->Open();
                        }
                        else
                        {
                            done = true;
                        }
                    }
                    if (responseContentStream->IsOpen())
                    {
                        // if stream is already open, simply append the read data
                        responseContentStream->Write(buf, contentLength);
                    }
                    else
                    {
                        // response content stream not open, so open -> write -> close
                        responseContentStream->SetAccessMode(Stream::WriteAccess);
                        if (responseContentStream->Open())
                        {
                            responseContentStream->Write(buf, contentLength);
                            responseContentStream->Close();
                        }
                        else
                        {
                            n_error("HttpClient: Failed to open '%s' for writing!", uri.AsString().AsCharPtr());
                            return HttpStatus::ServiceUnavailable;
                        }
                    }
                    Memory::Free(Memory::ScratchHeap, buf);
                }
            }
        }
        httpResponseReader->Close();
        return httpResponseReader->GetStatusCode();
    }
    // fallthrough: error
    return HttpStatus::ServiceUnavailable;
}

//------------------------------------------------------------------------------
/**
*/
HttpStatus::Code
HttpClient::SendRequest(HttpMethod::Code requestMethod, const IO::URI& uri, const Util::String & body, const Ptr<IO::Stream>& responseContentStream)
{
	n_assert(this->IsConnected());

	// write a proper HTTP request to the send stream
	Ptr<HttpRequestWriter> httpRequestWriter = HttpRequestWriter::Create();
	httpRequestWriter->SetMethod(requestMethod);
	httpRequestWriter->SetURI(uri);
	httpRequestWriter->SetUserAgent(this->userAgent);
	httpRequestWriter->SetStream(this->tcpClient->GetSendStream());
	if (httpRequestWriter->Open())
	{
		httpRequestWriter->WriteRequestHeaderWithBody(body);
		httpRequestWriter->Close();
	}
	else
	{
		return HttpStatus::ServiceUnavailable;
	}

	// send off the http request, and wait for the response
	if (!this->tcpClient->Send())
	{
		// send failed
		return HttpStatus::ServiceUnavailable;
	}

	// wait for the response (we're working in blocking mode)
	if (!this->tcpClient->Recv())
	{
		// receive failed
		return HttpStatus::ServiceUnavailable;
	}

	// decode the HTTP response from the receive stream
	Ptr<HttpResponseReader> httpResponseReader = HttpResponseReader::Create();
	httpResponseReader->SetStream(this->tcpClient->GetRecvStream());
	if (httpResponseReader->Open())
	{
		httpResponseReader->ReadResponse();
		if (httpResponseReader->IsValidHttpResponse())
		{
			if (HttpStatus::OK == httpResponseReader->GetStatusCode())
			{
				SizeT contentLength = httpResponseReader->GetContentLength();
				if (contentLength > 0)
				{
					uchar* buf = (uchar*)Memory::Alloc(Memory::ScratchHeap, contentLength);
					SizeT bufPos = 0;
					SizeT bytesToRead = contentLength;
					bool done = false;
					while (!done)
					{
						const Ptr<Stream>& recvStream = this->tcpClient->GetRecvStream();
						SizeT bytesRead = recvStream->Read(&(buf[bufPos]), bytesToRead);
						bytesToRead -= bytesRead;
						bufPos += bytesRead;
						if (bytesToRead > 0)
						{
							recvStream->Close();
							this->tcpClient->Recv();
							recvStream->Open();
						}
						else
						{
							done = true;
						}
					}
					if (responseContentStream->IsOpen())
					{
						// if stream is already open, simply append the read data
						responseContentStream->Write(buf, contentLength);
					}
					else
					{
						// response content stream not open, so open -> write -> close
						responseContentStream->SetAccessMode(Stream::WriteAccess);
						if (responseContentStream->Open())
						{
							responseContentStream->Write(buf, contentLength);
							responseContentStream->Close();
						}
						else
						{
							n_error("HttpClient: Failed to open '%s' for writing!", uri.AsString().AsCharPtr());
							return HttpStatus::ServiceUnavailable;
						}
					}
					Memory::Free(Memory::ScratchHeap, buf);
				}
			}
		}
		httpResponseReader->Close();
		return httpResponseReader->GetStatusCode();
	}
	// fallthrough: error
	return HttpStatus::ServiceUnavailable;
}
} // namespace Http
#endif // __WII__