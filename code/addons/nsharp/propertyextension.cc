//------------------------------------------------------------------------------
//  @file nsharp/propertyextension.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "propertyextension.h"
#include "nsharpserver.h"
#include "coreclr_delegates.h"

namespace Scripting
{

// The managed entry points, as the delegate pointers the server answers with. The signatures have to
// match the [UnmanagedCallersOnly] methods in NebulaEngine.AppEntry exactly.
typedef int(CORECLR_DELEGATE_CALLTYPE* SerializePropertiesFn)(uint64_t, void*, int);
typedef int(CORECLR_DELEGATE_CALLTYPE* ApplyPropertiesFn)(uint64_t, const char*, int);
typedef void(CORECLR_DELEGATE_CALLTYPE* DrawPropertiesFn)(uint64_t);
typedef int(CORECLR_DELEGATE_CALLTYPE* HasPropertyFn)(uint64_t, const char*, int);
typedef int(CORECLR_DELEGATE_CALLTYPE* DefaultPropertyJsonFn)(const char*, int, void*, int);
typedef int(CORECLR_DELEGATE_CALLTYPE* SerializeKeysFn)(uint64_t, const char*, int, void*, int);
typedef int(CORECLR_DELEGATE_CALLTYPE* JsonKeysFn)(const char*, int, void*, int);
typedef int(CORECLR_DELEGATE_CALLTYPE* MergeJsonFn)(const char*, int, const char*, int, void*, int);
typedef int(CORECLR_DELEGATE_CALLTYPE* ResolveJsonFn)(const char*, int, const char*, int, void*, int);
typedef void(CORECLR_DELEGATE_CALLTYPE* SetRefContextFn)(const char*, int);
typedef int(CORECLR_DELEGATE_CALLTYPE* TextRefsFn)(const char*, int, void*, int);
typedef int(CORECLR_DELEGATE_CALLTYPE* RemapRefsFn)(const char*, int, const char*, int, void*, int);

void* PropertyLevelExtension::serializeProperties = nullptr;
void* PropertyLevelExtension::applyProperties = nullptr;
void* PropertyLevelExtension::drawProperties = nullptr;
void* PropertyLevelExtension::hasProperty = nullptr;
void* PropertyLevelExtension::defaultPropertyJson = nullptr;
void* PropertyLevelExtension::serializeKeys = nullptr;
void* PropertyLevelExtension::jsonKeys = nullptr;
void* PropertyLevelExtension::mergeJson = nullptr;
void* PropertyLevelExtension::resolveJson = nullptr;
void* PropertyLevelExtension::setRefContext = nullptr;
void* PropertyLevelExtension::textRefs = nullptr;
void* PropertyLevelExtension::remapRefs = nullptr;
PropertyLevelExtension::ChangeFunc PropertyLevelExtension::changeCallback = nullptr;
PropertyLevelExtension::RevertFunc PropertyLevelExtension::revertCallback = nullptr;
PropertyLevelExtension::AuthoredFunc PropertyLevelExtension::authoredCallback = nullptr;
bool PropertyLevelExtension::registered = false;
Util::StringAtom PropertyLevelExtension::extensionName;
Util::Array<char> PropertyLevelExtension::buffer;
Util::Array<Util::String> PropertyLevelExtension::names;

//------------------------------------------------------------------------------
/**
    The extension is registered by the feature unit, which is what puts the hook in place before the
    plugin loads and what keeps the level formats from knowing anything about scripting.
*/
void
PropertyLevelExtension::Register(Scripting::NSharpServer* server, Scripting::AssemblyId assemblyId)
{
    if (server == nullptr || assemblyId == Scripting::AssemblyId::Invalid())
        return;

    PropertyLevelExtension::serializeProperties =
        server->GetUnmanagedFuncPointer(assemblyId, "NebulaEngine.AppEntry::SerializeProperties()");
    PropertyLevelExtension::applyProperties =
        server->GetUnmanagedFuncPointer(assemblyId, "NebulaEngine.AppEntry::ApplyProperties()");

    PropertyLevelExtension::drawProperties =
        server->GetUnmanagedFuncPointer(assemblyId, "NebulaEngine.AppEntry::DrawProperties()");
    PropertyLevelExtension::hasProperty =
        server->GetUnmanagedFuncPointer(assemblyId, "NebulaEngine.AppEntry::HasProperty()");
    PropertyLevelExtension::defaultPropertyJson =
        server->GetUnmanagedFuncPointer(assemblyId, "NebulaEngine.AppEntry::DefaultPropertyJson()");
    PropertyLevelExtension::serializeKeys =
        server->GetUnmanagedFuncPointer(assemblyId, "NebulaEngine.AppEntry::SerializeKeys()");
    PropertyLevelExtension::jsonKeys =
        server->GetUnmanagedFuncPointer(assemblyId, "NebulaEngine.AppEntry::JsonKeys()");
    PropertyLevelExtension::mergeJson =
        server->GetUnmanagedFuncPointer(assemblyId, "NebulaEngine.AppEntry::MergeJson()");
    PropertyLevelExtension::resolveJson =
        server->GetUnmanagedFuncPointer(assemblyId, "NebulaEngine.AppEntry::ResolveJson()");
    PropertyLevelExtension::setRefContext =
        server->GetUnmanagedFuncPointer(assemblyId, "NebulaEngine.AppEntry::SetRefContext()");
    PropertyLevelExtension::textRefs =
        server->GetUnmanagedFuncPointer(assemblyId, "NebulaEngine.AppEntry::TextRefs()");
    PropertyLevelExtension::remapRefs =
        server->GetUnmanagedFuncPointer(assemblyId, "NebulaEngine.AppEntry::RemapRefs()");

    if (PropertyLevelExtension::registered)
        return;

    Game::LevelExtension extension;
    extension.name = Util::StringAtom("properties");
    PropertyLevelExtension::extensionName = extension.name;
    extension.capture = &PropertyLevelExtension::Capture;
    extension.apply = &PropertyLevelExtension::Apply;
    extension.captureKeys = &PropertyLevelExtension::CaptureKeys;
    extension.keys = &PropertyLevelExtension::TextKeys;
    extension.merge = &PropertyLevelExtension::Merge;
    extension.resolve = &PropertyLevelExtension::Resolve;
    extension.refContext = &PropertyLevelExtension::SetRefContext;
    extension.refs = &PropertyLevelExtension::TextRefs;
    extension.remap = &PropertyLevelExtension::Remap;

    Game::LevelExtensionRegistry::Register(extension);
    PropertyLevelExtension::registered = true;
}

//------------------------------------------------------------------------------
/**
    An editor attaches itself here. A build that has no editor leaves the callback null, and the managed
    write path is a no-op for that build.
*/
void
PropertyLevelExtension::SetChangeCallback(ChangeFunc func)
{
    PropertyLevelExtension::changeCallback = func;
}

//------------------------------------------------------------------------------
/**
    An editor attaches itself here. A build that has no editor leaves the callback null, and the managed
    press is a no-op for that build.
*/
void
PropertyLevelExtension::SetRevertCallback(RevertFunc func)
{
    PropertyLevelExtension::revertCallback = func;
}

//------------------------------------------------------------------------------
/**
    An editor attaches itself here. A build that has no editor leaves the callback null, and the managed
    row is a no-op for that build.
*/
void
PropertyLevelExtension::SetAuthoredCallback(AuthoredFunc func)
{
    PropertyLevelExtension::authoredCallback = func;
}

//------------------------------------------------------------------------------
/**
*/
Util::StringAtom
PropertyLevelExtension::ExtensionName()
{
    return PropertyLevelExtension::extensionName;
}

//------------------------------------------------------------------------------
/**
    The json for one entity. The managed side writes into the buffer this owns and answers with the
    length it needs when the buffer was too small, which is what makes the call be made again with a
    bigger one.
*/
void
PropertyLevelExtension::Capture(Game::Entity entity, Util::String& out)
{
    if (PropertyLevelExtension::serializeProperties == nullptr)
        return;

    SerializePropertiesFn fn = (SerializePropertiesFn)PropertyLevelExtension::serializeProperties;

    if (PropertyLevelExtension::buffer.Size() == 0)
        PropertyLevelExtension::buffer.Resize(1024);

    for (;;)
    {
        int written = fn(uint64_t(entity), PropertyLevelExtension::buffer.Begin(), (int)PropertyLevelExtension::buffer.Size());
        if (written <= 0)
            return;

        if (written <= (int)PropertyLevelExtension::buffer.Size())
        {
            out.Set(PropertyLevelExtension::buffer.Begin(), (SizeT)written);
            return;
        }

        PropertyLevelExtension::buffer.Resize((SizeT)written);
    }
}

//------------------------------------------------------------------------------
/**
    One entity, put back from the json the file carried for it.
*/
void
PropertyLevelExtension::Apply(Game::Entity entity, const char* json)
{
    if (PropertyLevelExtension::applyProperties == nullptr || json == nullptr)
        return;

    ApplyPropertiesFn fn = (ApplyPropertiesFn)PropertyLevelExtension::applyProperties;
    fn(uint64_t(entity), json, (int)Util::String::StrLen(json));
}

//------------------------------------------------------------------------------
/**
    The text for one entity with only the keys it authored. The keys are joined here rather than passed as an
    array, because the managed side cannot read this side's containers, and the same buffer pattern as the
    entity capture is what makes the length come back right when the buffer was too small.
*/
void
PropertyLevelExtension::CaptureKeys(Game::Entity entity, const Util::Array<Util::String>& keys, Util::String& out)
{
    if (PropertyLevelExtension::serializeKeys == nullptr)
        return;

    SerializeKeysFn fn = (SerializeKeysFn)PropertyLevelExtension::serializeKeys;

    Util::String joined;
    for (IndexT i = 0; i < keys.Size(); i++)
    {
        if (i > 0)
            joined += "\n";
        joined += keys[i];
    }

    if (PropertyLevelExtension::buffer.Size() == 0)
        PropertyLevelExtension::buffer.Resize(1024);

    for (;;)
    {
        int written = fn(uint64_t(entity), joined.AsCharPtr(), (int)Util::String::StrLen(joined.AsCharPtr()),
            PropertyLevelExtension::buffer.Begin(), (int)PropertyLevelExtension::buffer.Size());
        if (written <= 0)
            return;

        if (written <= (int)PropertyLevelExtension::buffer.Size())
        {
            out.Set(PropertyLevelExtension::buffer.Begin(), (SizeT)written);
            return;
        }

        PropertyLevelExtension::buffer.Resize((SizeT)written);
    }
}

//------------------------------------------------------------------------------
/**
    The keys a text carries, as the paths this extension names them by. This is the only thing the level
    formats can know about a text they cannot read, and it is what tells an authored field from one the
    entity accepts from its prefab.
*/
void
PropertyLevelExtension::TextKeys(const char* text, Util::Array<Util::String>& out)
{
    if (PropertyLevelExtension::jsonKeys == nullptr || text == nullptr)
        return;

    JsonKeysFn fn = (JsonKeysFn)PropertyLevelExtension::jsonKeys;

    if (PropertyLevelExtension::buffer.Size() == 0)
        PropertyLevelExtension::buffer.Resize(1024);

    int textLen = (int)Util::String::StrLen(text);

    for (;;)
    {
        int written = fn(text, textLen, PropertyLevelExtension::buffer.Begin(), (int)PropertyLevelExtension::buffer.Size());
        if (written <= 0)
            return;

        if (written <= (int)PropertyLevelExtension::buffer.Size())
        {
            // The names come back as one block, separated by a newline. The buffer is this side's own, so
            // the strings have to be copied out of it before it is used for anything else
            const char* begin = PropertyLevelExtension::buffer.Begin();
            const char* end = begin + written;
            const char* pos = begin;
            while (pos < end)
            {
                const char* stop = pos;
                while (stop < end && *stop != '\n')
                    stop++;

                Util::String name;
                name.Set(pos, (SizeT)(stop - pos));
                if (!name.IsEmpty())
                    out.Append(name);

                pos = stop + 1;
            }
            return;
        }

        PropertyLevelExtension::buffer.Resize((SizeT)written);
    }
}

//------------------------------------------------------------------------------
/**
    What the base text gives when the keys the other text carries are put over it. The formats compose the
    text without reading it, which is what keeps a merge from needing to know what a managed property is.
*/
void
PropertyLevelExtension::Merge(const char* base, const char* over, Util::String& out)
{
    if (PropertyLevelExtension::mergeJson == nullptr)
        return;

    MergeJsonFn fn = (MergeJsonFn)PropertyLevelExtension::mergeJson;

    if (PropertyLevelExtension::buffer.Size() == 0)
        PropertyLevelExtension::buffer.Resize(1024);

    int baseLen = base == nullptr ? 0 : (int)Util::String::StrLen(base);
    int overLen = over == nullptr ? 0 : (int)Util::String::StrLen(over);

    for (;;)
    {
        int written = fn(base, baseLen, over, overLen,
            PropertyLevelExtension::buffer.Begin(), (int)PropertyLevelExtension::buffer.Size());
        if (written <= 0)
            return;

        if (written <= (int)PropertyLevelExtension::buffer.Size())
        {
            out.Set(PropertyLevelExtension::buffer.Begin(), (SizeT)written);
            return;
        }

        PropertyLevelExtension::buffer.Resize((SizeT)written);
    }
}

//------------------------------------------------------------------------------
/**
    The text for the named keys, each value read from the base text, or the field's default when the base has
    nothing. This is what a revert to the prefab writes, and the same buffer pattern as the merge is what makes
    the length come back right when the buffer was too small.
*/
void
PropertyLevelExtension::Resolve(const char* base, const Util::Array<Util::String>& keys, Util::String& out)
{
    if (PropertyLevelExtension::resolveJson == nullptr || keys.IsEmpty())
        return;

    ResolveJsonFn fn = (ResolveJsonFn)PropertyLevelExtension::resolveJson;

    Util::String joined;
    for (IndexT i = 0; i < keys.Size(); i++)
    {
        if (i > 0)
            joined += "\n";
        joined += keys[i];
    }

    if (PropertyLevelExtension::buffer.Size() == 0)
        PropertyLevelExtension::buffer.Resize(1024);

    int baseLen = base == nullptr ? 0 : (int)Util::String::StrLen(base);
    int keysLen = (int)Util::String::StrLen(joined.AsCharPtr());

    for (;;)
    {
        int written = fn(base, baseLen, joined.AsCharPtr(), keysLen,
            PropertyLevelExtension::buffer.Begin(), (int)PropertyLevelExtension::buffer.Size());
        if (written <= 0)
            return;

        if (written <= (int)PropertyLevelExtension::buffer.Size())
        {
            out.Set(PropertyLevelExtension::buffer.Begin(), (SizeT)written);
            return;
        }

        PropertyLevelExtension::buffer.Resize((SizeT)written);
    }
}

//------------------------------------------------------------------------------
/**
    The file's guid map, as "guid:entityId" pairs. The formats cannot resolve what a text names, because they
    do not read it, so this is how the map gets to the side that does. It is only valid for the duration of
    the call that sets it.
*/
void
PropertyLevelExtension::SetRefContext(const char* pairs)
{
    if (PropertyLevelExtension::setRefContext == nullptr)
        return;

    SetRefContextFn fn = (SetRefContextFn)PropertyLevelExtension::setRefContext;
    fn(pairs, pairs == nullptr ? 0 : (int)Util::String::StrLen(pairs));
}

//------------------------------------------------------------------------------
/**
    The entity references a text names. The managed side answers them as "path\tguid" lines, because this side
    cannot read the text, and the buffer is the only thing that comes back across the boundary. The strings
    have to be copied out of the buffer before it is used for anything else.
*/
void
PropertyLevelExtension::TextRefs(const char* text, Util::Array<Game::TextRef>& out)
{
    if (PropertyLevelExtension::textRefs == nullptr)
        return;

    TextRefsFn fn = (TextRefsFn)PropertyLevelExtension::textRefs;

    if (PropertyLevelExtension::buffer.Size() == 0)
        PropertyLevelExtension::buffer.Resize(1024);

    int textLen = text == nullptr ? 0 : (int)Util::String::StrLen(text);

    for (;;)
    {
        int written = fn(text, textLen, PropertyLevelExtension::buffer.Begin(), (int)PropertyLevelExtension::buffer.Size());
        if (written <= 0)
            return;

        if (written <= (int)PropertyLevelExtension::buffer.Size())
        {
            const char* begin = PropertyLevelExtension::buffer.Begin();
            const char* end = begin + written;
            const char* pos = begin;
            while (pos < end)
            {
                const char* stop = pos;
                while (stop < end && *stop != '\n')
                    stop++;

                const char* split = pos;
                while (split < stop && *split != '\t')
                    split++;

                if (split == stop)
                {
                    pos = stop + 1;
                    continue;
                }

                Util::String path;
                path.Set(pos, (SizeT)(split - pos));

                Util::String guid;
                guid.Set(split + 1, (SizeT)(stop - split - 1));

                Game::TextRef ref;
                ref.path = path;
                if (!guid.IsEmpty())
                    ref.guid = Util::Guid::FromString(guid);

                if (!ref.path.IsEmpty())
                    out.Append(ref);

                pos = stop + 1;
            }
            return;
        }

        PropertyLevelExtension::buffer.Resize((SizeT)written);
    }
}

//------------------------------------------------------------------------------
/**
    The text with the named paths' references put as the given guids. The same buffer pattern as the merge is
    what makes the length come back right when the buffer was too small.
*/
void
PropertyLevelExtension::Remap(const char* text, const char* pairs, Util::String& out)
{
    if (PropertyLevelExtension::remapRefs == nullptr)
        return;

    RemapRefsFn fn = (RemapRefsFn)PropertyLevelExtension::remapRefs;

    if (PropertyLevelExtension::buffer.Size() == 0)
        PropertyLevelExtension::buffer.Resize(1024);

    int textLen = text == nullptr ? 0 : (int)Util::String::StrLen(text);
    int pairsLen = pairs == nullptr ? 0 : (int)Util::String::StrLen(pairs);

    for (;;)
    {
        int written = fn(text, textLen, pairs, pairsLen,
            PropertyLevelExtension::buffer.Begin(), (int)PropertyLevelExtension::buffer.Size());
        if (written <= 0)
            return;

        if (written <= (int)PropertyLevelExtension::buffer.Size())
        {
            out.Set(PropertyLevelExtension::buffer.Begin(), (SizeT)written);
            return;
        }

        PropertyLevelExtension::buffer.Resize((SizeT)written);
    }
}

//------------------------------------------------------------------------------
/**
    The property section. The managed side draws through the editor's surface, so this has to be called from
    inside the editor's frame, and it does nothing when the entry point could not be resolved.
*/
void
PropertyLevelExtension::Draw(Game::Entity entity)
{
    if (PropertyLevelExtension::drawProperties == nullptr)
        return;

    DrawPropertiesFn fn = (DrawPropertiesFn)PropertyLevelExtension::drawProperties;
    fn(uint64_t(entity));
}

//------------------------------------------------------------------------------
/**
    The change the managed write path reported. The managed side does not write anything itself: it works
    out the text before and after and hands both here, and the callback is what runs the change through a
    command. Without a callback there is nothing to record, which is what makes a game build a no-op.

    The path is the key inside the text the change is about. A path with no field is the whole property,
    which is what an add and a removal report.
*/
void
PropertyLevelExtension::RecordChange(Game::Entity entity, const char* path, const char* oldText, const char* newText)
{
    if (PropertyLevelExtension::changeCallback == nullptr)
        return;

    PropertyLevelExtension::changeCallback(uint64_t(entity), path, oldText, newText);
}

//------------------------------------------------------------------------------
/**
    The key the managed side pressed the override marker on. The managed side cannot know what a press means, so
    it only says which key it was, and the callback is what builds the revert. Without a callback there is
    nothing to do, which is what makes a game build a no-op.
*/
void
PropertyLevelExtension::RequestRevert(Game::Entity entity, const char* path)
{
    if (PropertyLevelExtension::revertCallback == nullptr)
        return;

    PropertyLevelExtension::revertCallback(uint64_t(entity), path);
}

//------------------------------------------------------------------------------
/**
    Whether the editor's marker says this entity authored the path. The marker is the editor's own and this side
    cannot read it, so a row asks. Without a callback nothing is authored, which is what makes a game build a
    no-op.
*/
bool
PropertyLevelExtension::IsAuthored(Game::Entity entity, const char* path)
{
    if (PropertyLevelExtension::authoredCallback == nullptr)
        return false;

    return PropertyLevelExtension::authoredCallback(uint64_t(entity), path);
}

//------------------------------------------------------------------------------
/**
    One of the names the editor's dropdown lists. The panel has no way to read the managed registry, so the
    managed side mirrors it here.
*/
void
PropertyLevelExtension::AddPropertyName(const char* propertyName)
{
    if (propertyName == nullptr)
        return;

    PropertyLevelExtension::names.Append(Util::String(propertyName));
}

//------------------------------------------------------------------------------
/**
*/
void
PropertyLevelExtension::ClearPropertyNames()
{
    PropertyLevelExtension::names.Clear();
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Util::String>&
PropertyLevelExtension::PropertyNames()
{
    return PropertyLevelExtension::names;
}

//------------------------------------------------------------------------------
/**
    Whether the entity has the property. This is what makes a dropdown row read as already added, the same way
    a component that is already on the entity does.
*/
bool
PropertyLevelExtension::HasProperty(Game::Entity entity, const char* propertyName)
{
    if (PropertyLevelExtension::hasProperty == nullptr || propertyName == nullptr)
        return false;

    HasPropertyFn fn = (HasPropertyFn)PropertyLevelExtension::hasProperty;
    return fn(uint64_t(entity), propertyName, (int)Util::String::StrLen(propertyName)) != 0;
}

//------------------------------------------------------------------------------
/**
    The text a default instance of the property produces, which is what an add writes. Same buffer as the
    capture, because the managed side writes into it directly and nothing here is reentrant.
*/
void
PropertyLevelExtension::DefaultPropertyJson(const char* propertyName, Util::String& out)
{
    if (PropertyLevelExtension::defaultPropertyJson == nullptr || propertyName == nullptr)
        return;

    DefaultPropertyJsonFn fn = (DefaultPropertyJsonFn)PropertyLevelExtension::defaultPropertyJson;

    if (PropertyLevelExtension::buffer.Size() == 0)
        PropertyLevelExtension::buffer.Resize(1024);

    int nameLen = (int)Util::String::StrLen(propertyName);

    for (;;)
    {
        int written = fn(propertyName, nameLen, PropertyLevelExtension::buffer.Begin(), (int)PropertyLevelExtension::buffer.Size());
        if (written <= 0)
            return;

        if (written <= (int)PropertyLevelExtension::buffer.Size())
        {
            out.Set(PropertyLevelExtension::buffer.Begin(), (SizeT)written);
            return;
        }

        PropertyLevelExtension::buffer.Resize((SizeT)written);
    }
}

} // namespace Scripting
//------------------------------------------------------------------------------
