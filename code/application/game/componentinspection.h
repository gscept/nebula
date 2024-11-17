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
#include "util/color.h"
#include "imgui.h"

namespace Game
{

class ComponentInspection
{
public:
    using DrawFunc = void(*)(Game::ComponentId, void*, bool*);
    
    static ComponentInspection* Instance();
    static void Destroy();

    static void Register(ComponentId component, DrawFunc);

    static void DrawInspector(ComponentId component, void* data, bool* commit);

private:
    ComponentInspection();
    ~ComponentInspection();

    static ComponentInspection* Singleton;

    Util::Array<DrawFunc> inspectors;
};

template<typename TYPE>
inline void
ComponentDrawFuncT(ComponentId component, void* data, bool* commit);

template<typename TYPE, std::size_t i = 0>
inline void
InspectorDrawField(ComponentId component, void* data, bool* commit)
{
    if constexpr (i < TYPE::Traits::num_fields)
    {
        if constexpr (TYPE::Traits::field_hide_in_inspector[i])
        {
            // Just move to the next field
            InspectorDrawField<TYPE, i + 1>(component, data, commit);
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
                    ImGui::SetTooltip(TYPE::Traits::field_descriptions[i]);
                }
            }
            ImGui::TableSetColumnIndex(1);
            ComponentDrawFuncT<field_type>(component, (byte*)data + TYPE::Traits::field_byte_offsets[i], commit);

            ImGui::SameLine();
            ImGuiStyle const& style = ImGui::GetStyle();
            float widthNeeded =
                ImGui::CalcTextSize(TYPE::Traits::field_typenames[i]).x + style.FramePadding.x * 2.f + style.ItemSpacing.x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - widthNeeded);
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled(TYPE::Traits::field_typenames[i]);

            if constexpr (i < TYPE::Traits::num_fields - 1)
            {
                ImGui::TableNextRow();
                InspectorDrawField<TYPE, i + 1>(component, data, commit);
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
inline void
ComponentDrawFuncT(ComponentId component, void* data, bool* commit)
{
    if constexpr (requires { &TYPE::Traits::num_fields; })
    {
        if constexpr (TYPE::Traits::num_fields > 0 && !std::is_enum<TYPE>())
        {
            InspectorDrawField<TYPE>(component, data, commit);
        }
    }
    return;
}

template<> void ComponentDrawFuncT<Game::Entity>(ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<bool>(ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<int>(ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<int64>(ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<uint>(ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<uint64>(ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<float>(ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Util::StringAtom>(ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Math::mat4>(ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Math::vec3>(ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Math::vec4>(ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Math::quat>(ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Game::Position>(ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Game::Orientation>(ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Game::Scale>(ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Util::Color>(ComponentId, void*, bool*);

} // namespace Game
