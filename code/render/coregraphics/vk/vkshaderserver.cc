//------------------------------------------------------------------------------
// vkshaderserver.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkshaderserver.h"
#include "effectfactory.h"
#include "coregraphics/shaderpool.h"
#include "vkgraphicsdevice.h"
#include "vkshaderpool.h"
#include "vkshader.h"

using namespace Resources;
using namespace CoreGraphics;
namespace Vulkan
{

__ImplementClass(Vulkan::VkShaderServer, 'VKSS', Base::ShaderServerBase);
__ImplementSingleton(Vulkan::VkShaderServer);
//------------------------------------------------------------------------------
/**
*/
VkShaderServer::VkShaderServer()
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
VkShaderServer::~VkShaderServer()
{
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
bool
VkShaderServer::Open()
{
	n_assert(!this->IsOpen());

	// create anyfx factory
	this->factory = n_new(AnyFX::EffectFactory);
	ShaderServerBase::Open();

	auto func = [](uint32_t& val, IndexT i) -> void
	{
		val = i;
	};

	this->texture2DPool.SetSetupFunc(func);
	this->texture2DPool.Resize(MAX_2D_TEXTURES);
	this->texture2DMSPool.SetSetupFunc(func);
	this->texture2DMSPool.Resize(MAX_2D_MS_TEXTURES);
	this->texture3DPool.SetSetupFunc(func);
	this->texture3DPool.Resize(MAX_3D_TEXTURES);
	this->textureCubePool.SetSetupFunc(func);
	this->textureCubePool.Resize(MAX_CUBE_TEXTURES);

	// create shader state for textures, and fetch variables
	ShaderId shader = VkShaderServer::Instance()->GetShader("shd:shared.fxb"_atm);

	this->texture2DTextureVar = ShaderGetResourceSlot(shader, "Textures2D");
	this->texture2DMSTextureVar = ShaderGetResourceSlot(shader, "Textures2DMS");
	this->texture2DArrayTextureVar = ShaderGetResourceSlot(shader, "Textures2DArray");
	this->textureCubeTextureVar = ShaderGetResourceSlot(shader, "TexturesCube");
	this->texture3DTextureVar = ShaderGetResourceSlot(shader, "Textures3D");
	this->tableLayout = ShaderGetResourcePipeline(shader);
	
	this->ticksCbo = CoreGraphics::GetGraphicsConstantBuffer(MainThreadConstantBuffer);
	this->cboSlot = ShaderGetResourceSlot(shader, "PerTickParams");

	this->resourceTables.Resize(CoreGraphics::GetNumBufferedFrames());
	IndexT i;
	for (i = 0; i < this->resourceTables.Size(); i++)
	{
		this->resourceTables[i] = ShaderCreateResourceTable(shader, NEBULA_TICK_GROUP);
	}

	this->normalBufferTextureVar = ShaderGetConstantBinding(shader, "NormalBuffer");
	this->depthBufferTextureVar = ShaderGetConstantBinding(shader, "DepthBuffer");
	this->specularBufferTextureVar = ShaderGetConstantBinding(shader, "SpecularBuffer");
	this->albedoBufferTextureVar = ShaderGetConstantBinding(shader, "AlbedoBuffer");
	this->emissiveBufferTextureVar = ShaderGetConstantBinding(shader, "EmissiveBuffer");
	this->lightBufferTextureVar = ShaderGetConstantBinding(shader, "LightBuffer");

	this->environmentMapVar = ShaderGetConstantBinding(shader, "EnvironmentMap");
	this->irradianceMapVar = ShaderGetConstantBinding(shader, "IrradianceMap");
	this->numEnvMipsVar = ShaderGetConstantBinding(shader, "NumEnvMips");

	this->tickParams.NumEnvMips = 10;
	return true;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderServer::Close()
{
	n_assert(this->IsOpen());
	n_delete(this->factory);
	IndexT i;
	for (i = 0; i < this->resourceTables.Size(); i++)
	{
		DestroyResourceTable(this->resourceTables[i]);
	}
	ShaderServerBase::Close();
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
VkShaderServer::RegisterTexture(const CoreGraphics::TextureId& tex, CoreGraphics::TextureType type)
{
	uint32_t idx;
	IndexT var;
	switch (type)
	{
	case Texture2D:
		n_assert(!this->texture2DPool.IsFull());
		idx = this->texture2DPool.Alloc();
		var = this->texture2DTextureVar;
		break;
	case Texture3D:
		n_assert(!this->texture3DPool.IsFull());
		idx = this->texture3DPool.Alloc();
		var = this->texture3DTextureVar;
		break;
	case TextureCube:
		n_assert(!this->textureCubePool.IsFull());
		idx = this->textureCubePool.Alloc();
		var = this->textureCubeTextureVar;
		break;
	}

	ResourceTableTexture info;
	info.tex = tex;
	info.index = idx;
	info.sampler = SamplerId::Invalid();
	info.isDepth = false;
	info.slot = var;

	// update textures for all tables
	IndexT i;
	for (i = 0; i < this->resourceTables.Size(); i++)
	{
		ResourceTableSetTexture(this->resourceTables[i], info);
	}

	return idx;
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
VkShaderServer::RegisterTexture(const CoreGraphics::RenderTextureId& tex, bool depth, CoreGraphics::TextureType type)
{
	uint32_t idx;
	IndexT var;
	switch (type)
	{
	case Texture2D:
		n_assert(!this->texture2DPool.IsFull());
		idx = this->texture2DPool.Alloc();
		var = this->texture2DTextureVar;
		break;
	case Texture3D:
		n_assert(!this->texture3DPool.IsFull());
		idx = this->texture3DPool.Alloc();
		var = this->texture3DTextureVar;
		break;
	case TextureCube:
		n_assert(!this->textureCubePool.IsFull());
		idx = this->textureCubePool.Alloc();
		var = this->textureCubeTextureVar;
		break;
	}

	ResourceTableRenderTexture info;
	info.tex = tex;
	info.index = idx;
	info.sampler = SamplerId::Invalid();
	info.isDepth = depth;
	info.slot = var;

	// update textures for all tables
	IndexT i;
	for (i = 0; i < this->resourceTables.Size(); i++)
	{
		ResourceTableSetTexture(this->resourceTables[i], info);
	}

	return idx;
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
VkShaderServer::RegisterTexture(const CoreGraphics::ShaderRWTextureId& tex, CoreGraphics::TextureType type)
{
	uint32_t idx;
	IndexT var;
	switch (type)
	{
	case Texture2D:
		n_assert(!this->texture2DPool.IsFull());
		idx = this->texture2DPool.Alloc();
		var = this->texture2DTextureVar;
		break;
	case Texture3D:
		n_assert(!this->texture3DPool.IsFull());
		idx = this->texture3DPool.Alloc();
		var = this->texture3DTextureVar;
		break;
	case TextureCube:
		n_assert(!this->textureCubePool.IsFull());
		idx = this->textureCubePool.Alloc();
		var = this->textureCubeTextureVar;
		break;
	}

	ResourceTableShaderRWTexture info;
	info.tex = tex;
	info.index = idx;
	info.sampler = SamplerId::Invalid();
	info.slot = var;

	// update textures for all tables
	IndexT i;
	for (i = 0; i < this->resourceTables.Size(); i++)
	{
		ResourceTableSetTexture(this->resourceTables[i], info);
	}

	return idx;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderServer::UnregisterTexture(const uint32_t id, const CoreGraphics::TextureType type)
{
	switch (type)
	{
	case Texture2D:
		this->texture2DPool.Free(id);
		break;
	case Texture3D:
		this->texture3DPool.Free(id);
		break;
	case TextureCube:
		this->textureCubePool.Free(id);
		break;
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
VkShaderServer::SetGlobalEnvironmentTextures(const CoreGraphics::TextureId& env, const CoreGraphics::TextureId& irr, const SizeT numMips)
{
	this->tickParams.EnvironmentMap = CoreGraphics::TextureGetBindlessHandle(env);
	this->tickParams.IrradianceMap = CoreGraphics::TextureGetBindlessHandle(irr);
	this->tickParams.NumEnvMips = numMips;
}

//------------------------------------------------------------------------------
/**
*/
void 
VkShaderServer::SetupGBufferConstants()
{
	this->tickParams.NormalBuffer = RenderTextureGetBindlessHandle(CoreGraphics::GetRenderTexture("NormalBuffer"));
	this->tickParams.DepthBuffer = RenderTextureGetBindlessHandle(CoreGraphics::GetRenderTexture("DepthBuffer"));
	this->tickParams.SpecularBuffer = RenderTextureGetBindlessHandle(CoreGraphics::GetRenderTexture("SpecularBuffer"));
	this->tickParams.AlbedoBuffer = RenderTextureGetBindlessHandle(CoreGraphics::GetRenderTexture("AlbedoBuffer"));
	this->tickParams.EmissiveBuffer = RenderTextureGetBindlessHandle(CoreGraphics::GetRenderTexture("EmissiveBuffer"));
	this->tickParams.LightBuffer = RenderTextureGetBindlessHandle(CoreGraphics::GetRenderTexture("LightBuffer"));
}

//------------------------------------------------------------------------------
/**
*/
void 
VkShaderServer::BeforeView()
{
	// just allocate the memory
	this->cboOffset = CoreGraphics::AllocateGraphicsConstantBufferMemory(MainThreadConstantBuffer, sizeof(Shared::PerTickParams));
	IndexT bufferedFrameIndex = GetBufferedFrameIndex();

	// update resource table
	ResourceTableSetConstantBuffer(this->resourceTables[bufferedFrameIndex], { this->ticksCbo, this->cboSlot, 0, false, false, sizeof(Shared::PerTickParams), (SizeT)this->cboOffset });
	ResourceTableCommitChanges(this->resourceTables[bufferedFrameIndex]);
}

//------------------------------------------------------------------------------
/**
*/
void 
VkShaderServer::AfterView()
{
	// update the constant buffer with the data accumulated in this frame
	ConstantBufferUpdate(this->ticksCbo, this->tickParams, this->cboOffset);
}

} // namespace Vulkan