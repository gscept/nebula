//------------------------------------------------------------------------------
//  memorystreamtest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "memorystreamtest.h"
#include "io/memorystream.h"
#include "io/binarywriter.h"
#include "io/binaryreader.h"

namespace Test
{
__ImplementClass(Test::MemoryStreamTest, 'MSST', Test::TestCase);

using namespace Util;
using namespace Math;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
void
MemoryStreamTest::Run()
{
    // create a memory stream object and attach a binary writer 
    Ptr<MemoryStream> stream = MemoryStream::Create();
    Ptr<BinaryReader> reader = BinaryReader::Create();
    Ptr<BinaryWriter> writer = BinaryWriter::Create();
    reader->SetStream(stream.upcast<Stream>());
    writer->SetStream(stream.upcast<Stream>());

    // write some data to the stream
    stream->SetAccessMode(Stream::WriteAccess);
    VERIFY(stream->Open());
    VERIFY(writer->Open());
    VERIFY(writer->IsOpen());
    VERIFY(stream->IsOpen());
    writer->WriteChar('a');
    VERIFY(stream->GetSize() == 1);
    VERIFY(stream->GetPosition() == 1);
    writer->WriteShort(12);
    VERIFY(stream->GetSize() == 3);
    VERIFY(stream->GetPosition() == 3);
    writer->WriteInt(123);
    VERIFY(stream->GetSize() == 7);
    VERIFY(stream->GetPosition() == 7);
    writer->WriteString("This is a string.");
    VERIFY(stream->GetSize() == 26);
    VERIFY(stream->GetPosition() == 26);
    writer->WriteFloat(12.3f);
    VERIFY(stream->GetSize() == 30);
    VERIFY(stream->GetPosition() == 30);
    writer->WriteDouble(23.4);
    VERIFY(stream->GetSize() == 38);
    VERIFY(stream->GetPosition() == 38);
    writer->WriteVec4(vec4(1.0f, 2.0f, 3.0f, 4.0f));
    VERIFY(stream->GetSize() == 54);
    VERIFY(stream->GetPosition() == 54);
    writer->WriteMat4(mat4());
    VERIFY(stream->GetSize() == 118);
    VERIFY(stream->GetPosition() == 118);
    VERIFY(stream->Eof());
    writer->Close();
    stream->Close();
    
    stream->SetAccessMode(Stream::ReadAccess);
    // do some seeking
    stream->Open();
    stream->Seek(32, Stream::Begin);
    VERIFY(stream->GetPosition() == 32);
    stream->Seek(-4, Stream::Current);
    VERIFY(!stream->Eof());
    VERIFY(stream->GetPosition() == 28);
    stream->Seek(8, Stream::Current);
    VERIFY(stream->GetPosition() == 36);
    stream->Seek(-4, Stream::End);
    VERIFY(stream->GetPosition() == 114);
    stream->Seek(4, Stream::Current);
    VERIFY(stream->GetPosition() == 118);

    // stream past the beginning and end
    stream->Seek(-10, Stream::Begin);
    VERIFY(stream->GetPosition() == 0);
    stream->Seek(4000, Stream::Begin);
    VERIFY(stream->GetPosition() == 118);
    VERIFY(stream->Eof());

    // close the stream
    stream->Close();
    VERIFY(!stream->IsOpen());
    
    // open the stream for reading, and read back written data
    stream->SetAccessMode(Stream::ReadAccess);
    VERIFY(stream->Open());
    VERIFY(reader->Open());
    VERIFY('a' == reader->ReadChar());
    VERIFY(12 == reader->ReadShort());
    VERIFY(123 == reader->ReadInt());
    VERIFY("This is a string." == reader->ReadString());
    VERIFY(12.3f == reader->ReadFloat());
    VERIFY(23.4 == reader->ReadDouble());
    VERIFY(vec4(1.0f, 2.0f, 3.0f, 4.0f) == reader->ReadVec4());
    VERIFY(mat4() == reader->ReadMat4());

    // check if seeking and reading works...
    stream->Seek(26, Stream::Begin);
    VERIFY(12.3f == reader->ReadFloat());

    // close and quit
    reader->Close();
    stream->Close();
    VERIFY(!stream->IsOpen());
};

}; // namespace Test


