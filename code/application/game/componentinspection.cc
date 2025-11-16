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
ComponentInspection::Register(ComponentId component, DrawFunc func)
{
    ComponentInspection* reg = Instance();
    while (reg->inspectors.Size() <= component.id)
    {
        IndexT first = reg->inspectors.Size();
        reg->inspectors.Grow();
        reg->inspectors.Resize(reg->inspectors.Capacity());
        reg->inspectors.Fill(first, reg->inspectors.Size() - first, nullptr);
    }

    n_assert(reg->inspectors[component.id] == nullptr);
    reg->inspectors[component.id] = func;
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
    Game::World* world = Game::GetWorld(entity->world);

    if (*entity == Game::Entity::Invalid())
    {
        ImGui::Text("Unassigned");
    }
    else if (!world->IsValid(*entity) || !world->HasInstance(*entity))
    {
        ImGui::TextColored({1.0f,0.1f,0.1f,1.0f}, "Invalid");
    }
    else
    {
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
    if (ImGui::DragInt("##input_data", (int*)data, 1.0f, 0, 0xFFFFFFFF))
        *commit = true;
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
    if (ImGui::DragInt("##input_data", (int*)data, 1.0f, 0, 0xFFFFFFFF))
        *commit = true;
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

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<Util::StringAtom>(Game::Entity owner, ComponentId component, void* data, bool* commit)
{
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    if (((Util::StringAtom*)data)->IsValid())
    {
        ImGui::Text(((Util::StringAtom*)data)->Value());
    }
    else
    {
        ImGui::Text("None");
    }
    if (ImGui::BeginDragDropTarget())
    {
        auto payload = ImGui::AcceptDragDropPayload("resource");
        if (payload)
        {
            Util::String resourceName = (const char*)payload->Data;
            *(Util::StringAtom*)data = resourceName;
			*commit = true;
        }
        ImGui::EndDragDropTarget();
    }
    ImGui::PopID();
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
    ImGui::Text("Position");
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
    ImGui::Text("Orientation");
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    if (ImGui::DragFloat4("##orient", (float*)data, 0.01f))
        *commit = true;
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
    ImGui::Text("Scale");
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


} // namespace Game
