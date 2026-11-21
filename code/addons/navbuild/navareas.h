#pragma once
//------------------------------------------------------------------------------
/**
    Navigation::Build area table

    The areas a project classifies walkable surface with. The table is written as
    work/data/tables/navareas.json and compiled to nav:navareas.navareas.

    Area 0, "none", is not walkable and area 63, "default", is every surface nothing
    marks. Both always exist, the table adds areas 1 to 62.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/array.h"
#include "util/blob.h"
#include "util/string.h"
#include "math/vec4.h"
#include "io/uri.h"

namespace Navigation
{
namespace Build
{

/// the compiled table
static constexpr const char* AreaTableFile = "nav:navareas.navareas";
/// the table a project edits
static constexpr const char* AreaTableSource = "proj:work/data/tables/navareas.json";

static constexpr unsigned char NoArea = 0;
static constexpr unsigned char MaxAreas = 64;

struct AreaDefinition
{
    unsigned char id = 0;
    Util::String name;
    float cost = 1.0f;
    unsigned short flags = 0;
    Math::vec4 color;
};

class AreaTable
{
public:
    /// a table with the built in areas only
    AreaTable();

    /// load a compiled table, keeps the built in areas if the file is missing. Returns false if the file is invalid
    bool Load(const IO::URI& file);
    /// load from compiled table data
    bool Load(const void* data, SizeT size);

    /// find an area by name, null if there is none
    const AreaDefinition* Find(const Util::String& name) const;
    /// get an area by id, null if the id isn't defined
    const AreaDefinition* Get(unsigned char id) const;
    /// all defined areas, ordered by id
    const Util::Array<AreaDefinition>& Areas() const;
    /// the polygon flags of every area id, 0 for undefined ids
    void GetFlags(unsigned short (&flags)[MaxAreas]) const;
    /// the color of an area, a generated one if the area or its color is undefined
    Math::vec4 GetColor(unsigned char id) const;
    /// the compiled data the table was loaded from, empty for the built in table
    const Util::Blob& GetData() const;

private:
    void SetupBuiltIn();
    void Sort();

    Util::Array<AreaDefinition> areas;
    Util::Blob data;
};

/// compile the project table to AreaTableFile, returns false if there is no project table or it doesn't compile
bool CompileAreaTable();
/// write areas as a table to compile, like the project table. The built in areas are left out, they always exist
bool WriteAreaTableSource(const Util::Array<AreaDefinition>& areas, const IO::URI& file);

} // namespace Build
} // namespace Navigation
