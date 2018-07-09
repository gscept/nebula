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
	uint32_t RegisterTexture(const CoreGraphics::TextureId& tex, CoreGraphics::TextureType type);
	/// register new texture
	uint32_t RegisterTexture(const CoreGraphics::RenderTextureId& tex, bool depth, CoreGraphics::TextureType type);
	/// register new texture
	uint32_t RegisterTexture(const CoreGraphics::ShaderRWTextureId& tex, CoreGraphics::TextureType type);
	/// unregister texture
	void UnregisterTexture(const uint32_t id, const CoreGraphics::TextureType type);


	/// submit resource changes
	void SubmitTextureDescriptorChanges();
	/// commit texture library to shader
	void BindTextureDescriptorSets();

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

	CoreGraphics::ResourceTableId resourceTable;
	CoreGraphics::ShaderStateId textureShaderState;
	CoreGraphics::ShaderConstantId texture2DTextureVar;
	CoreGraphics::ShaderConstantId texture2DMSTextureVar;
	CoreGraphics::ShaderConstantId texture2DArrayTextureVar;
	CoreGraphics::ShaderConstantId texture3DTextureVar;
	CoreGraphics::ShaderConstantId textureCubeTextureVar;
	CoreGraphics::ShaderConstantId image2DTextureVar;
	CoreGraphics::ShaderConstantId image2DMSTextureVar;
	CoreGraphics::ShaderConstantId image3DTextureVar;
	CoreGraphics::ShaderConstantId imageCubeTextureVar;

	CoreGraphics::ShaderConstantId depthBufferTextureVar;
	CoreGraphics::ShaderConstantId normalBufferTextureVar;
	CoreGraphics::ShaderConstantId albedoBufferTextureVar;
	CoreGraphics::ShaderConstantId specularBufferTextureVar;
	CoreGraphics::ShaderConstantId lightBufferTextureVar;

	CoreGraphics::ShaderConstantId csmBufferTextureVar;
	CoreGraphics::ShaderConstantId spotlightAtlasShadowBufferTextureVar;

	AnyFX::EffectFactory* factory;
};

//------------------------------------------------------------------------------
/**
*/
inline void
VkShaderServer::SubmitTextureDescriptorChanges()
{
	ResourceTableCommitChanges(this->resourceTable);
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkShaderServer::BindTextureDescriptorSets()
{
	CoreGraphics::SetShaderState(this->textureShaderState);
}

} // namespace Vulkan