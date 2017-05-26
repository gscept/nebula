//------------------------------------------------------------------------------
//  visresolver.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "models/visresolver.h"
#include "models/model.h"
#include "models/modelnode.h"
#include "models/modelnodeinstance.h"
#include "models/modelinstance.h"
#include "framesync/framesynctimer.h"

namespace Models
{
__ImplementClass(Models::VisResolver, 'VSRV', Core::RefCounted);
__ImplementSingleton(Models::VisResolver);

using namespace Util;
using namespace FrameSync;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
VisResolver::VisResolver() :
    cameraTransform(matrix44::identity()),
    resolveCount(InvalidIndex),
    isOpen(false),
    inResolve(false)
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
VisResolver::~VisResolver()
{
    n_assert(!this->isOpen);
    n_assert(!this->inResolve);
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
VisResolver::Open()
{
    n_assert(!this->isOpen);
    n_assert(!this->inResolve);
    this->isOpen = true;
    this->resolveCount = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
VisResolver::Close()
{
    n_assert(this->isOpen);
    n_assert(!this->inResolve);
	n_assert(this->states.IsEmpty());
    this->visibleModels.Reset();
    this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
*/
void
VisResolver::BeginResolve(const matrix44& camTransform)
{
    n_assert(this->isOpen);
    n_assert(!this->inResolve);
    this->inResolve = true;
    this->cameraTransform = camTransform;
    this->resolveCount++;
    this->visibleModels.Reset();
}

//------------------------------------------------------------------------------
/**
*/
void
VisResolver::AttachVisibleModelInstance(IndexT frameIndex, const Ptr<ModelInstance>& inst, bool updateLod)
{
    inst->OnVisibilityResolve(frameIndex, this->resolveCount, this->cameraTransform, updateLod);
}

//------------------------------------------------------------------------------
/**
*/
void
VisResolver::AttachVisibleModelInstancePlayerCamera(IndexT frameIndex, const Ptr<ModelInstance>& inst, bool updateLod)
{
    this->AttachVisibleModelInstance(frameIndex, inst, updateLod);
    inst->UpdateRenderStats(this->cameraTransform);
}

//------------------------------------------------------------------------------
/**
*/
void
VisResolver::EndResolve()
{
    n_assert(this->inResolve);
    this->inResolve = false;
}

//------------------------------------------------------------------------------
/**
*/
const Array<Ptr<Model> >&
VisResolver::GetVisibleModels(const Materials::SurfaceName::Code& surfaceName) const
{
    n_assert(!this->inResolve);
    return this->visibleModels.Get(surfaceName);
}

//------------------------------------------------------------------------------
/**
*/
const Array<Ptr<ModelNode> >&
VisResolver::GetVisibleModelNodes(const Materials::SurfaceName::Code& surfaceName, const Ptr<Model>& model) const
{
    return model->GetVisibleModelNodes(surfaceName);
}

//------------------------------------------------------------------------------
/**
*/
const Array<Ptr<ModelNodeInstance> >&
VisResolver::GetVisibleModelNodeInstances(const Materials::SurfaceName::Code& surfaceName, const Ptr<ModelNode>& modelNode) const
{
    return modelNode->GetVisibleModelNodeInstances(surfaceName);
}

//------------------------------------------------------------------------------
/**
*/
void
VisResolver::PushResolve()
{
	this->states.Push(this->visibleModels);
}

//------------------------------------------------------------------------------
/**
*/
void
VisResolver::PopResolve()
{
	this->visibleModels = this->states.Pop();
}

} // namespace Models