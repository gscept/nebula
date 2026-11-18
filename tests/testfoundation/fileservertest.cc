//------------------------------------------------------------------------------
//  fileservertest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "fileservertest.h"
#include "io/assignregistry.h"
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
    IO::StartupFilesystem({});
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
    VERIFY(IO::CreateDirectory("test:one/two"));
    assignRegistry->SetAssign(Assign("test", "temp:one/two"));
    VERIFY(IO::DirectoryExists("temp:one"));
    VERIFY(IO::DirectoryExists("temp:one/two"));
    VERIFY(!IO::DirectoryExists("temp:one/two/three"));
    VERIFY(!IO::FileExists("temp:one"));

    // create a file in the test directory
    Ptr<Stream> file = IO::CreateStream("test:file1");
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
    VERIFY(IO::FileExists("test:file1"));
    VERIFY(!IO::FileExists("test:file2"));
    VERIFY(!IO::DirectoryExists("test:file1"));

    // copy file
    VERIFY(IO::CopyFile("test:file1", "test:file2"));
    VERIFY(IO::FileExists("test:file2"));
    VERIFY(IO::ComputeFileCrc("test:file1") == IO::ComputeFileCrc("test:file2"));
    
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

    VERIFY(!IO::IsReadOnly("test:file2"));
    IO::SetReadOnly("test:file2", true);
    VERIFY(IO::IsReadOnly("test:file2"));
    IO::SetReadOnly("test:file2", false);
    VERIFY(!IO::IsReadOnly("test:file2"));

    // test directory listing
    VERIFY(IO::CreateDirectory("temp:one/anotherdir"));
    Array<String> list = IO::ListFiles("temp:one/two", "*");
    VERIFY(list.Size() == 2);
    list = IO::ListDirectories("temp:one/two", "*");
    VERIFY(list.Size() == 0);
    list = IO::ListFiles("temp:one", "*");
    VERIFY(list.Size() == 0);
    list = IO::ListDirectories("temp:one", "*");
    VERIFY(list.Size() == 2);

    // test file deleting
    VERIFY(IO::DeleteFile("temp:one/two/file1"));
    VERIFY(!IO::FileExists("temp:one/two/file1"));
    VERIFY(!IO::DeleteDirectory("temp:one/two"));
    VERIFY(IO::DeleteFile("temp:one/two/file2"));
    VERIFY(IO::DeleteDirectory("temp:one/two"));
    VERIFY(!IO::DirectoryExists("temp:one/two"));
    VERIFY(IO::DeleteDirectory("temp:one/anotherdir"));
    VERIFY(IO::DeleteDirectory("temp:one"));
    #endif
}

}; // namespace Test
