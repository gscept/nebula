//------------------------------------------------------------------------------
//  vklightserver.cc
//  (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "lighting/vk/vklightserver.h"
#include "resources/resourcemanager.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/shadersemantics.h"
#include "coregraphics/transformdevice.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/texture.h"
#include "lighting/shadowserver.h"
#include "coregraphics/transformdevice.h"
#include "vkshadowserver.h"
#include "math/float4.h"
#include "graphics/graphicsserver.h"
#include "graphics/view.h"
#include "coregraphics/constantbuffer.h"
#include "coregraphics/config.h"

namespace Lighting
{
__ImplementClass(Lighting::VkLightServer, 'VKLS', Lighting::LightServerBase);

using namespace Vulkan;
using namespace Graphics;
using namespace CoreGraphics;
using namespace Resources;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
VkLightServer::VkLightServer() :
	renderBuffersAssigned(false)
{
	globalLightFeatureBits[NoShadows] = 0;
	globalLightFeatureBits[CastShadows] = 0;
	pointLightFeatureBits[NoShadows] = 0;
	pointLightFeatureBits[CastShadows] = 0;
	spotLightFeatureBits[NoShadows] = 0;
	spotLightFeatureBits[CastShadows] = 0;
}

//------------------------------------------------------------------------------
/**
*/
VkLightServer::~VkLightServer()
{
	n_assert(!this->IsOpen());
}

//------------------------------------------------------------------------------
/**
*/
bool 
VkLightServer::NeedsLightModelLinking() const
{
	return true;
}

//------------------------------------------------------------------------------
/**
*/
void
VkLightServer::Open()
{
	n_assert(!this->IsOpen());
	ResourceManager* resManager = ResourceManager::Instance();
	ShaderServer* shdServer = ShaderServer::Instance();

	// call parent class
	LightServerBase::Open();

	// load the light source shapes
	this->pointLightMesh = resManager->CreateManagedResource(Mesh::RTTI, ResourceId("msh:system/pointlightshape.nvx2")).downcast<ManagedMesh>();
	this->spotLightMesh = resManager->CreateManagedResource(Mesh::RTTI, ResourceId("msh:system/spotlightshape.nvx2")).downcast<ManagedMesh>();
	this->lightProbeMesh = resManager->CreateManagedResource(Mesh::RTTI, ResourceId("msh:system/box.nvx2")).downcast<ManagedMesh>();

	Util::String lightTexPath("tex:system/white");
	lightTexPath.Append(NEBULA3_TEXTURE_EXTENSION);

	// setup the shared light project map resource
	this->lightProjMap = ResourceManager::Instance()->CreateManagedResource(Texture::RTTI, ResourceId(lightTexPath)).downcast<ManagedTexture>();

	// light group is for shared variables, default is for shader local variables
	this->globalLightShader						= shdServer->CreateSharedShaderState("shd:lights", { NEBULAT_TICK_GROUP });
    this->globalLightShader->SetApplyShared(true);
	this->localLightShader						= shdServer->CreateShaderState("shd:lights", { NEBULAT_SYSTEM_GROUP });
	this->lightProbeShader						= shdServer->CreateShaderState("shd:reflectionprojector", { NEBULAT_DEFAULT_GROUP });

	this->globalLightFeatureBits[NoShadows]		= shdServer->FeatureStringToMask("Global");
	this->globalLightFeatureBits[CastShadows]	= shdServer->FeatureStringToMask("Global|Alt0");
	this->pointLightFeatureBits[NoShadows]		= shdServer->FeatureStringToMask("Point");    
	this->pointLightFeatureBits[CastShadows]	= shdServer->FeatureStringToMask("Point|Alt0");    
	this->spotLightFeatureBits[NoShadows]		= shdServer->FeatureStringToMask("Spot"); 
	this->spotLightFeatureBits[CastShadows]		= shdServer->FeatureStringToMask("Spot|Alt0");

	// todo: rename variations in shader...
	this->lightProbeFeatureBits[LightProbeEntity::Box] = shdServer->FeatureStringToMask("Alt0");
	this->lightProbeFeatureBits[LightProbeEntity::Sphere] = shdServer->FeatureStringToMask("Alt1");
	this->lightProbeFeatureBits[LightProbeEntity::Box + 2] = shdServer->FeatureStringToMask("Alt0|Alt2");
	this->lightProbeFeatureBits[LightProbeEntity::Sphere + 2] = shdServer->FeatureStringToMask("Alt1|Alt2");

	// global light variables used for shadowing
	this->globalLightCascadeOffset				= this->globalLightShader->GetVariableByName(NEBULA3_SEMANTIC_CASCADEOFFSET);
	this->globalLightCascadeScale				= this->globalLightShader->GetVariableByName(NEBULA3_SEMANTIC_CASCADESCALE);
	this->globalLightMinBorderPadding			= this->globalLightShader->GetVariableByName(NEBULA3_SEMANTIC_MINBORDERPADDING);
	this->globalLightMaxBorderPadding			= this->globalLightShader->GetVariableByName(NEBULA3_SEMANTIC_MAXBORDERPADDING);
	this->globalLightPartitionSize				= this->globalLightShader->GetVariableByName(NEBULA3_SEMANTIC_SHADOWPARTITIONSIZE);

    // setup block for global light, this will only be updated once per iteration and is shared across all shaders
	this->globalLightDirWorldspace				= this->globalLightShader->GetVariableByName("GlobalLightDirWorldspace");
    this->globalLightDir                        = this->globalLightShader->GetVariableByName(NEBULA3_SEMANTIC_GLOBALLIGHTDIR);
    this->globalLightColor                      = this->globalLightShader->GetVariableByName(NEBULA3_SEMANTIC_GLOBALLIGHTCOLOR);
    this->globalBackLightColor                  = this->globalLightShader->GetVariableByName(NEBULA3_SEMANTIC_GLOBALBACKLIGHTCOLOR);
    this->globalAmbientLightColor               = this->globalLightShader->GetVariableByName(NEBULA3_SEMANTIC_GLOBALAMBIENTLIGHTCOLOR);
    this->globalBackLightOffset                 = this->globalLightShader->GetVariableByName(NEBULA3_SEMANTIC_GLOBALBACKLIGHTOFFSET);
    this->globalLightShadowMatrixVar            = this->globalLightShader->GetVariableByName(NEBULA3_SEMANTIC_CSMSHADOWMATRIX);
	this->globalLightShadowMap					= this->globalLightShader->GetVariableByName("GlobalLightShadowBuffer");

	// create buffer for local lights
	this->localLightBuffer						= ConstantBuffer::Create();
	this->localLightBuffer->SetupFromBlockInShader(this->localLightShader, "LocalLightBlock", 1);

	// get local light variables from buffer
	this->lightPosRange							= this->localLightBuffer->GetVariableByName(NEBULA3_SEMANTIC_LIGHTPOSRANGE);
	this->lightColor							= this->localLightBuffer->GetVariableByName(NEBULA3_SEMANTIC_LIGHTCOLOR);
	this->lightProjTransform					= this->localLightBuffer->GetVariableByName(NEBULA3_SEMANTIC_LIGHTPROJTRANSFORM);
	this->lightTransform						= this->localLightBuffer->GetVariableByName(NEBULA3_SEMANTIC_LIGHTTRANSFORM);
	this->lightProjMapVar						= this->localLightBuffer->GetVariableByName("SpotLightProjectionTexture");
	this->shadowProjMapVar						= this->localLightBuffer->GetVariableByName("SpotLightShadowAtlas");
	this->lightProjCubeVar						= this->localLightBuffer->GetVariableByName("PointLightProjectionTexture");	
	this->shadowProjCubeVar						= this->localLightBuffer->GetVariableByName("PointLightShadowCube");
	this->shadowProjTransform					= this->localLightBuffer->GetVariableByName(NEBULA3_SEMANTIC_SHADOWPROJTRANSFORM);
	this->shadowOffsetScaleVar					= this->localLightBuffer->GetVariableByName(NEBULA3_SEMANTIC_SHADOWOFFSETSCALE);
	this->shadowIntensityVar					= this->localLightBuffer->GetVariableByName(NEBULA3_SEMANTIC_SHADOWINTENSITY);

	// setup default projection textures
	this->lightProjMapVar->SetTexture(this->lightProjMap->GetTexture());
	//this->lightProjCubeVar->SetTexture()

	// bind our light buffer to the binding slot
	this->localLightBlockVar = this->localLightShader->GetVariableByName("LocalLightBlock");
	this->localLightBlockVar->SetConstantBuffer(this->localLightBuffer);

	// setup derivative state to let us provide offsets for the lights
	VkShaderState::BufferMapping mapping = this->localLightShader->GetBufferMapping(NEBULAT_SYSTEM_GROUP, this->localLightBuffer->GetBinding());
	this->offsetIndex = mapping.offset;
	this->derivativeState = this->localLightShader->CreateDerivative(NEBULAT_SYSTEM_GROUP);
	this->derivativeState->bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	this->derivativeState->bindShared = false;
	this->derivativeState->buffers[mapping.offset] = this->localLightBuffer;

	this->offsetPool.Resize(1024);
	IndexT i;
	for (i = 0; i < this->offsetPool.Size(); i++)
	{
		this->localLightShader->CreateOffsetArray(this->offsetPool.Alloc(), NEBULAT_SYSTEM_GROUP);
	}
	this->offsetPool.Reset();
	//this->shadowConstants->SetFloat4(float4(100.0f, 100.0f, 0.003f, 1024.0f));
}

//------------------------------------------------------------------------------
/**
*/
void
VkLightServer::Close()
{
	n_assert(this->IsOpen());
	n_assert(this->pointLights[NoShadows].IsEmpty());   
	n_assert(this->pointLights[CastShadows].IsEmpty());
	n_assert(this->spotLights[NoShadows].IsEmpty());
	n_assert(this->spotLights[CastShadows].IsEmpty());

	if (this->renderBuffersAssigned)
	{
		// discard fullscreen quad renderer
		this->fullScreenQuadRenderer.Discard();
	}	

	// discard shader stuff
	this->globalLightShader->Discard();
	this->globalLightShader = 0;
	this->localLightShader->Discard();
	this->localLightShader = 0;

	this->lightProbeShader->Discard();
	this->lightProbeShader = 0;

	this->lightPosRange = 0;
	this->lightColor = 0;
	this->lightProjTransform = 0;
	this->lightProjMapVar = 0;

	this->shadowIntensityVar = 0;
	this->shadowProjTransform = 0;
	this->shadowOffsetScaleVar = 0;
	this->shadowProjMapVar = 0;

	this->globalAmbientLightColor = 0;
	this->globalBackLightColor = 0;
	this->globalBackLightOffset = 0;
	this->globalLightColor = 0;
	this->globalLightDir = 0;
	this->globalLightColor = 0;
	this->globalLightCascadeOffset = 0;
	this->globalLightCascadeScale = 0;				
	this->globalLightMinBorderPadding = 0;
	this->globalLightMaxBorderPadding = 0;		
	this->globalLightPartitionSize = 0;

	// discard resources
	ResourceManager* resManager = ResourceManager::Instance();
	resManager->DiscardManagedResource(this->pointLightMesh.upcast<ManagedResource>());
	this->pointLightMesh = 0;
	resManager->DiscardManagedResource(this->spotLightMesh.upcast<ManagedResource>());
	this->spotLightMesh = 0;
	resManager->DiscardManagedResource(this->lightProbeMesh.upcast<ManagedResource>());
	this->lightProbeMesh = 0;

	LightServerBase::Close();
}

//------------------------------------------------------------------------------
/**
*/
void
VkLightServer::AttachVisibleLight(const Ptr<AbstractLightEntity>& lightEntity)
{
	if (lightEntity->GetLightType() == LightType::Point)
	{
		// TODO: allow shadow casting spotlights, define shadow casting transform?
		n_assert(lightEntity->IsInstanceOf(PointLightEntity::RTTI));
		if (lightEntity->GetCastShadowsThisFrame())
		{
			this->pointLights[CastShadows].Append(lightEntity.cast<PointLightEntity>());
		}
		else
		{
			this->pointLights[NoShadows].Append(lightEntity.cast<PointLightEntity>());
		}
	}
	else if (lightEntity->GetLightType() == LightType::Spot)
	{
		// TODO: sort out the three most important shadow casting lights
		n_assert(lightEntity->IsInstanceOf(SpotLightEntity::RTTI));
		if (lightEntity->GetCastShadowsThisFrame())
		{
			this->spotLights[CastShadows].Append(lightEntity.cast<SpotLightEntity>());
		}
		else
		{
			this->spotLights[NoShadows].Append(lightEntity.cast<SpotLightEntity>());
		}
	}

	// allocate buffer (or index if buffer is big enough) for the light
	this->lightToInstanceMap.Add(lightEntity, this->localLightBuffer->AllocateInstance());
	//this->localLightBlockVar->SetConstantBuffer(this->localLightBuffer);
	this->lightToOffsetMap.Add(lightEntity, this->offsetPool.Alloc());
	this->localLightBlockVar->SetConstantBuffer(this->localLightBuffer);

	LightServerBase::AttachVisibleLight(lightEntity);
}

//------------------------------------------------------------------------------
/**
*/
void 
VkLightServer::EndFrame()
{
	// reset light slots so we can reuse them the next frame
	this->localLightBuffer->Reset();
	this->offsetPool.Reset();
	this->lightToInstanceMap.Clear();
	this->lightToOffsetMap.Clear();

	this->pointLights[NoShadows].Clear();
	this->pointLights[CastShadows].Clear();
	this->spotLights[NoShadows].Clear();
	this->spotLights[CastShadows].Clear();
	LightServerBase::EndFrame();
}

//------------------------------------------------------------------------------
/**
*/
void 
VkLightServer::AssignRenderBufferTextures()
{
	n_assert(!this->renderBuffersAssigned);
	const Ptr<ResourceManager>& resManager = ResourceManager::Instance();
	this->renderBuffersAssigned = true;

	ResourceId lightBufferId("LightBuffer");
	if (!resManager->HasResource(lightBufferId))
	{
		n_error("VkLightServer::RenderLights(): LightBuffer render target not found!\n");
	}
	Ptr<Texture> lightBuffer = resManager->LookupResource(lightBufferId).downcast<Texture>();

	// also setup the fullscreen quad renderer here
	this->fullScreenQuadRenderer.Setup(lightBuffer->GetWidth(), lightBuffer->GetHeight());       
}

//------------------------------------------------------------------------------
/**
*/
void
VkLightServer::UpdateDescriptor(const VkBuffer& buffer, const IndexT binding, VkDescriptorSet set)
{
	VkWriteDescriptorSet write;
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.pNext = NULL;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	write.dstBinding = binding;
	write.dstArrayElement = 0;
	write.dstSet = set;

	VkDescriptorBufferInfo buf;
	buf.buffer = buffer;
	buf.offset = 0;
	buf.range = VK_WHOLE_SIZE;
	write.pBufferInfo = &buf;
	write.pImageInfo = NULL;
	write.pTexelBufferView = NULL;

	vkUpdateDescriptorSets(VkRenderDevice::dev, 1, &write, 0, NULL);
}

//------------------------------------------------------------------------------
/**
*/
void 
VkLightServer::RenderLights()
{
	ShaderServer* shdServer = ShaderServer::Instance();
    ShadowServer* shadowServer = ShadowServer::Instance();

	// make sure to update the descriptor
	//this->UpdateDescriptor(this->localLightBuffer->GetVkBuffer(), this->localLightBlockVar->binding, this->localLightSet);

	// if not happened yet, set the NormalDepthBuffer and LightBuffer
	// as shader variables
	if (!this->renderBuffersAssigned)
	{
		this->AssignRenderBufferTextures();
	}

	// general preparations
	shdServer->SetActiveShader(this->globalLightShader->GetShader());

	// render the global light
	this->globalLightShadowMap->SetTexture(shadowServer->GetGlobalLightShadowBufferTexture());
	this->derivativeState->Reset();
	this->globalLightShader->SetApplyShared(true);
	this->RenderGlobalLight();

	if (this->spotLights[CastShadows].Size() > 0)
	{      
		// now set shadow buffer for local lights    
		this->shadowProjMapVar->SetTexture(shadowServer->GetSpotLightShadowBufferTexture());
	}	

	// render spot lights
	this->localLightShader->SetApplyShared(false);
	this->RenderSpotLights();
	
	// render point lights
	this->RenderPointLights();
}

//------------------------------------------------------------------------------
/**
*/
void 
VkLightServer::RenderGlobalLight()
{
	if (this->globalLightEntity.isvalid())
	{
		TransformDevice* transDev = TransformDevice::Instance();
		const matrix44& view = transDev->GetViewTransform();
		float4 worldSpaceLightDir = this->globalLightEntity->GetLightDirection();
		float4 viewSpaceLightDir = matrix44::transform(worldSpaceLightDir, view);

		// normalize was done in fragment shader, better here for gfx card's sake
		viewSpaceLightDir = float4::normalize(viewSpaceLightDir);

        // begin pass
        if (this->globalLightEntity->GetCastShadows()) this->globalLightShader->SelectActiveVariation(this->globalLightFeatureBits[CastShadows]);
        else                                           this->globalLightShader->SelectActiveVariation(this->globalLightFeatureBits[NoShadows]);

        // start pass
        this->globalLightShader->Apply();
		
		// setup general global light stuff
		this->globalLightDir->SetFloat4(viewSpaceLightDir);
		this->globalLightDirWorldspace->SetFloat4(worldSpaceLightDir);
		this->globalLightColor->SetFloat4(this->globalLightEntity->GetColor());
		this->globalBackLightColor->SetFloat4(this->globalLightEntity->GetBackLightColor());
		this->globalAmbientLightColor->SetFloat4(this->globalLightEntity->GetAmbientLightColor());
		this->globalBackLightOffset->SetFloat(this->globalLightEntity->GetBackLightOffset());

		matrix44 shadowView = *ShadowServer::Instance()->GetShadowView();
        shadowView = matrix44::multiply(transDev->GetInvViewTransform(), shadowView);
        this->globalLightShadowMatrixVar->SetMatrix(shadowView);

		// handle casting shadows using CSM
		if (this->globalLightEntity->GetCastShadows())
		{
			Ptr<CoreGraphics::Texture> CSMTexture = ShadowServer::Instance()->GetGlobalLightShadowBufferTexture();
			float CSMBufferWidth = (CSMTexture->GetWidth() / (float)ShadowServerBase::SplitsPerRow);
#if __DX11__
			matrix44 textureScale = matrix44::scaling(0.5f, -0.5f, 1.0f);
#elif __VULKAN__
			matrix44 textureScale = matrix44::scaling(0.5f, -1.0f, 1.0f);
#elif __OGL4__
			matrix44 textureScale = matrix44::scaling(0.5f, 0.5f, 1.0f);
#endif
			matrix44 textureTranslation = matrix44::translation(0.5f, 0.5f, 0);
			const float* CSMDistances = ShadowServer::Instance()->GetSplitDistances();
			const matrix44* CSMTransforms = ShadowServer::Instance()->GetSplitTransforms();
			float4 cascadeScales[CSMUtil::NumCascades];
			float4 cascadeOffsets[CSMUtil::NumCascades];
			for (int splitIndex = 0; splitIndex < CSMUtil::NumCascades; ++splitIndex)
			{
				matrix44 shadowTexture = matrix44::multiply(
					CSMTransforms[splitIndex],
					matrix44::multiply(textureScale, textureTranslation));
				float4 scale;
				scale.x() = shadowTexture.getrow0().x();
				scale.y() = shadowTexture.getrow1().y();
				scale.z() = shadowTexture.getrow2().z();
				scale.w() = 1;
				float4 offset = shadowTexture.getrow3();
				offset.w() = 0;
				cascadeOffsets[splitIndex] = offset;
				cascadeScales[splitIndex] = scale;
			}
			this->globalLightCascadeOffset->SetFloat4Array(cascadeOffsets, CSMUtil::NumCascades);
			this->globalLightCascadeScale->SetFloat4Array(cascadeScales, CSMUtil::NumCascades);
			this->globalLightMinBorderPadding->SetFloat(1.0f / float(CSMBufferWidth));
			this->globalLightMaxBorderPadding->SetFloat((CSMBufferWidth - 1.0f) / float(CSMBufferWidth));
			this->globalLightPartitionSize->SetFloat(1 / float(ShadowServerBase::SplitsPerRow));

			this->shadowIntensityVar->SetFloat(this->globalLightEntity->GetShadowIntensity());
		}
		
		// commit changes and draw
		this->fullScreenQuadRenderer.ApplyMesh();
		this->globalLightShader->Commit();
		this->fullScreenQuadRenderer.Draw();
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
VkLightServer::RenderPointLights()
{
	if ((this->pointLightMesh->GetState() == Resource::Loaded)
		&& (this->pointLights[CastShadows].Size() > 0
		|| this->pointLights[NoShadows].Size() > 0))
	{
		TransformDevice* tformDevice = TransformDevice::Instance();
		RenderDevice* renderDevice = RenderDevice::Instance();
		ShaderServer* shaderServer = ShaderServer::Instance();

		IndexT shadowIdx;
		for (shadowIdx = 0; shadowIdx < NumShadowFlags; ++shadowIdx)
		{
			if (this->pointLights[shadowIdx].Size())
			{
				this->localLightShader->SelectActiveVariation(this->pointLightFeatureBits[shadowIdx]);
                this->localLightShader->Apply();

				// apply mesh
				this->pointLightMesh->GetMesh()->ApplyPrimitives(0);

				IndexT i;
				for (i = 0; i < this->pointLights[shadowIdx].Size(); i++)
				{
					const Ptr<PointLightEntity>& curLight = this->pointLights[shadowIdx][i];

					// select active index in local light buffer, this will make this lights instance active this frame
					SizeT offset = this->lightToInstanceMap[curLight.upcast<AbstractLightEntity>()];
					Util::Array<uint32_t>& offsets = this->lightToOffsetMap[curLight.upcast<AbstractLightEntity>()];
					offsets[this->offsetIndex] = offset;
					this->derivativeState->offsets = offsets.Begin();
					this->derivativeState->offsetCount = offsets.Size();
					this->derivativeState->Apply();
					//this->localLightBuffer->SetBaseOffset(offset);

					const matrix44& lightTransform = curLight->GetTransform();
					tformDevice->SetModelTransform(lightTransform);

					// light position in view space, and set .w to inverted light range
					const matrix44& viewTransform = tformDevice->GetViewTransform();
					float4 posAndRange = matrix44::transform(lightTransform.get_position(), viewTransform);
					posAndRange.w() = 1.0f / lightTransform.get_zaxis().length();

					// set projection map
					if (curLight->GetProjectionTexture().isvalid())
					{
						this->lightProjCubeVar->SetTexture(curLight->GetProjectionTexture()->GetTexture());
					}

					// set light parameters
					this->lightPosRange->SetFloat4(posAndRange);
					this->lightColor->SetFloat4(curLight->GetColor());
					this->lightTransform->SetMatrix(lightTransform);

					if (CastShadows == (ShadowFlag)shadowIdx && curLight->GetCastShadowsThisFrame())
					{   
                        // set shadow cube if valid
                        if (curLight->GetShadowCube().isvalid())
                        {
                            this->shadowProjCubeVar->SetTexture(curLight->GetShadowCube()->GetResolveTexture());
                        }

						// set shadow intensity
						this->shadowIntensityVar->SetFloat(curLight->GetShadowIntensity());
					}

					// commit and draw

					//renderDevice->BindDescriptorsGraphics(&this->localLightSet, this->localLightLayout, NEBULAT_DEFAULT_GROUP, 1, offsets.Begin(), offsets.Size(), false);
					this->derivativeState->Commit();
					renderDevice->Draw();
				}
			}
		}                             
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
VkLightServer::RenderSpotLights()
{
	if ((this->spotLightMesh->GetState() == Resource::Loaded)
		&& (this->spotLights[CastShadows].Size() > 0
		|| this->spotLights[NoShadows].Size() > 0))
	{
		TransformDevice* tformDevice = TransformDevice::Instance();
		RenderDevice* renderDevice = RenderDevice::Instance();

		IndexT shadowIdx;
		for (shadowIdx = 0; shadowIdx < NumShadowFlags; ++shadowIdx)
		{
			if (this->spotLights[shadowIdx].Size())
			{
				this->localLightShader->SelectActiveVariation(this->spotLightFeatureBits[shadowIdx]);
                this->localLightShader->Apply();

				// apply mesh
				this->spotLightMesh->GetMesh()->ApplyPrimitives(0);

				IndexT i;
				for (i = 0; i < this->spotLights[shadowIdx].Size(); i++)
				{            
					const Ptr<SpotLightEntity>& curLight = this->spotLights[shadowIdx][i];

					// select active index in local light buffer, this will make this lights instance active this frame
					SizeT offset = this->lightToInstanceMap[curLight.upcast<AbstractLightEntity>()];
					Util::Array<uint32_t>& offsets = this->lightToOffsetMap[curLight.upcast<AbstractLightEntity>()];
					offsets[this->offsetIndex] = offset;
					this->derivativeState->offsets = offsets.Begin();
					this->derivativeState->offsetCount = offsets.Size();
					this->derivativeState->Apply();

					const matrix44& lightTransform = curLight->GetTransform();
					tformDevice->SetModelTransform(lightTransform);

					// light position in view space, and set .w to inverted light range
					const matrix44& viewTransform = tformDevice->GetViewTransform();
					float4 posAndRange = matrix44::transform(lightTransform.get_position(), viewTransform);
					posAndRange.w() = 1.0f / lightTransform.get_zaxis().length();

					// set projection map
					if (curLight->GetProjectionTexture().isvalid())
					{
						this->lightProjMapVar->SetTexture(curLight->GetProjectionTexture()->GetTexture());
					}					
					else
					{
						this->lightProjMapVar->SetTexture(this->lightProjMap->GetTexture());
					}

					this->lightPosRange->SetFloat4(posAndRange);
					this->lightColor->SetFloat4(curLight->GetColor());
					this->lightTransform->SetMatrix(lightTransform);
					//this->lightShadowBias->SetFloat(curLight->GetShadowBias());

					// needed for tex coordinates to lookup correct spotlight lightmap texel
					const matrix44& invViewTransform = tformDevice->GetInvViewTransform();
					matrix44 fromViewToLightProj = matrix44::multiply(invViewTransform, curLight->GetInvLightProjTransform());            
					this->lightProjTransform->SetMatrix(fromViewToLightProj);

					if (CastShadows == (ShadowFlag)shadowIdx && curLight->GetCastShadowsThisFrame())
					{                
						// needed for tex coordinates to lookup correct spotlight shadowmap texel
						matrix44 fromViewToShadowLightProj = matrix44::multiply(invViewTransform, curLight->GetShadowInvLightProjTransform());            
						this->shadowProjTransform->SetMatrix(fromViewToShadowLightProj);

						// set shadow map offset index for multiple shadows
						const float4& shadowOffsetScale = curLight->GetShadowBufferUvOffsetAndScale();
						this->shadowOffsetScaleVar->SetFloat4(shadowOffsetScale);

						// set shadow intensity
						this->shadowIntensityVar->SetFloat(curLight->GetShadowIntensity());
						this->shadowProjMapVar->SetTexture(ShadowServer::Instance()->GetSpotLightShadowBufferTexture());
					}

					// commit and draw
					//this->lightShader->SetConstantBufferOffset(NEBULAT_DEFAULT_GROUP, this->localLightBuffer->GetBinding(), offset);
					//Util::Array<uint32_t>& offsets = this->lightToOffsetMap[curLight.upcast<AbstractLightEntity>()];
					//offsets[this->offsetIndex] = offset;
					//renderDevice->BindDescriptorsGraphics(&this->localLightSet, this->localLightLayout, NEBULAT_DEFAULT_GROUP, 1, offsets.Begin(), offsets.Size(), false);
					this->derivativeState->Commit();
					renderDevice->Draw();
				}
			}
		}                     
	}
}

//------------------------------------------------------------------------------
/**
*/
bool
SortProbesLayer(const Ptr<LightProbeEntity>& lhs, const Ptr<LightProbeEntity>& rhs)
{
	int lhsPrio = lhs->GetLayer();
	int rhsPrio = rhs->GetLayer();
	return lhsPrio < rhsPrio;
}

//------------------------------------------------------------------------------
/**
	Probes are sorted by:
		Box					0
		Sphere				1
		Box + Parallax		2
		Sphere + Parallax	3
*/
bool
SortProbesType(const Ptr<LightProbeEntity>& lhs, const Ptr<LightProbeEntity>& rhs)
{
	int lhsPrio = lhs->GetShapeType() + lhs->GetCorrectionMode() * 2;
	int rhsPrio = rhs->GetShapeType() + rhs->GetCorrectionMode() * 2;
	return lhsPrio < rhsPrio;
}

//------------------------------------------------------------------------------
/**
	Hmm, this should be more efficient if we can render probes based on shaders
*/
void
VkLightServer::RenderLightProbes()
{
	// sort light probes based on layers
	this->visibleLightProbes.SortWithFunc(SortProbesType);

	// get transform and render device
	TransformDevice* transformDevice = TransformDevice::Instance();
	RenderDevice* renderDevice = RenderDevice::Instance();

	// apply mesh
	this->lightProbeMesh->GetMesh()->ApplyPrimitives(0);

	// traverse and render them!
	IndexT probeIdx;
	for (probeIdx = 0; probeIdx < this->visibleLightProbes.Size(); probeIdx++)
	{
		const Ptr<LightProbeEntity>& entity = this->visibleLightProbes[probeIdx];
		const Ptr<EnvironmentProbe>& probe = entity->GetEnvironmentProbe();
        const Ptr<CoreGraphics::ShaderState>& shader = entity->GetShaderState();

		// skip rendering invisible probes
		if (!entity->IsVisible()) continue;

		// get shape type as int
		int shapeType = entity->GetShapeType();
		 
		// 0 is for box, 1 is for sphere, but this would be better to use the same shape and render them based on shape instead...
		// 
		shader->SelectActiveVariation(this->lightProbeFeatureBits[shapeType + (entity->GetCorrectionMode() ? 2 : 0)]);

		// apply mesh at shape type
		entity->ApplyProbe(probe);

		// apply shader and draw
		shader->Apply();
        shader->Commit();
		renderDevice->Draw();
	}
}

} // namespace Lighting