//------------------------------------------------------------------------------
//  modelnodeinstance.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "models/modelnodeinstance.h"
#include "models/modelinstance.h"
#include "debug/debugserver.h"      
#include "models/modelserver.h"

namespace Models
{
__ImplementClass(Models::ModelNodeInstance, 'MNDI', Core::RefCounted);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
ModelNodeInstance::ModelNodeInstance():
    isVisible(true),
    frameInstanceIndex(InvalidIndex)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ModelNodeInstance::~ModelNodeInstance()
{
    n_assert(!this->IsValid());
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<ModelInstance>&
ModelNodeInstance::GetModelInstance() const
{
    return this->modelInstance;
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<ModelNode>&
ModelNodeInstance::GetModelNode() const
{
    return this->modelNode;
}

//------------------------------------------------------------------------------
/**
*/
void
ModelNodeInstance::Setup(const Ptr<ModelInstance>& inst, const Ptr<ModelNode>& node, const Ptr<ModelNodeInstance>& parentNodeInst)
{
    n_assert(!this->IsValid());
    this->name = node->GetName();
    this->modelInstance = inst;
    this->modelNode = node;
    this->parent = parentNodeInst;
    if (this->parent.isvalid())
    {
        this->parent->AddChild(this);
    }
    this->modelInstance->AttachNodeInstance(this);

#if NEBULA3_ENABLE_PROFILING
    Util::String debugTimerName(this->GetRtti()->GetName());
    this->modelNodeInstanceTimer = Debug::DebugServer::Instance()->GetDebugTimerByName(debugTimerName);
    if (!this->modelNodeInstanceTimer.isvalid())
    {
        this->modelNodeInstanceTimer = Debug::DebugTimer::Create();
        this->modelNodeInstanceTimer->Setup(debugTimerName, "Model node instances");
    }   
#endif
}

//------------------------------------------------------------------------------
/**
*/
void
ModelNodeInstance::Discard()
{
    n_assert(this->IsValid());
    this->modelInstance->RemoveNodeInstance(this);
    this->parent = 0;
    this->children.Clear();
    this->childIndexMap.Clear();
    this->modelInstance = 0;
    this->modelNode = 0;

#if NEBULA3_ENABLE_PROFILING
    Util::String debugTimerName(this->GetRtti()->GetName());
    this->modelNodeInstanceTimer = Debug::DebugServer::Instance()->GetDebugTimerByName(debugTimerName);
    if (this->modelNodeInstanceTimer.isvalid())
    {
        _discard_timer(this->modelNodeInstanceTimer);
        this->modelNodeInstanceTimer = 0;
    }   
#endif
}

//------------------------------------------------------------------------------
/**
    Discards this model node instance and all of its children 
    recursively.
*/
void
ModelNodeInstance::DiscardHierarchy()
{
    n_assert(this->IsValid());

    // first discard children...
    IndexT i;
    for (i = 0; i < this->children.Size(); i++)
    {
        this->children[i]->DiscardHierarchy();
    }

    // discard myself and remove from model instance
    this->Discard();
}

//------------------------------------------------------------------------------
/**
*/
void
ModelNodeInstance::OnNotifyCullingVisible(IndexT frameIndex, Timing::Time time)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
ModelNodeInstance::OnRenderBefore(IndexT frameIndex, Timing::Time time)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelNodeInstance::OnVisibilityResolve(IndexT frameIndex, IndexT resolveIndex, float distanceToViewer)
{
    // we are getting rendered this frame, get a instance index from modelserver
    this->frameInstanceIndex = ModelServer::Instance()->ConsumeNewModelNodeInstanceIndex();
}

//------------------------------------------------------------------------------
/**
    The ApplyState() method is called when per-instance shader-state should 
    be applied. This method may be called several times per frame. The
    calling order is always in "render order", first, the ApplyState()
    method on the ModelNode will be called, then each ApplyState() and Render()
    method of the ModelNodeInstance objects.
*/
void
ModelNodeInstance::ApplyState(IndexT frameIndex, const IndexT& pass)
{
    // n_printf("ModelNodeInstance::Apply() called on '%s'!\n", this->modelNode->GetName().Value());
}

//------------------------------------------------------------------------------
/**
    The Render() method is called when the ModelNodeInstance needs to render
    itself. There will always be a call to the Apply() method before Render()
    is called, however, Render() may be called several times per Apply()
    invocation.
*/
void
ModelNodeInstance::Render()
{
    // n_printf("ModelNodeInstance::Render() called on '%s'!\n", this->modelNode->GetName().Value());
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelNodeInstance::RenderDebug()
{
   // empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelNodeInstance::RenderInstanced(SizeT numInstances)
{
    // empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
ModelNodeInstance::AddChild(const Ptr<ModelNodeInstance>& c)
{
    this->children.Append(c);
    this->childIndexMap.Add(c->GetName(), this->children.Size() - 1);
}

//------------------------------------------------------------------------------
/**
*/
bool
ModelNodeInstance::HasChild(const StringAtom& name) const
{
    return this->childIndexMap.Contains(name);
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<ModelNodeInstance>&
ModelNodeInstance::LookupChild(const StringAtom& name) const
{
    return this->children[this->childIndexMap[name]];
}

//------------------------------------------------------------------------------
/**
*/
Ptr<ModelNodeInstance>
ModelNodeInstance::LookupPath(const String& path)
{
    Array<String> tokens = path.Tokenize("/");
    n_assert(tokens.Size() > 0);
    StringAtom parentName("..");
    Ptr<ModelNodeInstance> curPtr = this;
    IndexT i;
    for (i = 0; i < tokens.Size(); i++)
    {
        StringAtom curToken = tokens[i];
        if (curToken == parentName)
        {
            // soft error handling: Nebula2 has 1 more parent which
            // isn't necessary
            if (curPtr->HasParent())
            {
                curPtr = curPtr->GetParent();
            }
        }
        else if (curPtr->HasChild(curToken))
        {
            curPtr = curPtr->LookupChild(curToken);
        }
        else
        {
            // return an invalid ptr
            return Ptr<ModelNodeInstance>();
        }
    }
    return curPtr;
}


//------------------------------------------------------------------------------
/**
*/
void
ModelNodeInstance::SetVisible(bool b, Timing::Time time, bool recursive)
{
    // handle myself
    if (b != this->isVisible)
    {
        this->isVisible = b;
        if (this->isVisible)
        {
            this->OnShow(time);
        }
        else
        {
            this->OnHide(time);
        }
    }

    // recurse into children
    if (recursive)
    {
        IndexT childIndex;
        for (childIndex = 0; childIndex < this->children.Size(); childIndex++)
        {
            this->children[childIndex]->SetVisible(b, time, true);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelNodeInstance::OnShow(Timing::Time time)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelNodeInstance::OnHide(Timing::Time time)
{
    // empty
}


} // namespace Models
