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
#include "game/propertyinspection.h"
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
	for (auto const& p : this->tempProperties)
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
		for (auto& p : this->tempProperties)
			p.isDirty = false; // reset dirty status if we switch entity
		this->latestInspectedEntity = entity;
	}
	
	if (ImGui::InputText("##EntityName", name, 128, ImGuiInputTextFlags_EnterReturnsTrue))
	{
		Edit::SetEntityName(entity, name);
	}

    ImGui::Separator();
	ImGui::NewLine();
	this->ShowAddPropertyMenu();

	MemDb::TableId const category = entityMapping.category;
	MemDb::Row const row = entityMapping.instance;

	auto const& properties = Game::GetWorldDatabase(Editor::state.editorWorld)->GetTable(category).properties;
	while (this->tempProperties.Size() < properties.Size())
		this->tempProperties.Append({}); // fill up with empty intermediates

	// We need to double check the entity again here, since the add property function will change the entitys signature
	bool const entityChanged = (entityMapping.category != lastEntityMapping.category || entityMapping.instance != lastEntityMapping.instance);
	lastEntityMapping = Game::GetEntityMapping(Editor::state.editorWorld, entity);

	Util::StringAtom const ownerAtom = "Owner"_atm;
    for (int i = 0; i < properties.Size(); i++)
    {
		auto property = properties[i];
		
		if (MemDb::TypeRegistry::GetDescription(property)->name == ownerAtom)
		{
			continue;
		}
		ImGui::PushID(0xA3FC + (int)property.id); // offset the ids with some magic number
		ImGui::Text(MemDb::TypeRegistry::GetDescription(property)->name.Value());
		ImGui::SameLine();
		
		if (ImGui::Button("Remove"))
		{
			Edit::RemoveProperty(entity, property);
			ImGui::PopID();
			return; // return, otherwise we're reading stale data.
		}
		
		auto& tempProperty = this->tempProperties[i];
		if (entityChanged)
		{
			// reload the entity data if we've changed the selected entity
			tempProperty.isDirty = false;
			ImGui::PopID();
			continue;
		}

		SizeT const typeSize = MemDb::TypeRegistry::TypeSize(property);
		if (typeSize == 0)
		{
			// Type is flag type, just continue to the next one
			ImGui::Separator();
			ImGui::PopID();
			continue;
		}

		void* data = Game::GetInstanceBuffer(Editor::state.editorWorld, category, property);
		data = (byte*)data + (row * typeSize);
		if (typeSize > tempProperty.bufferSize)
		{
			if (tempProperty.buffer != nullptr)
				Memory::Free(Memory::HeapType::DefaultHeap, tempProperty.buffer);
			tempProperty.bufferSize = typeSize;
			tempProperty.buffer = Memory::Alloc(Memory::HeapType::DefaultHeap, tempProperty.bufferSize);
		}
		if (!tempProperty.isDirty)
		{
			Memory::Copy(data, tempProperty.buffer, typeSize);
			tempProperty.isDirty = true;
		}

		bool commitChange = false;
		Game::PropertyInspection::DrawInspector(property, tempProperty.buffer, &commitChange);

		if (commitChange)
		{
			Edit::SetProperty(entity, property, tempProperty.buffer);
			tempProperty.isDirty = false;
		}
		ImGui::Separator();
		ImGui::PopID();
    }

}

//------------------------------------------------------------------------------
/**
*/
void
Inspector::ShowAddPropertyMenu()
{
	ImGui::SameLine(0.f, 0.f);

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	const ImGuiStyle& style = ImGui::GetStyle();

	float x = ImGui::GetCursorPosX();
	float y = ImGui::GetCursorPosY();

	auto buttonSize = ImVec2(-1, 28);
	auto label = "Add Property...";

	ImVec2 size(buttonSize.x, buttonSize.y);
	bool pressed = ImGui::Button("Add Property", size);

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

		Util::Array<MemDb::PropertyDescription*> const& properties = MemDb::TypeRegistry::GetAllProperties();
		SizeT const numProperties = properties.Size();
		Util::StringAtom const ownerAtom = "Owner"_atm;
		for (SizeT i = 0; i < numProperties; i++)
		{
			MemDb::PropertyDescription* property = properties[i];
			if (property->name == ownerAtom || (property->externalFlags & Game::PROPERTYFLAG_MANAGED))
				continue;

			const char* name = property->name.Value();

			if (!filter.PassFilter(name))
				continue;

			if (ImGui::Button(name))
			{
				Editor::Entity const selectedEntity = Tools::SelectionTool::Selection()[0];
				if (!Game::IsValid(Editor::state.editorWorld, selectedEntity))
				{
					return;
				}

				Edit::AddProperty(selectedEntity, i);
				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::PopStyleColor(3);
		ImGui::EndPopup();
	}
}

} // namespace Presentation
