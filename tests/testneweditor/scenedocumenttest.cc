//------------------------------------------------------------------------------
//  @file scenedocumenttest.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "scenedocumenttest.h"
#include "game/componentserialization.h"
#include "game/levelextension.h"
#include "io/jsonreader.h"
#include "io/memorystream.h"
#include "neweditor/model/scenedocument.h"
#include "util/guid.h"
#include "util/string.h"

namespace Test
{
__ImplementClass(Test::SceneDocumentRefTest, 'SDRF', Test::TestCase);

using namespace Editor;

// The test extension. The document cannot read an extension's text, so the two reference hooks are
// all it has, and an extension has to answer for its own text. This is the smallest one that can:
// one property with one field that holds a guid and one that holds text, so a remap can be seen to
// leave the field it was not given alone.
static const char* TestExtensionName = "testext";
static const char* TestProperty = "TestProperty";
static const char* TestRefField = "target";
static const char* TestTextField = "note";

//------------------------------------------------------------------------------
/**
    The value of one string field as the text carries it. Only the extension can read its own text,
    and this is the reading its two hooks need.
*/
static Util::String
ReadField(const char* text, const char* field)
{
    Util::String out;
    if (text == nullptr)
        return out;

    const Util::String s(text);

    Util::String needle;
    needle.Format("\"%s\":\"", field);

    const IndexT found = s.FindStringIndex(needle);
    if (found == InvalidIndex)
        return out;

    const IndexT start = found + needle.Length();
    IndexT end = start;
    while (end < s.Length() && s[end] != '"')
        end++;

    out.Set(s.AsCharPtr() + start, (SizeT)(end - start));
    return out;
}

//------------------------------------------------------------------------------
/**
    The references the text names. This is how the document learns which part of a text it cannot read
    names an entity, and the path it answers with is the path it can put a guid back into.
*/
static void
TestRefs(const char* text, Util::Array<Game::TextRef>& out)
{
    const Util::String guid = ReadField(text, TestRefField);
    if (guid.IsEmpty())
        return;

    Game::TextRef ref;
    ref.path = Util::String(TestProperty) + "." + TestRefField;
    ref.guid = Util::Guid::FromString(guid);
    out.Append(ref);
}

//------------------------------------------------------------------------------
/**
    The text with the named path's reference put as the given guid. A path the text does not name is
    left as the text had it, and a path given with no guid is written as nothing, which is the only
    thing a file with no record for the target can carry.
*/
static void
TestRemap(const char* text, const char* pairs, Util::String& out)
{
    const Util::String note = ReadField(text, TestTextField);

    Util::String guid;
    if (pairs != nullptr)
    {
        const Util::String p(pairs);
        const IndexT split = p.FindStringIndex("\t");
        if (split != InvalidIndex)
            guid.Set(p.AsCharPtr() + split + 1, p.Length() - (SizeT)split - 1);
    }

    out = Util::String("{\"") + TestProperty + "\":{\"note\":\"" + note + "\",\"target\":";

    if (guid.IsEmpty())
        out += "null";
    else
        out += "\"" + guid + "\"";

    out += "}}";
}

//------------------------------------------------------------------------------
/**
    The text a written document carries for one extension node. The writer formats what it writes, so
    the test reads the produced json back rather than comparing the whole file, and the raw node is the
    text the extension produced.
*/
static Util::String
WrittenExtensionText(const Util::String& json, const Util::String& entityGuid, const char* extensionName)
{
    Ptr<IO::MemoryStream> stream = IO::MemoryStream::Create();
    stream->SetAccessMode(IO::Stream::WriteAccess);
    if (!stream->Open())
        return "";
    stream->Write(json.AsCharPtr(), json.Length());
    stream->Close();

    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (!stream->Open())
        return "";

    Ptr<IO::JsonReader> reader = IO::JsonReader::Create();
    reader->SetStream(stream);
    if (!reader->Open())
        return "";

    Util::String node;
    node.Format("/level/entities/%s/extensions/%s", entityGuid.AsCharPtr(), extensionName);

    Util::String out;
    if (reader->SetToNode(node))
        out = reader->GetRawJson();

    reader->Close();
    return out;
}

//------------------------------------------------------------------------------
/**
*/
void
SceneDocumentRefTest::Run()
{
    // The document writes components through the serialization registry, and that singleton is made by
    // whoever opens the game server. There is no game server here, so the test makes it
    Game::ComponentSerialization::Instance();

    Game::LevelExtension extension;
    extension.name = Util::StringAtom(TestExtensionName);
    extension.refs = &TestRefs;
    extension.remap = &TestRemap;
    Game::LevelExtensionRegistry::Register(extension);

    const Util::String guidA = "11111111-1111-1111-1111-111111111111";
    const Util::String guidB = "22222222-2222-2222-2222-222222222222";
    const Util::String guidC = "33333333-3333-3333-3333-333333333333";

    // A file with two records, one of them naming the other inside its extension data
    const Util::String json = "{\"level\":{\"version\":101,\"entities\":{\"" + guidA +
        "\":{\"name\":\"a\",\"extensions\":{\"testext\":{\"TestProperty\":{\"note\":\"hello\",\"target\":\"" +
        guidB + "\"}}}},\"" + guidB + "\":{\"name\":\"b\"}}}}";

    Ptr<SceneDocument> doc = SceneDocument::FromJson(json);
    VERIFY(doc.isvalid());
    if (!doc.isvalid())
        return;

    VERIFY(doc->entities.Size() == 2);
    VERIFY(doc->entityRefs.Size() == 1);
    if (doc->entityRefs.Size() != 1)
        return;

    const DocumentEntityRef& ref = doc->entityRefs[0];

    // The reference is recorded as the extension's, not as a component's, because the document has no
    // reflection of a text it cannot read
    VERIFY(ref.entity == 0);
    VERIFY(ref.component == Game::ComponentId::Invalid());
    VERIFY(ref.extension == Util::StringAtom(TestExtensionName));
    VERIFY(ref.field == Util::String(TestProperty) + "." + TestRefField);
    VERIFY(ref.target == Util::Guid::FromString(guidB));

    // It is still a reference the document can resolve against its own records
    VERIFY(doc->FindEntity(ref.target) == 1);

    Util::Array<Util::Guid> guids;
    guids.Append(Util::Guid::FromString(guidA));
    guids.Append(Util::Guid::FromString(guidB));

    // Written against the guids the text was read from, it comes back as it was
    VERIFY(WrittenExtensionText(doc->ToJson(guids), guidA, TestExtensionName) ==
        "{\"TestProperty\":{\"note\":\"hello\",\"target\":\"" + guidB + "\"}}");

    // Written against another file's guids, the reference is put as the guid that file has for the
    // target, and the field the remap was not given is left as the text had it
    guids[1] = Util::Guid::FromString(guidC);
    VERIFY(WrittenExtensionText(doc->ToJson(guids), guidA, TestExtensionName) ==
        "{\"TestProperty\":{\"note\":\"hello\",\"target\":\"" + guidC + "\"}}");

    // A text that names a guid the file has no record for cannot be written as that guid, so the
    // reference is given with nothing and written as nothing
    Ptr<SceneDocument> alone = SceneDocument::FromJson(
        "{\"level\":{\"version\":101,\"entities\":{\"" + guidA +
        "\":{\"name\":\"a\",\"extensions\":{\"testext\":{\"TestProperty\":{\"note\":\"hello\",\"target\":\"" +
        guidB + "\"}}}}}}}");
    VERIFY(alone.isvalid());
    if (!alone.isvalid())
        return;

    VERIFY(alone->entityRefs.Size() == 1);
    VERIFY(alone->FindEntity(Util::Guid::FromString(guidB)) == InvalidIndex);

    Util::Array<Util::Guid> own;
    own.Append(Util::Guid::FromString(guidA));
    VERIFY(WrittenExtensionText(alone->ToJson(own), guidA, TestExtensionName) ==
        "{\"TestProperty\":{\"note\":\"hello\",\"target\":null}}");

    // An extension this build does not have keeps its text whole, because there is nothing here that
    // could answer for it, and it records no references
    Ptr<SceneDocument> unknown = SceneDocument::FromJson(
        "{\"level\":{\"version\":101,\"entities\":{\"" + guidA +
        "\":{\"name\":\"a\",\"extensions\":{\"otherext\":{\"target\":\"" + guidB + "\"}}}}}}");
    VERIFY(unknown.isvalid());
    if (!unknown.isvalid())
        return;

    VERIFY(unknown->entityRefs.IsEmpty());
    VERIFY(WrittenExtensionText(unknown->ToJson(own), guidA, "otherext") == "{\"target\":\"" + guidB + "\"}");
}

} // namespace Test
//------------------------------------------------------------------------------
