//------------------------------------------------------------------------------
//  imguirenderer.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "imguirenderer.h"
#include "imgui.h"
#include "coregraphics/memorytextureloader.h"
#include "resources/resourcemanager.h"
#include "SOIL/SOIL.h"
#include "coregraphics/memoryvertexbufferloader.h"
#include "math/rectangle.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/displaydevice.h"
#include "coregraphics/memoryindexbufferloader.h"

using namespace Math;
using namespace CoreGraphics;
using namespace Base;
using namespace Input;



namespace Dynui
{

//------------------------------------------------------------------------------
/**
	Imgui rendering function
*/
static void
ImguiDrawFunction(ImDrawData* data)
{
	// get Imgui context
	ImGuiIO& io = ImGui::GetIO();
	int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
	int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
	data->ScaleClipRects(io.DisplayFramebufferScale);

	// get rendering device
	const Ptr<RenderDevice>& device = RenderDevice::Instance();

	// get renderer
	const Ptr<ImguiRenderer>& renderer = ImguiRenderer::Instance();
	ShaderId shader = renderer->GetShader();
	//const Ptr<BufferLock>& vboLock = renderer->GetVertexBufferLock();
	//const Ptr<BufferLock>& iboLock = renderer->GetIndexBufferLock();
	VertexBufferId vbo = renderer->GetVertexBuffer();
	IndexBufferId ibo = renderer->GetIndexBuffer();
	const ImguiRendererParams& params = renderer->GetParams();

	// apply shader
    shader->Apply();

	// create orthogonal matrix
	matrix44 proj = matrix44::orthooffcenterrh(0.0f, io.DisplaySize.x, io.DisplaySize.y, 0.0f, -1.0f, +1.0f);

	// set in shader
    shader->BeginUpdate();
	params.projVar->SetMatrix(proj);
    shader->EndUpdate();

	// setup device
	device->SetStreamSource(0, vbo, 0);
	device->SetVertexLayout(vbo->GetVertexLayout());
	device->SetIndexBuffer(ibo);

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
		n_assert(vertexBufferOffset + (IndexT)commandList->VtxBuffer.size() < vbo->GetNumVertices());
		n_assert(indexBufferOffset + (IndexT)commandList->IdxBuffer.size() < ibo->GetNumIndices());

		// wait for previous draws to finish...
		vboLock->WaitForRange(vertexBufferOffset, vertexBufferSize);
		iboLock->WaitForRange(indexBufferOffset, indexBufferSize);
		memcpy(renderer->GetVertexPtr() + vertexBufferOffset, vertexBuffer, vertexBufferSize);
		memcpy(renderer->GetIndexPtr() + indexBufferOffset, indexBuffer, indexBufferSize);
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
				device->SetScissorRect(scissorRect, 0);

				// set texture in shader, we shouldn't have to put it into ImGui
				params.fontVar->SetTexture((CoreGraphics::Texture*)command->TextureId);

				// commit variable changes
				shader->Commit();

				// setup primitive
				CoreGraphics::PrimitiveGroup primitive;
				primitive.SetNumIndices(command->ElemCount);
				primitive.SetBaseIndex(primitiveIndexOffset + indexOffset);
				primitive.SetBaseVertex(vertexOffset);
				primitive.SetPrimitiveTopology(CoreGraphics::PrimitiveTopology::TriangleList);

				// prepare render device and draw
				device->SetPrimitiveGroup(primitive);
				device->Draw();
			}

			// increment vertex offset
			primitiveIndexOffset += command->ElemCount;
		}

		// bump vertices
		vertexOffset += commandList->VtxBuffer.size();
		indexOffset += commandList->IdxBuffer.size();

		// lock buffers
		vboLock->LockRange(vertexBufferOffset, vertexBufferSize);
		vertexBufferOffset += vertexBufferSize;
		iboLock->LockRange(indexBufferOffset, indexBufferSize);
		indexBufferOffset += indexBufferSize;
	}
}

__ImplementClass(Dynui::ImguiRenderer, 'IMRE', Core::RefCounted);
__ImplementSingleton(Dynui::ImguiRenderer);

//------------------------------------------------------------------------------
/**
*/
ImguiRenderer::ImguiRenderer()
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
ImguiRenderer::~ImguiRenderer()
{
	__DestructSingleton
}

