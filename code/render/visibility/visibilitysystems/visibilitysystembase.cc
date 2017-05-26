//------------------------------------------------------------------------------
//  visibilityserver.cc
//  (C) 2010 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "visibility/visibilitysystems/visibilitysystembase.h"

namespace Visibility
{
__ImplementClass(Visibility::VisibilitySystemBase, 'VISS', Core::RefCounted);
         
using namespace Util;
using namespace Math;
using namespace Graphics;

//------------------------------------------------------------------------------
/**
*/
VisibilitySystemBase::VisibilitySystemBase() :
    isOpen(false),
    inAttachContainer(false)
{
}

//------------------------------------------------------------------------------
/**
*/
VisibilitySystemBase::~VisibilitySystemBase()
{
    n_assert(!this->isOpen);
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilitySystemBase::Open(IndexT orderIndex)
{
    n_assert(!this->isOpen);
    this->isOpen = true;
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilitySystemBase::Close()
{
    n_assert(this->isOpen);
    this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilitySystemBase::InsertVisibilityContext(const Ptr<VisibilityContext>& entityVis)
{
    // implement in derived class
    n_error("VisibilitySystemBase::InsertVisibilityContext: Implement in derived class!");
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilitySystemBase::RemoveVisibilityContext(const Ptr<VisibilityContext>& entityVis)
{
    // implement in derived class
    n_error("VisibilitySystemBase::RemoveVisibilityContext: Implement in derived class!");
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilitySystemBase::UpdateVisibilityContext(const Ptr<VisibilityContext>& entityVis)
{       
    // implement in derived class
    n_error("VisibilitySystemBase::UpdateVisibilityContext: Implement in derived class!");
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilitySystemBase::OnWorldChanged( const Math::bbox& box )
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilitySystemBase::OnRenderDebug()
{       
    // implement in derived class
    n_error("VisibilitySystemBase::OnRenderDebug: Implement in derived class!");
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilitySystemBase::InsertVisibilityContainer(const Ptr<VisibilityContainer>& container)
{        
    n_assert(this->inAttachContainer);
    // implement in derived class    
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilitySystemBase::BeginAttachVisibilityContainer()
{
    n_assert(!this->inAttachContainer);
    this->inAttachContainer = true;        
}

//------------------------------------------------------------------------------
/**
*/
void 
VisibilitySystemBase::EndAttachVisibilityContainer()
{
    n_assert(this->inAttachContainer);
    this->inAttachContainer = false;
}

//------------------------------------------------------------------------------
/**
*/
Ptr<Jobs::Job>
VisibilitySystemBase::CreateVisibilityJob(IndexT frameId, const Ptr<ObserverContext>& observer, Util::FixedArray<Ptr<VisibilityContext> >& outEntitiyArray, uint& entityMask)
{
    // implement in subclass
    n_error("VisibilitySystemBase::AttachVisibilityJob called: Implement in subclass! Do it!");

    Ptr<Jobs::Job> result;
    return result;
}

} // namespace Visibility
