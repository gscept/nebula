//------------------------------------------------------------------------------
//  lightprepassserver.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "lighting/lightprepass/lightprepassserver.h"
#include "resources/resourcemanager.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/shadersemantics.h"
#include "coregraphics/transformdevice.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/texture.h"
#include "coregraphics/config.h"
#include "lighting/shadowserver.h"

namespace Lighting
{
__ImplementClass(Lighting::LightPrePassServer, 'LPPS', Lighting::LightServerBase);

using namespace CoreGraphics;
using namespace Resources;
using namespace Math;
using namespace Graphics;

//------------------------------------------------------------------------------
/**
*/
LightPrePassServer::LightPrePassServer() :
    globalLightFeatureBits(0),
	alpha(false),
    renderBuffersAssigned(false)
{                                           
    pointLightFeatureBits[NoShadows] = 0;
    pointLightFeatureBits[CastShadows] = 0;
    spotLightFeatureBits[NoShadows] = 0;
    spotLightFeatureBits[CastShadows] = 0;
}

//------------------------------------------------------------------------------
/**
*/
LightPrePassServer::~LightPrePassServer()
{
    n_assert(!this->IsOpen());
}

//------------------------------------------------------------------------------
/**
    Indicate whether the light server requires visibility links between
    lights and models. Deferred lighting solutions usually don't need this.
    FIXME: EXCEPT for shadow map rendering!!
*/
bool
LightPrePassServer::NeedsLightModelLinking() const
{
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
LightPrePassServer::Open()
{
    n_assert(!this->IsOpen());
    ResourceManager* resManager = ResourceManager::Instance();
    ShaderServer* shdServer = ShaderServer::Instance();

    // call parent class
    LightServerBase::Open();

    // load the light source shapes
    this->pointLightMesh = resManager->CreateManagedResource(Mesh::RTTI, ResourceId("sysmsh:system/pointlightshape_s_0.nvx2")).downcast<ManagedMesh>();
    this->spotLightMesh = resManager->CreateManagedResource(Mesh::RTTI, ResourceId("sysmsh:system/spotlightshape_s_0.nvx2")).downcast<ManagedMesh>();

    Util::String lightTexPath("systex:lighting/lightcones");
    lightTexPath.Append(NEBULA_TEXTURE_EXTENSION);
    // setup the shared light project map resource
    this->lightProjMap = ResourceManager::Instance()->CreateManagedResource(Texture::RTTI, ResourceId(lightTexPath)).downcast<ManagedTexture>();


    // setup shader stuff
	this->lightShader						 = shdServer->CreateShaderState("shd:lightsources", { NEBULA_DEFAULT_GROUP });
	this->pointLightFeatureBits[NoShadows]   = shdServer->FeatureStringToMask("PointLight");    
	this->pointLightFeatureBits[CastShadows] = shdServer->FeatureStringToMask("PointLightShadows");    
	this->spotLightFeatureBits[NoShadows]    = shdServer->FeatureStringToMask("SpotLight"); 
	this->spotLightFeatureBits[CastShadows]  = shdServer->FeatureStringToMask("SpotLightShadows");
	this->globalLightFeatureBits             = shdServer->FeatureStringToMask("GlobalLight");
	this->lightPosRange           = this->lightShader->GetVariableByName(NEBULA_SEMANTIC_LIGHTPOSRANGE);
	this->lightColor              = this->lightShader->GetVariableByName(NEBULA_SEMANTIC_LIGHTCOLOR);
	this->globalLightDir          = this->lightShader->GetVariableByName(NEBULA_SEMANTIC_GLOBALLIGHTDIR);
	this->globalLightColor        = this->lightShader->GetVariableByName(NEBULA_SEMANTIC_GLOBALLIGHTCOLOR);
	this->globalBackLightColor    = this->lightShader->GetVariableByName(NEBULA_SEMANTIC_GLOBALBACKLIGHTCOLOR);
	this->globalAmbientLightColor = this->lightShader->GetVariableByName(NEBULA_SEMANTIC_GLOBALAMBIENTLIGHTCOLOR);
	this->globalBackLightOffset   = this->lightShader->GetVariableByName(NEBULA_SEMANTIC_GLOBALBACKLIGHTOFFSET);
	this->lightProjTransform      = this->lightShader->GetVariableByName(NEBULA_SEMANTIC_LIGHTPROJTRANSFORM);
	this->shadowProjTransform     = this->lightShader->GetVariableByName(NEBULA_SEMANTIC_SHADOWPROJTRANSFORM);
	this->shadowOffsetScaleVar    = this->lightShader->GetVariableByName(NEBULA_SEMANTIC_SHADOWOFFSETSCALE);
	this->shadowFadeVar           = this->lightShader->GetVariableByName(NEBULA_SEMANTIC_SHADOWINTENSITY);

    const Ptr<ShaderState>& sharedShader = shdServer->GetSharedShader();
	this->lightProjMapVar         = sharedShader->GetVariableByName(NEBULA_SEMANTIC_LIGHTPROJMAP); 
	this->normalBufferVar         = sharedShader->GetVariableByName(NEBULA_SEMANTIC_NORMALBUFFER);
	this->dsfObjectDepthBufferVar = sharedShader->GetVariableByName(NEBULA_SEMANTIC_DEPTHBUFFER);
	this->lightBufferVar          = sharedShader->GetVariableByName(NEBULA_SEMANTIC_LIGHTBUFFER);
	this->shadowProjMapVar        = sharedShader->GetVariableByName(NEBULA_SEMANTIC_SHADOWPROJMAP);
	this->shadowConstants         = sharedShader->GetVariableByName(NEBULA_SEMANTIC_SHADOWCONSTANTS);

	// shadowConstants default-values in shader didnt quite work for the ps3, so we commit it here once
	this->shadowConstants->SetFloat4(float4(100.0f, 100.0f, 0.003f, 512.0f));
}

//------------------------------------------------------------------------------
/**
*/
void
LightPrePassServer::Close()
{
    n_assert(this->IsOpen());
    n_assert(this->pointLights[NoShadows].IsEmpty());   
    n_assert(this->pointLights[CastShadows].IsEmpty());
    n_assert(this->spotLights[NoShadows].IsEmpty());
    n_assert(this->spotLights[CastShadows].IsEmpty());

    // discard fullscreen quad renderer
    this->fullScreenQuadRenderer.Discard();

    // discard shader stuff
    this->lightShader = 0;
    this->lightColor = 0;
    this->lightPosRange = 0;
    this->normalBufferVar = 0;
    this->dsfObjectDepthBufferVar = 0;
    this->lightBufferVar = 0;
    this->globalAmbientLightColor = 0;
    this->globalBackLightColor = 0;
    this->globalBackLightOffset = 0;
    this->globalLightColor = 0;
    this->globalLightDir = 0;
    this->lightProjTransform = 0;
    this->shadowProjTransform = 0;
    this->lightProjMapVar = 0;
    this->shadowProjMapVar = 0;
    this->shadowOffsetScaleVar = 0;
    this->shadowConstants = 0;

    // discard resources
    ResourceManager* resManager = ResourceManager::Instance();
    resManager->DiscardManagedResource(this->pointLightMesh.upcast<ManagedResource>());
    this->pointLightMesh = 0;
    resManager->DiscardManagedResource(this->spotLightMesh.upcast<ManagedResource>());
    this->spotLightMesh = 0;

    LightServerBase::Close();
}

//------------------------------------------------------------------------------
/**
*/
void
LightPrePassServer::AttachVisibleLight(const Ptr<AbstractLightEntity>& lightEntity)
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
    LightServerBase::AttachVisibleLight(lightEntity);
}

//------------------------------------------------------------------------------
/**
*/
void
LightPrePassServer::EndFrame()
{
    this->pointLights[NoShadows].Clear();
    this->pointLights[CastShadows].Clear();
    this->spotLights[NoShadows].Clear();
    this->spotLights[CastShadows].Clear();
    LightServerBase::EndFrame();
}

//------------------------------------------------------------------------------
/**
    This does a one-time init of setting the NormalDepthBuffer
    and LightBuffer as shader variables.
*/
void
LightPrePassServer::AssignRenderBufferTextures()
{
    n_assert(!this->renderBuffersAssigned);
    const Ptr<ResourceManager>& resManager = ResourceManager::Instance();
    this->renderBuffersAssigned = true;

    // lookup the resolve texture of the NormalDepthBuffer
    ResourceId nrmDepthBufferId("NormalBuffer");
    if (!resManager->HasResource(nrmDepthBufferId))
    {
        n_error("LightPrePassServer::RenderLights(): NormalBuffer render target not found!\n");
    }
    normalBufferTexture = resManager->LookupResource(nrmDepthBufferId).downcast<Texture>();
    this->normalBufferVar->SetTexture(normalBufferTexture);

    ResourceId dsfObjectDepthBufferId("DSFObjectDepthBuffer");
    if (!resManager->HasResource(dsfObjectDepthBufferId))
    {
        n_error("LightPrePassServer::RenderLights(): DSFObjectDepthBuffer render target not found!\n");
    }
    dsfObjectDepthBufferTexture = resManager->LookupResource(dsfObjectDepthBufferId).downcast<Texture>();
    this->dsfObjectDepthBufferVar->SetTexture(dsfObjectDepthBufferTexture);

    // ...and the same for the LightBuffer
    ResourceId lightBufferId("LightBuffer");
    if (!resManager->HasResource(lightBufferId))
    {
        n_error("LightPrePassServer::RenderLights(): LightBuffer render target not found!\n");
    }
    Ptr<Texture> lightBufferTexture = resManager->LookupResource(lightBufferId).downcast<Texture>();
    this->lightBufferVar->SetTexture(lightBufferTexture);
 
    // also setup the fullscreen quad renderer here
    this->fullScreenQuadRenderer.Setup(lightBufferTexture->GetWidth(), lightBufferTexture->GetHeight());                                              
}

//------------------------------------------------------------------------------
/**
*/
void
LightPrePassServer::RenderLights()
{
    ShaderServer* shdServer = ShaderServer::Instance();
    // if not happened yet, set the NormalDepthBuffer and LightBuffer
    // as shader variables
    if (!this->renderBuffersAssigned)
    {
        this->AssignRenderBufferTextures();
    }

    // general preparations
    shdServer->SetActiveShader(this->lightShader->GetShader());
    
    // render the global light
    this->RenderGlobalLight();
        
    if (this->pointLights[CastShadows].Size() > 0
        || this->spotLights[CastShadows].Size() > 0)
    {      
        // now set shadow buffer for local lights    
        ShadowServer* shadowServer = ShadowServer::Instance();
        
		this->shadowProjMapVar->SetTexture(shadowServer->GetSpotLightShadowBufferTexture());
    }

    // render point lights
    this->RenderPointLights();

    // render spot lights
    this->RenderSpotLights();
}

//------------------------------------------------------------------------------
/**
    Render the global light as a fullscreen quad.
*/
void
LightPrePassServer::RenderGlobalLight()
{
    if (this->globalLightEntity.isvalid())
    {
        const matrix44& view = TransformDevice::Instance()->GetViewTransform();
        float4 worldSpaceLightDir = this->globalLightEntity->GetLightDirection();
        float4 viewSpaceLightDir = matrix44::transform(worldSpaceLightDir, view);
        // normalize was done in fragment shader, better here for gfx card's sake
        viewSpaceLightDir = Math::float4::normalize(viewSpaceLightDir);

        this->lightShader->SelectActiveVariation(this->globalLightFeatureBits);
        this->lightShader->Apply();
        this->lightShader->BeginUpdateSync();
        this->globalLightDir->SetFloat4(viewSpaceLightDir);
        this->globalLightColor->SetFloat4(this->globalLightEntity->GetColor());
        this->globalBackLightColor->SetFloat4(this->globalLightEntity->GetBackLightColor());
        this->globalAmbientLightColor->SetFloat4(this->globalLightEntity->GetAmbientLightColor());
        this->globalBackLightOffset->SetFloat(this->globalLightEntity->GetBackLightOffset());
		this->lightShader->EndUpdateSync();
        this->lightShader->Commit();
        this->fullScreenQuadRenderer.Draw();
    }
}

//------------------------------------------------------------------------------
/**
    Render point lights into the LightBuffer.
*/
void
LightPrePassServer::RenderPointLights()
{
    if ((this->pointLightMesh->GetState() == Resource::Loaded)
        && (this->pointLights[CastShadows].Size() > 0
            || this->pointLights[NoShadows].Size() > 0))
    {
        TransformDevice* tformDevice = TransformDevice::Instance();
        RenderDevice* renderDevice = RenderDevice::Instance();

        IndexT shadowIdx;
        for (shadowIdx = 0; shadowIdx < NumShadowFlags; ++shadowIdx)
        {
            if(this->pointLights[shadowIdx].Size())
            {
                this->lightShader->SelectActiveVariation(this->pointLightFeatureBits[shadowIdx]);
                this->lightShader->Apply();
                this->pointLightMesh->GetMesh()->ApplyPrimitives(0);
                
                IndexT i;
                for (i = 0; i < this->pointLights[shadowIdx].Size(); i++)
                {            
                    const Ptr<PointLightEntity>& curLight = this->pointLights[shadowIdx][i];
                    const matrix44& lightTransform = curLight->GetTransform();
                    tformDevice->SetModelTransform(lightTransform);

                    // light position in view space, and set .w to inverted light range
                    const matrix44& viewTransform = tformDevice->GetViewTransform();
                    float4 posAndRange = matrix44::transform(lightTransform.get_position(), viewTransform);
                    posAndRange.w() = 1.0f / lightTransform.get_zaxis().length();

                    this->lightShader->BeginUpdateSync();
                    this->lightPosRange->SetFloat4(posAndRange);
                    this->lightColor->SetFloat4(curLight->GetColor());

                    if (CastShadows == (ShadowFlag)shadowIdx
                        && curLight->GetCastShadowsThisFrame())
                    {       

						const matrix44& invViewTransform = tformDevice->GetInvViewTransform();

                        // needed for tex coordinates to lookup correct shadowbuffer texel 
                        matrix44 fromViewToLightProj = matrix44::multiply(invViewTransform, curLight->GetShadowInvLightProjTransform());            
                        this->shadowProjTransform->SetMatrix(fromViewToLightProj);

                        // set shadowmap offset index for multiple shadows
                        const float4& shadowOffsetScale = curLight->GetShadowBufferUvOffsetAndScale();
                        this->shadowOffsetScaleVar->SetFloat4(shadowOffsetScale);

                        // set shadow intensity
                        this->shadowFadeVar->SetFloat(curLight->GetShadowIntensity());
                    }   

                    // update shader variables
                    tformDevice->ApplyModelTransforms(this->lightShader);
					this->lightShader->EndUpdateSync();

                    // commit and draw
                    this->lightShader->Commit();
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
LightPrePassServer::RenderSpotLights()
{
    if ((this->spotLightMesh->GetState() == Resource::Loaded)
       && (this->spotLights[CastShadows].Size() > 0
          || this->spotLights[NoShadows].Size() > 0))
    {
        TransformDevice* tformDevice = TransformDevice::Instance();
        RenderDevice* renderDevice = RenderDevice::Instance();

        // set shared light projection texture
        // TODO: allow other light projection map from spotlightentity
        this->lightProjMapVar->SetTexture(this->lightProjMap->GetTexture());
        
        IndexT shadowIdx;
        for (shadowIdx = 0; shadowIdx < NumShadowFlags; ++shadowIdx)
        {
            if(this->spotLights[shadowIdx].Size())
            {
                this->lightShader->SelectActiveVariation(this->spotLightFeatureBits[shadowIdx]);
                this->lightShader->Apply();
                this->spotLightMesh->GetMesh()->ApplyPrimitives(0);

                IndexT i;
                for (i = 0; i < this->spotLights[shadowIdx].Size(); i++)
                {            
                    const Ptr<SpotLightEntity>& curLight = this->spotLights[shadowIdx][i];
                    const matrix44& lightTransform = curLight->GetTransform();
                    tformDevice->SetModelTransform(lightTransform);

                    // light position in view space, and set .w to inverted light range
                    const matrix44& viewTransform = tformDevice->GetViewTransform();
                    float4 posAndRange = matrix44::transform(lightTransform.get_position(), viewTransform);
                    posAndRange.w() = 1.0f / lightTransform.get_zaxis().length();

					this->lightShader->BeginUpdateSync();
                    this->lightPosRange->SetFloat4(posAndRange);
                    this->lightColor->SetFloat4(curLight->GetColor());

                    // needed for tex coordinates to lookup correct spotlight lightmap texel
                    const matrix44& invViewTransform = tformDevice->GetInvViewTransform();
                    matrix44 fromViewToLightProj = matrix44::multiply(invViewTransform, curLight->GetInvLightProjTransform());            
                    this->lightProjTransform->SetMatrix(fromViewToLightProj);

                    if (CastShadows == (ShadowFlag)shadowIdx
                        && curLight->GetCastShadowsThisFrame())
                    {                
                        // needed for tex coordinates to lookup correct spotlight shadowmap texel
                        matrix44 fromViewToShadowLightProj = matrix44::multiply(invViewTransform, curLight->GetShadowInvLightProjTransform());            
                        this->shadowProjTransform->SetMatrix(fromViewToShadowLightProj);

                        // set shadow map offset index for multiple shadows
                        const float4& shadowOffsetScale = curLight->GetShadowBufferUvOffsetAndScale();
                        this->shadowOffsetScaleVar->SetFloat4(shadowOffsetScale);

                        // set shadow intensity
                        this->shadowFadeVar->SetFloat(curLight->GetShadowIntensity());
                    }
                    
                    // update shader variables
                    tformDevice->ApplyModelTransforms(this->lightShader);
                    this->lightShader->EndUpdateSync();

                    // commit and draw
                    this->lightShader->Commit();
                    renderDevice->Draw();
                }
            }
        }                     
    }
}

} // namespace Lighting
