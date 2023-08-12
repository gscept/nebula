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
    // make valgrind happy
    Memory::Clear(&writeBlob, sizeof(TestStruct));
    writeBlob.b = false;
    writeBlob.i = 345;
    writeBlob.d = 143.23;
    Util::Blob testBlob(&writeBlob, sizeof(TestStruct));
    const mat4 m44(vec4(1.0f, 2.0f, 3.0f, 4.0f),
        vec4(5.0f, 6.0f, 7.0f, 8.0f),
        vec4(9.0f, 10.0f, 11.0f, 12.0f),
        vec4(13.0f, 14.0f, 15.0f, 16.0f));
    const vec4 v4(6.0f, 7.0f, 8.0f, 9.0f);

    // create and setup objects
    Ptr<MemoryStream> stream = MemoryStream::Create();
    Ptr<BinaryWriter> writer = BinaryWriter::Create();
    Ptr<BinaryReader> reader = BinaryReader::Create();

    // write some stuff to the stream without memory mapping
    stream->SetAccessMode(Stream::WriteAccess);
    VERIFY(stream->Open());
    writer->SetStream(stream.upcast<Stream>());
    writer->SetMemoryMappingEnabled(false);
    VERIFY(writer->Open());
    writer->WriteChar('a');
    writer->WriteUChar(uchar(150));
    writer->WriteShort(-12);
    writer->WriteUShort(13);
    writer->WriteInt(-123);
    writer->WriteUInt(1023);
    writer->WriteFloat(1.23f);
    writer->WriteDouble(2.34);
    writer->WriteBool(true);
    writer->WriteBool(false);
    writer->WriteString("Ein String");
    writer->WriteVec4(v4);
    writer->WriteMat4(m44);
    writer->WriteBlob(testBlob);
    writer->Close();
    stream->Close();

    // read back with memory mapping enabled
    stream->SetAccessMode(Stream::ReadAccess);
    VERIFY(stream->Open());
    reader->SetStream(stream.upcast<Stream>());
    reader->SetMemoryMappingEnabled(true);
    VERIFY(reader->Open());
    VERIFY(reader->ReadChar() == 'a');
    VERIFY(reader->ReadUChar() == (unsigned char) 150);
    VERIFY(reader->ReadShort() == -12);
    VERIFY(reader->ReadUShort() == 13);
    VERIFY(reader->ReadInt() == -123);
    VERIFY(reader->ReadUInt() == 1023);
    VERIFY(reader->ReadFloat() == 1.23f);
    VERIFY(reader->ReadDouble() == 2.34);
    VERIFY(reader->ReadBool() == true);
    VERIFY(reader->ReadBool() == false);
    VERIFY(reader->ReadString() == "Ein String");
    VERIFY(reader->ReadVec4() == v4);
    VERIFY(reader->ReadMat4() == m44);
    VERIFY(reader->ReadBlob() == testBlob);
    reader->Close();
    stream->Close();

    // now write stuff with memory mapping enabled
    stream->SetAccessMode(Stream::WriteAccess);
    VERIFY(stream->Open());
    writer->SetStream(stream.upcast<Stream>());
    writer->SetMemoryMappingEnabled(true);
    VERIFY(writer->Open());
    writer->WriteChar('a');
    writer->WriteUChar(uchar(150));
    writer->WriteShort(-12);
    writer->WriteUShort(13);
    writer->WriteInt(-123);
    writer->WriteUInt(1023);
    writer->WriteFloat(1.23f);
    writer->WriteDouble(2.34);
    writer->WriteBool(true);
    writer->WriteBool(false);
    writer->WriteString("Ein String");
    writer->WriteVec4(v4);
    writer->WriteMat4(m44);
    writer->WriteBlob(testBlob);
    writer->Close();
    stream->Close();

    // and read back without memory mapping
    stream->SetAccessMode(Stream::ReadAccess);
    VERIFY(stream->Open());
    reader->SetStream(stream.upcast<Stream>());
    reader->SetMemoryMappingEnabled(false);
    VERIFY(reader->Open());
    VERIFY(reader->ReadChar() == 'a');
    VERIFY(reader->ReadUChar() == (unsigned char) 150);
    VERIFY(reader->ReadShort() == -12);
    VERIFY(reader->ReadUShort() == 13);
    VERIFY(reader->ReadInt() == -123);
    VERIFY(reader->ReadUInt() == 1023);
    VERIFY(reader->ReadFloat() == 1.23f);
    VERIFY(reader->ReadDouble() == 2.34);
    VERIFY(reader->ReadBool() == true);
    VERIFY(reader->ReadBool() == false);
    VERIFY(reader->ReadString() == "Ein String");
    VERIFY(reader->ReadVec4() == v4);
    VERIFY(reader->ReadMat4() == m44);
    VERIFY(reader->ReadBlob() == testBlob);
    reader->Close();
    stream->Close();
}

} // namespace Test

