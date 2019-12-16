#pragma once
//------------------------------------------------------------------------------
/**
	Implements the shader server used by Vulkan.
	
	(C) 2016-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "util/fixedpool.h"
#include "coregraphics/base/shaderserverbase.h"
#include "coregraphics/config.h"
#include "effectfactory.h"
#include "vkshaderpool.h"
#include "coregraphics/graphicsdevice.h"
#include "shared.h"

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
	uint32_t RegisterTexture(const CoreGraphics::TextureId& tex, bool depth, CoreGraphics::TextureType type);
	/// unregister texture
	void UnregisterTexture(const uint32_t id, const CoreGraphics::TextureType type);

	/// set global irradiance and cubemaps
	void SetGlobalEnvironmentTextures(const CoreGraphics::TextureId& env, const CoreGraphics::TextureId& irr, const SizeT numMips);

	/// get tick params constant buffer
	Shared::PerTickParams& GetTickParams();

	/// submit resource changes
	void SubmitTextureDescriptorChanges();
	/// commit texture library to graphics pipeline
	void BindTextureDescriptorSetsGraphics();
	/// commit texture library to compute pipeline
	void BindTextureDescriptorSetsCompute(const CoreGraphicsQueueType queue = GraphicsQueueType);

	/// setup gbuffer bindings
	void SetupGBufferConstants();

	/// begin frame
	void BeforeView();
	/// end frame
	void AfterView();

private:

	Util::FixedPool<uint32_t> texture1DPool;
	Util::FixedPool<uint32_t> texture1DArrayPool;
	Util::FixedPool<uint32_t> texture2DPool;
	Util::FixedPool<uint32_t> texture2DMSPool;
	Util::FixedPool<uint32_t> texture2DArrayPool;
	Util::FixedPool<uint32_t> texture3DPool;
	Util::FixedPool<uint32_t> textureCubePool;
	Util::FixedPool<uint32_t> textureCubeArrayPool;


	Util::FixedArray<CoreGraphics::ResourceTableId> resourceTables;
	CoreGraphics::ResourcePipelineId tableLayout;
	IndexT texture2DTextureVar;
	IndexT texture2DMSTextureVar;
	IndexT texture2DArrayTextureVar;
	IndexT texture3DTextureVar;
	IndexT textureCubeTextureVar;
	IndexT image2DTextureVar;
	IndexT image2DMSTextureVar;
	IndexT image3DTextureVar;
	IndexT imageCubeTextureVar;

	CoreGraphics::ConstantBinding normalBufferTextureVar;
	CoreGraphics::ConstantBinding depthBufferTextureVar;
	CoreGraphics::ConstantBinding specularBufferTextureVar;
	CoreGraphics::ConstantBinding albedoBufferTextureVar;
	CoreGraphics::ConstantBinding emissiveBufferTextureVar;
	CoreGraphics::ConstantBinding lightBufferTextureVar;

	CoreGraphics::ConstantBinding environmentMapVar;
	CoreGraphics::ConstantBinding irradianceMapVar;
	CoreGraphics::ConstantBinding numEnvMipsVar;

	IndexT csmBufferTextureVar;
	IndexT spotlightAtlasShadowBufferTextureVar;

	alignas(16) Shared::PerTickParams tickParams;
	CoreGraphics::ConstantBufferId ticksCbo;
	IndexT cboOffset;
	IndexT cboSlot;

	AnyFX::EffectFactory* factory;
};

//------------------------------------------------------------------------------
/**
*/
inline void
VkShaderServer::SubmitTextureDescriptorChanges()
{
	IndexT bufferedFrameIndex = CoreGraphics::GetBufferedFrameIndex();
	ResourceTableCommitChanges(this->resourceTables[bufferedFrameIndex]);
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkShaderServer::BindTextureDescriptorSetsGraphics()
{
	IndexT bufferedFrameIndex = CoreGraphics::GetBufferedFrameIndex();
	CoreGraphics::SetResourceTable(this->resourceTables[bufferedFrameIndex], NEBULA_TICK_GROUP, CoreGraphics::GraphicsPipeline, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
inline void 
VkShaderServer::BindTextureDescriptorSetsCompute(const CoreGraphicsQueueType queue)
{
	IndexT bufferedFrameIndex = CoreGraphics::GetBufferedFrameIndex();
	CoreGraphics::SetResourceTable(this->resourceTables[bufferedFrameIndex], NEBULA_TICK_GROUP, CoreGraphics::ComputePipeline, nullptr, queue);
}

//------------------------------------------------------------------------------
/**
*/
inline Shared::PerTickParams&
VkShaderServer::GetTickParams()
{
	return this->tickParams;
}

} // namespace Vulkan