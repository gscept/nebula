//------------------------------------------------------------------------------
//  vkshadowserver.cc
//  (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "lighting/vk/vkshadowserver.h"
#include "coregraphics/transformdevice.h"
#include "models/visresolver.h"
#include "graphics/modelentity.h"
#include "coregraphics/shaderserver.h"     
#include "graphics/spotlightentity.h"
#include "materials/materialserver.h"
#include "math/polar.h"
#include "lighting/lightserver.h"
#include "math/bbox.h"
#include "math/vector.h"
#include "graphics/view.h"
#include "graphics/graphicsserver.h"
#include "graphics/view.h"
#include "graphics/graphicsserver.h"
#include "coregraphics/base/rendertargetbase.h"
#include "coregraphics/renderdevice.h"
#include "framesync/framesynctimer.h"
#include "coregraphics/shadersemantics.h"

#define DivAndRoundUp(a, b) (a % b != 0) ? (a / b + 1) : (a / b)
namespace Lighting
{
__ImplementClass(Lighting::VkShadowServer, 'SM5S', ShadowServerBase);

using namespace Math;
using namespace Util;
using namespace Resources;
using namespace CoreGraphics;
using namespace Graphics;
using namespace Models;
using namespace Materials;
using namespace Base;
using namespace FrameSync;

//------------------------------------------------------------------------------
/**
*/
VkShadowServer::VkShadowServer()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
VkShadowServer::~VkShadowServer()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
VkShadowServer::Open()
{
    // call parent class
    ShadowServerBase::Open();

    // load the ShadowBuffer frame shader
    const Ptr<MaterialServer> materialServer = MaterialServer::Instance();
    this->script = Frame::FrameServer::Instance()->LoadFrameScript("shadowmapping", "frame:vkshadowmapping.json");
    this->globalLightShadowBuffer = this->script->GetColorTexture("GlobalLightShadow")->GetTexture();
    this->spotLightShadowBuffer = this->script->GetColorTexture("SpotLightInstance")->GetTexture();
    this->spotLightShadowBufferAtlas = this->script->GetColorTexture("SpotLightShadowAtlas")->GetTexture();

    IndexT i;
    for (i = 0; i < NumShadowCastingLights; i++)
    {
        this->shaderStates[i] = ShaderServer::Instance()->CreateShaderState("shd:shadow", { NEBULA_SYSTEM_GROUP });
        this->shaderStates[i]->SetApplyShared(false);
        this->viewArrayVar[i] = this->shaderStates[i]->GetVariableByName(NEBULA_SEMANTIC_VIEWMATRIXARRAY);
    }
    this->lightIndexPool.SetSetupFunc([](IndexT& val, IndexT idx) { val = idx;  });
    this->lightIndexPool.Resize(NumShadowCastingLights);

    this->globalLightBatch = Frame::FrameSubpassBatch::Create();
    this->globalLightBatch->SetBatchCode(Graphics::BatchGroup::FromName("GlobalShadow"));
    this->spotLightBatch = Frame::FrameSubpassBatch::Create();
    this->spotLightBatch->SetBatchCode(Graphics::BatchGroup::FromName("SpotLightShadow"));
    this->pointLightBatch = Frame::FrameSubpassBatch::Create();
    this->pointLightBatch->SetBatchCode(Graphics::BatchGroup::FromName("PointLightShadow"));

    this->csmUtil.SetTextureWidth(this->globalLightShadowBuffer->GetWidth() / SplitsPerRow);
    this->csmUtil.SetClampingMethod(CSMUtil::AABB);
    this->csmUtil.SetFittingMethod(CSMUtil::Scene);

#if NEBULA_ENABLE_PROFILING
    this->globalShadow = Debug::DebugTimer::Create();
    String name("ShadowServer_GlobalShadow");
    this->globalShadow->Setup(name, "Shadowmapping");

    this->spotLightShadow = Debug::DebugTimer::Create();
    name.Format("ShadowServer_SpotLightShadow");
    this->spotLightShadow->Setup(name, "Shadowmapping");

    this->pointLightShadow = Debug::DebugTimer::Create();
    name.Format("ShadowServer_PointLightShadow");
    this->pointLightShadow->Setup(name, "Shadowmapping");
#endif
}

//------------------------------------------------------------------------------
/**
*/
void
VkShadowServer::Close()
{
    // discard buffer references
    this->globalLightShadowBuffer = 0;
    this->spotLightShadowBuffer = 0;

    IndexT i;
    for (i = 0; i < NumShadowCastingLights; i++)
    {
        this->viewArrayVar[i] = 0;
        this->shaderStates[i]->Discard();
        this->shaderStates[i] = 0;
    }

    // discard script
    this->script->Discard();
    this->script = 0;

    // call parent class
    ShadowServerBase::Close();

    _discard_timer(globalShadow);
    _discard_timer(pointLightShadow);
    _discard_timer(spotLightShadow);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShadowServer::AttachVisibleLight(const Ptr<Graphics::AbstractLightEntity>& lightEntity)
{
    ShadowServerBase::AttachVisibleLight(lightEntity);
    this->lightToIndexMap.Add(lightEntity, this->lightIndexPool.Alloc());
}

//------------------------------------------------------------------------------
/**
*/
void
VkShadowServer::EndFrame()
{
    ShadowServerBase::EndFrame();
    this->lightIndexPool.Reset();
    this->lightToIndexMap.Clear();
}

//------------------------------------------------------------------------------
/**
    This method updates the  shadow buffer
*/
void
VkShadowServer::UpdateShadowBuffers()
{
    n_assert(this->inBeginFrame);
    n_assert(!this->inBeginAttach);

    // simply run the script, it will call UpdateSpotLightShadowBuffers, UpdatePointLightShadowBuffers and UpdateGlobalLightBuffers
    const Ptr<FrameSyncTimer>& timer = FrameSyncTimer::Instance();
    IndexT frameIndex = timer->GetFrameIndex();
    if (this->spotLightEntities.Size() > 0 || this->pointLightEntities.Size() > 0 || this->globalLightEntity.isvalid())
    {
        this->script->Run(frameIndex);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
VkShadowServer::UpdateSpotLightShadowBuffers()
{
    if (this->spotLightEntities.Size() > 0)
    {
        const Ptr<VisResolver>& visResolver = VisResolver::Instance();
        const Ptr<TransformDevice>& transDev = TransformDevice::Instance();
        const Ptr<RenderDevice>& renderDev = RenderDevice::Instance();
        const Ptr<FrameSyncTimer>& timer = FrameSyncTimer::Instance();
        IndexT frameIndex = timer->GetFrameIndex();

        // for each shadow casting light...
        SizeT numLights = this->spotLightEntities.Size();
        if (numLights > MaxNumShadowSpotLights)
        {
            numLights = MaxNumShadowSpotLights;
        }

        _start_timer(spotLightShadow);
        IndexT lightIndex;
        for (lightIndex = 0; lightIndex < numLights; lightIndex++)
        {
            // render shadow casters in current light volume to shadow buffer
            const Ptr<SpotLightEntity>& lightEntity = this->spotLightEntities[lightIndex];

            // perform visibility resolve for current light
            visResolver->BeginResolve(lightEntity->GetTransform());
            const Array<Ptr<GraphicsEntity> >& visLinks = lightEntity->GetLinks(GraphicsEntity::LightLink);
            if (visLinks.Size() == 0) continue;
            IndexT linkIndex;
            for (linkIndex = 0; linkIndex < visLinks.Size(); linkIndex++)
            {
                const Ptr<GraphicsEntity>& curEntity = visLinks[linkIndex];
                n_assert(GraphicsEntityType::Model == curEntity->GetType());
                if (curEntity->IsA(ModelEntity::RTTI) && curEntity->GetCastsShadows())
                {
                    const Ptr<ModelEntity>& modelEntity = curEntity.downcast<ModelEntity>();
                    visResolver->AttachVisibleModelInstance(frameIndex, modelEntity->GetModelInstance(), false);
                }
            }
            visResolver->EndResolve();

            // prepare shadow buffer render target for rendering and 
            // patch current shadow buffer render target into the frame shader  
            Math::rectangle<int> resolveRect;
            resolveRect.left = (lightIndex % ShadowLightsPerRow) * this->spotLightShadowBuffer->GetWidth();
            resolveRect.right = resolveRect.left + this->spotLightShadowBuffer->GetWidth();
            resolveRect.top = (lightIndex / ShadowLightsPerColumn) * this->spotLightShadowBuffer->GetHeight();
            resolveRect.bottom = resolveRect.top + this->spotLightShadowBuffer->GetHeight();

            // render into shadow map
            renderDev->SetViewport(resolveRect, 0);
            renderDev->SetScissorRect(resolveRect, 0);
            matrix44 viewProj = matrix44::multiply(lightEntity->GetShadowInvTransform(), lightEntity->GetShadowProjTransform());
            const IndexT lightIdx = this->lightToIndexMap[lightEntity.upcast<AbstractLightEntity>()];
            this->viewArrayVar[lightIdx]->SetMatrixArray(&viewProj, 1);
            this->shaderStates[lightIdx]->Commit();

            // TODO: Render!
            this->spotLightBatch->Run(frameIndex);

            // patch shadow buffer and shadow buffer uv offset into the light source  
            // uvOffset.xy is offset
            // uvOffset.zw is modulate
            // also moves projection space coords into uv space
            float shadowBufferHoriPixelSize = 1.0f / resolveRect.width();
            float shadowBufferVertPixelSize = 1.0f / resolveRect.height();
            float borderPaddingX = float(ShadowAtlasBorderPixels / ShadowLightsPerRow) / ShadowLightsPerRow;
            float borderPaddingY = float(ShadowAtlasBorderPixels / ShadowLightsPerColumn) / ShadowLightsPerColumn;
            Math::float4 uvOffset;
            uvOffset.x() = float(lightIndex%ShadowLightsPerRow) / float(ShadowLightsPerRow);
            uvOffset.y() = float(lightIndex / ShadowLightsPerColumn) / float(ShadowLightsPerColumn);
            uvOffset.z() = (1.0f - shadowBufferHoriPixelSize) / float(ShadowLightsPerRow);
            uvOffset.w() = (1.0f - shadowBufferVertPixelSize) / float(ShadowLightsPerColumn);
            lightEntity->SetShadowBufferUvOffsetAndScale(uvOffset);
        }
        _stop_timer(spotLightShadow);
    }   
}

//------------------------------------------------------------------------------
/**
*/
void
VkShadowServer::UpdatePointLightShadowBuffers()
{
    if (this->pointLightEntities.Size() > 0)
    {
        const Ptr<VisResolver>& visResolver = VisResolver::Instance();
        const Ptr<TransformDevice>& transDev = TransformDevice::Instance();
        const Ptr<RenderDevice>& renderDev = RenderDevice::Instance();
        const Ptr<FrameSyncTimer>& timer = FrameSyncTimer::Instance();
        IndexT frameIndex = timer->GetFrameIndex();

        // for each shadow casting light...
        SizeT numLights = this->pointLightEntities.Size();
        if (numLights > MaxNumShadowPointLights)
        {
            numLights = MaxNumShadowPointLights;
        }

        _start_timer(pointLightShadow);
        IndexT lightIndex;
        for (lightIndex = 0; lightIndex < numLights; lightIndex++)
        {
            // render shadow casters in current light volume to shadow buffer
            const Ptr<PointLightEntity>& lightEntity = this->pointLightEntities[lightIndex];

            // perform visibility resolve for current light
            visResolver->BeginResolve(lightEntity->GetTransform());
            const Array<Ptr<GraphicsEntity> >& visLinks = lightEntity->GetLinks(GraphicsEntity::LightLink);
            if (visLinks.Size() == 0) continue;
            IndexT linkIndex;
            for (linkIndex = 0; linkIndex < visLinks.Size(); linkIndex++)
            {
                const Ptr<GraphicsEntity>& curEntity = visLinks[linkIndex];
                n_assert(GraphicsEntityType::Model == curEntity->GetType());
                if (curEntity->IsA(ModelEntity::RTTI) && curEntity->GetCastsShadows())
                {
                    const Ptr<ModelEntity>& modelEntity = curEntity.downcast<ModelEntity>();
                    visResolver->AttachVisibleModelInstance(frameIndex, modelEntity->GetModelInstance(), false);
                }
            }
            visResolver->EndResolve();

            // generate view projection matrix
            matrix44 proj = matrix44::perspfovrh(n_deg2rad(90.0f), 1, 0.1f, 100.0f);
            //viewProj.set_position(0);

            // generate matrices
            matrix44 views[6];
            const float4 lightPos = lightEntity->GetTransform().get_position();

            views[0] = matrix44::lookatrh(vector(0), vector(1, 0, 0), -vector::upvec());
            views[0].set_position(lightPos);
            views[0] = matrix44::multiply(matrix44::inverse(views[0]), proj);

            views[1] = matrix44::lookatrh(vector(0), vector(-1, 0, 0), -vector::upvec());
            views[1].set_position(lightPos);
            views[1] = matrix44::multiply(matrix44::inverse(views[1]), proj);

            views[2] = matrix44::lookatrh(vector(0), vector(0, 1, 0), vector(0, 0, 1));
            views[2].set_position(lightPos);
            views[2] = matrix44::multiply(matrix44::inverse(views[2]), proj);

            views[3] = matrix44::lookatrh(vector(0), vector(0, -1, 0), vector(0, 0, -1));
            views[3].set_position(lightPos);
            views[3] = matrix44::multiply(matrix44::inverse(views[3]), proj);

            views[4] = matrix44::lookatrh(vector(0), vector(0, 0, 1), -vector::upvec());
            views[4].set_position(lightPos);
            views[4] = matrix44::multiply(matrix44::inverse(views[4]), proj);

            views[5] = matrix44::lookatrh(vector(0), vector(0, 0, -1), -vector::upvec());
            views[5].set_position(lightPos);
            views[5] = matrix44::multiply(matrix44::inverse(views[5]), proj);

            const IndexT lightIdx = this->lightToIndexMap[lightEntity.upcast<AbstractLightEntity>()];
            this->viewArrayVar[lightIdx]->SetMatrixArray(views, 6);
            this->shaderStates[lightIdx]->Commit();

            // TODO: Render!
        }
        _stop_timer(pointLightShadow);
    }
}

//------------------------------------------------------------------------------
/**
    Update the parallel-split-shadow-map shadow buffers for the
    global light source.
*/
void
VkShadowServer::UpdateGlobalLightShadowBuffers()
{
    // since we run this from a script, we have to check here if we have a global light
    if (this->globalLightEntity.isvalid())
    {
        // get timer, visibility resolver and transform device
        const Ptr<TransformDevice>& transDev = TransformDevice::Instance();

        // begin profiling
        _start_timer(globalShadow);

        // update CSM util
        this->csmUtil.SetCameraEntity(this->cameraEntity);
        this->csmUtil.SetGlobalLight(this->globalLightEntity);
        Math::bbox box = GraphicsServer::Instance()->GetDefaultView()->GetStage()->GetGlobalBoundingBox();

        // add extension to bounding box, this ensures our ENTIRE level gets into a cascade (hopefully), but clamp the box to not be bigger than 1000, 1000, 1000
        vector extents = box.extents() + vector(10, 10, 10);
        extents = float4::clamp(extents, vector(-500), vector(500));

        box.set(point(0), extents);
        this->csmUtil.SetShadowBox(box);
        this->csmUtil.Compute();

        // set transforms in device
        //transDev->ApplyViewMatrixArray(splitMatrices, CSMUtil::NumCascades);

        // render objects
        const Ptr<VisResolver>& visResolver = VisResolver::Instance();
        const Ptr<FrameSyncTimer>& timer = FrameSyncTimer::Instance();
        IndexT frameIndex = timer->GetFrameIndex();

        // get the first view projection matrix which will resolve all visible shadow casting entities
        matrix44 cascadeViewProjection = this->csmUtil.GetCascadeViewProjection(0);

        // perform visibility resolve, directional lights don't really have a position
        // thus we're feeding an identity matrix as camera transform
        visResolver->BeginResolve(cascadeViewProjection);
        const Array<Ptr<GraphicsEntity>>& visLinks = this->globalLightEntity->GetLinks(GraphicsEntity::LightLink);
        IndexT linkIndex;
        for (linkIndex = 0; linkIndex < visLinks.Size(); linkIndex++)
        {
            const Ptr<GraphicsEntity>& curEntity = visLinks[linkIndex];
            n_assert(GraphicsEntityType::Model == curEntity->GetType());

            if (curEntity->IsA(ModelEntity::RTTI) && curEntity->GetCastsShadows())
            {
                const Ptr<ModelEntity>& modelEntity = curEntity.downcast<ModelEntity>();
                visResolver->AttachVisibleModelInstance(frameIndex, modelEntity->GetModelInstance(), false);
            }
        }
        visResolver->EndResolve();

        // update view matrix array
        matrix44 splitMatrices[CSMUtil::NumCascades];

        // render shadow casters for each view volume split
        IndexT cascadeIndex;
        for (cascadeIndex = 0; cascadeIndex < CSMUtil::NumCascades; cascadeIndex++)
        {
            splitMatrices[cascadeIndex] = this->csmUtil.GetCascadeViewProjection(cascadeIndex);
        }

        const IndexT lightIdx = this->lightToIndexMap[this->globalLightEntity.upcast<AbstractLightEntity>()];
        this->viewArrayVar[lightIdx]->SetMatrixArray(splitMatrices, CSMUtil::NumCascades);
        this->shaderStates[lightIdx]->Commit();

        // TODO: Render!
        this->globalLightBatch->Run(frameIndex);

        // stop profiling
        _stop_timer(globalShadow);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
VkShadowServer::SortLights()
{
    // @todo: sort without dictionary 
    Util::Dictionary<float, Ptr<SpotLightEntity> > sortedArray;
    sortedArray.BeginBulkAdd();
    IndexT i;
    for (i = 0; i < this->spotLightEntities.Size(); ++i)
    {
        const matrix44& lightTrans = this->spotLightEntities[i]->GetTransform();                                                                                                            
        vector vec2Poi = (float4)this->pointOfInterest - lightTrans.get_position();
        float distance = vec2Poi.length();
        float range = lightTrans.get_zaxis().length();
        float attenuation = n_saturate(1.0f - (distance / range));
        if (this->spotLightEntities[i]->IsA(SpotLightEntity::RTTI))
        {
            // consider spotlight frustum            
            if (!matrix44::ispointinside(this->pointOfInterest, this->spotLightEntities[i]->GetInvLightProjTransform()))
            { 
                attenuation = 0.0f;
            }
        }
        attenuation = 1.0f - attenuation;
        sortedArray.Add(attenuation, this->spotLightEntities[i]);
    }
    sortedArray.EndBulkAdd();     

    // patch positions if maxnumshadowlights  = 1
    if (sortedArray.Size() > MaxNumShadowSpotLights)
    {   
        if (MaxNumShadowSpotLights == 1)
        {   
            IndexT lastShadowCastingLight = MaxNumShadowSpotLights - 1;
            const Ptr<AbstractLightEntity>& lightEntity = sortedArray.ValueAtIndex(lastShadowCastingLight).upcast<AbstractLightEntity>();
            const matrix44& shadowLightTransform = lightEntity->GetTransform();
            float range0 = shadowLightTransform.get_zaxis().length();
            vector vecPoiLight = shadowLightTransform.get_position() - this->pointOfInterest;
            vector normedPoiLight = float4::normalize(vecPoiLight);

            point interpolatedPos = shadowLightTransform.get_position();
            polar lightDir(shadowLightTransform.get_zaxis());
            float interpolatedXRot = lightDir.theta;
            float interpolatedYRot = lightDir.rho;
            SizeT numLightsInRange = 0;
            IndexT i;
            for (i = MaxNumShadowSpotLights; i < sortedArray.Size(); ++i)
            {
                const Ptr<AbstractLightEntity>& curLightEntity = sortedArray.ValueAtIndex(i).upcast<AbstractLightEntity>();
                const matrix44& curTransform = curLightEntity->GetTransform();            
                vector distVec = shadowLightTransform.get_position() - curTransform.get_position();
                float range1 = curTransform.get_zaxis().length();
                bool lerpLightTransform = (distVec.length() - range0 - range1 < 0);
                if (curLightEntity->IsA(SpotLightEntity::RTTI))
                {
                    // consider spotlight frustum            
                    if (!matrix44::ispointinside(shadowLightTransform.get_position(), curLightEntity->GetInvLightProjTransform()))
                    {
                        lerpLightTransform = false;
                    }
                }
                if (lerpLightTransform)
                {                      
                    float dotPoi = float4::dot3(normedPoiLight, float4::normalize(distVec));
                    if (dotPoi > 0)
                    {                
                        // project poi - shadowlight to shadowlight next light vector
                        float len0 = distVec.length();
                        float projLen = float4::dot3(vecPoiLight, distVec);
                        projLen /= (len0 * len0);
                        // do a smoothstep
                        float lerpFactor = n_smoothstep(0, 1, projLen);
                        interpolatedPos = float4::lerp(interpolatedPos, curTransform.get_position(), lerpFactor);                                                      

                        polar curDir(curTransform.get_zaxis());
                        interpolatedXRot = n_lerp(interpolatedXRot, curDir.theta, lerpFactor);
                        interpolatedYRot = n_lerp(interpolatedYRot, curDir.rho, lerpFactor);
                    }                
                }            
            }       
            interpolatedPos.w() = 1.0f;
            matrix44 rotM = matrix44::rotationyawpitchroll(interpolatedYRot, interpolatedXRot - (N_PI * 0.5f), 0.0);
            matrix44 interpolatedTransform = matrix44::scaling(shadowLightTransform.get_xaxis().length(),
                shadowLightTransform.get_yaxis().length(),
                shadowLightTransform.get_zaxis().length());        
            interpolatedTransform = matrix44::multiply(interpolatedTransform, rotM);
            interpolatedTransform.set_position(interpolatedPos);
            lightEntity->SetShadowTransform(interpolatedTransform);
        }
        // more than one casting lights allowed, use shadow fading
        else
        {
            IndexT lastShadowCastingLight = MaxNumShadowSpotLights - 1;
            IndexT firstNotAllowedShadowCastingLight = MaxNumShadowSpotLights;
            // fade out shadow of casting light depended on next shadowcasting light
            float att0 = 1 - sortedArray.KeyAtIndex(lastShadowCastingLight);
            float att1 = 1 - sortedArray.KeyAtIndex(firstNotAllowedShadowCastingLight);

            float shadowIntensity = n_saturate(4.0f * att0);
            sortedArray.ValueAtIndex(lastShadowCastingLight)->SetShadowIntensity(shadowIntensity);                                                                                                                
        }
    }
    // assign sorted array
    this->spotLightEntities = sortedArray.ValuesAsArray();
}

} // namespace Lighting