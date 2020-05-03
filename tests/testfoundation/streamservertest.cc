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

    VERIFY(schemeRegistry->IsUriSchemeRegistered("file"));
    VERIFY(schemeRegistry->IsUriSchemeRegistered("memory"));
    VERIFY(!schemeRegistry->IsUriSchemeRegistered("ftp"));
    VERIFY(&schemeRegistry->GetStreamClassByUriScheme("file") == &FileStream::RTTI);
    VERIFY(&schemeRegistry->GetStreamClassByUriScheme("memory") == &MemoryStream::RTTI);

    URI absFileURI("file:///c:/temp");
    URI relFileURI("temp/bla.txt");
    URI memURI("memory:///test");
    Ptr<Stream> absFileStream = ioServer->CreateStream(absFileURI);
    Ptr<Stream> relFileStream = ioServer->CreateStream(relFileURI);
    Ptr<Stream> memStream = ioServer->CreateStream(memURI);
    VERIFY(absFileStream->IsInstanceOf(FileStream::RTTI));
    VERIFY(relFileStream->IsInstanceOf(FileStream::RTTI));
    VERIFY(memStream->IsInstanceOf(MemoryStream::RTTI));
    VERIFY(absFileStream->GetURI() == absFileURI);
    VERIFY(relFileStream->GetURI() == relFileURI);
    VERIFY(memStream->GetURI() == memURI);
}

} // namespace Test