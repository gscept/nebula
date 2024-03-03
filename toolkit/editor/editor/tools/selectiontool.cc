//------------------------------------------------------------------------------
//  selectiontool.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "util/array.h"
#include "selectiontool.h"
#include "dynui/im3d/im3d.h"
#include "editor/commandmanager.h"
#include "editor/cmds.h"
#include "models/modelcontext.h"
#include "graphicsfeature/managers/graphicsmanager.h"
#include "dynui/im3d/im3dcontext.h"
#include "basegamefeature/components/basegamefeature.h"
#include "graphicsfeature/components/graphicsfeature.h"
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/orientation.h"
#include "basegamefeature/components/scale.h"

Util::Array<Editor::Entity> Tools::SelectionTool::selection = {};
static bool isDirty = false;
static Im3d::Mat4 tempTransform;
static bool isTransforming = false;

namespace Tools
{

//------------------------------------------------------------------------------
/**
*/
Util::Array<Editor::Entity> const&
SelectionTool::Selection()
{
    return SelectionTool::selection;
}

//------------------------------------------------------------------------------
/**
*/
void
SelectionTool::RenderGizmo()
{
    if (selection.IsEmpty())
		return;
	
	if (!isDirty)
	{
		auto pos = Editor::state.editorWorld->GetComponent<Game::Position>(selection[0]);
		auto orientation = Editor::state.editorWorld->GetComponent<Game::Orientation>(selection[0]);
		auto scale = Editor::state.editorWorld->GetComponent<Game::Scale>(selection[0]);

		tempTransform = Math::trs(pos, orientation, scale);
	}

	Game::World* world = Game::GetWorld(WORLD_DEFAULT);

	isTransforming = Im3d::Gizmo("GizmoEntity", tempTransform);
	Math::vec3 pos;
	Math::quat rot;
	Math::vec3 scale;
	Math::decompose(tempTransform, scale, rot, pos);

	if (isTransforming)
	{
		isDirty = true;
		world->SetComponent<Game::Position>(Editor::state.editables[selection[0].index].gameEntity, { pos });
        world->SetComponent<Game::Orientation>(Editor::state.editables[selection[0].index].gameEntity, { rot } );
        world->SetComponent<Game::Scale>(Editor::state.editables[selection[0].index].gameEntity, { scale } );
	}
	else if(isDirty)
	{
		// User has release gizmo, we can set real transform and add to undo queue
		Edit::CommandManager::BeginMacro("Modify transform", false);
		Edit::SetComponent(selection[0], Game::GetComponentId<Game::Position>(), &pos);
		Edit::SetComponent(selection[0], Game::GetComponentId<Game::Orientation>(), &rot);
		Edit::SetComponent(selection[0], Game::GetComponentId<Game::Scale>(), &scale);
		Edit::CommandManager::EndMacro();
		isTransforming = false;
		isDirty = false;
	}

	for (auto const editorEntity : selection)
	{
		Game::Entity const gameEntity = Editor::state.editables[editorEntity.index].gameEntity;
		if (world->HasComponent<GraphicsFeature::Model>(gameEntity))
		{
			// TODO: Fixme!
			//Graphics::GraphicsEntityId const gfxEntity = Game::GetComponent<GraphicsFeature::ModelEntityData>(Game::GetWorld(WORLD_DEFAULT), gameEntity, mdlPid).gid;
			//Math::bbox const bbox = Models::ModelContext::GetBoundingBox(gfxEntity);
			//Math::mat4 const transform = Models::ModelContext::GetTransform(gfxEntity);
			//Im3d::Im3dContext::DrawOrientedBox(Math::mat4::identity, bbox, {1.0f, 0.30f, 0.0f, 1.0f});
		}
	}

}

//------------------------------------------------------------------------------
/**
*/
bool
SelectionTool::IsTransforming()
{
    return isTransforming;
}

} // namespace Tools
