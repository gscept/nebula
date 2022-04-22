#pragma once
//------------------------------------------------------------------------------
/**
	SelectionTool

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "editor/editor.h"

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
	static void RenderGizmo();

private:
	friend Edit::CMDSetSelection;
	static Util::Array<Editor::Entity> selection;
};


} // namespace Tools
