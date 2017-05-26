//------------------------------------------------------------------------------
//  shaderbase.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/base/shaderbase.h"
#include "coregraphics/shaderstate.h"
#include "coregraphics/shader.h"
#include "coregraphics/shaderserver.h"
#include "shaderserverbase.h"

namespace Base
{
__ImplementClass(Base::ShaderBase, 'SHDB', Resources::Resource);

using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
ShaderBase::ShaderBase() :
    inBeginUpdate(false),
	mainState(NULL)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ShaderBase::~ShaderBase()
{
    n_assert(0 == this->shaderInstances.Size());
    n_assert(this->variations.IsEmpty());
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderBase::Unload()
{
	// assume instances are empty
    n_assert(0 == this->shaderInstances.Size());

	IndexT i;
	for (i = 0; i < this->variations.Size(); i++)
	{
		this->variations.ValueAtIndex(i);
	}
    this->variations.Clear();

    Resource::Unload();
}

//------------------------------------------------------------------------------
/**
*/
Ptr<CoreGraphics::ShaderState>
ShaderBase::CreateState(const Util::Array<IndexT>& groups, bool createResourceSet)
{
	Ptr<ShaderState> newInst = ShaderState::Create();
	Ptr<ShaderBase> thisPtr(this);
	newInst->Setup(thisPtr.downcast<Shader>(), groups, createResourceSet);
	this->shaderInstances.Append(newInst);
	return newInst;
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderBase::DiscardShaderInstance(const Ptr<ShaderState>& inst)
{
    inst->Cleanup();
    IndexT i = this->shaderInstances.FindIndex(inst);
    n_assert(InvalidIndex != i);
    this->shaderInstances.EraseIndex(i);
}

//------------------------------------------------------------------------------
/**
*/
bool
ShaderBase::SelectActiveVariation(CoreGraphics::ShaderFeature::Mask featureMask)
{
    IndexT i = this->variations.FindIndex(featureMask);
    if (InvalidIndex != i)
    {
        const Ptr<ShaderVariation>& shdVar = this->variations.ValueAtIndex(i);
        const Ptr<ShaderServer>& shdSrv = ShaderServer::Instance();
        if (shdVar != this->activeVariation)
        {
            this->activeVariation = shdVar;
            return true;
        }
    }
    else
    {
        n_error("Unknown shader variation '%s' in shader '%s'\n",
            ShaderServer::Instance()->FeatureMaskToString(featureMask).AsCharPtr(),
            this->GetResourceId().Value());
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderBase::BeginUpdate()
{
    n_assert(!this->inBeginUpdate);
    this->inBeginUpdate = true;
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderBase::EndUpdate()
{
    n_assert(this->inBeginUpdate);
    this->inBeginUpdate = false;
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderBase::Apply()
{
    n_assert(this->activeVariation.isvalid());
    this->activeVariation->Apply();
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderBase::Commit()
{
    n_assert(this->activeVariation.isvalid());
    this->activeVariation->Commit();
}

} // namespace Base