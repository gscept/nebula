//------------------------------------------------------------------------------
//  zipfstest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "zipfstest.h"
#include "io/ioserver.h"
#include "io/memorystream.h"

namespace Test
{
__ImplementClass(Test::ZipFSTest, 'ZPTS', Test::TestCase);

using namespace IO;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
ZipFSTest::Run()
{
    Ptr<IoServer> ioServer = IoServer::Create();
    ioServer->MountStandardArchives();

    // open a file, read its data and write it into a temp file
    Ptr<Stream> stream = ioServer->CreateStream("export:data/tables/blueprints.xml");
    stream->SetAccessMode(Stream::ReadAccess);
    if (stream->Open())
    {
        Stream::Size size = stream->GetSize();
        void* buf = Memory::Alloc(Memory::ScratchHeap, size);
        stream->Read(buf, size);
        stream->Close();

        Ptr<Stream> outFile = MemoryStream::Create();
        outFile->SetAccessMode(Stream::WriteAccess);
        if (outFile->Open())
        {
            outFile->Write(buf, size);
            outFile->Close();
        }
        Memory::Free(Memory::ScratchHeap, buf);

        Array<String> files = ioServer->ListFiles("export:data/tables", "*");
        this->Verify(files.Size() == 5);
        this->Verify(0 != files.Find("anims.xml"));
        this->Verify(0 != files.Find("blueprints.xml"));
        this->Verify(0 != files.Find("globals.xml"));
        this->Verify(0 != files.Find("materials.xml"));
    }
}

}