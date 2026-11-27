//------------------------------------------------------------------------------
//  componentinspection.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "componentinspection.h"
#include "memdb/attributeregistry.h"
#include "game/entity.h"
#include "memdb/attributeid.h"
#include "imgui.h"
#include "math/mat4.h"
#include "util/stringatom.h"
#include "editorstate.h"
#include "core/cvar.h"
//#include "dynui/im3d/im3dcontext.h"
#include "game/world.h"

namespace Game
{

ComponentInspection* ComponentInspection::Singleton = nullptr;

static Core::CVar* cl_draw_entity_references;

//------------------------------------------------------------------------------
/**
    The registry's constructor is called by the Instance() method, and
    nobody else.
*/
ComponentInspection*
ComponentInspection::Instance()
{
    if (nullptr == Singleton)
    {
        Singleton = new ComponentInspection;
        n_assert(nullptr != Singleton);

        cl_draw_entity_references = Core::CVarCreate(Core::CVarType::CVar_Int, "cl_draw_entity_references", "1", "Draw entity references: (1) when inspecting entity, (2) always, (0) never.");
    }
    return Singleton;
}

//------------------------------------------------------------------------------
/**
    This static method is used to destroy the registry object and should be
    called right before the main function exits. It will make sure that
    no accidential memory leaks are reported by the debug heap.
*/
void
ComponentInspection::Destroy()
{
    if (nullptr != Singleton)
    {
        delete Singleton;
        Singleton = nullptr;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentInspection::DrawInspector(Game::Entity owner, ComponentId component, void* data, bool* commit)
{
    ComponentInspection* reg = Instance();

    if (reg->inspectors.Size() > component.id)
    {
        if (reg->inspectors[component.id] != nullptr)
            reg->inspectors[component.id](owner, component, data, commit);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentInspection::OverrideInspector(ComponentId component, DrawFunc func)
{
    ComponentInspection* reg = Instance();

    // The inspectors are indexed by the component id, so the array has to reach the slot before it can hold the function
    while (reg->inspectors.Size() <= component.id)
    {
        IndexT first = reg->inspectors.Size();
        reg->inspectors.Grow();
        reg->inspectors.Resize(reg->inspectors.Capacity());
        reg->inspectors.Fill(first, reg->inspectors.Size() - first, nullptr);
    }

    reg->inspectors[component.id] = func;
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentInspection::OverrideField(ComponentId component, const Util::StringAtom& field, DrawFunc func)
{
    ComponentInspection* reg = Instance();
    for (FieldOverride& fieldOverride : reg->fieldOverrides)
    {
        if (fieldOverride.component == component && fieldOverride.field == field)
        {
            fieldOverride.func = func;
            return;
        }
    }
    reg->fieldOverrides.Append({ component, field, func });
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentInspection::SetResourceFieldDrawer(ResourceDrawFunc func)
{
    Instance()->resourceDrawer = func;
}

//------------------------------------------------------------------------------
/**
    Without a drawer a resource is shown the way any string is, as text
*/
void
ComponentInspection::DrawResourceField(Game::Entity owner, ComponentId component, const char* field, const char* resourceType, void* data, bool* commit)
{
    ComponentInspection* reg = Instance();
    if (reg->resourceDrawer != nullptr)
        reg->resourceDrawer(owner, component, field, resourceType, data, commit);
    else
        ComponentDrawFuncT<Util::StringAtom>(owner, component, data, commit);
}

//------------------------------------------------------------------------------
/**
    A field of type resource holds one whether or not it says which type, and a string field holds
    one when it names the type
*/
bool
ComponentInspection::IsResourceField(const char* fieldTypename, const char* resourceType)
{
    return resourceType != nullptr || (fieldTypename != nullptr && Util::String::StrCmp(fieldTypename, "Resources::ResourceName") == 0);
}

//------------------------------------------------------------------------------
/**
    Few fields are overridden, a linear search is cheaper than a lookup structure
*/
ComponentInspection::DrawFunc
ComponentInspection::GetFieldOverride(ComponentId component, const char* field)
{
    ComponentInspection* reg = Instance();
    for (const FieldOverride& fieldOverride : reg->fieldOverrides)
    {
        if (fieldOverride.component == component && fieldOverride.field == field)
        {
            return fieldOverride.func;
        }
    }
    return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
ComponentInspection::ComponentInspection()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ComponentInspection::~ComponentInspection()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<Game::Entity>(Game::Entity owner, ComponentId component, void* data, bool* commit)
{
    MemDb::Attribute* desc = MemDb::AttributeRegistry::GetAttribute(component);

    Game::Entity* entity = (Game::Entity*)data;

    if (*entity == Game::Entity::Invalid())
    {
        ImGui::Text("Unassigned");
    }
    else if (!Game::GetWorld(entity->world)->IsValid(*entity) || !Game::GetWorld(entity->world)->HasInstance(*entity))
    {
        ImGui::TextColored({1.0f,0.1f,0.1f,1.0f}, "Invalid");
    }
    else
    {
        Game::World* world = Game::GetWorld(entity->world);
        ImGui::BeginGroup();
        //if (Game::EditorState::Instance()->isRunning)
        //{
        //    // TODO: We should show the name of the entity here
        //    //ImGui::Text("%s", );
        //}
        //else
        {
            Ids::Id64 id = (Ids::Id64)*entity;
            ImGui::Text("%s %lu", desc->name.Value(), id);
            ImGui::SameLine();
            ImGui::TextDisabled("| gen: %i | index: %i", entity->generation, entity->index);
        }
        ImGui::EndGroup();

        //if (Core::CVarReadInt(cl_draw_entity_references) == 1)
        //{
        //    if (owner.world == entity->world)
        //    {
        //        Game::Position p0 = world->GetComponent<Position>(owner);
        //        Game::Position p1 = world->GetComponent<Position>(*entity);
        //        Math::line line = Math::line(p0, p1); 
        //        Im3d::Im3dContext::DrawLine(line, 1.0f, Math::vec4(0.0f, 1.0f, 0.0f, 1.0f));
        //    }
        //}
    }
    if (ImGui::BeginDragDropTarget())
    {
        auto payload = ImGui::AcceptDragDropPayload("entity");
        if (payload)
        {
            Game::Entity entityPayload = *(Game::Entity*)payload->Data;
            *(Game::Entity*)data = entityPayload;
            *commit = true;
        }
        ImGui::EndDragDropTarget();
    }
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<bool>(Game::Entity owner, ComponentId component, void* data, bool* commit)
{
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    if (ImGui::Checkbox("##input_data", (bool*)data))
        *commit = true;
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<int>(Game::Entity owner, ComponentId component, void* data, bool* commit)
{
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    if (ImGui::DragInt("##input_data", (int*)data))
        *commit = true;
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
template <>
void
ComponentDrawFuncT<int64_t>(Game::Entity owner, ComponentId component, void* data, bool* commit)
{
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    if (ImGui::DragInt("##input_data", (int*)data))
        *commit = true;
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<uint>(Game::Entity owner, ComponentId component, void* data, bool* commit)
{
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    int value = *(uint*)data;
    if (ImGui::DragInt("##input_data", &value, 1.0f, 0, 0xFFFFFFFF))
    {
        *commit = true;
        *(uint*)data = (uint)value;
    }
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<uint16_t>(Game::Entity owner, ComponentId component, void* data, bool* commit)
{
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    int value = *(uint16_t*)data;
    if (ImGui::DragInt("##input_data", &value, 1.0f, 0, UINT16_MAX))
    {
        *commit = true;
        *(uint16_t*)data = (uint16_t)value;
    }
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<int16_t>(Game::Entity owner, ComponentId component, void* data, bool* commit)
{
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    int value = *(int16_t*)data;
    if (ImGui::DragInt("##input_data", &value, 1.0f, INT16_MIN, INT16_MAX))
    {
        *commit = true;
        *(int16_t*)data = (int16_t)value;
    }
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
template <>
void
ComponentDrawFuncT<uint64_t>(Game::Entity owner, ComponentId component, void* data, bool* commit)
{
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    int value = *(uint64_t*)data;
    if (ImGui::DragInt("##input_data", &value, 1.0f, 0, 0xFFFFFFFF))
    {
        *commit = true;
        *(uint64_t*)data = (uint64_t)value;
    }
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<float>(Game::Entity owner, ComponentId component, void* data, bool* commit)
{
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    if (ImGui::DragFloat("##float_input", (float*)data))
        *commit = true;
    ImGui::PopID();
}

void (*StringAtomComponentDrawOverride) (Game::Entity, ComponentId, void*, bool*) = nullptr;
//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<Util::StringAtom>(Game::Entity owner, ComponentId component, void* data, bool* commit)
{
    if (StringAtomComponentDrawOverride)
    {
        StringAtomComponentDrawOverride(owner, component, data, commit);
    }
    else
    {
        // The text is written when the field is left or Enter is pressed, not while typing, so an
        // edit is one change and no atom is made for every key
        ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));

        Util::StringAtom* atom = (Util::StringAtom*)data;
        char buffer[1024];
        strncpy(buffer, atom->IsValid() ? atom->Value() : "", sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
        ImGui::InputText("##string", buffer, sizeof(buffer));
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            const Util::StringAtom edited(buffer);
            if (edited != *atom)
            {
                *atom = edited;
                *commit = true;
            }
        }
        ImGui::PopID();
    }

}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<Math::mat4>(Game::Entity owner, ComponentId component, void* data, bool* commit)
{
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    if (ImGui::InputFloat4("##row0", (float*)data))
        *commit = true;
    if (ImGui::InputFloat4("##row1", (float*)data + 4))
        *commit = true;
    if (ImGui::InputFloat4("##row2", (float*)data + 8))
        *commit = true;
    if (ImGui::InputFloat4("##row3", (float*)data + 12))
        *commit = true;
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<Math::vec3>(Game::Entity owner, ComponentId component, void* data, bool* commit)
{
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    if (ImGui::InputFloat3("##vec3", (float*)data))
        *commit = true;
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
template <>
void
ComponentDrawFuncT<Math::vec4>(Game::Entity owner, ComponentId component, void* data, bool* commit)
{
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    if (ImGui::InputFloat4("##vec4", (float*)data))
        *commit = true;
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
template <>
void
ComponentDrawFuncT<Math::quat>(Game::Entity owner, ComponentId component, void* data, bool* commit)
{
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    if (ImGui::InputFloat4("##quat", (float*)data))
        *commit = true;
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<Game::Position>(Game::Entity owner, ComponentId component, void* data, bool* commit)
{
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("Position");
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    if (ImGui::DragFloat3("##pos", (float*)data, 0.01f))
        *commit = true;
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<Game::Orientation>(Game::Entity owner, ComponentId component, void* data, bool* commit)
{
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("Orientation");
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));

    static bool showQuaternion = false;

    if (showQuaternion)
    {
        if (ImGui::DragFloat4("##orient", (float*)data, 0.01f))
            *commit = true;
    }
    else
    {
        // The component holds a quaternion, so the angles are converted on the way in and back on the way out.
        // They are shown in degrees, because that is what an editor shows.
        //
        // to_euler() only ever gives back one decomposition, with y limited to [-90, 90]. Deriving the angles from the
        // quaternion every frame turns a y of 91 into (180, 89, 180), the drag then continues from 89 and the rotation
        // bounces around 90 instead of reaching 180. So the angles last entered are kept for as long as the quaternion
        // is still the one they produced, and only re-derived once something else (gizmo, undo) has changed it.
        static struct
        {
            Game::Entity owner = Game::Entity::Invalid();
            float orientation[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            float angles[3] = { 0.0f, 0.0f, 0.0f };
        } entered;

        float const* current = (float const*)data;
        bool isEntered = entered.owner == owner;
        for (int i = 0; i < 4 && isEntered; i++)
            isEntered = Math::abs(current[i] - entered.orientation[i]) < 1e-5f;

        float angles[3];
        if (isEntered)
        {
            angles[0] = entered.angles[0];
            angles[1] = entered.angles[1];
            angles[2] = entered.angles[2];
        }
        else
        {
            Math::quat q;
            q.loadu(current);
            Math::vec3 euler = Math::to_euler(q);
            angles[0] = Math::rad2deg(euler.x);
            angles[1] = Math::rad2deg(euler.y);
            angles[2] = Math::rad2deg(euler.z);
        }

        if (ImGui::DragFloat3("##euler", angles, 1.0f))
        {
            Math::vec3 rad(Math::deg2rad(angles[0]), Math::deg2rad(angles[1]), Math::deg2rad(angles[2]));
            Math::quat q = Math::from_euler(rad);
            q.storeu((float*)data);
            *commit = true;

            entered.owner = owner;
            q.storeu(entered.orientation);
            entered.angles[0] = angles[0];
            entered.angles[1] = angles[1];
            entered.angles[2] = angles[2];
        }
    }

    ImGui::SameLine();
    ImGui::Checkbox("Quaternion", &showQuaternion);

    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<Game::Scale>(Game::Entity owner, ComponentId component, void* data, bool* commit)
{
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("Scale");
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));

    static bool uniformScaling = true;
    if (uniformScaling)
    {
        float* f = (float*)data;
        if (ImGui::DragFloat("##scl", f, 0.01f))
        {
            f[1] = f[0];
            f[2] = f[0];
            *commit = true;
        }
    }
    else if (ImGui::DragFloat3("##scl", (float*)data, 0.01f))
    {
        *commit = true;
    }

    ImGui::SameLine();
    ImGui::Checkbox("Uniform scale", &uniformScaling);

    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<Util::Color>(Game::Entity owner, ComponentId component, void* data, bool* commit)
{
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    if (ImGui::ColorEdit4("##color", (float*)data))
        *commit = true;
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<Util::BitField<16>>(Game::Entity owner, ComponentId component, void* data, bool* commit)
{
    Util::BitField<16>* bitfield = (Util::BitField<16>*)data;
    const char* labels[16];
    for (uint i = 0; i < 16; i++)
    {
        bool bitSet = bitfield->IsSet(i);
        labels[i] = bitSet ? "1" : "0";
        if (i > 0)
        {
            ImGui::SameLine();
            if (i % 8 == 0)
            {
                ImGui::NewLine();
            }
        }
        ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data) + i);
        if (ImGui::Button(labels[i]))
        {
            if (bitSet)
                bitfield->ClearBit(i);
            else
                bitfield->SetBit(i);
            *commit = true;
        }
        ImGui::PopID();
    }
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<Util::BitField<32>>(Game::Entity owner, ComponentId component, void* data, bool* commit)
{
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    if (ImGui::ColorEdit4("##color", (float*)data))
        *commit = true;
    ImGui::PopID();
}


//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<Util::BitField<64>>(Game::Entity owner, ComponentId component, void* data, bool* commit)
{
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    if (ImGui::ColorEdit4("##color", (float*)data))
        *commit = true;
    ImGui::PopID();
}
} // namespace Game
