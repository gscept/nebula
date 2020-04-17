//------------------------------------------------------------------------------
//  imguirenderer.cc
//  (C) 2012-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "imguicontext.h"
#include "imgui.h"
#include "graphics/graphicsserver.h"
#include "resources/resourceserver.h"
#include "math/rectangle.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/displaydevice.h"
#include "input/inputserver.h"
#include "io/ioserver.h"

using namespace Math;
using namespace CoreGraphics;
using namespace Base;
using namespace Input;

namespace Dynui
{

ImguiContext::ImguiState ImguiContext::state;

//------------------------------------------------------------------------------
/**
	Imgui rendering function
*/
void
ImguiContext::ImguiDrawFunction()
{
    ImDrawData* data = ImGui::GetDrawData();
	// get Imgui context
	ImGuiIO& io = ImGui::GetIO();
	int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
	int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
	data->ScaleClipRects(io.DisplayFramebufferScale);

	// get renderer	
	//const Ptr<BufferLock>& vboLock = renderer->GetVertexBufferLock();
	//const Ptr<BufferLock>& iboLock = renderer->GetIndexBufferLock();
	IndexT currentBuffer = CoreGraphics::GetBufferedFrameIndex();
	VertexBufferId vbo = state.vbos[currentBuffer];
	IndexBufferId ibo = state.ibos[currentBuffer];
	const ImguiRendererParams& params = state.params;

	// apply shader
	CoreGraphics::SetShaderProgram(state.prog);

	// create orthogonal matrix
#if __VULKAN__
	mat4 proj = orthooffcenterrh(0.0f, io.DisplaySize.x, 0.0f, io.DisplaySize.y, -1.0f, +1.0f);
#else
	mat4 proj = orthooffcenterrh(0.0f, io.DisplaySize.x, io.DisplaySize.y, 0.0f, -1.0f, +1.0f);
#endif

	// setup device
	CoreGraphics::SetVertexLayout(CoreGraphics::VertexBufferGetLayout(state.vbos[currentBuffer]));
	CoreGraphics::SetPrimitiveTopology(CoreGraphics::PrimitiveTopology::TriangleList);
	CoreGraphics::SetGraphicsPipeline();

	// setup input buffers
	CoreGraphics::SetResourceTable(state.resourceTable, NEBULA_BATCH_GROUP, GraphicsPipeline, nullptr);
	CoreGraphics::SetStreamVertexBuffer(0, state.vbos[currentBuffer], 0);
	CoreGraphics::SetIndexBuffer(state.ibos[currentBuffer], 0);

	// set projection
	CoreGraphics::PushConstants(CoreGraphics::GraphicsPipeline, state.textProjectionConstant, sizeof(proj), (byte*)&proj);

	struct TextureInfo
	{
		uint32 type : 4;
		uint32 layer : 8;
		uint32 mip : 8;
		uint32 useAlpha : 1;
		uint32 id : 11;
	};

	IndexT vertexOffset = 0;
	IndexT indexOffset = 0;
	IndexT vertexBufferOffset = 0;
	IndexT indexBufferOffset = 0;
	IndexT i;
	for (i = 0; i < data->CmdListsCount; i++)
	{
		// get buffer data and calculate size of data to copy
		const ImDrawList* commandList = data->CmdLists[i];
		const unsigned char* vertexBuffer = (unsigned char*)&commandList->VtxBuffer.front();
		const unsigned char* indexBuffer = (unsigned char*)&commandList->IdxBuffer.front();
		const SizeT vertexBufferSize = commandList->VtxBuffer.size() * sizeof(ImDrawVert);					// 2 for position, 2 for uvs, 1 int for color
		const SizeT indexBufferSize = commandList->IdxBuffer.size() * sizeof(ImDrawIdx);					// using 16 bit indices

		// if we render too many vertices, we will simply assert
		n_assert(vertexBufferOffset + (IndexT)commandList->VtxBuffer.size() < CoreGraphics::VertexBufferGetNumVertices(state.vbos[currentBuffer]));
		n_assert(indexBufferOffset + (IndexT)commandList->IdxBuffer.size() < CoreGraphics::IndexBufferGetNumIndices(state.ibos[currentBuffer]));

		// wait for previous draws to finish...
		memcpy(state.vertexPtrs[currentBuffer] + vertexBufferOffset, vertexBuffer, vertexBufferSize);
		memcpy(state.indexPtrs[currentBuffer] + indexBufferOffset, indexBuffer, indexBufferSize);
		IndexT j;
		IndexT primitiveIndexOffset = 0;
		for (j = 0; j < commandList->CmdBuffer.size(); j++)
		{
			const ImDrawCmd* command = &commandList->CmdBuffer[j];
			if (command->UserCallback)
			{
				command->UserCallback(commandList, command);
			}
			else
			{
				// setup scissor rect
				Math::rectangle<int> scissorRect((int)command->ClipRect.x, (int)command->ClipRect.y, (int)command->ClipRect.z, (int)command->ClipRect.w);
				CoreGraphics::SetScissorRect(scissorRect, 0);
				ImguiTextureId tex = *(ImguiTextureId*)command->TextureId;

				TextureInfo texInfo;
				texInfo.type = 0;
				texInfo.useAlpha = 1;

				// set texture in shader, we shouldn't have to put it into ImGui
				CoreGraphics::TextureId texture = tex.nebulaHandle;
				auto usage = CoreGraphics::TextureGetUsage(texture);
				if (usage & CoreGraphics::TextureUsage::RenderUsage || usage & CoreGraphics::TextureUsage::ReadWriteUsage)
				{
					texInfo.useAlpha = false;
				}
				SizeT layers = CoreGraphics::TextureGetNumLayers(texture);
				if (layers > 1)
				{
					texInfo.type = 1;
				}
				texInfo.layer = tex.layer;
				texInfo.mip = tex.mip;
				texInfo.id = CoreGraphics::TextureGetBindlessHandle(texture);
				
				CoreGraphics::PushConstants(CoreGraphics::GraphicsPipeline, state.packedTextureInfo, sizeof(TextureInfo), (byte*)& texInfo);

				// setup primitive
				CoreGraphics::PrimitiveGroup primitive;
				primitive.SetNumIndices(command->ElemCount);
				primitive.SetBaseIndex(primitiveIndexOffset + indexOffset);
				primitive.SetBaseVertex(vertexOffset);

				CoreGraphics::SetPrimitiveGroup(primitive);

				// prepare render device and draw
				CoreGraphics::Draw();
			}

			// increment vertex offset
			primitiveIndexOffset += command->ElemCount;
		}

		// bump vertices
		vertexOffset += commandList->VtxBuffer.size();
		indexOffset += commandList->IdxBuffer.size();

		// lock buffers
		vertexBufferOffset += vertexBufferSize;
		indexBufferOffset += indexBufferSize;
	}

	// reset clip settings
	CoreGraphics::ResetClipSettings();
}

_ImplementPluginContext(ImguiContext);
//------------------------------------------------------------------------------
/**
*/
ImguiContext::ImguiContext()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ImguiContext::~ImguiContext()
{
   
}

//------------------------------------------------------------------------------
/**
*/
void
ImguiContext::Create()
{
    __bundle.OnBegin = ImguiContext::OnBeforeFrame;
    __bundle.OnWindowResized = ImguiContext::OnWindowResized;
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

	// allocate imgui shader
	state.uiShader = ShaderServer::Instance()->GetShader("shd:imgui.fxb");
    state.params.projVar = CoreGraphics::ShaderGetConstantBinding(state.uiShader,"TextProjectionModel");
    state.params.fontVar = CoreGraphics::ShaderGetConstantBinding(state.uiShader, "Texture");
    state.prog = CoreGraphics::ShaderGetProgram(state.uiShader, CoreGraphics::ShaderFeatureFromString("Static"));

	state.resourceTable = CoreGraphics::ShaderCreateResourceTable(state.uiShader, NEBULA_BATCH_GROUP);

	state.textureConstant = CoreGraphics::ShaderGetConstantBinding(state.uiShader, "Texture");
	state.textProjectionConstant = CoreGraphics::ShaderGetConstantBinding(state.uiShader, "TextProjectionModel");
	state.packedTextureInfo = CoreGraphics::ShaderGetConstantBinding(state.uiShader, "PackedTextureInfo");

	state.inputHandler = ImguiInputHandler::Create();
	Input::InputServer::Instance()->AttachInputHandler(Input::InputPriority::DynUi, state.inputHandler.upcast<Input::InputHandler>());

	// create vertex buffer
	Util::Array<CoreGraphics::VertexComponent> components;
    components.Append(VertexComponent((VertexComponent::SemanticName)0, 0, VertexComponentBase::Float2, 0));
	components.Append(VertexComponent((VertexComponent::SemanticName)1, 0, VertexComponentBase::Float2, 0));
    components.Append(VertexComponent((VertexComponent::SemanticName)2, 0, VertexComponentBase::UByte4N, 0));

	Frame::FramePlugin::AddCallback("ImGUI", [](const IndexT frameIndex)
		{
			CoreGraphics::BeginBatch(Frame::FrameBatchType::System);
			ImGui::Render();
			ImguiContext::ImguiDrawFunction();
			CoreGraphics::EndBatch();
		});

	SizeT numBuffers = CoreGraphics::GetNumBufferedFrames();

	CoreGraphics::VertexBufferCreateInfo vboInfo =
	{
		"ImGUI VBO"_atm,
		CoreGraphics::GpuBufferTypes::AccessWrite,
		CoreGraphics::GpuBufferTypes::UsageDynamic,
		CoreGraphics::GpuBufferTypes::SyncingAutomatic,
		100000 * 3,
		components,
		nullptr,
		0
	};
	state.vbos.Resize(numBuffers);
	IndexT i;
	for (i = 0; i < numBuffers; i++)
	{
		state.vbos[i] = CoreGraphics::CreateVertexBuffer(vboInfo);
	}

	CoreGraphics::IndexBufferCreateInfo iboInfo = 
	{
		""_atm,
		"system",
		CoreGraphics::GpuBufferTypes::AccessWrite,
		CoreGraphics::GpuBufferTypes::UsageDynamic,
		CoreGraphics::GpuBufferTypes::SyncingAutomatic,
		IndexType::Index16,
		100000 * 3,
		nullptr,
		0
	};
	state.ibos.Resize(numBuffers);
	for (i = 0; i < numBuffers; i++)
	{
		iboInfo.name = Util::String::Sprintf("imgui_ibo_%d", i);
		state.ibos[i] = CoreGraphics::CreateIndexBuffer(iboInfo);
	}

	// map buffer
	state.vertexPtrs.Resize(numBuffers);
	state.indexPtrs.Resize(numBuffers);
	for (i = 0; i < numBuffers; i++)
	{
		state.vertexPtrs[i] = (byte*)CoreGraphics::VertexBufferMap(state.vbos[i], CoreGraphics::GpuBufferTypes::MapWrite);
		state.indexPtrs[i] = (byte*)CoreGraphics::IndexBufferMap(state.ibos[i], CoreGraphics::GpuBufferTypes::MapWrite);
	}    

	// get display mode, this will be our default size
	Ptr<DisplayDevice> display = DisplayDevice::Instance();
	DisplayMode mode = CoreGraphics::WindowGetDisplayMode(display->GetCurrentWindow());

	// setup Imgui
    ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)mode.GetWidth(), (float)mode.GetHeight());
	io.DeltaTime = 1 / 60.0f;
	//io.PixelCenterOffset = 0.0f;
	//io.FontTexUvForWhite = ImVec2(1, 1);
	//io.RenderDrawListsFn = ImguiDrawFunction;

	ImGuiStyle& style = ImGui::GetStyle();
	
	style.FrameRounding = 4.0f;
	style.GrabRounding = 8.0f;
	style.ChildRounding = 0.0f;
	style.WindowRounding = 6.0f;
	style.PopupRounding = 0.0f;
	style.ScrollbarRounding = 32.0f;

	style.WindowTitleAlign = { 0.01f, 0.38f };

	style.WindowPadding = { 8.0f, 8.0f };
	style.FramePadding = { 4, 3 };
	style.ItemInnerSpacing = { 4, 2 };
	style.ItemSpacing = { 10, 5 };
	style.IndentSpacing = 8.0f;
	style.GrabMinSize = 8.0f;

	style.FrameBorderSize = 0.0f;
	style.WindowBorderSize = 1.5f;
	style.PopupBorderSize = 1.0f;
	style.ChildBorderSize = 0.0f;

	ImVec4 nebulaOrange(1.0f, 0.30f, 0.0f, 1.0f);
	ImVec4 nebulaOrangeActive(0.9f, 0.20f, 0.05f, 1.0f);
	nebulaOrange.w = 0.3f;
	style.Colors[ImGuiCol_TitleBg] = nebulaOrange;
	nebulaOrange.w = 0.6f;
	style.Colors[ImGuiCol_TitleBgCollapsed] = nebulaOrange;
	nebulaOrange.w = 0.9f;
	style.Colors[ImGuiCol_TitleBgActive] = nebulaOrange;
	nebulaOrange.w = 0.2f;
	style.Colors[ImGuiCol_ScrollbarBg] = nebulaOrange;
	nebulaOrange.w = 0.7f;
	style.Colors[ImGuiCol_ScrollbarGrab] = nebulaOrange;
	style.Colors[ImGuiCol_SliderGrab] = nebulaOrange;
	nebulaOrange.w = 0.9f;
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = nebulaOrange;
	style.Colors[ImGuiCol_ScrollbarGrabActive] = nebulaOrange;
	nebulaOrange.w = 0.6f;
	style.Colors[ImGuiCol_Header] = nebulaOrange;
	style.Colors[ImGuiCol_FrameBg] = nebulaOrange;
	nebulaOrange.w = 0.7f;
	style.Colors[ImGuiCol_HeaderHovered] = nebulaOrange;
	style.Colors[ImGuiCol_FrameBgHovered] = nebulaOrange;
	nebulaOrange.w = 0.9f;
	style.Colors[ImGuiCol_HeaderActive] = nebulaOrange;
	style.Colors[ImGuiCol_FrameBgActive] = nebulaOrange;
	nebulaOrange.w = 0.7f;	
	nebulaOrange.w = 0.5f;
	style.Colors[ImGuiCol_Button] = nebulaOrange;
	nebulaOrange.w = 0.9f;
	style.Colors[ImGuiCol_ButtonActive] = nebulaOrange;
	nebulaOrange.w = 0.7f;
	style.Colors[ImGuiCol_ButtonHovered] = nebulaOrange;	
	style.Colors[ImGuiCol_Border] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_CheckMark] = nebulaOrange;
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.95f);

	style.Colors[ImGuiCol_Button] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.85f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(1.0f, 1.0f, 1.0f, 0.25f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.3f, 0.33f, 0.33f, 1.0f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.3f, 0.33f, 0.33f, 1.0f);
	style.Colors[ImGuiCol_Text] = ImVec4(0.73f, 0.73f, 0.73f, 1.0f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.13f, 0.13f, 0.13f, 1.0f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.3f, 0.33f, 0.33f, 1.0f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 0.47f, 0.0f, 1.0f);

	style.Colors[ImGuiCol_Separator] = ImVec4(0.33f, 0.33f, 0.33f, 0.3f);
	style.Colors[ImGuiCol_SeparatorHovered] = nebulaOrange;
	style.Colors[ImGuiCol_SeparatorActive] = nebulaOrangeActive;

	// Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
	io.KeyMap[ImGuiKey_Tab] = Key::Tab;             
	io.KeyMap[ImGuiKey_LeftArrow] = Key::Left;
	io.KeyMap[ImGuiKey_RightArrow] = Key::Right;
	io.KeyMap[ImGuiKey_UpArrow] = Key::Up;
	io.KeyMap[ImGuiKey_DownArrow] = Key::Down;
	io.KeyMap[ImGuiKey_Home] = Key::Home;
	io.KeyMap[ImGuiKey_End] = Key::End;
	io.KeyMap[ImGuiKey_Delete] = Key::Delete;
	io.KeyMap[ImGuiKey_Backspace] = Key::Back;
	io.KeyMap[ImGuiKey_Enter] = Key::Return;
	io.KeyMap[ImGuiKey_Escape] = Key::Escape;
	io.KeyMap[ImGuiKey_Space] = Key::Space;
	io.KeyMap[ImGuiKey_A] = Key::A;
	io.KeyMap[ImGuiKey_C] = Key::C;
	io.KeyMap[ImGuiKey_V] = Key::V;
	io.KeyMap[ImGuiKey_X] = Key::X;
	io.KeyMap[ImGuiKey_Y] = Key::Y;
	io.KeyMap[ImGuiKey_Z] = Key::Z;

	// enable keyboard navigation
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	// load default font
	ImFontConfig config;
	config.OversampleH = 3;
	config.OversampleV = 1;
