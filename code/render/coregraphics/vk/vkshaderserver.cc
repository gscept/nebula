//------------------------------------------------------------------------------
// vkshaderserver.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkshaderserver.h"
#include "effectfactory.h"
#include "coregraphics/shaderpool.h"
#include "vkgraphicsdevice.h"
#include "vkshaderpool.h"
#include "vkshader.h"

#include "shared.h"

using namespace Resources;
using namespace CoreGraphics;
namespace Vulkan
{

__ImplementClass(Vulkan::VkShaderServer, 'VKSS', Base::ShaderServerBase);
__ImplementInterfaceSingleton(Vulkan::VkShaderServer);
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
	this->texture2DPool.Resize(Shared::MAX_2D_TEXTURES);
	this->texture2DMSPool.SetSetupFunc(func);
	this->texture2DMSPool.Resize(Shared::MAX_2D_MS_TEXTURES);
	this->texture3DPool.SetSetupFunc(func);
	this->texture3DPool.Resize(Shared::MAX_3D_TEXTURES);
	this->textureCubePool.SetSetupFunc(func);
	this->textureCubePool.Resize(Shared::MAX_CUBE_TEXTURES);
	this->texture2DArrayPool.SetSetupFunc(func);
	this->texture2DArrayPool.Resize(Shared::MAX_2D_ARRAY_TEXTURES);

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
	this->pendingViewDeletes.Resize(CoreGraphics::GetNumBufferedFrames());
	IndexT i;
	for (i = 0; i < this->resourceTables.Size(); i++)
	{
		this->resourceTables[i] = ShaderCreateResourceTable(shader, NEBULA_TICK_GROUP);

		// fill up all slots with placeholders
		IndexT j;
		for (j = 0; j < Shared::MAX_2D_TEXTURES; j++)
			ResourceTableSetTexture(this->resourceTables[i], {CoreGraphics::White2D, this->texture2DTextureVar, j, CoreGraphics::SamplerId::Invalid(), false});

		for (j = 0; j < Shared::MAX_2D_MS_TEXTURES; j++)
			ResourceTableSetTexture(this->resourceTables[i], { CoreGraphics::White2D, this->texture2DMSTextureVar, j, CoreGraphics::SamplerId::Invalid(), false });

		for (j = 0; j < Shared::MAX_3D_TEXTURES; j++)
			ResourceTableSetTexture(this->resourceTables[i], { CoreGraphics::White3D, this->texture3DTextureVar, j, CoreGraphics::SamplerId::Invalid(), false });

		for (j = 0; j < Shared::MAX_CUBE_TEXTURES; j++)
			ResourceTableSetTexture(this->resourceTables[i], { CoreGraphics::WhiteCube, this->textureCubeTextureVar, j, CoreGraphics::SamplerId::Invalid(), false });

		for (j = 0; j < Shared::MAX_2D_ARRAY_TEXTURES; j++)
			ResourceTableSetTexture(this->resourceTables[i], { CoreGraphics::White2DArray, this->texture2DArrayTextureVar, j, CoreGraphics::SamplerId::Invalid(), false });

		ResourceTableCommitChanges(this->resourceTables[i]);
	}

	this->normalBufferTextureVar = ShaderGetConstantBinding(shader, "NormalBuffer");
	this->depthBufferTextureVar = ShaderGetConstantBinding(shader, "DepthBuffer");
	this->specularBufferTextureVar = ShaderGetConstantBinding(shader, "SpecularBuffer");
	this->albedoBufferTextureVar = ShaderGetConstantBinding(shader, "AlbedoBuffer");
	this->emissiveBufferTextureVar = ShaderGetConstantBinding(shader, "EmissiveBuffer");
	this->lightBufferTextureVar = ShaderGetConstantBinding(shader, "LightBuffer");
	this->depthBufferCopyTextureVar = ShaderGetConstantBinding(shader, "DepthBufferCopy");

