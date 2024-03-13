#pragma once
//------------------------------------------------------------------------------
/**
	SelectionTool

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "editor/editor.h"

namespace Editor
{
class Camera;
}

namespace Edit
{
struct CMDSetSelection;
}

namespace Tools
{

class SelectionTool
{
public:
	static Util::Array<Editor::Entity> const& Selection();
	/// Call before Update
	static void RenderGizmo();
	/// Call after render
	static void Update(Math::vec2 const& viewPortPosition, Math::vec2 const& viewPortSize, Editor::Camera const* camera);
	static bool IsTransforming();

private:
	friend Edit::CMDSetSelection;
	static Util::Array<Editor::Entity> selection;
};


} // namespace Tools
