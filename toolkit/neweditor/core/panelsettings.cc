//------------------------------------------------------------------------------
//  @file panelsettings.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/core/panelsettings.h"
#include "neweditor/api/shell.h"
#include "neweditor/base/editorlog.h"
#include "io/ioserver.h"
#include "io/jsonreader.h"
#include "io/jsonwriter.h"

namespace Editor
{

__ImplementClass(Editor::PanelSettings, 'PSET', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
PanelSettings::PanelSettings() :
    core(nullptr),
    path("bin:editor-panels.json")
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
PanelSettings::~PanelSettings()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
PanelSettings::Setup(Shell* core)
{
    this->core = core;
}

//------------------------------------------------------------------------------
/**
*/
void
PanelSettings::Discard()
{
    this->records.Clear();
    this->core = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
PanelSettings::SetPath(const Util::String& path)
{
    this->path = path;
}

//------------------------------------------------------------------------------
/**
*/
const Util::String&
PanelSettings::GetPath() const
{
    return this->path;
}

//------------------------------------------------------------------------------
/**
*/
IndexT
PanelSettings::FindIndex(const Util::String& name) const
{
    for (IndexT i = 0; i < this->records.Size(); i++)
    {
        if (this->records[i].name == name)
            return i;
    }
    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
bool
PanelSettings::IsOpen(const Util::String& name, bool fallback) const
{
    const IndexT index = this->FindIndex(name);
    if (index == InvalidIndex)
        return fallback;

    return this->records[index].open;
}

//------------------------------------------------------------------------------
/**
*/
void
PanelSettings::SetOpen(const Util::String& name, bool open)
{
    const IndexT index = this->FindIndex(name);
    if (index == InvalidIndex)
    {
        PanelRecord record;
        record.name = name;
        record.open = open;
        this->records.Append(record);
        return;
    }

    this->records[index].open = open;
}

//------------------------------------------------------------------------------
/**
*/
bool
PanelSettings::Load()
{
    if (this->path.IsEmpty())
        return false;

    const IO::URI uri(this->path);
    if (!IO::FileExists(uri))
        return false;

    Ptr<IO::Stream> stream = IO::CreateStream(uri);
    if (!stream.isvalid())
    {
        if (this->core != nullptr)
            this->core->Log(LogMessageType::N_WARNING, Util::String("Could not open the panel file ") + this->path);
        return false;
    }

    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (!stream->Open())
    {
        if (this->core != nullptr)
            this->core->Log(LogMessageType::N_WARNING, Util::String("Could not read the panel file ") + this->path);
        return false;
    }

    Ptr<IO::JsonReader> reader = IO::JsonReader::Create();
    reader->SetStream(stream);
    if (!reader->Open())
    {
        if (this->core != nullptr)
            this->core->Log(LogMessageType::N_WARNING, Util::String("Could not read the panel file ") + this->path + " as json");
        if (stream->IsOpen())
            stream->Close();
        return false;
    }

    if (!reader->HasNode("panels"))
    {
        if (this->core != nullptr)
            this->core->Log(LogMessageType::N_WARNING, "Panel file has no panels");
        reader->Close();
        return false;
    }

    reader->SetToNode("panels");

    this->records.Clear();

    IndexT read = 0;
    if (reader->SetToFirstChild())
    {
        do
        {
            if (!reader->IsObject())
            {
                if (this->core != nullptr)
                    this->core->Log(LogMessageType::N_WARNING, "Panel file has a panel that is not an object");
                continue;
            }

            if (!reader->HasAttr("name"))
            {
                if (this->core != nullptr)
                    this->core->Log(LogMessageType::N_WARNING, "Panel file has a panel with no name");
                continue;
            }

            const Util::String name = reader->GetString("name");
            if (name.IsEmpty())
            {
                if (this->core != nullptr)
                    this->core->Log(LogMessageType::N_WARNING, "Panel file has a panel with no name");
                continue;
            }

            PanelRecord record;
            record.name = name;
            record.open = reader->GetOptBool("open", true);
            this->records.Append(record);
            read++;
        }
        while (reader->SetToNextChild());
    }

    // Closing the reader closes the stream with it, so the stream is not closed on its own here
    reader->Close();

    if (this->core != nullptr && read > 0)
        this->core->Log(LogMessageType::N_SYSTEM, Util::String("Read ") + Util::String::Sprintf("%d", (int)read) + " panel states from " + this->path);

    return read > 0;
}

//------------------------------------------------------------------------------
/**
*/
bool
PanelSettings::Save()
{
    if (this->path.IsEmpty())
        return false;

    const IO::URI uri(this->path);
    IO::EnsureDirectoriesForFile(uri);

    Ptr<IO::Stream> stream = IO::CreateStream(uri);
    if (!stream.isvalid())
    {
        if (this->core != nullptr)
            this->core->Log(LogMessageType::N_WARNING, Util::String("Could not open the panel file ") + this->path + " for writing");
        return false;
    }

    stream->SetAccessMode(IO::Stream::WriteAccess);
    if (!stream->Open())
    {
        if (this->core != nullptr)
            this->core->Log(LogMessageType::N_WARNING, Util::String("Could not write the panel file ") + this->path);
        return false;
    }

    Ptr<IO::JsonWriter> writer = IO::JsonWriter::Create();
    writer->SetStream(stream);
    if (!writer->Open())
    {
        if (this->core != nullptr)
            this->core->Log(LogMessageType::N_WARNING, Util::String("Could not write the panel file ") + this->path);
        if (stream->IsOpen())
            stream->Close();
        return false;
    }

    // The document is already an object when the writer opens, so the version and the panels are
    // added to it directly
    writer->Add(1, "version");

    writer->BeginArray("panels");
    for (IndexT i = 0; i < this->records.Size(); i++)
    {
        const PanelRecord& record = this->records[i];

        writer->BeginObject();
        writer->Add(record.name, "name");
        writer->Add(record.open, "open");
        writer->End();
    }
    writer->End();

    // The document is serialized into the stream when the writer closes, and the writer closes the
    // stream with it
    writer->Close();

    return true;
}

} // namespace Editor
//------------------------------------------------------------------------------
