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

Util::Array<Editor::Entity> Tools::SelectionTool::selection = {};
static bool isDirty = false;
static Im3d::Mat4 tempTransform;

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
	
	Game::PropertyId const transformPid = Game::GetPropertyId("WorldTransform");
	Game::PropertyId const mdlPid = Game::GetPropertyId("ModelEntityData");

	if (Game::HasProperty(Editor::state.editorWorld, selection[0], transformPid))
	{
		if (!isDirty)
		{
			tempTransform = Game::GetProperty<Math::mat4>(Editor::state.editorWorld, selection[0], transformPid);
		}

		bool isTransforming = Im3d::Gizmo("GizmoEntity", tempTransform);
		if (isTransforming)
		{
			isDirty = true;
			Game::SetProperty(Game::GetWorld(WORLD_DEFAULT), Editor::state.editables[selection[0].index].gameEntity, transformPid, tempTransform);
		}
		else if(isDirty)
		{
			// User has release gizmo, we can set real transform and add to undo queue
			Edit::SetProperty(selection[0], transformPid, &tempTransform);
			isTransforming = false;
			isDirty = false;
		}
	}

	for (auto const editorEntity : selection)
	{
		Game::Entity const gameEntity = Editor::state.editables[editorEntity.index].gameEntity;
		if (Game::HasProperty(Game::GetWorld(WORLD_DEFAULT), gameEntity, mdlPid))
		{
			// TODO: Fixme!
			//Graphics::GraphicsEntityId const gfxEntity = Game::GetProperty<GraphicsFeature::ModelEntityData>(Game::GetWorld(WORLD_DEFAULT), gameEntity, mdlPid).gid;
			//Math::bbox const bbox = Models::ModelContext::GetBoundingBox(gfxEntity);
			//Math::mat4 const transform = Models::ModelContext::GetTransform(gfxEntity);
			//Im3d::Im3dContext::DrawOrientedBox(Math::mat4::identity, bbox, {1.0f, 0.30f, 0.0f, 1.0f});
		}
	}

}

} // namespace Tools
