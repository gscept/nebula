//------------------------------------------------------------------------------
//  visibilityhandler.cc
//  (C) 2013
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "visibility/visibilityprotocol.h"
#include "graphics/graphicsserver.h"
#include "graphics/stage.h"
#include "visibility/visibilitychecker.h"

                          
using namespace Util;
using namespace Visibility;
using namespace Graphics;

namespace Messaging
{

//------------------------------------------------------------------------------
/**
*/
__StaticHandler(ChangeVisibilityBounds)
{
	const Ptr<Stage>& stage = GraphicsServer::Instance()->GetStageByName(msg->GetStageName());
	Visibility::VisibilityChecker& visChecker = stage->GetVisibilityChecker();

	// notify visibility checker to update its bounding boxes
	visChecker.WorldChanged(msg->GetWorldBoundingBox());
}

} // namespace Messaging

