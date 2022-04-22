//------------------------------------------------------------------------------
//  history.cc
//  (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "history.h"
#include "editor/editor.h"
#include "editor/commandmanager.h"
#include "editor/ui/uimanager.h"

using namespace Editor;

namespace Presentation
{
__ImplementClass(Presentation::History, 'HsWn', Presentation::BaseWindow);

//------------------------------------------------------------------------------
/**
*/
History::History()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
History::~History()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
History::Update()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
History::Run()
{
	Edit::CommandManager::CommandList const& undoList = Edit::CommandManager::GetUndoList();
	uint uniqueId = 0x2f3dd2dau;
	static uint lastHistorySize = 0;
	uint historySize = 0;
	for (Edit::CommandManager::CommandList::Iterator it = undoList.Begin(); it != undoList.End(); it++)
	{
		bool const macro = it->commands.Size() > 1;
		if (macro)
		{
			ImGui::PushID(uniqueId++);
			int const numLines = it->listAll ? it->commands.Size() + 1 : 1;
			ImGui::BeginChild("macro", {0, numLines * ImGui::GetTextLineHeightWithSpacing()});
			if (it->name == nullptr)
				ImGui::Text("[MACRO]");
			else
				ImGui::Text("[%s]", it->name.AsCharPtr());
		
			ImGui::Indent();
		}

		if (!macro || it->listAll)
		{
			for (Edit::Command* cmd : it->commands)
			{
				ImGui::Text("%s", cmd->Name());
			}
		}

		if (macro)
		{
			ImGui::Unindent();
			ImGui::EndChild();
			ImGui::PopID();
		}
		historySize++;
	}
	Edit::CommandManager::CommandList const& redoList = Edit::CommandManager::GetRedoList();
	if (redoList.Begin() != nullptr)
	{
		for (Edit::CommandManager::CommandList::Iterator it = redoList.Last();; it--)
		{	
			bool const macro = it->commands.Size() > 1;
			if (macro)
			{
				ImGui::PushID(uniqueId++);
				int const numLines = it->listAll ? it->commands.Size() + 1 : 1;
				ImGui::BeginChild("macro", {0, numLines * ImGui::GetTextLineHeightWithSpacing()});
				if (it->name == nullptr)
					ImGui::TextDisabled("[MACRO]");
				else
					ImGui::TextDisabled("[%s]", it->name.AsCharPtr());
			
				ImGui::Indent();
			}

			if (!macro || it->listAll)
			{
				for (int i = (*it).commands.Size() - 1; i >= 0; i-- )
				{
					Edit::Command* cmd = it->commands[i];
					ImGui::TextDisabled("%s", cmd->Name());
				}
			}

			if (macro)
			{
				ImGui::Unindent();
				ImGui::EndChild();
				ImGui::PopID();
			}
			historySize++;

			if (it == redoList.Begin())
				break;
		}
	}
	if (lastHistorySize != historySize)
		ImGui::SetScrollHereY(1.0f);
	lastHistorySize = historySize;
}

} // namespace Presentation
