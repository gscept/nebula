//------------------------------------------------------------------------------
//  streamservertest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "streamservertest.h"
#include "io/ioserver.h"
#include "io/filestream.h"
#include "io/memorystream.h"

namespace Test
{
__ImplementClass(Test::StreamServerTest, 'SSVT', Core::RefCounted);

using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
void
StreamServerTest::Run()
{
    Ptr<IoServer> ioServer = IoServer::Create();
    SchemeRegistry* schemeRegistry = SchemeRegistry::Instance();

    // register some classes with the stream server
    schemeRegistry->RegisterUriScheme("file", FileStream::RTTI);
    schemeRegistry->RegisterUriScheme("memory", MemoryStream::RTTI);

    this->Verify(schemeRegistry->IsUriSchemeRegistered("file"));
    this->Verify(schemeRegistry->IsUriSchemeRegistered("memory"));
    this->Verify(!schemeRegistry->IsUriSchemeRegistered("ftp"));
    this->Verify(&schemeRegistry->GetStreamClassByUriScheme("file") == &FileStream::RTTI);
    this->Verify(&schemeRegistry->GetStreamClassByUriScheme("memory") == &MemoryStream::RTTI);

    URI absFileURI("file:///c:/temp");
    URI relFileURI("temp/bla.txt");
    URI memURI("memory:///test");
    Ptr<Stream> absFileStream = ioServer->CreateStream(absFileURI);
    Ptr<Stream> relFileStream = ioServer->CreateStream(relFileURI);
    Ptr<Stream> memStream = ioServer->CreateStream(memURI);
    this->Verify(absFileStream->IsInstanceOf(FileStream::RTTI));
    this->Verify(relFileStream->IsInstanceOf(FileStream::RTTI));
    this->Verify(memStream->IsInstanceOf(MemoryStream::RTTI));
    this->Verify(absFileStream->GetURI() == absFileURI);
    this->Verify(relFileStream->GetURI() == relFileURI);
    this->Verify(memStream->GetURI() == memURI);
}

} // namespace Test