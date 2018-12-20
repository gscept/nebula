#pragma once
//------------------------------------------------------------------------------
/**
    @class Lighting::VkShadowServer
    
    Vulkan shadow server.

	Implements Moment shadow maps for global lights, http://cg.cs.uni-bonn.de/en/publications/paper-details/peters-2015-msm/
	Variance shadow maps for local lights (spot, point), http://developer.download.nvidia.com/SDK/10/direct3d/Source/VarianceShadowMapping/Doc/VarianceShadowMapping.pdf
    
    (C) 2012-2018 Individual contributors, see AUTHORS file
*/
#include "lighting/base/shadowserverbase.h"
#include "frame2/frameserver.h"
#include "frame2/framesubpassbatch.h"
#include "lighting/csmutil.h"
#include "debug/debugtimer.h"
#include "util/fixedpool.h"

//------------------------------------------------------------------------------
namespace Lighting
{
class VkShadowServer : public ShadowServerBase
{
	__DeclareClass(VkShadowServer);
public:
	/// constructor
	VkShadowServer();
	/// destructor
	virtual ~VkShadowServer();

	/// open the shadow server
	void Open();
	/// close the shadow server
	void Close();

	/// attach a visible shadow casting light source
	void AttachVisibleLight(const Ptr<Graphics::AbstractLightEntity>& lightEntity);
	/// end lighting frame
	void EndFrame();

	/// update shadow buffer
	void UpdateShadowBuffers();

	/// get pointer to shadow buffer for local lights
	const Ptr<CoreGraphics::Texture>& GetSpotLightShadowBufferTexture() const;
	/// get pointer to PSSM shadow buffer for global lights
	const Ptr<CoreGraphics::Texture>& GetGlobalLightShadowBufferTexture() const;
	/// get array of PSSM split distances
	const float* GetSplitDistances() const;
	/// get array of PSSM LightProjTransforms
	const Math::matrix44* GetSplitTransforms() const;  
	/// gets CSM shadow view
	const Math::matrix44* GetShadowView() const;

	/// update spot light shadow buffers
	void UpdateSpotLightShadowBuffers();
	/// update point light shadow buffers
	void UpdatePointLightShadowBuffers();
	/// update global light shadow buffers
	void UpdateGlobalLightShadowBuffers();
private:

	/// sort local lights by priority
	virtual void SortLights();

	// we need one shader state per shadow casting light
	static const SizeT NumShadowCastingLights = MaxNumShadowSpotLights + MaxNumShadowPointLights + 1;
	Ptr<CoreGraphics::ShaderState> shaderStates[NumShadowCastingLights];
	Ptr<CoreGraphics::ShaderVariable> viewArrayVar[NumShadowCastingLights];

	Ptr<CoreGraphics::Texture> globalLightShadowBuffer;
	Ptr<CoreGraphics::Texture> spotLightShadowBuffer;
	Ptr<CoreGraphics::Texture> spotLightShadowBufferAtlas;
	Ptr<Frame::FrameSubpassBatch> globalLightBatch;
	Ptr<Frame::FrameSubpassBatch> spotLightBatch;
	Ptr<Frame::FrameSubpassBatch> pointLightBatch;

	Util::Dictionary<Ptr<Graphics::AbstractLightEntity>, IndexT> lightToIndexMap;
	Util::FixedPool<IndexT> lightIndexPool;
	

	Ptr<Frame::FrameScript> script;
	CSMUtil csmUtil;

	_declare_timer(globalShadow);
	_declare_timer(pointLightShadow);
	_declare_timer(spotLightShadow);
}; 

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::Texture>&
VkShadowServer::GetSpotLightShadowBufferTexture() const
{
	return this->spotLightShadowBufferAtlas;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::Texture>& 
VkShadowServer::GetGlobalLightShadowBufferTexture() const
{
	return this->globalLightShadowBuffer;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::matrix44*
VkShadowServer::GetShadowView() const
{
	return &this->csmUtil.GetShadowView();
}

//------------------------------------------------------------------------------
/**
    Get raw pointer to array of PSSM split distances.
*/
inline const float*
VkShadowServer::GetSplitDistances() const
{
    return this->csmUtil.GetCascadeDistances();
}

//------------------------------------------------------------------------------
/**
    Get raw pointer to array of PSSM split LightProjTransform matrices.
*/
inline const Math::matrix44*
VkShadowServer::GetSplitTransforms() const
{
    return this->csmUtil.GetCascadeTransforms();
}

} // namespace Lighting
//------------------------------------------------------------------------------
