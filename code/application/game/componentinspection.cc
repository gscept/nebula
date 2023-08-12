//------------------------------------------------------------------------------
//  componentinspection.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "componentinspection.h"
#include "memdb/typeregistry.h"
#include "game/entity.h"
#include "memdb/componentid.h"
#include "imgui.h"
#include "math/mat4.h"
#include "util/stringatom.h"

namespace Game
{

ComponentInspection* ComponentInspection::Singleton = nullptr;

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
        Singleton = n_new(ComponentInspection);
        n_assert(nullptr != Singleton);
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
        n_delete(Singleton);
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
ComponentInspection::DrawInspector(ComponentId component, void* data, bool* commit)
{
    ComponentInspection* reg = Instance();

    if (reg->inspectors.Size() > component.id)
    {
        if (reg->inspectors[component.id] != nullptr)
            reg->inspectors[component.id](component, data, commit);
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
ComponentDrawFuncT<Game::Entity>(ComponentId component, void* data, bool* commit)
{
	MemDb::ComponentDescription* desc = MemDb::TypeRegistry::GetDescription(component);

	Game::Entity* entity = (Game::Entity*)data;
	Ids::Id32 id = (Ids::Id32)*entity;
	ImGui::Text("%s: %u", desc->name.Value(), id);
	ImGui::SameLine();
	ImGui::TextDisabled("| gen: %i | index: %i", entity->generation, entity->index);
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<int>(ComponentId component, void* data, bool* commit)
{
    MemDb::ComponentDescription* desc = MemDb::TypeRegistry::GetDescription(component);
    
    if (ImGui::InputInt(desc->name.Value(), (int*)data))
        *commit = true;
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<uint>(ComponentId component, void* data, bool* commit)
{
    MemDb::ComponentDescription* desc = MemDb::TypeRegistry::GetDescription(component);
    
    if (ImGui::InputInt(desc->name.Value(), (int*)data))
        *commit = true;
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<float>(ComponentId component, void* data, bool* commit)
{
    MemDb::ComponentDescription* desc = MemDb::TypeRegistry::GetDescription(component);
    
    if (ImGui::InputFloat(desc->name.Value(), (float*)data))
        *commit = true;
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<Util::StringAtom>(ComponentId component, void* data, bool* commit)
{
    MemDb::ComponentDescription* desc = MemDb::TypeRegistry::GetDescription(component);
    ImGui::Text("%s: %s", desc->name.Value(), ((Util::StringAtom*)data)->Value());
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
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<Math::mat4>(ComponentId component, void* data, bool* commit)
{
    MemDb::ComponentDescription* desc = MemDb::TypeRegistry::GetDescription(component);
    
    ImGui::Text(desc->name.Value());
    if (ImGui::InputFloat4("##row0", (float*)data))
        *commit = true;
    if (ImGui::InputFloat4("##row1", (float*)data + 4))
        *commit = true;
    if (ImGui::InputFloat4("##row2", (float*)data + 8))
        *commit = true;
    if (ImGui::InputFloat4("##row3", (float*)data + 12))
        *commit = true;
}

} // namespace Game
