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
#include "vkshaderpool.h"

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
	uint32_t RegisterTexture(const VkImageView tex, Base::TextureBase::Type type);
	/// unregister texture
	void UnregisterTexture(const uint32_t id, const Base::TextureBase::Type type);
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

	CoreGraphics::ShaderStateId textureShaderState;
	CoreGraphics::ShaderVariableId texture2DTextureVar;
	CoreGraphics::ShaderVariableId texture2DMSTextureVar;
	CoreGraphics::ShaderVariableId texture3DTextureVar;
	CoreGraphics::ShaderVariableId textureCubeTextureVar;
	CoreGraphics::ShaderVariableId image2DTextureVar;
	CoreGraphics::ShaderVariableId image2DMSTextureVar;
	CoreGraphics::ShaderVariableId image3DTextureVar;
	CoreGraphics::ShaderVariableId imageCubeTextureVar;

	CoreGraphics::ShaderVariableId depthBufferTextureVar;
	CoreGraphics::ShaderVariableId normalBufferTextureVar;
	CoreGraphics::ShaderVariableId albedoBufferTextureVar;
	CoreGraphics::ShaderVariableId specularBufferTextureVar;
	CoreGraphics::ShaderVariableId lightBufferTextureVar;

	CoreGraphics::ShaderVariableId csmBufferTextureVar;
	CoreGraphics::ShaderVariableId spotlightAtlasShadowBufferTextureVar;

	AnyFX::EffectFactory* factory;
};

//------------------------------------------------------------------------------
/**
*/
inline void
VkShaderServer::BindTextureDescriptorSets()
{
	CoreGraphics::shaderPool->ApplyState(this->textureShaderState);
}

} // namespace Vulkan