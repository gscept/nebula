#pragma once
//------------------------------------------------------------------------------
/**
    @class Game::ComponentInspection
    
    Component inspection functions.

    Implements various inspection functions for different types of components
    
    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/delegate.h"
#include "util/stringatom.h"
#include "game/entity.h"
#include "game/componentid.h"
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/orientation.h"
#include "basegamefeature/components/scale.h"
#include "basegamefeature/components/velocity.h"
#include "util/bitfield.h"
#include "util/color.h"
#include "imgui.h"

namespace Game
{

class ComponentInspection
{
public:
    using DrawFunc = void(*)(Game::Entity, Game::ComponentId, void*, bool*);
    
    static ComponentInspection* Instance();
    static void Destroy();

    template<typename COMPONENT_TYPE>
    static void Register(ComponentId component);

    static void DrawInspector(Game::Entity owner, ComponentId component, void* data, bool* commit);

    /// Set a custom component inspector draw function
    static void OverrideInspector(ComponentId component, DrawFunc func);

    // set to true if you want full information to be displayed in the inspector
    bool debug = false;

private:
    ComponentInspection();
    ~ComponentInspection();

    static ComponentInspection* Singleton;

    Util::Array<DrawFunc> inspectors;
};

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
inline void
ComponentDrawFuncT(Game::Entity owner, ComponentId component, void* data, bool* commit);

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE, std::size_t i = 0>
inline void
InspectorDrawField(Game::Entity owner, ComponentId component, void* data, bool* commit)
{
    if constexpr (i < TYPE::Traits::num_fields)
    {
        if (TYPE::Traits::field_hide_in_inspector[i] && !ComponentInspection::Instance()->debug)
        {
            // Just move to the next field
            InspectorDrawField<TYPE, i + 1>(owner, component, data, commit);
        }
        else
        {
            using field_tuple = typename TYPE::Traits::field_types;
            using field_type = typename std::tuple_element<i, field_tuple>::type;
            Util::String fieldName = TYPE::Traits::field_names[i];
            fieldName.CamelCaseToWords();
            fieldName.Capitalize();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::Text(fieldName.AsCharPtr());
            if (TYPE::Traits::field_descriptions[i] != nullptr)
            {
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                {
                    if (ImGui::BeginTooltip())
                    {
                        ImGui::TextDisabled(TYPE::Traits::field_typenames[i]);
                        ImGui::Text(TYPE::Traits::field_descriptions[i]);
                        ImGui::EndTooltip();
                    }
                }
            }
            ImGui::TableSetColumnIndex(1);
            if constexpr (std::is_enum<field_type>())
            {
                ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
                void* fieldData = (byte*)data + TYPE::Traits::field_byte_offsets[i];
                const char* const* names = TYPE::Traits::field_enum_names[i];
                int count = (int)TYPE::Traits::field_enum_counts[i];

                if (names != nullptr && count > 0)
                {
                    const int* const values = TYPE::Traits::field_enum_values[i];
                    const int current = (int)*(field_type*)fieldData;

                    // the field holds the enum value, the combo works on indices into the sorted
                    // name and value arrays, which only match for enums that are contiguous from zero
                    int currentIdx = 0;
                    for (int k = 1; k < count; k++)
                    {
                        if (values[k] == current)
                        {
                            currentIdx = k;
                            break;
                        }
                    }

                    if (ImGui::Combo("##enum_input", &currentIdx, names, count))
                    {
                        *commit = true;
                        *(field_type*)fieldData = (field_type)values[currentIdx];
                    }
                }
                else
                {
                    // an enum we have no value names for
                    int value = (int)*(field_type*)fieldData;
                    if (ImGui::DragInt("##enum_input", &value))
                    {
                        *commit = true;
                        *(field_type*)fieldData = (field_type)value;
                    }
                }
                ImGui::PopID();
            }
            else
            {
                ComponentDrawFuncT<field_type>(owner, component, (byte*)data + TYPE::Traits::field_byte_offsets[i], commit);
            }

            if constexpr (i < TYPE::Traits::num_fields - 1)
            {
                ImGui::TableNextRow();
                InspectorDrawField<TYPE, i + 1>(owner, component, data, commit);
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
inline void
ComponentDrawFuncT(Game::Entity owner, ComponentId component, void* data, bool* commit)
{
    if constexpr (requires { &TYPE::Traits::num_fields; })
    {
        if constexpr (TYPE::Traits::num_fields > 0 && !std::is_enum<TYPE>())
        {
            InspectorDrawField<TYPE>(owner, component, data, commit);
        }
    }
    return;
}

template<> void ComponentDrawFuncT<Game::Entity>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<bool>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<int>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<int64_t>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<uint>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<uint64_t>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<uint16_t>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<int16_t>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<float>(Game::Entity, ComponentId, void*, bool*);
extern void (*StringAtomComponentDrawOverride) (Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Util::StringAtom>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Math::mat4>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Math::vec3>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Math::vec4>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Math::quat>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Game::Position>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Game::Orientation>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Game::Scale>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Util::Color>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Util::BitField<16>>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Util::BitField<32>>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Util::BitField<64>>(Game::Entity, ComponentId, void*, bool*);

//------------------------------------------------------------------------------
/**
*/
template<typename COMPONENT_TYPE>
void
ComponentInspection::Register(ComponentId component)
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
    reg->inspectors[component.id] = &Game::ComponentDrawFuncT<COMPONENT_TYPE>;
}

} // namespace Game
