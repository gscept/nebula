//------------------------------------------------------------------------------
//  editorfeatureunit.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "editorfeatureunit.h"
#include "editor/editor.h"
#include "editor/ui/uimanager.h"
#include "editor/components/editorcomponents.h"
#include "graphicsfeature/components/graphicsfeature.h"

// TEMP: Move this to editor game manager
#include "game/world.h"
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/orientation.h"
#include "basegamefeature/components/scale.h"
#include "graphicsfeature/components/graphicsfeature.h"
#include "models/modelcontext.h"

namespace EditorFeature
{

__ImplementClass(EditorFeature::EditorFeatureUnit, 'edfu', Game::FeatureUnit);
__ImplementSingleton(EditorFeature::EditorFeatureUnit);

//------------------------------------------------------------------------------
/**
*/
EditorFeatureUnit::EditorFeatureUnit()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
EditorFeatureUnit::~EditorFeatureUnit()
{
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
EditorFeatureUnit::OnAttach()
{
    Game::RegisterType<Editor::EditorEntity>();
}

//------------------------------------------------------------------------------
/**
*/
void
EditorFeatureUnit::OnActivate()
{
    FeatureUnit::OnActivate();
    if (this->args.GetBoolFlag("-editor"))
    {
        this->AttachManager(Editor::UIManager::Create());
        
        Editor::Create();

        // TODO: move this to a game manager that is created by the editor

        Game::World* world = Game::GetWorld(WORLD_DEFAULT);
        Game::ProcessorBuilder(world, "EditorGameManager.UpdateModelTransforms"_atm)
            .OnlyModified()
            .Func(
                [](Game::World* world, Game::Position const& pos, Game::Orientation const& orient, Game::Scale const& scale, GraphicsFeature::Model const& model)
                {
                    Math::mat4 worldTransform = Math::trs(pos, orient, scale);
                    Models::ModelContext::SetTransform(model.graphicsEntityId, worldTransform);
                }
            )
            .Build();

        //if (!Editor::ConnectToBackend(...))
        //    Editor::SpawnLocalBackend();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
EditorFeatureUnit::OnDeactivate()
{
    FeatureUnit::OnDeactivate();
    if (this->args.GetBoolFlag("-editor"))
    {
        Editor::Destroy();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
EditorFeatureUnit::OnEndFrame()
{
    FeatureUnit::OnEndFrame();
}

//------------------------------------------------------------------------------
/**
*/
void
EditorFeatureUnit::OnRenderDebug()
{
    FeatureUnit::OnRenderDebug();
}

//------------------------------------------------------------------------------
/**
*/
void
EditorFeatureUnit::OnFrame()
{
    FeatureUnit::OnFrame();
}

} // namespace EditorFeature
