//------------------------------------------------------------------------------
//  inspector.cc
//  (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "inspector.h"
#include "editor/editor.h"
#include "editor/commandmanager.h"
#include "editor/ui/uimanager.h"
#include "graphicsfeature/graphicsfeatureunit.h"
#include "editor/tools/selectiontool.h"
#include "game/componentinspection.h"
#include "editor/cmds.h"
#include "imgui_internal.h"

using namespace Editor;

namespace Presentation
{
__ImplementClass(Presentation::Inspector, 'InWn', Presentation::BaseWindow);

//------------------------------------------------------------------------------
/**
*/
Inspector::Inspector()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
Inspector::~Inspector()
{
	for (auto const& p : this->tempComponents)
	{
		if (p.buffer != nullptr)
    		Memory::Free(Memory::HeapType::DefaultHeap, p.buffer);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
Inspector::Update()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
Inspector::Run()
{
	auto const& selection = Tools::SelectionTool::Selection();
	
	if (selection.Size() != 1)
		return;
	
	Editor::Entity const entity = selection[0];
	static Game::EntityMapping lastEntityMapping;
	Game::EntityMapping const entityMapping = Game::GetEntityMapping(Editor::state.editorWorld, entity);

	if (!Game::IsValid(Editor::state.editorWorld, entity) || !Game::IsActive(Editor::state.editorWorld, entity))
		return;

	Editor::Editable& edit = Editor::state.editables[entity.index];

    static char name[128];
	if (this->latestInspectedEntity != entity)
	{
		edit.name.CopyToBuffer(name, 128);
		for (auto& c : this->tempComponents)
			c.isDirty = false; // reset dirty status if we switch entity
		this->latestInspectedEntity = entity;
	}
	
	if (ImGui::InputText("##EntityName", name, 128, ImGuiInputTextFlags_EnterReturnsTrue))
	{
		Edit::SetEntityName(entity, name);
	}

    ImGui::Separator();
	ImGui::NewLine();
	this->ShowAddComponentMenu();

	MemDb::TableId const table = entityMapping.table;
	MemDb::Row const row = entityMapping.instance;

	auto const& components = Game::GetWorldDatabase(Editor::state.editorWorld)->GetTable(table).components;
	while (this->tempComponents.Size() < components.Size())
		this->tempComponents.Append({}); // fill up with empty intermediates

	// We need to double check the entity again here, since the add component function will change the entitys signature
	bool const entityChanged = (entityMapping.table != lastEntityMapping.table || entityMapping.instance != lastEntityMapping.instance);
	lastEntityMapping = Game::GetEntityMapping(Editor::state.editorWorld, entity);

	Util::StringAtom const ownerAtom = "Owner"_atm;
    for (int i = 0; i < components.Size(); i++)
    {
		auto component = components[i];
		
		if (MemDb::TypeRegistry::GetDescription(component)->name == ownerAtom)
		{
			continue;
		}
		ImGui::PushID(0xA3FC + (int)component.id); // offset the ids with some magic number
		ImGui::Text(MemDb::TypeRegistry::GetDescription(component)->name.Value());
		ImGui::SameLine();
		
		if (ImGui::Button("Remove"))
		{
			Edit::RemoveComponent(entity, component);
			ImGui::PopID();
			return; // return, otherwise we're reading stale data.
		}
		
		auto& tempComponent = this->tempComponents[i];
		if (entityChanged)
		{
			// reload the entity data if we've changed the selected entity
			tempComponent.isDirty = false;
			ImGui::PopID();
			continue;
		}

		SizeT const typeSize = MemDb::TypeRegistry::TypeSize(component);
		if (typeSize == 0)
		{
			// Type is flag type, just continue to the next one
			ImGui::Separator();
			ImGui::PopID();
			continue;
		}

		void* data = Game::GetInstanceBuffer(Editor::state.editorWorld, table, component);
		data = (byte*)data + (row * typeSize);
		if (typeSize > tempComponent.bufferSize)
		{
			if (tempComponent.buffer != nullptr)
				Memory::Free(Memory::HeapType::DefaultHeap, tempComponent.buffer);
			tempComponent.bufferSize = typeSize;
			tempComponent.buffer = Memory::Alloc(Memory::HeapType::DefaultHeap, tempComponent.bufferSize);
		}
		if (!tempComponent.isDirty)
		{
			Memory::Copy(data, tempComponent.buffer, typeSize);
			tempComponent.isDirty = true;
		}

		bool commitChange = false;
		Game::ComponentInspection::DrawInspector(component, tempComponent.buffer, &commitChange);

		if (commitChange)
		{
			Edit::SetComponent(entity, component, tempComponent.buffer);
			tempComponent.isDirty = false;
		}
		ImGui::Separator();
		ImGui::PopID();
    }

}

//------------------------------------------------------------------------------
/**
*/
void
Inspector::ShowAddComponentMenu()
{
	ImGui::SameLine(0.f, 0.f);

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	const ImGuiStyle& style = ImGui::GetStyle();

	float x = ImGui::GetCursorPosX();
	float y = ImGui::GetCursorPosY();

	auto buttonSize = ImVec2(-1, 28);
	auto label = "Add Component...";

	ImVec2 size(buttonSize.x, buttonSize.y);
	bool pressed = ImGui::Button("Add Component", size);

	// Popup

	ImVec2 popupPos;

	popupPos.x = window->Pos.x + x - buttonSize.x;
	popupPos.y = window->Pos.y + y + buttonSize.y;

	ImGui::SetNextWindowPos(popupPos);

	if (pressed)
	{
		ImGui::OpenPopup(label);
	}

	if (ImGui::BeginPopup(label))
	{
		ImGui::PushStyleColor(ImGuiCol_FrameBg, style.Colors[ImGuiCol_Button]);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, style.Colors[ImGuiCol_Button]);
		ImGui::PushStyleColor(ImGuiCol_ChildBg, style.Colors[ImGuiCol_Button]);
		
		// filter 
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		static ImGuiTextFilter filter;
		filter.Draw("Filter", window->Size.x - 100);
		ImGui::PopStyleVar();
		ImGui::Separator();

		Util::Array<const char *> cStrArray;

		Util::Array<MemDb::ComponentDescription*> const& components = MemDb::TypeRegistry::GetAllComponents();
		SizeT const numComponents = components.Size();
		Util::StringAtom const ownerAtom = "Owner"_atm;
		for (SizeT i = 0; i < numComponents; i++)
		{
			MemDb::ComponentDescription* component = components[i];
			if (component->name == ownerAtom || (component->externalFlags & Game::COMPONENTFLAG_MANAGED))
				continue;

			const char* name = component->name.Value();

			if (!filter.PassFilter(name))
				continue;

			if (ImGui::Button(name))
			{
				Editor::Entity const selectedEntity = Tools::SelectionTool::Selection()[0];
				if (!Game::IsValid(Editor::state.editorWorld, selectedEntity))
				{
					return;
				}

				Edit::AddComponent(selectedEntity, i);
				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::PopStyleColor(3);
		ImGui::EndPopup();
	}
}

} // namespace Presentation
