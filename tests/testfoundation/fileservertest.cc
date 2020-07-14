//------------------------------------------------------------------------------
//  fileservertest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "fileservertest.h"
#include "io/ioserver.h"
#include "io/assign.h"
#include "io/filestream.h"
#include "io/binarywriter.h"
#include "io/binaryreader.h"
#include "core/ptr.h"

namespace Test
{
__ImplementClass(Test::FileServerTest, 'FSRT', Test::TestCase);

using namespace IO;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
FileServerTest::Run()
{
    Ptr<IoServer> ioServer = IoServer::Create();
    AssignRegistry* assignRegistry = AssignRegistry::Instance();

    // check for standard assigns
    VERIFY(assignRegistry->HasAssign("home"));
    VERIFY(!assignRegistry->HasAssign("test"));
    #if __WIN32__
    VERIFY(assignRegistry->HasAssign("user"));
    VERIFY(assignRegistry->HasAssign("bin"));
    VERIFY(assignRegistry->HasAssign("temp"));

    // add a new assign
    assignRegistry->SetAssign(Assign("test", "file:///c:/temp"));
    VERIFY(assignRegistry->HasAssign("test"));
    VERIFY(assignRegistry->GetAssign("test") == "file:///c:/temp");

    // test assign resolution
    VERIFY(assignRegistry->ResolveAssigns("test:bla.txt") == "file:///c:/temp/bla.txt");

    // set test assign to another location
    assignRegistry->SetAssign(Assign("test", "temp:"));
    VERIFY(assignRegistry->GetAssign("test") == "temp:");

    // create a test directory in test:
    VERIFY(ioServer->CreateDirectory("test:one/two"));
    assignRegistry->SetAssign(Assign("test", "temp:one/two"));
    VERIFY(ioServer->DirectoryExists("temp:one"));
    VERIFY(ioServer->DirectoryExists("temp:one/two"));
    VERIFY(!ioServer->DirectoryExists("temp:one/two/three"));
    VERIFY(!ioServer->FileExists("temp:one"));

    // create a file in the test directory
    Ptr<Stream> file = ioServer->CreateStream("test:file1");
    file->SetAccessMode(Stream::WriteAccess);
    VERIFY(file->Open());

    Ptr<BinaryWriter> writer = BinaryWriter::Create();
    writer->SetStream(file);
    VERIFY(writer->Open());
    writer->WriteChar('A');
    writer->WriteChar('B');
    writer->WriteChar('C');
    writer->WriteChar('\n');
    writer->Close();
    file->Close();

    // test if file exists
    VERIFY(ioServer->FileExists("test:file1"));
    VERIFY(!ioServer->FileExists("test:file2"));
    VERIFY(!ioServer->DirectoryExists("test:file1"));

    // copy file
    VERIFY(ioServer->CopyFile("test:file1", "test:file2"));
    VERIFY(ioServer->FileExists("test:file2"));
    VERIFY(ioServer->ComputeFileCrc("test:file1") == ioServer->ComputeFileCrc("test:file2"));
    
    // check copied contents
    file->SetURI("test:file2");
    file->SetAccessMode(Stream::ReadAccess);
    VERIFY(file->Open());

    Ptr<BinaryReader> reader = BinaryReader::Create();
    reader->SetStream(file);
    VERIFY(reader->Open());
    VERIFY(reader->ReadChar() == 'A');
    VERIFY(reader->ReadChar() == 'B');
    VERIFY(reader->ReadChar() == 'C');
    VERIFY(reader->ReadChar() == '\n');
    VERIFY(file->Eof());
    reader->Close();
    file->Close();

    VERIFY(!ioServer->IsReadOnly("test:file2"));
    ioServer->SetReadOnly("test:file2", true);
    VERIFY(ioServer->IsReadOnly("test:file2"));
    ioServer->SetReadOnly("test:file2", false);
    VERIFY(!ioServer->IsReadOnly("test:file2"));

    // test directory listing
    VERIFY(ioServer->CreateDirectory("temp:one/anotherdir"));
    Array<String> list = ioServer->ListFiles("temp:one/two", "*");
    VERIFY(list.Size() == 2);
    list = ioServer->ListDirectories("temp:one/two", "*");
    VERIFY(list.Size() == 0);
    list = ioServer->ListFiles("temp:one", "*");
    VERIFY(list.Size() == 0);
    list = ioServer->ListDirectories("temp:one", "*");
    VERIFY(list.Size() == 2);

    // test file deleting
    VERIFY(ioServer->DeleteFile("temp:one/two/file1"));
    VERIFY(!ioServer->FileExists("temp:one/two/file1"));
    VERIFY(!ioServer->DeleteDirectory("temp:one/two"));
    VERIFY(ioServer->DeleteFile("temp:one/two/file2"));
    VERIFY(ioServer->DeleteDirectory("temp:one/two"));
    VERIFY(!ioServer->DirectoryExists("temp:one/two"));
    VERIFY(ioServer->DeleteDirectory("temp:one/anotherdir"));
    VERIFY(ioServer->DeleteDirectory("temp:one"));
    #endif
}

}; // namespace Test