#if __WIN32__
	ImFont* font = io.Fonts->AddFontFromFileTTF("c:/windows/fonts/segoeui.ttf", 17, &config);
#else
	ImFont* font = io.Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/freefont/FreeSans.ttf", 18, &config);
#endif
	
	unsigned char* buffer;
	int width, height, channels;
	io.Fonts->GetTexDataAsRGBA32(&buffer, &width, &height, &channels);

	// load image using SOIL
	// unsigned char* texData = SOIL_load_image_from_memory(buffer, width * height * channels, &width, &height, &channels, SOIL_LOAD_AUTO);

	CoreGraphics::TextureCreateInfo texInfo;
	texInfo.name = "imgui_font_tex"_atm;
	texInfo.usage = TextureUsage::ImmutableUsage;
	texInfo.tag = "system"_atm;
	texInfo.buffer = buffer;
	texInfo.type = TextureType::Texture2D;
	texInfo.format = CoreGraphics::PixelFormat::R8G8B8A8;
	texInfo.width = width;
	texInfo.height = height;

    state.fontTexture.nebulaHandle = CoreGraphics::CreateTexture(texInfo).HashCode64();
	state.fontTexture.mip = 0;
	state.fontTexture.layer = 0;
	io.Fonts->TexID = &state.fontTexture;

	// load settings from disk. If we don't do this here we	need to
	// run an entire frame before being able to create or load settings
	if (!IO::IoServer::Instance()->FileExists("imgui.ini"))
	{
		ImGui::SaveIniSettingsToDisk("imgui.ini");
	}
	ImGui::LoadIniSettingsFromDisk("imgui.ini");
}

