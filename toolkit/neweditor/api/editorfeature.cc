//------------------------------------------------------------------------------
//  @file editorfeature.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/api/editorfeature.h"
#include "neweditor/api/shell.h"
#include "neweditor/core/editorpanel.h"
#include "neweditor/model/scene.h"

namespace Editor
{

Util::Array<Ptr<EditorPanel>> EditorFeatureRegistry::panels;
Util::Array<EditorInspectorSection> EditorFeatureRegistry::sections;
Util::Array<EditorInspectorAddEntrySource> EditorFeatureRegistry::entrySources;
Util::Array<EditorAddComponentConstraint> EditorFeatureRegistry::constraints;
Util::Array<Game::ComponentId> EditorFeatureRegistry::runtimeComponents;
Util::Array<EditorOnComponentChangeListener> EditorFeatureRegistry::listeners;
Util::Array<EditorStatusNumbers> EditorFeatureRegistry::statusNumbers;
Shell* EditorFeatureRegistry::shell = nullptr;

//------------------------------------------------------------------------------
/**
*/
void
EditorFeatureRegistry::RegisterPanel(Ptr<EditorPanel> panel)
{
    n_assert(panel.isvalid());
    n_assert(!panel->GetName().IsEmpty());

    EditorFeatureRegistry::panels.Append(panel);
}

//------------------------------------------------------------------------------
/**
*/
void
EditorFeatureRegistry::RegisterInspectorSection(Util::Delegate<void(Game::Entity)> draw)
{
    n_assert(draw.IsValid());

    EditorInspectorSection section;
    section.draw = draw;
    EditorFeatureRegistry::sections.Append(section);
}

//------------------------------------------------------------------------------
/**
*/
void
EditorFeatureRegistry::RegisterInspectorAddEntries(Util::Delegate<void(Game::Entity, Util::Array<InspectorEntry>&)> collect)
{
    n_assert(collect.IsValid());

    EditorInspectorAddEntrySource source;
    source.collect = collect;
    EditorFeatureRegistry::entrySources.Append(source);
}

//------------------------------------------------------------------------------
/**
    The rule is stored as the set of components and the count, so the panel answers it from data. A
    feature with a rule the panel cannot answer from data is a feature the panel has to know about,
    which is what this is meant to avoid.
*/
void
EditorFeatureRegistry::RegisterAddComponentConstraint(const Util::Array<Game::ComponentId>& components, IndexT maxPerEntity)
{
    n_assert(!components.IsEmpty());
    n_assert(maxPerEntity > 0);

    EditorAddComponentConstraint constraint;
    constraint.components = components;
    constraint.maxPerEntity = maxPerEntity;
    EditorFeatureRegistry::constraints.Append(constraint);
}

//------------------------------------------------------------------------------
/**
    The components are kept as ids, so the runtime rule is data as well. A feature that had to
    answer with a function would be a feature the panel has to know about.
*/
void
EditorFeatureRegistry::RegisterRuntimeComponents(const Util::Array<Game::ComponentId>& components)
{
    n_assert(!components.IsEmpty());

    for (IndexT i = 0; i < components.Size(); i++)
        EditorFeatureRegistry::runtimeComponents.Append(components[i]);
}

//------------------------------------------------------------------------------
/**
*/
void
EditorFeatureRegistry::RegisterOnComponentChangeListener(Util::Delegate<void(Scene*, Game::Entity, Game::ComponentId)> onChange)
{
    n_assert(onChange.IsValid());

    EditorOnComponentChangeListener listener;
    listener.onChange = onChange;
    EditorFeatureRegistry::listeners.Append(listener);
}

//------------------------------------------------------------------------------
/**
*/
void
EditorFeatureRegistry::RegisterStatusNumbers(Util::Delegate<void(Util::String& out)> append)
{
    n_assert(append.IsValid());

    EditorStatusNumbers numbers;
    numbers.append = append;
    EditorFeatureRegistry::statusNumbers.Append(numbers);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Ptr<EditorPanel>>&
EditorFeatureRegistry::GetPanels()
{
    return EditorFeatureRegistry::panels;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<EditorInspectorSection>&
EditorFeatureRegistry::GetSections()
{
    return EditorFeatureRegistry::sections;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<EditorInspectorAddEntrySource>&
EditorFeatureRegistry::GetEntrySources()
{
    return EditorFeatureRegistry::entrySources;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<EditorAddComponentConstraint>&
EditorFeatureRegistry::GetConstraints()
{
    return EditorFeatureRegistry::constraints;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<EditorOnComponentChangeListener>&
EditorFeatureRegistry::GetListeners()
{
    return EditorFeatureRegistry::listeners;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<EditorStatusNumbers>&
EditorFeatureRegistry::GetStatusNumbers()
{
    return EditorFeatureRegistry::statusNumbers;
}

//------------------------------------------------------------------------------
/**
    An entity cannot be given a component when a pushed constraint says the set it belongs to is
    already full. The panel asks this rather than calling a feature's predicate, because the rule is
    data.
*/
bool
EditorFeatureRegistry::IsBlocked(Scene* scene, Game::Entity entity, Game::ComponentId component)
{
    for (IndexT i = 0; i < EditorFeatureRegistry::constraints.Size(); i++)
    {
        const EditorAddComponentConstraint& constraint = EditorFeatureRegistry::constraints[i];

        bool inSet = false;
        for (IndexT c = 0; c < constraint.components.Size(); c++)
        {
            if (constraint.components[c] == component)
            {
                inSet = true;
                break;
            }
        }
        if (!inSet)
            continue;

        IndexT has = 0;
        const Util::Array<Game::ComponentId>& components = scene->GetComponents(entity);
        for (IndexT k = 0; k < components.Size(); k++)
        {
            for (IndexT c = 0; c < constraint.components.Size(); c++)
            {
                if (components[k] == constraint.components[c])
                {
                    has++;
                    break;
                }
            }
        }

        if (has >= constraint.maxPerEntity)
            return true;
    }

    return false;
}

//------------------------------------------------------------------------------
/**
    Runtime state is only meaningful in the world that made it. The panel does not draw a section
    for a component in the list, and an entity cannot be given one. The answer comes from the ids a
    feature gave.
*/
bool
EditorFeatureRegistry::IsRuntimeComponent(Game::ComponentId component)
{
    for (IndexT i = 0; i < EditorFeatureRegistry::runtimeComponents.Size(); i++)
    {
        if (EditorFeatureRegistry::runtimeComponents[i] == component)
            return true;
    }

    return false;
}

//------------------------------------------------------------------------------
/**
    The core gives the shell at Open. A static part that nothing injected uses it.
*/
void
EditorFeatureRegistry::SetShell(Shell* shell)
{
    EditorFeatureRegistry::shell = shell;
}

//------------------------------------------------------------------------------
/**
*/
Shell*
EditorFeatureRegistry::GetShell()
{
    return EditorFeatureRegistry::shell;
}

//------------------------------------------------------------------------------
/**
    A delegate holds the object it was bound to, so a registry that outlived the feature would be a
    delegate pointing at something that is gone.
*/
void
EditorFeatureRegistry::Clear()
{
    EditorFeatureRegistry::panels.Clear();
    EditorFeatureRegistry::sections.Clear();
    EditorFeatureRegistry::entrySources.Clear();
    EditorFeatureRegistry::constraints.Clear();
    EditorFeatureRegistry::runtimeComponents.Clear();
    EditorFeatureRegistry::listeners.Clear();
    EditorFeatureRegistry::statusNumbers.Clear();
    EditorFeatureRegistry::shell = nullptr;
}

} // namespace Editor
//------------------------------------------------------------------------------
