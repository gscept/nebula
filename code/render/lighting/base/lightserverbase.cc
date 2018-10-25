//------------------------------------------------------------------------------
//  lightserverbase.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "lighting/base/lightserverbase.h"
#include "lighting/environmentprobe.h"

namespace Lighting
{
__ImplementClass(Lighting::LightServerBase, 'LISB', Core::RefCounted);
__ImplementSingleton(Lighting::LightServerBase);

using namespace Graphics;

//------------------------------------------------------------------------------
/**
*/
LightServerBase::LightServerBase() :
    isOpen(false),
    inBeginFrame(false),
    inBeginAttach(false)
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
LightServerBase::~LightServerBase()
{
    n_assert(!this->isOpen);
    n_assert(!this->inBeginFrame);
    n_assert(!this->inBeginAttach);
    n_assert(!this->cameraEntity.isvalid());
    n_assert(this->visibleLightEntities.IsEmpty());
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
LightServerBase::Open()
{
    n_assert(!this->isOpen);
    this->isOpen = true;

	// setup default environment probe
	EnvironmentProbe::DefaultEnvironmentProbe = EnvironmentProbe::Create();
	n_assert(EnvironmentProbe::DefaultEnvironmentProbe->AssignReflectionMap("tex:system/sky_refl.dds"));
	n_assert(EnvironmentProbe::DefaultEnvironmentProbe->AssignIrradianceMap("tex:system/sky_irr.dds"));
}

//------------------------------------------------------------------------------
/**
*/
void
LightServerBase::Close()
{
    n_assert(this->isOpen);
    n_assert(!this->inBeginFrame);
    n_assert(!this->inBeginAttach);
    n_assert(!this->cameraEntity.isvalid());
    n_assert(!this->globalLightEntity.isvalid());
    n_assert(this->visibleLightEntities.IsEmpty());
    this->isOpen = false;

	EnvironmentProbe::DefaultEnvironmentProbe->Discard();
	EnvironmentProbe::DefaultEnvironmentProbe = 0;
}

//------------------------------------------------------------------------------
/**
    Indicate whether the light server requires visibility links between
    lights and models. Deferred lighting solutions usually don't need this.
    FIXME: EXCEPT for shadow map rendering!!
*/
bool
LightServerBase::NeedsLightModelLinking() const
{
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
LightServerBase::BeginFrame(const Ptr<CameraEntity>& camEntity)
{
    n_assert(this->isOpen);
    n_assert(!this->inBeginFrame);
    n_assert(!this->inBeginAttach);
    n_assert(!this->cameraEntity.isvalid());
    n_assert(camEntity.isvalid());
    this->cameraEntity = camEntity;
    this->inBeginFrame = true;
}

//------------------------------------------------------------------------------
/**
*/
void
LightServerBase::EndFrame()
{
    n_assert(this->isOpen);
    n_assert(this->inBeginFrame);
    n_assert(!this->inBeginAttach);
    this->visibleLightEntities.Clear();
	this->visibleLightProbes.Clear();
    this->cameraEntity = 0;
    this->globalLightEntity = 0;
    this->inBeginFrame = false;
}

//------------------------------------------------------------------------------
/**
*/
void
LightServerBase::BeginAttachVisibleLights()
{
    n_assert(this->inBeginFrame);
    n_assert(!this->inBeginAttach);
    n_assert(this->visibleLightEntities.IsEmpty());
	n_assert(this->visibleLightProbes.IsEmpty());
    n_assert(this->cameraEntity.isvalid());
    this->inBeginAttach = true;
    this->globalLightEntity = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
LightServerBase::AttachVisibleLight(const Ptr<AbstractLightEntity>& lightEntity)
{
    n_assert(this->inBeginAttach);
    if (lightEntity->GetLightType() == LightType::Global)
    {
        this->globalLightEntity = lightEntity.downcast<GlobalLightEntity>();
    }
    else
    {
		// touch light projection map (update if needed)
		lightEntity->TouchProjectionTexture();
        this->visibleLightEntities.Append(lightEntity);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
LightServerBase::AttachVisibleLightProbe(const Ptr<LightProbeEntity>& lightProbe)
{
	n_assert(this->inBeginAttach);
	this->visibleLightProbes.Append(lightProbe);
}

//------------------------------------------------------------------------------
/**
*/
void
LightServerBase::EndAttachVisibleLights()
{
    n_assert(this->inBeginAttach);
    this->inBeginAttach = false;

    // @todo: sort light source by importance
}

//------------------------------------------------------------------------------
/**
    This method is called during rendering to apply lighting parameters 
    to the provided ModelEntity.
*/
void
LightServerBase::ApplyModelEntityLights(const Ptr<ModelEntity>& modelEntity)
{
    // empty, override in subclass
}

//------------------------------------------------------------------------------
/**
    This method is called when a FrameBatch of type Lighting is rendered.
*/
void
LightServerBase::RenderLights()
{
    // empty, override in subclass as needed
}

//------------------------------------------------------------------------------
/**
*/
void
LightServerBase::RenderLightProbes()
{
	// empty, override in subclass as needed
}

} // namespace Lighting
