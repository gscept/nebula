//------------------------------------------------------------------------------
//  pointlightentityhandler.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "graphics/pointlightentity.h"
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
__StaticHandler(CreatePointLightEntity)
{
    // create a new spotlight entity
    Ptr<Graphics::PointLightEntity> lightEntity = Graphics::PointLightEntity::Create();
    lightEntity->SetTransform(msg->GetTransform());
    lightEntity->SetVisible(msg->GetVisible());
    lightEntity->SetColor(msg->GetLightColor());
    lightEntity->SetCastShadows(msg->GetCastShadows());
    lightEntity->SetProjMapUvOffsetAndScale(msg->GetProjMapUvOffsetAndScale());
	lightEntity->SetShadowIntensity(msg->GetShadowIntensity());
	lightEntity->SetVolumetric(msg->GetVolumetric());
	lightEntity->SetVolumetricScale(msg->GetVolumetricScale());
	lightEntity->SetVolumetricIntensity(msg->GetVolumetricIntensity());

    // lookup stage and attach entity
    const Ptr<Stage>& stage = GraphicsServer::Instance()->GetStageByName(msg->GetStageName());
    stage->AttachEntity(lightEntity.cast<GraphicsEntity>());

    // set return value
    msg->GetObjectRef()->Validate<Graphics::PointLightEntity>(lightEntity.get());
}

//------------------------------------------------------------------------------
/**
*/
__Dispatcher(PointLightEntity)
{
    __HandleUnknown(AbstractLightEntity);
}

} // namespace Messaging