#pragma once
//------------------------------------------------------------------------------
/**
    @class Lighting::VkLightServer
    
    A newer version of the light prepass server, used for deferred lighting
    
    (C) 2011-2013 Individual contributors, see AUTHORS file
*/
#include "lighting/base/lightserverbase.h"
#include "graphics/pointlightentity.h"
#include "graphics/spotlightentity.h"
#include "coregraphics/shaderstate.h"
#include "resources/managedmesh.h"
#include "resources/managedtexture.h"
#include "renderutil/drawfullscreenquad.h"
#include "coregraphics/rendertarget.h"
#include "renderutil/drawquad.h"
#include "graphics/modelentity.h"
#include "util/fixedpool.h"

//------------------------------------------------------------------------------
namespace Lighting
{
class VkLightServer : public LightServerBase
{
	__DeclareClass(VkLightServer);
public:
	/// constructor
	VkLightServer();
	/// destructor
	virtual ~VkLightServer();

	/// open the light server
	void Open();
	/// close the light server
	void Close();
	/// returns if the server needs model-light linking
	bool NeedsLightModelLinking() const;

	/// attach a visible light source
	void AttachVisibleLight(const Ptr<Graphics::AbstractLightEntity>& lightEntity);
	/// end lighting frame
	void EndFrame();
	/// render lights
	void RenderLights();
	/// render light probes
	void RenderLightProbes();
	
protected:

	/// render the global light
	void RenderGlobalLight();
	/// render all point lights
	void RenderPointLights();
	/// render all spot lights
	void RenderSpotLights();
	/// assign render buffers to shaders
	void AssignRenderBufferTextures();

	/// update descriptor set
	void UpdateDescriptor(const VkBuffer& buffer, const IndexT binding, VkDescriptorSet set);

	enum ShadowFlag
	{
		CastShadows = 0,
		NoShadows,

		NumShadowFlags
	};

	enum ProbeFlag
	{
		Box,
		Sphere,
		BoxParallax,
		SphereParallax,

		NumProbeFlags
	};

	Util::Array<Ptr<Graphics::PointLightEntity> > pointLights[NumShadowFlags];
	Util::Array<Ptr<Graphics::SpotLightEntity> > spotLights[NumShadowFlags];

	CoreGraphics::ShaderFeature::Mask pointLightFeatureBits[NumShadowFlags];
	CoreGraphics::ShaderFeature::Mask spotLightFeatureBits[NumShadowFlags];
	CoreGraphics::ShaderFeature::Mask globalLightFeatureBits[NumShadowFlags];
	CoreGraphics::ShaderFeature::Mask lightProbeFeatureBits[Graphics::LightProbeEntity::NumProbeShapeTypes + 2];
	RenderUtil::DrawFullScreenQuad fullScreenQuadRenderer;          // fs quad renderer

	Ptr<CoreGraphics::ShaderState> globalLightShader;
	Ptr<CoreGraphics::ShaderState> localLightShader;
	Ptr<CoreGraphics::ShaderState> lightProbeShader;
	Ptr<Resources::ManagedTexture> lightProjMap;

	/// global light variables
    Ptr<CoreGraphics::ConstantBuffer> lightServerUniformBuffer;
    Ptr<CoreGraphics::ShaderVariable> lightServerUniformBufferVar;
	Ptr<CoreGraphics::ShaderVariable> globalLightDirWorldspace;
	Ptr<CoreGraphics::ShaderVariable> globalLightDir;
	Ptr<CoreGraphics::ShaderVariable> globalLightColor;
	Ptr<CoreGraphics::ShaderVariable> globalBackLightColor;
	Ptr<CoreGraphics::ShaderVariable> globalAmbientLightColor;
	Ptr<CoreGraphics::ShaderVariable> globalBackLightOffset;
    Ptr<CoreGraphics::ShaderVariable> globalLightShadowMatrixVar;

	Ptr<CoreGraphics::ShaderVariable> globalLightCascadeOffset;
	Ptr<CoreGraphics::ShaderVariable> globalLightCascadeScale;
	Ptr<CoreGraphics::ShaderVariable> globalLightMinBorderPadding;
	Ptr<CoreGraphics::ShaderVariable> globalLightMaxBorderPadding;

	Ptr<CoreGraphics::ShaderVariable> globalLightPartitionSize;
	Ptr<CoreGraphics::ShaderVariable> globalLightShadowMap;

	/// local light variables
	Ptr<CoreGraphics::ShaderVariable> lightProjMapVar;
	Ptr<CoreGraphics::ShaderVariable> lightProjCubeVar;
	Ptr<CoreGraphics::ShaderVariable> shadowProjMapVar;     
    Ptr<CoreGraphics::ShaderVariable> shadowProjCubeVar;

	/// light variables
	Ptr<CoreGraphics::ShaderVariable> lightPosRange;
	Ptr<CoreGraphics::ShaderVariable> lightColor;
	Ptr<CoreGraphics::ShaderVariable> lightProjTransform;
	Ptr<CoreGraphics::ShaderVariable> lightTransform;
	Ptr<CoreGraphics::ConstantBuffer> localLightBuffer;
	Ptr<CoreGraphics::ShaderVariable> localLightBlockVar;
	Util::Dictionary<Ptr<Graphics::AbstractLightEntity>, IndexT> lightToInstanceMap;
	Util::Dictionary<Ptr<Graphics::AbstractLightEntity>, Util::Array<uint32_t>> lightToOffsetMap;

	/// shadow variables
	Ptr<CoreGraphics::ShaderVariable> shadowIntensityVar;
	Ptr<CoreGraphics::ShaderVariable> shadowProjTransform;
	Ptr<CoreGraphics::ShaderVariable> shadowOffsetScaleVar;

	Ptr<Vulkan::VkShaderState::VkDerivativeState> derivativeState;
	Util::FixedPool<Util::Array<uint32_t>> offsetPool;
	uint32_t offsetIndex;

	Ptr<Resources::ManagedMesh> pointLightMesh;         // point light mesh
	Ptr<Resources::ManagedMesh> spotLightMesh;          // spot light mesh
	Ptr<Resources::ManagedMesh> lightProbeMesh;
	bool renderBuffersAssigned;     

}; 
} // namespace Lighting
//------------------------------------------------------------------------------