//------------------------------------------------------------------------------
/**
*/
void
ImguiContext::Discard()
{
	IndexT i;
	for (i = 0; i < state.vbos.Size(); i++)
	{
		CoreGraphics::VertexBufferUnmap(state.vbos[i]);
		CoreGraphics::IndexBufferUnmap(state.ibos[i]);

		CoreGraphics::DestroyVertexBuffer(state.vbos[i]);
		CoreGraphics::DestroyIndexBuffer(state.ibos[i]);

		state.vertexPtrs[i] = nullptr;
		state.indexPtrs[i] = nullptr;
	}

	Input::InputServer::Instance()->RemoveInputHandler(state.inputHandler.upcast<InputHandler>());
	state.inputHandler = nullptr;

	CoreGraphics::DestroyTexture((CoreGraphics::TextureId)state.fontTexture.nebulaHandle);
	ImGui::DestroyContext();
}

//------------------------------------------------------------------------------
/**
*/
bool
ImguiContext::HandleInput(const Input::InputEvent& event)
{
	ImGuiIO& io = ImGui::GetIO();
	switch (event.GetType())
	{
	case InputEvent::KeyDown:
		io.KeysDown[event.GetKey()] = true;
		if (event.GetKey() == Key::LeftControl || event.GetKey() == Key::RightControl) io.KeyCtrl = true;
		if (event.GetKey() == Key::LeftShift || event.GetKey() == Key::RightShift) io.KeyShift = true;
		return io.WantCaptureKeyboard;
	case InputEvent::KeyUp:
		io.KeysDown[event.GetKey()] = false;
		if (event.GetKey() == Key::LeftControl || event.GetKey() == Key::RightControl) io.KeyCtrl = false;
		if (event.GetKey() == Key::LeftShift || event.GetKey() == Key::RightShift) io.KeyShift = false;
		return io.WantCaptureKeyboard;									// not a bug, this allows keys to be let go even if we are over the UI
	case InputEvent::Character:
	{
		char c = event.GetChar();

		// ignore backspace as a character
		if (c > 0 && c < 0x10000)
		{
			const char chars[] = { c, '\0' };
			io.AddInputCharactersUTF8(chars);
		}
		return io.WantTextInput;
	}
	case InputEvent::MouseMove:
		io.MousePos = ImVec2(event.GetAbsMousePos().x, event.GetAbsMousePos().y);
		return io.WantCaptureMouse;
	case InputEvent::MouseButtonDown:
		io.MouseDown[event.GetMouseButton()] = true;
		return io.WantCaptureMouse;
	case InputEvent::MouseButtonUp:
		io.MouseDown[event.GetMouseButton()] = false;
		return false;									// not a bug, this allows keys to be let go even if we are over the UI
	case InputEvent::MouseWheelForward:
		io.MouseWheel = 1;
		return io.WantCaptureMouse;
	case InputEvent::MouseWheelBackward:
		io.MouseWheel = -1;
		return io.WantCaptureMouse;
	}
	
	return false;
}

//------------------------------------------------------------------------------
/**
*/
void
ImguiContext::OnWindowResized(const CoreGraphics::WindowId windowId, SizeT width, SizeT height)
{
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)width, (float)height);
}

//------------------------------------------------------------------------------
/**
*/
void 
ImguiContext::OnBeforeFrame(const Graphics::FrameContext& ctx)
{
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = ctx.frameTime;
    ImGui::NewFrame();
}

} // namespace Dynui