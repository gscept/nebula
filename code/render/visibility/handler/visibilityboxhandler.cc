//------------------------------------------------------------------------------
//  graphicshandler.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "visibility/visibilityprotocol.h"
#include "messaging/staticmessagehandler.h"
#include "visibility/visibilitysystems/visibilitybox.h"
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
__StaticHandler(CreateVisibilityBoxes)
{
    const Ptr<Stage>& stage = GraphicsServer::Instance()->GetStageByName(msg->GetStageName());
    Visibility::VisibilityChecker& visChecker = stage->GetVisibilityChecker();
                            
    visChecker.BeginAttachVisibilityContainer();
    SizeT numBoxes = msg->GetBoundingBoxes().Size();
    IndexT i;
    for (i = 0; i < numBoxes; ++i)
    {           
        Ptr<VisibilityBox> box = VisibilityBox::Create();    
        box->SetTransform(msg->GetBoundingBoxes()[i]);
        visChecker.AttachVisibilityContainer(box.cast<VisibilityContainer>());    	
    }
    visChecker.EndAttachVisibilityContainer();
}



} // namespace Messaging

