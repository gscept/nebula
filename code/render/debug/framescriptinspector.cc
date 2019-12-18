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

	auto textures = script->GetTextures();
	ImGui::InputInt("mip", &state.selectedMip);
	ImGui::InputInt("layer", &state.selectedLayer);

	ImGui::Text("Textures");
	for (int i = 0; i < textures.Size(); i++)
	{
		if (ImGui::Selectable(textures.KeyAtIndex(i).Value(), i == state.selectedTarget))
		{
			state.selectedTarget = i;
			break;
		}
	}

	CoreGraphics::TextureId textureId = textures.ValueAtIndex(state.selectedTarget);
	CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(textureId);

	using namespace CoreGraphics;

	// Needs to not be nuked scope since we're sending a void*
	static CoreGraphics::TextureId id;
	id = textureId;

	auto windowSize = ImGui::GetWindowSize();
	windowSize.y -= ImGui::GetCursorPosY();
	static Dynui::ImguiTextureId textureInfo;
	textureInfo.nebulaHandle = id.HashCode64();
	textureInfo.mip = state.selectedMip;
	textureInfo.layer = state.selectedLayer;

	windowSize.x = dims.width / 4;
	windowSize.y = dims.height / 4;
	ImGui::Image((void*)& textureInfo, windowSize);
	ImGui::End();

}
} // namespace Debug
