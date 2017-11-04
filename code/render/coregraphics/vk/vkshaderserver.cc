//------------------------------------------------------------------------------
// vkshaderserver.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkshaderserver.h"
#include "effectfactory.h"
#include "coregraphics/shaderpool.h"
#include "vkrenderdevice.h"
#include "vkshaderpool.h"
#include "coregraphics/coregraphics.h"

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
	this->textureShaderState = CoreGraphics::shaderPool->CreateState("shd:shared", { NEBULAT_TICK_GROUP });
	this->texture2DTextureVar = CoreGraphics::shaderPool->GetShaderVariable(this->textureShaderState, "Textures2D");
	this->texture2DMSTextureVar = CoreGraphics::shaderPool->GetShaderVariable(this->textureShaderState, "Textures2DMS");
	this->textureCubeTextureVar = CoreGraphics::shaderPool->GetShaderVariable(this->textureShaderState, "TexturesCube");
	this->texture3DTextureVar = CoreGraphics::shaderPool->GetShaderVariable(this->textureShaderState, "Textures3D");

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
	CoreGraphics::shaderPool->DestroyState(this->textureShaderState);
	ShaderServerBase::Close();
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderServer::ReloadShader(Ptr<CoreGraphics::Shader> shader)
{
	n_assert(0 != shader);
	shader->SetLoader(ShaderPool::Create());
	shader->SetAsyncEnabled(false);
	shader->Load();
	if (shader->IsLoaded())
	{
		shader->SetLoader(0);
	}
	else
	{
		n_error("Failed to load shader '%s'!", shader->GetResourceId().Value());
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderServer::LoadShader(const Resources::ResourceId& shdName)
{
	n_assert(shdName.IsValid());
	Ptr<Shader> shader = Shader::Create();
	shader->SetResourceId(shdName);
	shader->SetLoader(ShaderPool::Create());
	shader->SetAsyncEnabled(false);
	shader->Load();
	if (shader->IsLoaded())
	{
		shader->SetLoader(0);
		this->shaders.Add(shdName, shader);
	}
	else
	{
		n_warning("Failed to explicitly load shader '%s'!", shdName.Value());
		shader->Unload();
	}
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
VkShaderServer::RegisterTexture(const VkImageView tex, Base::TextureBase::Type type)
{
	uint32_t idx;
	Ptr<VkShaderVariable> var;
	VkDescriptorImageInfo* img;
	switch (type)
	{
	case Texture::Texture2D:
		n_assert(!this->texture2DPool.IsFull());
		idx = this->texture2DPool.Alloc();
		var = this->texture2DTextureVar;
		img = &this->texture2DDescriptors[idx];
		break;
	case Texture::Texture3D:
		n_assert(!this->texture3DPool.IsFull());
		idx = this->texture3DPool.Alloc();
		var = this->texture3DTextureVar;
		img = &this->texture3DDescriptors[idx];
		break;
	case Texture::TextureCube:
		n_assert(!this->textureCubePool.IsFull());
		idx = this->textureCubePool.Alloc();
		var = this->textureCubeTextureVar;
		img = &this->textureCubeDescriptors[idx];
		break;
	}
	
	// do the magic where we update the descriptor
	VkWriteDescriptorSet write;
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.pNext = NULL;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	write.dstArrayElement = idx;							// insert image into array index, use this index in shader to fetch texture back
	write.dstBinding = var->binding;
	write.dstSet = var->set;	
	write.pTexelBufferView = NULL;
	write.pBufferInfo = NULL;

	img->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	img->imageView = tex;
	img->sampler = VK_NULL_HANDLE;
	write.pImageInfo = img;
	this->textureShaderState->AddDescriptorWrite(write);
	return idx;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderServer::UnregisterTexture(const uint32_t id, const Base::TextureBase::Type type)
{
	switch (type)
	{
	case Texture::Texture2D:
		this->texture2DPool.Free(id);
		break;
	case Texture::Texture3D:
		this->texture3DPool.Free(id);
		break;
	case Texture::TextureCube:
		this->textureCubePool.Free(id);
		break;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderServer::SetRenderTarget(const Util::StringAtom& name, const Ptr<Vulkan::VkTexture>& tex)
{
	n_assert(this->textureShaderState->HasVariableByName(name));
	this->textureShaderState->GetVariableByName(name)->SetTexture(tex.downcast<CoreGraphics::Texture>());
}

} // namespace Vulkan