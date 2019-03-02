//------------------------------------------------------------------------------
//  imguiconsole.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "imguiconsole.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "input/key.h"
#include "input/inputserver.h"
#include "input/keyboard.h"
#include "io/textreader.h"
#include "io/textwriter.h"
#include "io/ioserver.h"
#include "app/application.h"
#include "pybind11/embed.h"
#include "scripting/bindings.h"

namespace py = pybind11;

struct completion_t
{
    Util::String name;
    Util::String complete;
    Util::String doc;
};

Util::Array<completion_t> completions;

static Util::String selectedCompletion;
static bool open_autocomplete = false;

static int
TextEditCallback(ImGuiInputTextCallbackData* data)
{
	Dynui::ImguiConsole* console = (Dynui::ImguiConsole*)data->UserData;

	completions.Clear();

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
			// n_printf("No completion for '%s'\n", command.AsCharPtr());
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

		// python command completion using jedi
        try {
            py::object jedi = py::module::import("jedi");
            py::object inter = jedi.attr("Interpreter");
            py::object scope = py::module::import("__main__").attr("__dict__");
            py::list scopes;
            scopes.append(scope);
            py::list pcompletions = inter(data->Buf, scopes).attr("completions")();
            if (pcompletions.size() == 1)
            {
                std::string rest = (std::string)py::str(pcompletions[0].attr("complete"));
                data->InsertChars(data->CursorPos, rest.c_str(), rest.c_str() + rest.size());                
            }
            else if (pcompletions.size() > 0)
            {
                open_autocomplete = true;
                IndexT j = 0;
                for (auto const& c : pcompletions)
                {
                    Util::String name = ((std::string)py::str(pcompletions[j].attr("name"))).c_str();
                    Util::String complete = ((std::string)py::str(pcompletions[j].attr("complete"))).c_str();
                    Util::String tooltip = ((std::string)py::str(pcompletions[j].attr("docstring")())).c_str();
                    completions.Append({ name,complete,tooltip });
                    ++j;
					if (j == 10)
						break;
                }
                
            }            
        }
        catch (pybind11::error_already_set e)
        {
            n_printf("%s", e.what());
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
			data->CursorPos = data->SelectionStart = data->SelectionEnd = data->BufTextLen = (int)strlen(data->Buf);
		}

		break;
	}

	}
    if (!selectedCompletion.IsEmpty())
    {
        data->InsertChars(data->CursorPos, selectedCompletion.AsCharPtr(), selectedCompletion.AsCharPtr() + selectedCompletion.Length());
        selectedCompletion.Clear();
        completions.Clear();
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
	previousCommandIndex(-1),
	scrollToBottom(true)
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
    // load persistent history
    auto reader = IO::TextReader::Create();
    Util::String history = App::Application::Instance()->GetAppTitle() + "_history.txt";    
    reader->SetStream(IO::IoServer::Instance()->CreateStream("bin:" + history));
    if (reader->Open())
    {
        this->previousCommands = reader->ReadAllLines();
        reader->Close();
    }
    this->persistentHistory = IO::TextWriter::Create();
    auto stream = IO::IoServer::Instance()->CreateStream("bin:" + history);
    stream->SetAccessMode(IO::Stream::AppendAccess);
    if (stream->Open())
    {
        this->persistentHistory->SetStream(stream);
        this->persistentHistory->Open();
    }
    
	this->consoleBuffer.SetCapacity(2048);
}

//------------------------------------------------------------------------------
/**
*/
void
ImguiConsole::Discard()
{
	// empty
    this->persistentHistory->Close();
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
	
    ImGui::Begin("Nebula Console", &this->visible);// , ImVec2(300, 300), -1.0f, ImGuiWindowFlags_NoScrollbar);

	RenderContent();
	
	ImGui::End();

	//ImGui::ShowStyleEditor();
	// reset input
	//ImGui::Reset();
}

