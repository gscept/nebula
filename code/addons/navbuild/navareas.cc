//------------------------------------------------------------------------------
//  @file navareas.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "navbuild/navareas.h"
#include "navbuild/navmeshbuilder.h"
#include "nflatbuffer/nebula_flat.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "flat/navigation/navareas.h"
#include "io/ioserver.h"
#include "io/jsonwriter.h"

namespace Navigation
{
namespace Build
{

//------------------------------------------------------------------------------
/**
*/
AreaTable::AreaTable()
{
    this->SetupBuiltIn();
}

//------------------------------------------------------------------------------
/**
*/
void
AreaTable::SetupBuiltIn()
{
    this->areas.Clear();
    AreaDefinition none;
    none.id = NoArea;
    none.name = "none";
    none.flags = 0;
    none.color = Math::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    this->areas.Append(none);

    AreaDefinition walkable;
    walkable.id = DefaultWalkableArea;
    walkable.name = "default";
    walkable.flags = PolyFlag_Walk;
    walkable.color = Math::vec4(0.0f, 0.75f, 1.0f, 1.0f);
    this->areas.Append(walkable);
}

//------------------------------------------------------------------------------
/**
*/
void
AreaTable::Sort()
{
    this->areas.SortWithFunc([](const AreaDefinition& a, const AreaDefinition& b)
    {
        return a.id < b.id;
    });
}

//------------------------------------------------------------------------------
/**
*/
bool
AreaTable::Load(const IO::URI& file)
{
    if (!IO::FileExists(file))
    {
        this->SetupBuiltIn();
        this->data = Util::Blob();
        return true;
    }
    Ptr<IO::Stream> stream = IO::CreateStream(file);
    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (!stream->Open())
    {
        n_warning("AreaTable: could not open '%s'\n", file.LocalPath().AsCharPtr());
        return false;
    }
    const void* mapped = stream->MemoryMap();
    const bool loaded = this->Load(mapped, stream->GetSize());
    stream->MemoryUnmap();
    stream->Close();
    return loaded;
}

//------------------------------------------------------------------------------
/**
    Areas with an id or name which is taken, or an id outside 1 to 62, are left out with a warning
*/
bool
AreaTable::Load(const void* buffer, SizeT size)
{
    this->SetupBuiltIn();
    this->data = Util::Blob();
    if (buffer == nullptr || size == 0)
    {
        return false;
    }
    flatbuffers::Verifier verifier((const uint8_t*)buffer, (size_t)size);
    if (!verifier.VerifyBuffer<Navigation::NavAreas>(nullptr))
    {
        n_warning("AreaTable: the data is not an area table\n");
        return false;
    }
    this->data.Set(buffer, size);

    NavAreasT table;
    Flat::FlatbufferInterface::DeserializeFlatbuffer<Navigation::NavAreas>(table, (const uint8_t*)buffer);
    bool valid = true;
    for (const auto& entry : table.areas)
    {
        if (entry->id == NoArea || entry->id >= DefaultWalkableArea)
        {
            n_warning("AreaTable: area '%s' has id %d, project areas go from 1 to %d\n", entry->name.c_str(), entry->id, DefaultWalkableArea - 1);
            valid = false;
            continue;
        }
        if (entry->name.empty() || this->Find(entry->name.c_str()) != nullptr || this->Get(entry->id) != nullptr)
        {
            n_warning("AreaTable: area '%s' with id %d is a duplicate or has no name\n", entry->name.c_str(), entry->id);
            valid = false;
            continue;
        }
        AreaDefinition area;
        area.id = entry->id;
        area.name = entry->name.c_str();
        area.cost = entry->cost;
        area.flags = (unsigned short)entry->flags;
        area.color = entry->color;
        this->areas.Append(area);
    }
    this->Sort();
    return valid;
}

//------------------------------------------------------------------------------
/**
*/
const AreaDefinition*
AreaTable::Find(const Util::String& name) const
{
    for (const AreaDefinition& area : this->areas)
    {
        if (area.name == name)
        {
            return &area;
        }
    }
    return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
const AreaDefinition*
AreaTable::Get(unsigned char id) const
{
    for (const AreaDefinition& area : this->areas)
    {
        if (area.id == id)
        {
            return &area;
        }
    }
    return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<AreaDefinition>&
AreaTable::Areas() const
{
    return this->areas;
}

//------------------------------------------------------------------------------
/**
*/
void
AreaTable::GetFlags(unsigned short (&flags)[MaxAreas]) const
{
    for (int i = 0; i < MaxAreas; i++)
    {
        flags[i] = 0;
    }
    for (const AreaDefinition& area : this->areas)
    {
        flags[area.id] = area.flags;
    }
}

//------------------------------------------------------------------------------
/**
*/
Math::vec4
AreaTable::GetColor(unsigned char id) const
{
    const AreaDefinition* area = this->Get(id);
    if (area != nullptr && area->color.w > 0.0f)
    {
        return area->color;
    }
    // spread the hues of undefined colors over the circle by the golden angle, neighbour ids differ a lot
    const float hue = Math::fmod((float)id * 0.618034f, 1.0f) * 6.0f;
    const float x = 1.0f - Math::abs(Math::fmod(hue, 2.0f) - 1.0f);
    Math::vec4 color(0.0f, 0.0f, 0.0f, 1.0f);
    switch ((int)hue)
    {
        case 0: color.x = 1.0f; color.y = x; break;
        case 1: color.x = x; color.y = 1.0f; break;
        case 2: color.y = 1.0f; color.z = x; break;
        case 3: color.y = x; color.z = 1.0f; break;
        case 4: color.x = x; color.z = 1.0f; break;
        default: color.x = 1.0f; color.z = x; break;
    }
    return color;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Blob&
AreaTable::GetData() const
{
    return this->data;
}

//------------------------------------------------------------------------------
/**
*/
bool
CompileAreaTable()
{
    if (!IO::FileExists(AreaTableSource))
    {
        return false;
    }
    IO::CreateDirectory("nav:");
    return CompileFlatbuffer(Navigation::NavAreas, IO::URI(AreaTableSource), "nav:");
}

//------------------------------------------------------------------------------
/**
    The flags are written the way the schema names them, separated by spaces, the color as its fields
*/
bool
WriteAreaTableSource(const Util::Array<AreaDefinition>& areas, const IO::URI& file)
{
    static const char* flagNames[] = { "Walk", "Swim", "Door", "Disabled" };
    Ptr<IO::JsonWriter> writer = IO::JsonWriter::Create();
    writer->SetStream(IO::CreateStream(file));
    if (!writer->Open())
    {
        n_warning("WriteAreaTableSource: could not write '%s'\n", file.AsString().AsCharPtr());
        return false;
    }
    writer->BeginArray("areas");
    for (const AreaDefinition& area : areas)
    {
        if (area.id == NoArea || area.id >= DefaultWalkableArea)
        {
            continue;
        }
        writer->BeginObject();
        writer->Add((int)area.id, "id");
        writer->Add(area.name, "name");
        writer->Add(area.cost, "cost");
        Util::String flags;
        for (int bit = 0; bit < 4; bit++)
        {
            if (area.flags & (1 << bit))
            {
                if (flags.IsValid())
                {
                    flags.Append(" ");
                }
                flags.Append(flagNames[bit]);
            }
        }
        if (flags.IsValid())
        {
            writer->Add(flags, "flags");
        }
        else
        {
            writer->Add(0, "flags");
        }
        writer->BeginObject("color");
        writer->Add(area.color.x, "x");
        writer->Add(area.color.y, "y");
        writer->Add(area.color.z, "z");
        writer->Add(area.color.w, "w");
        writer->End();
        writer->End();
    }
    writer->End();
    writer->Close();
    return true;
}

} // namespace Build
} // namespace Navigation
