//------------------------------------------------------------------------------
//  globallightentityhandler.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "graphics/graphicsprotocol.h"
#include "messaging/staticmessagehandler.h"
#include "graphics/globallightentity.h"
#include "graphics/graphicsserver.h"
#include "graphics/stage.h"

using namespace Util;
using namespace Graphics;
using namespace Graphics;
using namespace Lighting;

namespace Messaging
{

//------------------------------------------------------------------------------
/**
*/
__StaticHandler(CreateGlobalLightEntity)
{
    // create a new global light entity
    Ptr<Graphics::GlobalLightEntity> lightEntity = Graphics::GlobalLightEntity::Create();
    lightEntity->SetTransform(msg->GetTransform());
    lightEntity->SetVisible(msg->GetVisible());
    lightEntity->SetColor(msg->GetLightColor());
    lightEntity->SetBackLightColor(msg->GetBackLightColor());
    lightEntity->SetCastShadows(msg->GetCastShadows());
    lightEntity->SetProjMapUvOffsetAndScale(msg->GetProjMapUvOffsetAndScale());
    lightEntity->SetAmbientLightColor(msg->GetAmbientLightColor());
    lightEntity->SetBackLightOffset(msg->GetBackLightOffset());
    lightEntity->SetShadowIntensity(msg->GetShadowIntensity());
    lightEntity->SetVolumetric(msg->GetVolumetric());
    lightEntity->SetVolumetricScale(msg->GetVolumetricScale());
    lightEntity->SetVolumetricIntensity(msg->GetVolumetricIntensity());

    // lookup stage and attach entity
    const Ptr<Stage>& stage = GraphicsServer::Instance()->GetStageByName(msg->GetStageName());
    stage->AttachEntity(lightEntity.cast<GraphicsEntity>());

    // set return values
    msg->GetObjectRef()->Validate<Graphics::GlobalLightEntity>(lightEntity.get());
}

//------------------------------------------------------------------------------
/**
*/
__Handler(GlobalLightEntity, SetGlobalBackLightColor)
{
    obj->SetBackLightColor(msg->GetColor());
}

//------------------------------------------------------------------------------
/**
*/
__Handler(GlobalLightEntity, SetGlobalAmbientLightColor)
{
    obj->SetAmbientLightColor(msg->GetColor());
}

//------------------------------------------------------------------------------
/**
*/
__Handler(GlobalLightEntity, SetGlobalBackLightOffset)
{
    obj->SetBackLightOffset(msg->GetOffset());
}

//------------------------------------------------------------------------------
/**
*/
__Handler(GlobalLightEntity, SetGlobalLightParams)
{
    obj->SetColor(msg->GetLightColor());
    obj->SetBackLightColor(msg->GetBackLightColor());
    obj->SetAmbientLightColor(msg->GetAmbientLightColor());
    obj->SetBackLightOffset(msg->GetBackLightOffset());
    obj->SetCastShadows(msg->GetCastShadows());
}

//------------------------------------------------------------------------------
/**
*/
__Dispatcher(GlobalLightEntity)
{
    __Handle(GlobalLightEntity, SetGlobalBackLightColor);
    __Handle(GlobalLightEntity, SetGlobalAmbientLightColor);
    __Handle(GlobalLightEntity, SetGlobalBackLightOffset);
    __Handle(GlobalLightEntity, SetGlobalLightParams);  
    __HandleUnknown(AbstractLightEntity);
}

} // namespace Messaging