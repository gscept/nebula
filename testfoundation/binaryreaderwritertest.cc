//------------------------------------------------------------------------------
//  binaryreaderwritertest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "binaryreaderwritertest.h"
#include "io/memorystream.h"
#include "io/binaryreader.h"
#include "io/binarywriter.h"
#include "math/vector.h"

namespace Test
{
__ImplementClass(Test::BinaryReaderWriterTest, 'BRWT', Test::TestCase);

using namespace IO;
using namespace Util;
using namespace Math;

struct TestStruct
{
    bool b;
    int i;
    double d;
};

//------------------------------------------------------------------------------
/**
*/
void
BinaryReaderWriterTest::Run()
{
    TestStruct writeBlob;
    writeBlob.b = false;
    writeBlob.i = 345;
    writeBlob.d = 143.23;
    const matrix44 m44(float4(1.0f, 2.0f, 3.0f, 4.0f), 
                       float4(5.0f, 6.0f, 7.0f, 8.0f),
                       float4(9.0f, 10.0f, 11.0f, 12.0f),
                       float4(13.0f, 14.0f, 15.0f, 16.0f));
    const float4 v4(6.0f, 7.0f, 8.0f, 9.0f);

    // create and setup objects
    Ptr<MemoryStream> stream = MemoryStream::Create();
    Ptr<BinaryWriter> writer = BinaryWriter::Create();
    Ptr<BinaryReader> reader = BinaryReader::Create();

    // write some stuff to the stream without memory mapping
    stream->SetAccessMode(Stream::WriteAccess);
    this->Verify(stream->Open());
    writer->SetStream(stream.upcast<Stream>());
    writer->SetMemoryMappingEnabled(false);
    this->Verify(writer->Open());
    writer->WriteChar('a');
    writer->WriteUChar(uchar('ü'));
    writer->WriteShort(-12);
    writer->WriteUShort(13);
    writer->WriteInt(-123);
    writer->WriteUInt(1023);
    writer->WriteFloat(1.23f);
    writer->WriteDouble(2.34);
    writer->WriteBool(true);
    writer->WriteBool(false);
    writer->WriteString("Ein String");
    writer->WriteFloat4(v4);
    writer->WriteMatrix44(m44);
    writer->WriteBlob(Blob(&writeBlob, sizeof(writeBlob)));
    writer->Close();
    stream->Close();

    // read back with memory mapping enabled
    stream->SetAccessMode(Stream::ReadAccess);
    this->Verify(stream->Open());
    reader->SetStream(stream.upcast<Stream>());
    reader->SetMemoryMappingEnabled(true);
    this->Verify(reader->Open());
    this->Verify(reader->ReadChar() == 'a');
    this->Verify(reader->ReadUChar() == (unsigned char) 'ü');
    this->Verify(reader->ReadShort() == -12);
    this->Verify(reader->ReadUShort() == 13);
    this->Verify(reader->ReadInt() == -123);
    this->Verify(reader->ReadUInt() == 1023);
    this->Verify(reader->ReadFloat() == 1.23f);
    this->Verify(reader->ReadDouble() == 2.34);
    this->Verify(reader->ReadBool() == true);
    this->Verify(reader->ReadBool() == false);
    this->Verify(reader->ReadString() == "Ein String");
    this->Verify(reader->ReadFloat4() == v4);
    this->Verify(reader->ReadMatrix44() == m44);
    this->Verify(reader->ReadBlob() == Blob(&writeBlob, sizeof(writeBlob)));
    reader->Close();
    stream->Close();

    // now write stuff with memory mapping enabled
    stream->SetAccessMode(Stream::WriteAccess);
    this->Verify(stream->Open());
    writer->SetStream(stream.upcast<Stream>());
    writer->SetMemoryMappingEnabled(true);
    this->Verify(writer->Open());
    writer->WriteChar('a');
    writer->WriteUChar(uchar('ü'));
    writer->WriteShort(-12);
    writer->WriteUShort(13);
    writer->WriteInt(-123);
    writer->WriteUInt(1023);
    writer->WriteFloat(1.23f);
    writer->WriteDouble(2.34);
    writer->WriteBool(true);
    writer->WriteBool(false);
    writer->WriteString("Ein String");
    writer->WriteFloat4(v4);
    writer->WriteMatrix44(m44);
    writer->WriteBlob(Blob(&writeBlob, sizeof(TestStruct)));
    writer->Close();
    stream->Close();

    // and read back without memory mapping
    stream->SetAccessMode(Stream::ReadAccess);
    this->Verify(stream->Open());
    reader->SetStream(stream.upcast<Stream>());
    reader->SetMemoryMappingEnabled(false);
    this->Verify(reader->Open());
    this->Verify(reader->ReadChar() == 'a');
    this->Verify(reader->ReadUChar() == (unsigned char) 'ü');
    this->Verify(reader->ReadShort() == -12);
    this->Verify(reader->ReadUShort() == 13);
    this->Verify(reader->ReadInt() == -123);
    this->Verify(reader->ReadUInt() == 1023);
    this->Verify(reader->ReadFloat() == 1.23f);
    this->Verify(reader->ReadDouble() == 2.34);
    this->Verify(reader->ReadBool() == true);
    this->Verify(reader->ReadBool() == false);
    this->Verify(reader->ReadString() == "Ein String");
    this->Verify(reader->ReadFloat4() == v4);
    this->Verify(reader->ReadMatrix44() == m44);
    this->Verify(reader->ReadBlob() == Blob(&writeBlob, sizeof(writeBlob)));
    reader->Close();
    stream->Close();
}

} // namespace Test