	this->environmentMapVar = ShaderGetConstantBinding(shader, "EnvironmentMap");
	this->irradianceMapVar = ShaderGetConstantBinding(shader, "IrradianceMap");
	this->numEnvMipsVar = ShaderGetConstantBinding(shader, "NumEnvMips");
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
VkShaderServer::RegisterTexture(const CoreGraphics::TextureId& tex, CoreGraphics::TextureType type, bool depth, bool stencil)
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
	case Texture2DArray:
		n_assert(!this->texture2DArrayPool.IsFull());
		idx = this->texture2DArrayPool.Alloc();
		var = this->texture2DArrayTextureVar;
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
	default:
		n_error("Should not happen");
		idx = UINT_MAX;
		var = InvalidIndex;
	}

	ResourceTableTexture info;
	info.tex = tex;
	info.index = idx;
	info.sampler = SamplerId::Invalid();
	info.isDepth = depth;
	info.isStencil = stencil;
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
VkShaderServer::ReregisterTexture(const CoreGraphics::TextureId& tex, CoreGraphics::TextureType type, uint32_t slot, bool depth, bool stencil)
{
	IndexT var;
	switch (type)
	{
	case Texture2D:
		var = this->texture2DTextureVar;
		break;
	case Texture2DArray:
		var = this->texture2DArrayTextureVar;
		break;
	case Texture3D:
		var = this->texture3DTextureVar;
		break;
	case TextureCube:
		var = this->textureCubeTextureVar;
		break;
	}

	ResourceTableTexture info;
	info.tex = tex;
	info.index = slot;
	info.sampler = SamplerId::Invalid();
	info.isDepth = depth;
	info.isStencil = stencil;
	info.slot = var;

	// update textures for all tables
	IndexT i;
	for (i = 0; i < this->resourceTables.Size(); i++)
	{
		ResourceTableSetTexture(this->resourceTables[i], info);
	}
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
	case Texture2DArray:
		this->texture2DArrayPool.Free(id);
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
	this->tickParams.NormalBuffer = TextureGetBindlessHandle(CoreGraphics::GetTexture("NormalBuffer"));
	this->tickParams.DepthBuffer = TextureGetBindlessHandle(CoreGraphics::GetTexture("ZBuffer"));
	this->tickParams.SpecularBuffer = TextureGetBindlessHandle(CoreGraphics::GetTexture("SpecularBuffer"));
	this->tickParams.AlbedoBuffer = TextureGetBindlessHandle(CoreGraphics::GetTexture("AlbedoBuffer"));
	this->tickParams.EmissiveBuffer = TextureGetBindlessHandle(CoreGraphics::GetTexture("EmissiveBuffer"));
	this->tickParams.LightBuffer = TextureGetBindlessHandle(CoreGraphics::GetTexture("LightBuffer"));
	this->tickParams.DepthBufferCopy = TextureGetBindlessHandle(CoreGraphics::GetTexture("ZBufferCopy"));
}

//------------------------------------------------------------------------------
/**
*/
void 
VkShaderServer::UpdateResources()
{
	// just allocate the memory
	this->cboOffset = CoreGraphics::AllocateGraphicsConstantBufferMemory(MainThreadConstantBuffer, sizeof(Shared::PerTickParams));
	IndexT bufferedFrameIndex = GetBufferedFrameIndex();

	// delete views which have been discarded due to LOD streaming
	this->viewCreationCriticalSection.Enter();
	for (int i = 0; i < this->pendingViewDeletes[bufferedFrameIndex].Size(); i++)
		vkDestroyImageView(GetCurrentDevice(), this->pendingViewDeletes[bufferedFrameIndex][i], nullptr);
	this->pendingViewDeletes[bufferedFrameIndex].Clear();

	// setup new views for newly streamed LODs
	for (int i = 0; i < this->pendingViewCreations.Size(); i++)
	{
		const _PendingView& pend = this->pendingViewCreations[i];
		VkImageView view;
		vkCreateImageView(GetCurrentDevice(), &pend.info, nullptr, &view);
		VkTextureRuntimeInfo& info = textureAllocator.GetSafe<Texture_RuntimeInfo>(pend.tex.resourceId);
		pendingViewDeletes[bufferedFrameIndex].Append(info.view);
		info.view = view;
		this->ReregisterTexture(pend.tex, info.type, info.bind);
	}
	this->pendingViewCreations.Clear();
	this->viewCreationCriticalSection.Leave();

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
	CoreGraphics::SetGraphicsConstants(MainThreadConstantBuffer, this->cboOffset, this->tickParams);
}

//------------------------------------------------------------------------------
/**
*/
void 
VkShaderServer::AddPendingImageView(VkImageViewCreateInfo info, CoreGraphics::TextureId tex)
{
	this->viewCreationCriticalSection.Enter();
	_PendingView pend;
	pend.tex = tex;
	pend.info = info;
	this->pendingViewCreations.Append(pend);
	this->viewCreationCriticalSection.Leave();
}


} // namespace Vulkan