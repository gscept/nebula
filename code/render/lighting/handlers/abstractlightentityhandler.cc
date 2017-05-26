//------------------------------------------------------------------------------
//  abstractlightentityhandler.cc
//  (C) 2010 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "graphics/graphicsprotocol.h"
#include "messaging/staticmessagehandler.h"
#include "graphics/abstractlightentity.h"

using namespace Graphics;
using namespace Lighting;
using namespace Graphics;

namespace Messaging
{

//------------------------------------------------------------------------------
/**
*/
__Handler(AbstractLightEntity, SetLightColor)
{
    obj->SetColor(msg->GetColor());
}

//------------------------------------------------------------------------------
/**
*/
__Handler(AbstractLightEntity, SetLightCastShadows)
{
    obj->SetCastShadows(msg->GetCastShadows());
}

//------------------------------------------------------------------------------
/**
*/
__Handler(AbstractLightEntity, SetLightShadowIntensity)
{
	obj->SetShadowIntensity(msg->GetIntensity());
}

//------------------------------------------------------------------------------
/**
*/
__Handler(AbstractLightEntity, SetLightShadowBias)
{
	obj->SetShadowBias(msg->GetBias());
}

//------------------------------------------------------------------------------
/**
*/
__Handler(AbstractLightEntity, SetLightProjectionTexture)
{
	obj->SetProjectionTexture(msg->GetTexture());
}

//------------------------------------------------------------------------------
/**
*/
__Handler(AbstractLightEntity, SetLightVolumetric)
{
	obj->SetVolumetric(msg->GetEnabled());
}

//------------------------------------------------------------------------------
/**
*/
__Handler(AbstractLightEntity, SetLightVolumetricScale)
{
	obj->SetVolumetricScale(msg->GetScale());
}

//------------------------------------------------------------------------------
/**
*/
__Handler(AbstractLightEntity, SetLightVolumetricIntensity)
{
	obj->SetVolumetricIntensity(msg->GetIntensity());
}

//------------------------------------------------------------------------------
/**
*/
__Dispatcher(AbstractLightEntity)
{
    __Handle(AbstractLightEntity, SetLightColor);
    __Handle(AbstractLightEntity, SetLightCastShadows);
	__Handle(AbstractLightEntity, SetLightShadowIntensity);
	__Handle(AbstractLightEntity, SetLightShadowBias);
	__Handle(AbstractLightEntity, SetLightProjectionTexture);
	__Handle(AbstractLightEntity, SetLightVolumetric);
	__Handle(AbstractLightEntity, SetLightVolumetricScale);
	__Handle(AbstractLightEntity, SetLightVolumetricIntensity);
    __HandleUnknown(GraphicsEntity);
}

} // namespace Messaging
