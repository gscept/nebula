//------------------------------------------------------------------------------
//  propertyinspection.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "propertyinspection.h"
#include "memdb/typeregistry.h"
#include "game/entity.h"
#include "memdb/propertyid.h"
#include "imgui.h"
#include "math/mat4.h"
#include "util/stringatom.h"

namespace Game
{

PropertyInspection* PropertyInspection::Singleton = nullptr;

//------------------------------------------------------------------------------
/**
    The registry's constructor is called by the Instance() method, and
    nobody else.
*/
PropertyInspection*
PropertyInspection::Instance()
{
    if (nullptr == Singleton)
    {
        Singleton = n_new(PropertyInspection);
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
PropertyInspection::Destroy()
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
PropertyInspection::Register(PropertyId pid, DrawFunc func)
{
    PropertyInspection* reg = Instance();
    while (reg->inspectors.Size() <= pid.id)
    {
        IndexT first = reg->inspectors.Size();
        reg->inspectors.Grow();
        reg->inspectors.Resize(reg->inspectors.Capacity());
        reg->inspectors.Fill(first, reg->inspectors.Size() - first, nullptr);
    }

    n_assert(reg->inspectors[pid.id] == nullptr);
    reg->inspectors[pid.id] = func;
}

//------------------------------------------------------------------------------
/**
*/
void
PropertyInspection::DrawInspector(PropertyId pid, void* data, bool* commit)
{
    PropertyInspection* reg = Instance();

    if (reg->inspectors.Size() > pid.id)
    {
        if (reg->inspectors[pid.id] != nullptr)
            reg->inspectors[pid.id](pid, data, commit);
    }
}

//------------------------------------------------------------------------------
/**
*/
PropertyInspection::PropertyInspection()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
PropertyInspection::~PropertyInspection()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
PropertyDrawFuncT<int>(PropertyId pid, void* data, bool* commit)
{
    MemDb::PropertyDescription* desc = MemDb::TypeRegistry::GetDescription(pid);
    
    if (ImGui::InputInt(desc->name.Value(), (int*)data))
        *commit = true;
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
PropertyDrawFuncT<uint>(PropertyId pid, void* data, bool* commit)
{
    MemDb::PropertyDescription* desc = MemDb::TypeRegistry::GetDescription(pid);
    
    if (ImGui::InputInt(desc->name.Value(), (int*)data))
        *commit = true;
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
PropertyDrawFuncT<float>(PropertyId pid, void* data, bool* commit)
{
    MemDb::PropertyDescription* desc = MemDb::TypeRegistry::GetDescription(pid);
    
    if (ImGui::InputFloat(desc->name.Value(), (float*)data))
        *commit = true;
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
PropertyDrawFuncT<Util::StringAtom>(PropertyId pid, void* data, bool* commit)
{
    MemDb::PropertyDescription* desc = MemDb::TypeRegistry::GetDescription(pid);
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
PropertyDrawFuncT<Math::mat4>(PropertyId pid, void* data, bool* commit)
{
    MemDb::PropertyDescription* desc = MemDb::TypeRegistry::GetDescription(pid);
    
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
