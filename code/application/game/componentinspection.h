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
    using DrawFunc = void(*)(Game::Entity, Game::ComponentId, void*, bool*);
    
    static ComponentInspection* Instance();
    static void Destroy();

    static void Register(ComponentId component, DrawFunc);

    static void DrawInspector(Game::Entity owner, ComponentId component, void* data, bool* commit);

    // set to true if you want full information to be displayed in the inspector
    bool debug = false;

private:
    ComponentInspection();
    ~ComponentInspection();

    static ComponentInspection* Singleton;

    Util::Array<DrawFunc> inspectors;
};

template<typename TYPE>
inline void
ComponentDrawFuncT(Game::Entity owner, ComponentId component, void* data, bool* commit);

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
            ComponentDrawFuncT<field_type>(owner, component, (byte*)data + TYPE::Traits::field_byte_offsets[i], commit);

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
template<> void ComponentDrawFuncT<int64>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<uint>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<uint64>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<float>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Util::StringAtom>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Math::mat4>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Math::vec3>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Math::vec4>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Math::quat>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Game::Position>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Game::Orientation>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Game::Scale>(Game::Entity, ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Util::Color>(Game::Entity, ComponentId, void*, bool*);

} // namespace Game
