//------------------------------------------------------------------------------
//  graphicshandler.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "visibility/visibilityprotocol.h"
#include "messaging/staticmessagehandler.h"
#include "visibility/visibilitysystems/visibilitycluster.h"
#include "visibility/visibilitychecker.h"
#include "graphics/stage.h"
#include "graphics/graphicsserver.h"
                          
using namespace Util;
using namespace Visibility;
using namespace Graphics;

namespace Messaging
{

//------------------------------------------------------------------------------
/**
*/
__StaticHandler(CreateVisibilityCluster)
{
    const Ptr<Stage>& stage = GraphicsServer::Instance()->GetStageByName(msg->GetStageName());
    Visibility::VisibilityChecker& visChecker = stage->GetVisibilityChecker();
    // get references to its  entities
    Util::Array<Ptr<GraphicsEntity> > entities;
    IndexT i;
    for (i = 0; i < msg->GetEntities().Size(); ++i)
    {
        entities.Append(msg->GetEntities()[i]);    	
    }     

    //    msg->SetResult(DisplayDevice::Instance()->AdapterExists(msg->GetAdapter()));
    Ptr<VisibilityCluster> cluster = VisibilityCluster::Create();
    cluster->SetGraphicsEntities(entities);
    cluster->SetBoxTransforms(msg->GetBoundingBoxes());
    visChecker.BeginAttachVisibilityContainer();
    visChecker.AttachVisibilityContainer(cluster.cast<VisibilityContainer>());
    visChecker.EndAttachVisibilityContainer();
}

} // namespace Messaging

