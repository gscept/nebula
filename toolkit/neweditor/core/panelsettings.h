#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::PanelSettings

    Which panels the editor had open the last time it ran.

    The file is 'bin:editor-panels.json' unless something sets another path.

    @see
    Editor::EditorCore

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "util/array.h"
#include "util/string.h"

namespace Editor
{

class Shell;

//------------------------------------------------------------------------------
/**
    One panel and the state the file has for it. The name is the identity, because it is the name the
    window is opened with.
*/
struct PanelRecord
{
    Util::String name;
    bool open = true;
};

//------------------------------------------------------------------------------
/**
*/
class PanelSettings : public Core::RefCounted
{
    __DeclareClass(PanelSettings)
public:
    PanelSettings();
    virtual ~PanelSettings();

    void Setup(Shell* core);
    void Discard();

    void SetPath(const Util::String& path);
    const Util::String& GetPath() const;

    /// read the file. Returns false when there was nothing to read
    bool Load();
    /// writes every record out as it is now
    bool Save();

    /// Check if a panel is open. A panel the file has no record of is as open as fallback says
    bool IsOpen(const Util::String& name, bool fallback = true) const;
    /// Set panel state
    void SetOpen(const Util::String& name, bool open);

private:
    /// the index of the record a name refers to, or InvalidIndex when nothing has it
    IndexT FindIndex(const Util::String& name) const;

    Shell* core;
    Util::String path;
    Util::Array<PanelRecord> records;
};

} // namespace Editor
//------------------------------------------------------------------------------
