//------------------------------------------------------------------------------
//  @file jsonrawtest.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "jsonrawtest.h"
#include "io/memorystream.h"
#include "io/jsonreader.h"
#include "io/jsonwriter.h"

namespace Test
{
__ImplementClass(Test::JsonRawTest, 'JRAW', Test::TestCase);

using namespace IO;
using namespace Util;

//------------------------------------------------------------------------------
/**
    The round trip a level format needs from a feature it knows nothing about: text goes in through the
    writer as a node it cannot build, and the reader hands the node back as text. The writer formats
    what it writes, so the test compares the text the reader gives back rather than the file text, and
    that is also the text a save writes again for an extension nothing here can interpret.
*/
void
JsonRawTest::Run()
{
    // A string in the node is what makes this a regression test: pjson parses in place and a parsed
    // string's size does not include the null terminator it wrote over the delimiter, so a copy that
    // keeps that size leaves the string unterminated
    const String extensionJson = "{\"Health\":{\"value\":25,\"tags\":[\"a\",\"b\"],\"name\":\"thing\"}}";

    Ptr<MemoryStream> stream = MemoryStream::Create();
    stream->SetAccessMode(Stream::WriteAccess);

    Ptr<JsonWriter> writer = JsonWriter::Create();
    writer->SetStream(stream);
    VERIFY(writer->Open());

    writer->BeginObject("entity");
    writer->AddRaw(extensionJson.AsCharPtr(), "extensions");
    writer->End();
    writer->Close();

    VERIFY(stream->GetSize() > 0);

    stream->SetAccessMode(Stream::ReadAccess);
    VERIFY(stream->Open());

    Ptr<JsonReader> reader = JsonReader::Create();
    reader->SetStream(stream);
    VERIFY(reader->Open());

    VERIFY(reader->HasNode("/entity/extensions/Health"));

    VERIFY(reader->SetToNode("/entity/extensions"));
    VERIFY(reader->GetRawJson() == extensionJson);

    VERIFY(reader->SetToNode("/entity/extensions/Health"));
    VERIFY(reader->GetRawJson() == "{\"value\":25,\"tags\":[\"a\",\"b\"],\"name\":\"thing\"}");

    // The text the reader gives back is what a save writes again, which is the path an extension this
    // build does not have takes: nothing interpreted, the same text out
    Ptr<MemoryStream> resaveStream = MemoryStream::Create();
    resaveStream->SetAccessMode(Stream::WriteAccess);

    Ptr<JsonWriter> resaveWriter = JsonWriter::Create();
    resaveWriter->SetStream(resaveStream);
    VERIFY(resaveWriter->Open());

    VERIFY(reader->SetToNode("/entity/extensions"));
    resaveWriter->BeginObject("entity");
    resaveWriter->AddRaw(reader->GetRawJson().AsCharPtr(), "extensions");
    resaveWriter->End();
    resaveWriter->Close();

    reader->Close();

    resaveStream->SetAccessMode(Stream::ReadAccess);
    VERIFY(resaveStream->Open());

    Ptr<JsonReader> resaveReader = JsonReader::Create();
    resaveReader->SetStream(resaveStream);
    VERIFY(resaveReader->Open());

    VERIFY(resaveReader->SetToNode("/entity/extensions"));
    VERIFY(resaveReader->GetRawJson() == extensionJson);

    resaveReader->Close();

    // a bitfield is written as the list of its set bits, and read back from its attribute, with the
    // list replacing whatever the field held
    {
        Ptr<MemoryStream> bitStream = MemoryStream::Create();
        Ptr<JsonWriter> bitWriter = JsonWriter::Create();
        bitWriter->SetStream(bitStream);
        VERIFY(bitWriter->Open());
        bitWriter->BeginObject("component");
        bitWriter->Add(Util::BitField<16>({ 0, 1, 9 }), "stages");
        bitWriter->Add(7, "after");
        bitWriter->End();
        bitWriter->Close();

        Ptr<JsonReader> bitReader = JsonReader::Create();
        bitReader->SetStream(bitStream);
        VERIFY(bitReader->Open());
        VERIFY(bitReader->SetToNode("/component"));
        Util::BitField<16> stages({ 2, 3 });
        bitReader->Get<16>(stages, "stages");
        VERIFY(stages.IsSet(0) && stages.IsSet(1) && stages.IsSet(9));
        VERIFY(!stages.IsSet(2) && !stages.IsSet(3));
        VERIFY(bitReader->GetInt("after") == 7);
        bitReader->Close();
    }
}

} // namespace Test
//------------------------------------------------------------------------------