//------------------------------------------------------------------------------
/**
*/
void
ImguiRenderer::Setup()
{
	// allocate imgui shader
	this->uiShader = ShaderServer::Instance()->GetShader("shd:imgui");
	this->params.projVar = CoreGraphics::ShaderGetConstantBinding(this->uiShader,"TextProjectionModel");
	this->params.fontVar = CoreGraphics::ShaderGetConstantBinding(this->uiShader, "Texture");
    this->cbo = CoreGraphics::ShaderCreateConstantBuffer(this->uiShader, "GuiBlock");

	// create vertex buffer
	Util::Array<CoreGraphics::VertexComponent> components;
    components.Append(VertexComponent((VertexComponent::SemanticName)0, 0, VertexComponentBase::Float2, 0));
	components.Append(VertexComponent((VertexComponent::SemanticName)1, 0, VertexComponentBase::Float2, 0));
    components.Append(VertexComponent((VertexComponent::SemanticName)2, 0, VertexComponentBase::UByte4N, 0));
    
	// load vbo
	Ptr<MemoryVertexBufferLoader> vboLoader = MemoryVertexBufferLoader::Create();
	vboLoader->Setup(components, 1000000 * 3, NULL, 0, ResourceBase::UsageDynamic, ResourceBase::AccessWrite, ResourceBase::SyncingCoherentPersistent);

	// load buffer
	this->vbo = VertexBuffer::Create();
	this->vbo->SetLoader(vboLoader.upcast<Resources::ResourceLoader>());
	this->vbo->SetAsyncEnabled(false);
	this->vbo->Load();
	n_assert(this->vbo->IsLoaded());
	this->vbo->SetLoader(NULL);	

	// load ibo
	Ptr<MemoryIndexBufferLoader> iboLoader = MemoryIndexBufferLoader::Create();
	iboLoader->Setup(IndexType::Index16, 100000 * 3, NULL, 0, ResourceBase::UsageDynamic, ResourceBase::AccessWrite, ResourceBase::SyncingCoherentPersistent);

	// load buffer
	this->ibo = IndexBuffer::Create();
	this->ibo->SetLoader(iboLoader.upcast<Resources::ResourceLoader>());
	this->ibo->SetAsyncEnabled(false);
	this->ibo->Load();
	n_assert(this->ibo->IsLoaded());
	this->ibo->SetLoader(NULL);

	// map buffer
	this->vertexPtr = (byte*)this->vbo->Map(ResourceBase::MapWrite);
	this->indexPtr = (byte*)this->ibo->Map(ResourceBase::MapWrite);

	// create buffer lock
	this->vboBufferLock = BufferLock::Create();
	this->iboBufferLock = BufferLock::Create();

	// get display mode, this will be our default size
	Ptr<DisplayDevice> display = DisplayDevice::Instance();
	DisplayMode mode = display->GetCurrentWindow()->GetDisplayMode();

	// setup Imgui
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)mode.GetWidth(), (float)mode.GetHeight());
	io.DeltaTime = 1 / 60.0f;
	//io.PixelCenterOffset = 0.0f;
	//io.FontTexUvForWhite = ImVec2(1, 1);
	io.RenderDrawListsFn = ImguiDrawFunction;

	ImGuiStyle& style = ImGui::GetStyle();
	style.FrameRounding = 2.5f;
	style.GrabRounding = 2.5f;
	style.ChildRounding = 2.5f;
	//style.WindowTitleAlign = ImGuiAlign_Center;
	//style.WindowRounding = 1.0f;
	
	ImVec4 nebulaOrange(1.0f, 0.30f, 0.0f, 1.0f);
	nebulaOrange.w = 0.3f;
	style.Colors[ImGuiCol_TitleBg] = nebulaOrange;
	nebulaOrange.w = 0.6f;
	style.Colors[ImGuiCol_TitleBgCollapsed] = nebulaOrange;
	nebulaOrange.w = 0.9f;
	style.Colors[ImGuiCol_TitleBgActive] = nebulaOrange;
	nebulaOrange.w = 0.2f;
	style.Colors[ImGuiCol_ScrollbarBg] = nebulaOrange;
	nebulaOrange.w = 0.9f;
	style.Colors[ImGuiCol_ScrollbarGrab] = nebulaOrange;
	nebulaOrange.w = 0.7f;
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = nebulaOrange;
	nebulaOrange.w = 0.6f;
	style.Colors[ImGuiCol_Header] = nebulaOrange;
	//style.Colors[ImGuiCol_FrameBg] = nebulaOrange;
	nebulaOrange.w = 0.7f;
	style.Colors[ImGuiCol_HeaderHovered] = nebulaOrange;
	//style.Colors[ImGuiCol_FrameBgHovered] = nebulaOrange;
	nebulaOrange.w = 0.9f;
	style.Colors[ImGuiCol_HeaderActive] = nebulaOrange;
	//style.Colors[ImGuiCol_FrameBgActive] = nebulaOrange;
	nebulaOrange.w = 0.7f;	
	nebulaOrange.w = 0.5f;
	style.Colors[ImGuiCol_Button] = nebulaOrange;
	nebulaOrange.w = 0.9f;
	style.Colors[ImGuiCol_ButtonActive] = nebulaOrange;
	nebulaOrange.w = 0.7f;
	style.Colors[ImGuiCol_ButtonHovered] = nebulaOrange;	
	style.Colors[ImGuiCol_Border] = nebulaOrange;
	style.Colors[ImGuiCol_CheckMark] = nebulaOrange;
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0, 0, 0, 0.2f);

	style.Colors[ImGuiCol_Button] = ImVec4(0.3f, 0.3f, 0.33f, 1.0f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.2f, 0.25f, 0.25f, 1.0f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.3f, 0.33f, 0.33f, 1.0f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.3f, 0.33f, 0.33f, 1.0f);
	style.Colors[ImGuiCol_Text] = ImVec4(0.73f, 0.73f, 0.73f, 1.0f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.13f, 0.13f, 0.13f, 1.0f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.3f, 0.33f, 0.33f, 1.0f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 0.47f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.13f, 0.13f, 0.13f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.3f, 0.33f, 0.33f, 1.0f);

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
	io.KeyMap[ImGuiKey_A] = Key::A;
	io.KeyMap[ImGuiKey_C] = Key::C;
	io.KeyMap[ImGuiKey_V] = Key::V;
	io.KeyMap[ImGuiKey_X] = Key::X;
	io.KeyMap[ImGuiKey_Y] = Key::Y;
	io.KeyMap[ImGuiKey_Z] = Key::Z;

	// start a new frame
	//ImGui::NewFrame();

	// load default font
	ImFontConfig config;
	config.OversampleH = 1;
	config.OversampleV = 1;
