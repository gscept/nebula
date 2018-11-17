//------------------------------------------------------------------------------
//  imguiconsole.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "imguiconsole.h"
#include "imgui.h"
#include "input/key.h"
#include "input/inputserver.h"
#include "input/keyboard.h"

static int
TextEditCallback(ImGuiTextEditCallbackData* data)
{
	Dynui::ImguiConsole* console = (Dynui::ImguiConsole*)data->UserData;

	switch (data->EventFlag)
	{
	case ImGuiInputTextFlags_CallbackCompletion:
	{
		// Locate beginning of current word
		const char* word_end = data->Buf + data->CursorPos;
		const char* word_start = word_end;
		while (word_start > data->Buf)
		{
			const char c = word_start[-1];
			if (c == ' ' || c == '\t' || c == ',' || c == ';')
				break;
			word_start--;
		}

		// get command
		Util::String command(word_start);

		Util::Array<Util::String> commands;
		IndexT i;
		/*for (i = 0; i < console->commands.Size(); i++)
		{
			const Util::String& name = console->commands.KeyAtIndex(i);
			if (name.FindStringIndex(command) == 0 && name != command)
			{
				commands.Append(name);
			}
		}
        */
		if (commands.IsEmpty())
		{
			n_printf("No completion for '%s'\n", command.AsCharPtr());
		}
		else if (commands.Size() == 1)
		{
			// Single match. Delete the beginning of the word and replace it entirely so we've got nice casing
			data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
			data->InsertChars(data->CursorPos, commands[0].AsCharPtr());
			data->InsertChars(data->CursorPos, " ");
		}
		else
		{
            /*
			int match_len = command.Length();
			for (;;)
			{
				int c = 0;
				bool all_candidates_matches = true;
				for (int i = 0; i < commands.Size() && all_candidates_matches; i++)
					if (i == 0)
						c = toupper(commands[i][match_len]);
					else if (c != toupper(commands[i][match_len]))
						all_candidates_matches = false;
				if (!all_candidates_matches)
					break;
				match_len++;
			}
            
			if (match_len > 0)
			{
				data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
				data->InsertChars(data->CursorPos, commands[0].AsCharPtr(), commands[0].AsCharPtr() + match_len);
			}

			n_printf("Candidates:\n");
			IndexT i;
			for (i = 0; i < commands.Size(); i++)
			{
				n_printf("- %s\n", commands[i].AsCharPtr());
			}
            */
		}
		
		
		break;
	}
	case ImGuiInputTextFlags_CallbackHistory:
	{
		// Example of HISTORY
		const int prev_history_pos = console->previousCommandIndex;
		if (data->EventKey == ImGuiKey_UpArrow)
		{
			if (console->previousCommandIndex == -1)
				console->previousCommandIndex = console->previousCommands.Size() - 1;
			else if (console->previousCommandIndex > 0)
				console->previousCommandIndex--;
		}
		else if (data->EventKey == ImGuiKey_DownArrow)
		{
			if (console->previousCommandIndex != -1)
				if (++console->previousCommandIndex >= console->previousCommands.Size())
					console->previousCommandIndex = -1;
		}

		// A better implementation would preserve the data on the current input line along with cursor position.
		if (prev_history_pos != console->previousCommandIndex)
		{
			Util::String lastCommand = (console->previousCommandIndex >= 0) ? console->previousCommands[console->previousCommandIndex] : "";
			lastCommand = lastCommand.ExtractRange(0, Math::n_min(lastCommand.Length(), data->BufSize));
			sprintf(data->Buf, "%s", lastCommand.AsCharPtr());
			data->BufDirty = true;
			data->CursorPos = data->SelectionStart = data->SelectionEnd = (int)strlen(data->Buf);
		}

		break;
	}

	}
		
	

	return 0;
}

