//------------------------------------------------------------------------------
//  shadowserverbase.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "lighting/base/shadowserverbase.h"
#include "lighting/lightserver.h"

namespace Lighting
{
__ImplementClass(Lighting::ShadowServerBase, 'SDSB', Core::RefCounted);
__ImplementSingleton(Lighting::ShadowServerBase);

using namespace Graphics;
using namespace CoreGraphics;
using namespace Math;
using namespace Threading;
//------------------------------------------------------------------------------
/**
*/
ShadowServerBase::ShadowServerBase() :
    isOpen(false),
    inBeginFrame(false),
    inBeginAttach(false)
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
ShadowServerBase::~ShadowServerBase()
{
    n_assert(!this->isOpen);
    n_assert(!this->inBeginFrame);
    n_assert(!this->inBeginAttach);
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
ShadowServerBase::Open()
{
    n_assert(!this->isOpen);
    this->isOpen = true;
}

//------------------------------------------------------------------------------
/**
*/
void
ShadowServerBase::Close()
{
    n_assert(this->isOpen);
    n_assert(!this->inBeginFrame);
    n_assert(!this->inBeginAttach);
    this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
*/
void
ShadowServerBase::BeginFrame(const Ptr<CameraEntity>& camEntity)
{
    n_assert(this->isOpen);
    n_assert(!this->inBeginFrame);
    n_assert(!this->inBeginAttach);
    n_assert(this->spotLightEntities.IsEmpty());
	n_assert(this->pointLightEntities.IsEmpty());
    n_assert(!this->globalLightEntity.isvalid());
    n_assert(!this->cameraEntity.isvalid());
    n_assert(camEntity.isvalid());
    this->cameraEntity = camEntity;
    this->inBeginFrame = true;
}

//------------------------------------------------------------------------------
/**
*/
void
ShadowServerBase::EndFrame()
{
    n_assert(this->inBeginFrame);
    n_assert(!this->inBeginAttach);
    this->inBeginFrame = false;
    this->spotLightEntities.Clear();
	this->pointLightEntities.Clear();
    this->globalLightEntity = 0;
    this->cameraEntity = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
ShadowServerBase::BeginAttachVisibleLights()
{
    n_assert(this->inBeginFrame);
    n_assert(!this->inBeginAttach);
    n_assert(this->spotLightEntities.IsEmpty());
	n_assert(this->pointLightEntities.IsEmpty());
    n_assert(!this->globalLightEntity.isvalid());
    this->inBeginAttach = true;
    this->globalLightEntity = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
ShadowServerBase::AttachVisibleLight(const Ptr<AbstractLightEntity>& lightEntity)
{
    n_assert(lightEntity->GetCastShadows());

	// only attach light if it casts shadows this frame
	if (lightEntity->GetCastShadowsThisFrame())
	{
		if (lightEntity->GetLightType() == LightType::Global)
		{
			this->globalLightEntity = lightEntity.downcast<GlobalLightEntity>();
		}
		else
		{
			// if light is pointlight, unset shadow cube pointer
			if (lightEntity->GetLightType() == LightType::Point)
			{
				lightEntity.downcast<PointLightEntity>()->SetShadowCube(0);
				this->pointLightEntities.Append(lightEntity.downcast<PointLightEntity>());
			}
			else
			{
				this->spotLightEntities.Append(lightEntity.downcast<SpotLightEntity>());
			}
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
ShadowServerBase::EndAttachVisibleLights()
{
    n_assert(this->inBeginAttach);
    this->inBeginAttach = false;

    // @todo: sort shadow casting light sources by priority
    this->SortLights();

    // set only max num lights to shadow casting
    IndexT i;
    for (i = 0; i < this->spotLightEntities.Size(); ++i)
    {           
		// attach to light server
        LightServer::Instance()->AttachVisibleLight(this->spotLightEntities[i].upcast<AbstractLightEntity>());			
    }

	for (i = 0; i < this->pointLightEntities.Size(); ++i)
	{
		// attach to light server
		LightServer::Instance()->AttachVisibleLight(this->pointLightEntities[i].upcast<AbstractLightEntity>());
	}

	// if the global light goes here, it should always be enabled to cast shadows
	if (this->globalLightEntity.isvalid())
	{
		// attach to light server
		LightServer::Instance()->AttachVisibleLight(this->globalLightEntity.upcast<AbstractLightEntity>());
	}
}

//------------------------------------------------------------------------------
/**
    This method updates the shadow buffers (if supported on the
    platform).
*/
void
ShadowServerBase::UpdateShadowBuffers()
{
    // override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
ShadowServerBase::UpdateSpotLightShadowBuffers()
{
	// override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
ShadowServerBase::UpdatePointLightShadowBuffers()
{
	// override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
ShadowServerBase::UpdateGlobalLightShadowBuffers()
{
	// override in subclass
}

//------------------------------------------------------------------------------
/**
    Sort lights by their attenuation at point of interest
*/
void 
ShadowServerBase::SortLights()      
{
    // implement in platform dependent class    
}

//------------------------------------------------------------------------------
/**
*/
const float* 
ShadowServerBase::GetSplitDistances() const
{
	// implement in platform dependent class 
	return 0;
}

//------------------------------------------------------------------------------
/**
*/
const Math::matrix44* 
ShadowServerBase::GetSplitTransforms() const
{
	// implement in platform dependent class 
	return 0;
}


//------------------------------------------------------------------------------
/**
*/
const Math::matrix44* 
ShadowServerBase::GetShadowView() const
{
	// implement in platform dependent class
	return 0;
}

//------------------------------------------------------------------------------
/**
*/
const Math::float4*
ShadowServerBase::GetFarPlane() const
{
	// implement in platform dependent class
	return 0;
}

//------------------------------------------------------------------------------
/**
*/
const Math::float4* 
ShadowServerBase::GetNearPlane() const
{
	// implement in platform dependent class
	return 0;
}

} // namespace Lighting
