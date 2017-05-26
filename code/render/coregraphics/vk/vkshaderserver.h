#pragma once
//------------------------------------------------------------------------------
/**
	Implements the shader server used by Vulkan.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "util/fixedpool.h"
#include "coregraphics/base/shaderserverbase.h"
#include "coregraphics/config.h"
#include "effectfactory.h"

namespace Vulkan
{
class VkShaderServer : public Base::ShaderServerBase
{
	__DeclareClass(VkShaderServer);
	__DeclareSingleton(VkShaderServer);
public:
	/// constructor
	VkShaderServer();
	/// destructor
	virtual ~VkShaderServer();

	/// open the shader server
	bool Open();
	/// close the shader server
	void Close();
	
	/// register new texture
	uint32_t RegisterTexture(const Ptr<Vulkan::VkTexture>& tex);
	/// unregister texture
	void UnregisterTexture(const Ptr<Vulkan::VkTexture>& tex);
	/// commit texture library to shader
	void BindTextureDescriptorSets();

	/// update global shader variable
	void SetRenderTarget(const Util::StringAtom& name, const Ptr<Vulkan::VkTexture>& tex);

	/// reloads a shader
	void ReloadShader(Ptr<CoreGraphics::Shader> shader);
	/// explicitly loads a shader by resource id
	void LoadShader(const Resources::ResourceId& shdName);

private:

	Util::FixedPool<uint32_t> texture2DPool;
	Util::FixedPool<uint32_t> texture2DMSPool;
	Util::FixedPool<uint32_t> texture3DPool;
	Util::FixedPool<uint32_t> textureCubePool;

	VkDescriptorImageInfo texture2DDescriptors[MAX_2D_TEXTURES];
	VkDescriptorImageInfo texture2DMSDescriptors[MAX_2D_MS_TEXTURES];
	VkDescriptorImageInfo texture3DDescriptors[MAX_3D_TEXTURES];
	VkDescriptorImageInfo textureCubeDescriptors[MAX_CUBE_TEXTURES];
	Util::FixedPool<uint32_t> image2DPool;
	Util::FixedPool<uint32_t> image2DMSPool;
	Util::FixedPool<uint32_t> image3DPool;
	Util::FixedPool<uint32_t> imageCubePool;

	Ptr<VkShaderState> textureShaderState;
	Ptr<VkShaderVariable> texture2DTextureVar;
	Ptr<VkShaderVariable> texture2DMSTextureVar;
	Ptr<VkShaderVariable> texture3DTextureVar;
	Ptr<VkShaderVariable> textureCubeTextureVar;
	Ptr<VkShaderVariable> image2DTextureVar;
	Ptr<VkShaderVariable> image2DMSTextureVar;
	Ptr<VkShaderVariable> image3DTextureVar;
	Ptr<VkShaderVariable> imageCubeTextureVar;

	Ptr<VkShaderVariable> depthBufferTextureVar;
	Ptr<VkShaderVariable> normalBufferTextureVar;
	Ptr<VkShaderVariable> albedoBufferTextureVar;
	Ptr<VkShaderVariable> specularBufferTextureVar;
	Ptr<VkShaderVariable> lightBufferTextureVar;

	Ptr<VkShaderVariable> csmBufferTextureVar;
	Ptr<VkShaderVariable> spotlightAtlasShadowBufferTextureVar;

	AnyFX::EffectFactory* factory;
};

//------------------------------------------------------------------------------
/**
*/
inline void
VkShaderServer::BindTextureDescriptorSets()
{
	this->textureShaderState->Commit();
}

} // namespace Vulkan