#if __WIN32__
	ImFont* font = io.Fonts->AddFontFromFileTTF("c:/windows/fonts/segoeui.ttf", 18, &config);
#else
	ImFont* font = io.Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/freefont/FreeSans.ttf", 18, &config);
#endif
	
	unsigned char* buffer;
	int width, height, channels;
	io.Fonts->GetTexDataAsRGBA32(&buffer, &width, &height, &channels);

	// load image using SOIL
	// unsigned char* texData = SOIL_load_image_from_memory(buffer, width * height * channels, &width, &height, &channels, SOIL_LOAD_AUTO);

	// setup texture
	this->fontTexture = Resources::ResourceManager::Instance()->CreateUnmanagedResource("ImguiFontTexture", Texture::RTTI).downcast<Texture>();
	Ptr<MemoryTextureLoader> texLoader = MemoryTextureLoader::Create();
	texLoader->SetImageBuffer(buffer, width, height, PixelFormat::A8R8G8B8);
	this->fontTexture->SetLoader(texLoader.upcast<Resources::ResourceLoader>());
	this->fontTexture->SetAsyncEnabled(false);
	this->fontTexture->Load();
	n_assert(this->fontTexture->IsLoaded());
	this->fontTexture->SetLoader(0);
	io.Fonts->TexID = this->fontTexture;

	// set texture in shader, we shouldn't have to put it into ImGui
	this->params.fontVar->SetTexture(this->fontTexture);
}

//------------------------------------------------------------------------------
/**
*/
void
ImguiRenderer::Discard()
{
	this->uiShader = 0;

	this->vbo->Unmap();
	this->vbo->Unload();
	this->vbo = 0;
	this->vertexPtr = 0;

	this->ibo->Unmap();
	this->ibo->Unload();
	this->ibo = 0;
	this->indexPtr = 0;

	this->vboBufferLock = 0;
	this->iboBufferLock = 0;

	this->fontTexture->Unload();
	this->fontTexture = 0;
	ImGui::Shutdown();
}

//------------------------------------------------------------------------------
/**
*/
void
ImguiRenderer::Render()
{
	// render ImGui
	ImGui::Render();
}

//------------------------------------------------------------------------------
/**
*/
bool
ImguiRenderer::HandleInput(const Input::InputEvent& event)
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
		io.MousePos = ImVec2(event.GetAbsMousePos().x(), event.GetAbsMousePos().y());
		return io.WantCaptureMouse;
	case InputEvent::MouseButtonDoubleClick:
		io.MouseDoubleClicked[event.GetMouseButton()] = true;
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
ImguiRenderer::SetRectSize(SizeT width, SizeT height)
{
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)width, (float)height);
}

} // namespace Dynui