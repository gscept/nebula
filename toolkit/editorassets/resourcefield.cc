//------------------------------------------------------------------------------
//  @file resourcefield.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "editorassets/resourcefield.h"
#include "editorassets/assetfield.h"
#include "game/componentinspection.h"
#include "imgui.h"

namespace Editor
{

namespace
{

//------------------------------------------------------------------------------
/**
    Which field of which entity, the field by its name, which is the trait's string
*/
struct FieldKey
{
    Game::Entity entity = Game::Entity::Invalid();
    Game::ComponentId component = Game::ComponentId::Invalid();
    Util::String field;

    bool
    Is(Game::Entity otherEntity, Game::ComponentId otherComponent, const char* otherField) const
    {
        return this->entity == otherEntity && this->component == otherComponent && this->field == otherField;
    }
};

/// the field drawing now, and the copy it draws into
struct Drawing
{
    FieldKey key;
    Util::StringAtom* data = nullptr;
    bool* commit = nullptr;
};
Drawing drawing;

/// a picker's answer, waiting for its field to draw
struct PendingPick
{
    FieldKey key;
    Util::StringAtom value;
    int frame = 0;
    bool valid = false;
};
PendingPick pending;

/// the frames an answer waits for its field. The picker answers after the frame, so the field
/// draws in the next one if the inspector still shows it
const int PendingFrames = 2;

//------------------------------------------------------------------------------
/**
*/
void
WriteField(Util::StringAtom* data, bool* commit, const Util::StringAtom& value)
{
    if (*data == value)
        return;
    *data = value;
    *commit = true;
}

//------------------------------------------------------------------------------
/**
*/
void
DrawResourceField(Game::Entity owner, Game::ComponentId component, const char* field, const char* resourceType, void* data, bool* commit)
{
    // A field that does not say what it takes could hold anything, a path the loader reads rather
    // than a URN, so it is edited as the text it is
    if (resourceType == nullptr)
    {
        Game::ComponentDrawFuncT<Util::StringAtom>(owner, component, data, commit);
        return;
    }

    Util::StringAtom* atom = (Util::StringAtom*)data;

    // An answer that waited too long is for a field that is no longer shown. Dropping it before
    // looking at the key is what keeps it from being written when that entity is shown again
    if (pending.valid && ImGui::GetFrameCount() > pending.frame + PendingFrames)
        pending.valid = false;
    if (pending.valid && pending.key.Is(owner, component, field))
    {
        WriteField(atom, commit, pending.value);
        pending.valid = false;
    }

    const IO::Path asset = atom->IsValid() ? IO::Path::Parse(atom->Value()) : IO::Path();

    FieldKey key;
    key.entity = owner;
    key.component = component;
    key.field = field;

    drawing.key = key;
    drawing.data = atom;
    drawing.commit = commit;

    // the same width the inspector gives the other widgets, which leaves room for the prefab marker
    AssetField(field, asset, resourceType, [key](const IO::Path& picked)
    {
        const Util::StringAtom value(picked.AsString());
        if (drawing.data != nullptr && drawing.key.Is(key.entity, key.component, key.field.AsCharPtr()))
        {
            WriteField(drawing.data, drawing.commit, value);
        }
        else
        {
            pending.key = key;
            pending.value = value;
            pending.frame = ImGui::GetFrameCount();
            pending.valid = true;
        }
    }, ImGui::CalcItemWidth());

    drawing = Drawing();
}

} // namespace

//------------------------------------------------------------------------------
/**
*/
void
SetupResourceFields()
{
    Game::ComponentInspection::SetResourceFieldDrawer(&DrawResourceField);
}

//------------------------------------------------------------------------------
/**
*/
void
DiscardResourceFields()
{
    Game::ComponentInspection::SetResourceFieldDrawer(nullptr);
    pending = PendingPick();
    drawing = Drawing();
}

} // namespace Editor
