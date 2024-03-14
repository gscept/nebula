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
#include "graphicsfeature/graphicsfeatureunit.h"
#include "editor/components/editorcomponents.h"
#include "input/inputserver.h"
#include "input/mouse.h"
#include "util/bvh.h"
#include "renderutil/mouserayutil.h"
#include "camera.h"

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
SelectionTool::Update(Math::vec2 const& viewPortPosition, Math::vec2 const& viewPortSize, Editor::Camera const* camera)
{
	Ptr<Input::Mouse> mouse = Input::InputServer::Instance()->GetDefaultMouse();
	bool performPicking = mouse->ButtonUp(Input::MouseButton::Code::LeftButton) && !isTransforming;

	if (performPicking)
	{
		selection.Clear();
		
		Math::vec2 mousePos = mouse->GetScreenPosition();
		//TODO: move mousepos to viewport space
		mousePos -= viewPortPosition;
		mousePos = { mousePos.x / viewPortSize.x, mousePos.y / viewPortSize.y };
		
		Math::mat4 const camTransform = Math::inverse(camera->GetViewTransform());
		Math::mat4 const invProj = Math::inverse(camera->GetProjectionTransform());

		Math::line ray = RenderUtil::MouseRayUtil::ComputeWorldMouseRay(mousePos, 10000, camTransform, invProj, 0.01f);

		Util::Bvh bvh;
		
		Util::Array<Editor::EditorEntity> editorEntities;
		Util::Array<Math::bbox> bboxes;

		Game::Filter filter = Game::FilterBuilder().Including<
			const Game::Entity, 
			const GraphicsFeature::Model, 
			const Editor::EditorEntity
		>().Build();

		Game::Dataset dataset = Game::GetWorld(WORLD_DEFAULT)->Query(filter);

		for (IndexT v = 0; v < dataset.numViews; v++)
		{
			Game::Dataset::View const* view = dataset.views + v;
			for (IndexT i = 0; i < view->numInstances; i++)
			{
				Game::Entity const& gameEntity = *((Game::Entity*)view->buffers[0] + i);
				GraphicsFeature::Model const& model = *((GraphicsFeature::Model*)view->buffers[1] + i);
				Editor::EditorEntity const& editorEntity = *((Editor::EditorEntity*)view->buffers[2] + i);

				Math::bbox const bbox = Models::ModelContext::ComputeBoundingBox(model.graphicsEntityId);
				editorEntities.Append(editorEntity);
				bboxes.Append(bbox);
			}
		}

		if (bboxes.Size() > 0)
		{
			bvh.Build(bboxes.Begin(), bboxes.Size());
			Util::Array<uint32_t> intersectionIndices = bvh.Intersect(ray);

			Util::Array<Editor::Entity> newSelection;

			for (IndexT i = 0; i < intersectionIndices.Size(); i++)
			{
				uint32_t const idx = intersectionIndices[i];
				Editor::EditorEntity const& editorEntity = editorEntities[idx];
				Math::bbox const& bbox = bboxes[idx];

				float t;
				if (bbox.intersects(ray, t))
				{
					newSelection.Append(editorEntity.id);
				}
			}
			Edit::SetSelection(newSelection);
		}

		Game::DestroyFilter(filter);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
SelectionTool::RenderGizmo()
{
	for (size_t i = 0; i < selection.Size(); i++)
	{
		if (!Editor::state.editorWorld->IsValid(selection[i]) || !Editor::state.editorWorld->HasInstance(selection[i]))
		{
			selection.EraseIndex(i);
			i--;
			continue;
		}
	}

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
		Game::Entity const gameEntity = Editor::state.editables[selection[0].index].gameEntity;
		world->SetComponent<Game::Position>(gameEntity, { pos });
		world->SetComponent<Game::Orientation>(gameEntity, { rot });
		world->SetComponent<Game::Scale>(gameEntity, { scale });
		world->MarkAsModified(gameEntity);
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
			Graphics::GraphicsEntityId const gfxEntity = world->GetComponent<GraphicsFeature::Model>(gameEntity).graphicsEntityId;
			Math::bbox const bbox = Models::ModelContext::ComputeBoundingBox(gfxEntity);
			Math::mat4 const transform = Models::ModelContext::GetTransform(gfxEntity);
			Im3d::Im3dContext::DrawOrientedBox(Math::mat4::identity, bbox, {1.0f, 0.30f, 0.0f, 1.0f});
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
