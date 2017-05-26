//------------------------------------------------------------------------------
//  entityvisibility.cc
//  (C) 2010 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "visibility/visibilityquery.h"
#include "graphics/graphicsentity.h"
#include "graphics/graphicsserver.h"

namespace Visibility
{
__ImplementClass(Visibility::VisibilityQuery, 'VIQU', Core::RefCounted);

using namespace Util;
using namespace Math;
using namespace Jobs;
using namespace Graphics;

//------------------------------------------------------------------------------
/**
*/
VisibilityQuery::VisibilityQuery()
{
    this->jobPort = JobPort::Create();
    this->jobPort->Setup();
}

//------------------------------------------------------------------------------
/**
*/
VisibilityQuery::~VisibilityQuery()
{
    this->jobPort = 0;
    this->observer = 0;
    this->observerContext = 0;
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityQuery::AttachVisibilitySystem(const Ptr<VisibilitySystemBase>& visSystem)
{
    this->visibilitySystems.Append(visSystem);
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilityQuery::Run(IndexT frameId)
{
    // not necessary, job will be destroyed after results are applied          
    this->visibleEntities.SetSize(GraphicsServer::Instance()->GetEntities().Size());     
    this->visibleEntities.Fill(0);
    
    // TODO: create worker thread which goes thru all vis systems
    this->observerContext = ObserverContext::Create();
    this->observerContext->Setup(this->observer);    

    // for now, no multi threading: call directly check visibility function
    Util::Array<Ptr<Job> > jobs;
    IndexT i;
    for (i = 0; i < this->visibilitySystems.Size(); ++i)
    {   
        const Ptr<VisibilitySystemBase>& visSystem = this->visibilitySystems[i];
        Ptr<Job> newJob = visSystem->CreateVisibilityJob(frameId, 
                                                         this->observerContext, 
                                                         this->visibleEntities, 
                                                         this->entityMask);
        // for non multi-threaded 
        if (newJob.isvalid())
        {
            jobs.Append(newJob);        
        }  
        // if multi threading, all visibility systems must work doublebuffered over 2 frames
        //
    }
    // visibility systems are dependent on results of previous visibility system
    this->jobPort->PushJobChain(jobs);
}  

//------------------------------------------------------------------------------
/**
*/
bool 
VisibilityQuery::IsFinished() const
{
    return this->jobPort->CheckDone();
}
    
//------------------------------------------------------------------------------
/**
*/
bool
VisibilityQuery::WaitForFinished() const
{
    this->jobPort->WaitDone();
    return true;
}    

//------------------------------------------------------------------------------
/**
*/
Graphics::GraphicsEntity::LinkType 
VisibilityQuery::GetObserverType()
{       
    if (this->observer->GetType() == GraphicsEntityType::Light)
    {
        return GraphicsEntity::LightLink;
    }
    return GraphicsEntity::CameraLink;
}
} // namespace Visibility
