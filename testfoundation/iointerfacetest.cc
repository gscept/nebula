//------------------------------------------------------------------------------
//  iointerfacetest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "iointerfacetest.h"
#include "io/iointerface.h"
#include "io/memorystream.h"

namespace Test
{
__ImplementClass(Test::IOInterfaceTest, 'IOIT', Test::TestCase);

using namespace Interface;
using namespace IO;
using namespace Messaging;

//------------------------------------------------------------------------------
/**
*/
void
IOInterfaceTest::Run()
{
    Ptr<IoServer> ioServer = IoServer::Create();
    Ptr<IoInterface> iface = IoInterface::Create();
    iface->Open();

    #if __WIN32__
    URI uri("temp:largefile.test");
    URI copyUri("temp:copyfile.test");
    const SizeT fileSize = 20000000;

    // first do some cleanup...
    if (ioServer->FileExists(uri))
    {
        ioServer->DeleteFile(uri);
    }
    if (ioServer->FileExists(copyUri))
    {
        ioServer->DeleteFile(copyUri);
    }
    this->Verify(!ioServer->FileExists(uri));
    this->Verify(!ioServer->FileExists(copyUri));

    // write a large file (100 MB)
    n_printf("Writing large file...\n");
    Ptr<MemoryStream> writeStream = MemoryStream::Create();
    writeStream->SetAccessMode(Stream::WriteAccess);
    writeStream->SetSize(fileSize);
    if (writeStream->Open())
    {
        void* ptr = writeStream->Map();
        Memory::Clear(ptr, writeStream->GetSize());
        writeStream->Unmap();
        writeStream->Close();
    }
    Ptr<WriteStream> writeStreamMsg = WriteStream::Create();
    writeStreamMsg->SetURI(uri);
    writeStreamMsg->SetStream(writeStream.upcast<Stream>());
    iface->Send(writeStreamMsg.upcast<Message>());

    // do some work while waiting for the write to finish
    int counter = 0;
    while (!iface->Peek(writeStreamMsg.upcast<Message>()))
    {
        counter++;
    }
    n_printf("File written, idle counter: %d\n", counter);
    this->Verify(writeStreamMsg->Handled());
    this->Verify(ioServer->FileExists(uri));

    // read back the written data into a new stream
    Ptr<MemoryStream> readStream = MemoryStream::Create();
    Ptr<ReadStream> readStreamMsg = ReadStream::Create();
    readStreamMsg->SetURI(uri);
    readStreamMsg->SetStream(readStream.upcast<Stream>());
    iface->Send(readStreamMsg.upcast<Message>());

    // do some work while waiting for the result
    counter = 0;
    while (!iface->Peek(readStreamMsg.upcast<Message>()))
    {
        counter++;
    }
    n_printf("File read, idle counter: %d\n", counter);
    this->Verify(readStreamMsg->Handled());
    this->Verify(readStream->GetSize() == fileSize);

    // asynchronously copy the file
    Ptr<CopyFile> copyMsg = CopyFile::Create();
    copyMsg->SetFromURI(uri);
    copyMsg->SetToURI(copyUri);
    iface->Send(copyMsg.upcast<Message>());

    // do some work while waiting for the result
    counter = 0;
    while (!iface->Peek(copyMsg.upcast<Message>()))
    {
        counter++;
    }
    n_printf("File copied, idle counter: %d\n", counter);
    this->Verify(copyMsg->Handled());
    this->Verify(ioServer->FileExists(copyUri));

    // asynchronously delete the files
    Ptr<DeleteFile> delMsg0 = DeleteFile::Create();
    delMsg0->SetURI(uri);
    Ptr<DeleteFile> delMsg1 = DeleteFile::Create();
    delMsg1->SetURI(copyUri);
    iface->Send(delMsg0.upcast<Message>());
    iface->Send(delMsg1.upcast<Message>());

    // do some work while waiting for the result
    counter = 0;
    while (!(iface->Peek(delMsg0.upcast<Message>()) && iface->Peek(delMsg1.upcast<Message>())))
    {
        counter++;
    }
    n_printf("2 Files deleted, idle counter: %d\n", counter);
    this->Verify(delMsg0->Handled());
    this->Verify(delMsg1->Handled());
    this->Verify(!ioServer->FileExists(uri));
    this->Verify(!ioServer->FileExists(copyUri));
    #endif

    iface->Close();
}

} // namespace Test