//------------------------------------------------------------------------------
/**
*/
void
ImguiConsole::RenderContent()
{
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
	static ImGuiTextFilter filter;
	filter.Draw("Filter", 180);
	ImGui::PopStyleVar();
	ImGui::Separator();

	ImGui::BeginChild("ScrollingRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
	if (ImGui::BeginPopupContextWindow())
	{
		if (ImGui::Selectable("Clear"))
			this->consoleBuffer.Reset();
		ImGui::EndPopup();
	}

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing

	// Unfortunately we can't use ImGui::Clipper here since each entry might have a different height.
	// TODO: We could roll our own "clipper".
	ImGui::PushTextWrapPos(ImGui::GetWindowContentRegionMax().x);
	for (int i = 0; i < consoleBuffer.Size(); i++)
	{
		const char* item = consoleBuffer[i].msg.AsCharPtr();

		//Filter on both time, prefix and entry
		if (!filter.PassFilter(item) && !filter.PassFilter(this->LogEntryTypeAsCharPtr(consoleBuffer[i].type)))
			continue;

		ImVec4 col;
		switch (consoleBuffer[i].type)
		{
		case N_MESSAGE:
			col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
			break;
		case N_INPUT:
			col = ImVec4(0.4f, 1.0f, 0.4f, 1.0f);
			break;
		case N_WARNING:
			col = ImVec4(1.0f, 1.0f, 0.4f, 1.0f);
			break;
		case N_ERROR:
			col = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
			break;
		case N_EXCEPTION:
			col = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
			break;
		case N_SYSTEM:
			col = ImVec4(1.0f, 5.0f, 3.0f, 1.0f);
			break;
		default:
			break;
		}

		ImGui::PushStyleColor(ImGuiCol_Text, col);
		ImGui::SameLine();
		//Print message prefix
		if (consoleBuffer[i].type != N_MESSAGE)
		{
			ImGui::TextUnformatted(this->LogEntryTypeAsCharPtr(consoleBuffer[i].type));
			ImGui::SameLine();
		}
		//Print log entry
		ImGui::TextUnformatted(item);
		ImGui::NewLine();
		ImGui::PopStyleColor();
	}
	ImGui::PopTextWrapPos();

	if (this->scrollToBottom)
		ImGui::SetScrollHereY();

	ImGui::PopStyleVar();
	ImGui::EndChild();
	ImGui::Separator();

	// Command-line / Input ----------------------------------------------------

	if (ImGui::InputText("|", this->command, sizeof(this->command), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackAlways, &TextEditCallback, (void*)this))
	{
		char* input_end = this->command + strlen(this->command);
		while (input_end > this->command && input_end[-1] == ' ') input_end--; *input_end = 0;
		if (this->command[0])
		{
			this->AppendToLog({ LogMessageType::N_INPUT, this->command });

			// execute script
			this->Execute(this->command);
		}
		memset(this->command, '\0', sizeof(this->command));
	}

	//keeping auto focus on the input box
	if (ImGui::IsItemHovered() || (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)))
		ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget

	ImGui::SameLine();
	ImGui::PushItemWidth(-140);
	if (ImGui::SmallButton("Auto Scroll"))
		this->scrollToBottom = !this->scrollToBottom;

	ImGui::PopItemWidth();
	ImGui::Separator();



	if (completions.size() > 0 && completions.size() < 10)
	{
		{
			if (open_autocomplete)
			{
				ImGui::OpenPopup("autocomplete");
				auto pos = ImGui::GetCurrentContext()->PlatformImePos;
				pos.y += 20;
				ImGui::SetNextWindowPos(pos);
				this->selectedSuggestion = 0;
			}

			open_autocomplete = false;
			if (ImGui::BeginPopup("autocomplete", ImGuiWindowFlags_NoNavInputs))
			{
				if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow))) ++selectedSuggestion;
				if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow))) --selectedSuggestion;
				if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)))
				{
					selectedCompletion = completions[selectedSuggestion].complete;
					completions.Clear();
					ImGui::CloseCurrentPopup();
				}
				if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape)))
				{
					completions.Clear();
					ImGui::CloseCurrentPopup();
				}
				selectedSuggestion = Math::n_iclamp(selectedSuggestion, 0, completions.size() - 1);
				for (int i = 0, c = completions.size(); i < c; ++i)
				{
					completion_t const & comp = completions[i];
					if (ImGui::Selectable(comp.name.AsCharPtr(), selectedSuggestion == i))
					{
						selectedCompletion = comp.complete;
					}
					if (ImGui::IsItemHovered())
					{
						if (!comp.doc.IsEmpty())
						{
							ImGui::SetTooltip(comp.doc.AsCharPtr());
						}
					}
				}

				ImGui::EndPopup();
			}
			else
			{
				completions.Clear();
			}
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
ImguiConsole::Execute(const Util::String& command)
{
	n_assert(!command.IsEmpty());
	Util::Array<Util::String> splits = command.Tokenize(" ");
	if (splits[0] == "HELP")
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
		this->consoleBuffer.Add({ LogMessageType::N_MESSAGE, "Go away" });
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
    if (this->persistentHistory->IsOpen())
    {
        this->persistentHistory->WriteLine(command);
        this->persistentHistory->GetStream()->Flush();
    }
    this->previousCommandIndex = -1;
}

//------------------------------------------------------------------------------
/**
*/
void
ImguiConsole::AppendToLog(const LogEntry& msg)
{
	this->consoleBuffer.Add(msg);	
}

//------------------------------------------------------------------------------
/**
*/
const char*
ImguiConsole::LogEntryTypeAsCharPtr(const LogMessageType& type) const
{
	//static const prefixes that are appended to messages. Doing this saves a ton of memory.
	static const Util::String prefix_message = "[Message]: ";
	static const Util::String prefix_input = ">> ";
	static const Util::String prefix_warning = "[Warning]: ";
	static const Util::String prefix_error = "[ Error ]: ";
	static const Util::String prefix_exception = "[FATAL ERROR]: ";
	static const Util::String prefix_system ="[System]: ";

	switch (type)
	{
	case N_INPUT:
		return prefix_input.AsCharPtr();
	case N_WARNING:
		return prefix_warning.AsCharPtr();
	case N_ERROR:
		return prefix_error.AsCharPtr();
	case N_EXCEPTION:
		return prefix_exception.AsCharPtr();
	case N_SYSTEM:
		return prefix_system.AsCharPtr();
	default:
		return nullptr;
	}
}

} // namespace Dynui