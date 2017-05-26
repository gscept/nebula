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
    this->Verify(stream->Open());
    this->Verify(writer->Open());
    this->Verify(writer->IsOpen());
    this->Verify(stream->IsOpen());
    writer->WriteChar('a');
    this->Verify(stream->GetSize() == 1);
    this->Verify(stream->GetPosition() == 1);
    writer->WriteShort(12);
    this->Verify(stream->GetSize() == 3);
    this->Verify(stream->GetPosition() == 3);
    writer->WriteInt(123);
    this->Verify(stream->GetSize() == 7);
    this->Verify(stream->GetPosition() == 7);
    writer->WriteString("This is a string.");
    this->Verify(stream->GetSize() == 26);
    this->Verify(stream->GetPosition() == 26);
    writer->WriteFloat(12.3f);
    this->Verify(stream->GetSize() == 30);
    this->Verify(stream->GetPosition() == 30);
    writer->WriteDouble(23.4);
    this->Verify(stream->GetSize() == 38);
    this->Verify(stream->GetPosition() == 38);
    writer->WriteFloat4(float4(1.0f, 2.0f, 3.0f, 4.0f));
    this->Verify(stream->GetSize() == 54);
    this->Verify(stream->GetPosition() == 54);
    writer->WriteMatrix44(matrix44::identity());
    this->Verify(stream->GetSize() == 118);
    this->Verify(stream->GetPosition() == 118);
    this->Verify(stream->Eof());
    writer->Close();
    stream->Close();
    
    stream->SetAccessMode(Stream::ReadAccess);
    // do some seeking
    stream->Open();
    stream->Seek(32, Stream::Begin);
    this->Verify(stream->GetPosition() == 32);
    stream->Seek(-4, Stream::Current);
    this->Verify(!stream->Eof());
    this->Verify(stream->GetPosition() == 28);
    stream->Seek(8, Stream::Current);
    this->Verify(stream->GetPosition() == 36);
    stream->Seek(-4, Stream::End);
    this->Verify(stream->GetPosition() == 114);
    stream->Seek(4, Stream::Current);
    this->Verify(stream->GetPosition() == 118);

    // stream past the beginning and end
    stream->Seek(-10, Stream::Begin);
    this->Verify(stream->GetPosition() == 0);
    stream->Seek(4000, Stream::Begin);
    this->Verify(stream->GetPosition() == 118);
    this->Verify(stream->Eof());

    // close the stream
    stream->Close();
    this->Verify(!stream->IsOpen());
    
    // open the stream for reading, and read back written data
    stream->SetAccessMode(Stream::ReadAccess);
    this->Verify(stream->Open());
    this->Verify(reader->Open());
    this->Verify('a' == reader->ReadChar());
    this->Verify(12 == reader->ReadShort());
    this->Verify(123 == reader->ReadInt());
    this->Verify("This is a string." == reader->ReadString());
    this->Verify(12.3f == reader->ReadFloat());
    this->Verify(23.4 == reader->ReadDouble());
    this->Verify(float4(1.0f, 2.0f, 3.0f, 4.0f) == reader->ReadFloat4());
    this->Verify(matrix44::identity() == reader->ReadMatrix44());

    // check if seeking and reading works...
    stream->Seek(26, Stream::Begin);
    this->Verify(12.3f == reader->ReadFloat());

    // close and quit
    reader->Close();
    stream->Close();
    this->Verify(!stream->IsOpen());
};

}; // namespace Test


