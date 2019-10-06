//------------------------------------------------------------------------------
//  framescriptinspector.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "framescriptinspector.h"
#include "frame/framescript.h"
#include "dynui/imguicontext.h"
#include "imgui.h"
namespace Debug
{

struct
{
	int selectedTarget = 0;
	int selectedMip = 0;
	int selectedLayer = 0;
} state;
//------------------------------------------------------------------------------
/**
*/
void 
FrameScriptInspector::Run(const Ptr<Frame::FrameScript>& script)
{
	ImGui::Begin(script->GetResourceName().Value());

	auto renderTextures = script->GetColorTextures();
	ImGui::InputInt("mip", &state.selectedMip);
	ImGui::InputInt("layer", &state.selectedLayer);

	ImGui::Text("Render textures");
	for (int i = 0; i < renderTextures.Size(); i++)
	{
		if (ImGui::Selectable(renderTextures.KeyAtIndex(i).Value(), i == state.selectedTarget))
		{
			state.selectedTarget = i;
		}
	}


	CoreGraphics::RenderTextureId textureId = renderTextures.ValueAtIndex(state.selectedTarget);

	using namespace CoreGraphics;

	// Needs to persist since we're sending a void*
	static Resources::ResourceId id;
	id.poolId = textureId.id24;
	id.poolIndex = textureId.id8;
	id.resourceId = textureId.id24;
	id.resourceType = textureId.id8;

	auto windowSize = ImGui::GetWindowSize();
	windowSize.y -= ImGui::GetCursorPosY();
	static Dynui::ImguiTextureId textureInfo;
	textureInfo.nebulaHandle = id.HashCode64();
	textureInfo.mip = state.selectedMip;
	textureInfo.layer = state.selectedLayer;

	ImGui::Image((void*)& textureInfo, windowSize);
	ImGui::End();

}
} // namespace Debug