using namespace Input;
namespace Dynui
{
__ImplementClass(Dynui::ImguiConsole, 'IMCO', Core::RefCounted);
__ImplementInterfaceSingleton(Dynui::ImguiConsole);


//------------------------------------------------------------------------------
/**
*/
ImguiConsole::ImguiConsole() :
	moveScroll(false),
	visible(false),
	selectedSuggestion(0),
	previousCommandIndex(-1)
{
	__ConstructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
ImguiConsole::~ImguiConsole()
{
	__DestructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
ImguiConsole::Setup()
{
	// clear command buffer
	memset(this->command, '\0', 65535);

	// get script server
	this->scriptServer = Scripting::ScriptServer::Instance();

	// load commands into dictionary
/*	SizeT numCommands = this->scriptServer->GetNumCommands();
	for (IndexT i = 0; i < numCommands; i++)
	{
		const Ptr<Scripting::Command>& command = this->scriptServer->GetCommandByIndex(i);
		this->commands.Add(command->GetName(), command);
	}
    */
	this->consoleBuffer.SetCapacity(2048);
}

//------------------------------------------------------------------------------
/**
*/
void
ImguiConsole::Discard()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
ImguiConsole::Render()
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.KeysDownDuration[Key::F8] == 0.0f)
	{
		this->visible = !this->visible;
	}	
	if (!this->visible) return;
	
	ImGui::Begin("Nebula Console", NULL, ImVec2(300, 300), -1.0f, ImGuiWindowFlags_NoScrollbar);
	ImGui::PushItemWidth(ImGui::GetWindowWidth());
	ImVec2 windowSize = ImGui::GetWindowSize();
	ImVec2 windowPos = ImGui::GetWindowPos();	
	ImGui::BeginChild("output", ImVec2(0, windowSize.y - 75), true);
		for (int i = 0; i < this->consoleBuffer.Size(); i++)
		{
			ImGui::TextUnformatted(this->consoleBuffer[i].AsCharPtr());
		}
		if (moveScroll)
		{
			moveScroll = false;
			ImGui::SetScrollHere();
		}
	ImGui::EndChild();

	if (ImGui::InputText("console_input", this->command, sizeof(this->command), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackCompletion, TextEditCallback, (void*)this))
	{
		//ImGui::SetKeyboardFocusHere();
		moveScroll = true;
		if (this->command[0] != '\0')
		{
			this->consoleBuffer.Add((const char*)this->command);
			this->consoleBuffer.Add("\n");

			this->previousCommandIndex = -1;
			for (int i = 0; i < this->previousCommands.Size(); i++)
			{
				if (this->previousCommands[i] == this->command)
				{
					this->previousCommands.EraseIndex(i);
					break;
				}
			}

			// execute script
			this->Execute(this->command);
			this->previousCommands.Append(this->command);

			// reset command to b empty
			memset(this->command, '\0', sizeof(this->command));
		}
	}		

	/*
	if (this->command[0] != '\0')
	{
		Util::Array<Ptr<Scripting::Command>> matches;
		IndexT i;
		for (i = 0; i < this->commands.Size(); i++)
		{
			const Util::String& name = this->commands.KeyAtIndex(i);
			if (name.FindStringIndex(this->command) == 0 && name != this->command) matches.Append(this->commands.ValueAtIndex(i));
		}

		// handle matches
		if (matches.Size() > 0)
		{
			if (io.KeysDownDuration[Key::Up] == 0.0f)	this->selectedSuggestion--;
			if (io.KeysDownDuration[Key::Down] == 0.0f) this->selectedSuggestion++;
			this->selectedSuggestion = Math::n_iclamp(this->selectedSuggestion, 0, matches.Size() - 1);

			if (io.KeysDownDuration[Key::Tab] == 0.0f)
			{
				const Util::String& firstCommand = matches[this->selectedSuggestion]->GetName();
				memcpy(this->command, firstCommand.AsCharPtr(), firstCommand.Length());
				this->selectedSuggestion = 0;
			}
			else
			{
				ImGui::Begin("suggestions", NULL, ImVec2(0, 0), 0.9f, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);
				ImGui::SetWindowPos(ImVec2(windowPos.x, windowPos.y + windowSize.y - 10));
				IndexT i;
				for (i = 0; i < matches.Size(); i++)
				{
					if (i == this->selectedSuggestion) ImGui::TextColored(ImVec4(0.5, 0.5, 0.7, 1.0f), matches[i]->GetName().AsCharPtr());
					else							   ImGui::Text(matches[i]->GetName().AsCharPtr());
				}
				ImGui::End();
			}
		}
	}
	*/
	
	ImGui::PopItemWidth();
	ImGui::End();

	//ImGui::ShowStyleEditor();
	// reset input
	//ImGui::Reset();
}

//------------------------------------------------------------------------------
/**
*/
void
ImguiConsole::Execute(const Util::String& command)
{
	n_assert(!command.IsEmpty());
	Util::Array<Util::String> splits = command.Tokenize(" ");
	if (splits[0] == "help")
	{
		/*SizeT numCommands = this->scriptServer->GetNumCommands();
		IndexT i;
		for (i = 0; i < numCommands; i++)
		{ 
			const Ptr<Scripting::Command> cmd = this->scriptServer->GetCommandByIndex(i);
			Util::String output;
			output.Format("%s - %s\n", cmd->GetSyntax().AsCharPtr(), cmd->GetHelp().AsCharPtr());
			output.SubstituteString("<br />", "\n");
			this->consoleBuffer.Add(output);
		}
        */
        this->consoleBuffer.Add("Go away");
	}
	else if (splits[0] == "clear")
	{
		this->consoleBuffer.Reset();
	}
/*	else if (this->commands.Contains(splits[0]) && splits.Size() == 2)
	{
		if (splits[1] == "help")
		{
			this->consoleBuffer.Add(this->commands[splits[0]]->GetHelp());
		}
		else if (splits[1] == "syntax")
		{
			this->consoleBuffer.Add(this->commands[splits[0]]->GetSyntax());
		}
	}*/
	else if (!this->scriptServer->Eval(command))
	{
		//this->consoleBuffer.Add(this->scriptServer->GetError() + "\n");
	}
	

	// add to previous commands
	this->previousCommands.Append(command);
}

//------------------------------------------------------------------------------
/**
*/
void
ImguiConsole::AppendToLog(const Util::String & msg)
{
	this->consoleBuffer.Add(msg);	
}
} // namespace Dynui