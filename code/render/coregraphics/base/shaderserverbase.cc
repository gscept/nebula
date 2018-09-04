//------------------------------------------------------------------------------
//  shaderserverbase.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/base/shaderserverbase.h"
#include "coregraphics/shaderpool.h"
#include "io/ioserver.h"
#include "io/textreader.h"
#include "coregraphics/shadersemantics.h"
#include "coregraphics/config.h"
#include "resources/resourcemanager.h"

namespace Base
{
__ImplementClass(Base::ShaderServerBase, 'SSRV', Core::RefCounted);
__ImplementSingleton(Base::ShaderServerBase);

using namespace CoreGraphics;
using namespace IO;
using namespace Util;
using namespace Resources;

//------------------------------------------------------------------------------
/**
*/
ShaderServerBase::ShaderServerBase() :
	sharedVariableShaderState(Ids::InvalidId64),
	objectIdShaderVar(Ids::InvalidId32),
    curShaderFeatureBits(0),        
    activeShader(NULL),
    isOpen(false)
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
ShaderServerBase::~ShaderServerBase()
{
    n_assert(!this->IsOpen());
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
bool
ShaderServerBase::Open()
{
    n_assert(!this->isOpen);
    n_assert(this->shaders.IsEmpty());

    // open the shaders dictionary file
    Ptr<Stream> stream = IoServer::Instance()->CreateStream("shd:shaders.dic");
    Ptr<TextReader> textReader = TextReader::Create();
    textReader->SetStream(stream);
    if (textReader->Open())
    {
        Array<String> shaderPaths = textReader->ReadAllLines();
        textReader->Close();
        textReader = nullptr;
        
        IndexT i;
        for (i = 0; i < shaderPaths.Size(); i++)
        {
			const Util::String& path = shaderPaths[i];
            ResourceName resId = shaderPaths[i];

			// load
			this->LoadShader(resId);
        }
    }
    else
    {
        n_error("ShaderServerBase: Failed to open shader dictionary!\n");
    }

    // create standard shader for access to shared variables
    if (this->shaders.Contains(ResourceName("shd:shared")))
    {
		this->sharedVariableShader = this->GetShader("shd:shared");
		n_assert(this->sharedVariableShader != ShaderId::Invalid());
		this->sharedVariableShaderState = CoreGraphics::shaderPool->CreateState(this->sharedVariableShader, { NEBULAT_FRAME_GROUP, NEBULAT_TICK_GROUP, NEBULAT_DYNAMIC_OFFSET_GROUP }, false);
        n_assert(this->sharedVariableShaderState != ShaderStateId::Invalid());

        // get shared object id shader variable
#if !__WII__ && !__PS3__
       // this->objectIdShaderVar = this->sharedVariableShaderInst->GetVariableBySemantic(NEBULA3_SEMANTIC_OBJECTID);
        //n_assert(this->objectIdShaderVar.isvalid());
#endif
    }

    this->isOpen = true;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderServerBase::Close()
{
    n_assert(this->isOpen);

    // release shared instance shader
    if (this->sharedVariableShaderState != ShaderStateId::Invalid())
    {        
		CoreGraphics::shaderPool->DestroyState(this->sharedVariableShaderState);
    } 

    // unload all currently loaded shaders
    IndexT i;
    for (i = 0; i < this->shaders.Size(); i++)
    {
		CoreGraphics::shaderPool->DiscardResource(this->shaders.ValueAtIndex(i));
    }
    this->shaders.Clear();

    this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
	Create a shared state (that is, it can be split between many shaders, and have the same global variables)
*/
CoreGraphics::ShaderStateId
ShaderServerBase::ShaderCreateState(const Resources::ResourceName& resId, const Util::Array<IndexT>& groups, bool createResourceSet)
{
	n_assert(resId.IsValid());

	CoreGraphics::ShaderStateId state;

	// first check if the shader is already loaded
	if (!this->shaders.Contains(resId))
	{
		n_error("ShaderServer: shader '%s' not found!", resId.Value());
	}
	else
	{
		state = CoreGraphics::shaderPool->CreateState(this->shaders[resId], groups, createResourceSet);
	}

	// create a shader instance object from the shader
	return state;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ShaderStateId
ShaderServerBase::ShaderCreateSharedState(const Resources::ResourceName& resId, const Util::Array<IndexT>& groups)
{
	n_assert(resId.IsValid());

	CoreGraphics::ShaderStateId state;

	// format a signature
	Util::String signature = resId.Value();
	Util::Array<IndexT> sortedGroups = groups;
	sortedGroups.Sort();
	IndexT i;
	for (i = 0; i < sortedGroups.Size(); i++) signature.AppendInt(sortedGroups[i]);

	// if we don't have the shared state, create it, otherwise just return it
	if (this->sharedShaderStates.Contains(signature))
	{
		state = this->sharedShaderStates[signature];
	}
	else
	{
		state = CoreGraphics::shaderPool->CreateState(this->shaders[resId], groups, false);
		this->sharedShaderStates.Add(signature, state);
	}

	return state;
}

//------------------------------------------------------------------------------
/**
*/
void 
ShaderServerBase::ApplyObjectId(IndexT i)
{   
    n_assert(i >= 0);
    n_assert(i < 256);
#if __PS3__
    if (this->GetActiveShader()->HasVariableBySemantic(NEBULA3_SEMANTIC_OBJECTID))
    {
        this->objectIdShaderVar = this->GetActiveShader()->GetVariableBySemantic(NEBULA3_SEMANTIC_OBJECTID);
    }       
#endif
    if (this->objectIdShaderVar != Ids::InvalidId32)
    {
		CoreGraphics::shaderPool->ShaderConstantSet(this->objectIdShaderVar, this->sharedVariableShaderState, ((float)i) / 255.0f);
    }       
}

//------------------------------------------------------------------------------
/**
	IMPLEMENT ME!
*/
void
ShaderServerBase::ReloadShader(const Resources::ResourceId shader)
{
	n_assert(shader != Ids::InvalidId64);
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderServerBase::LoadShader(const Resources::ResourceName& shdName)
{
	n_assert(shdName.IsValid());
	CoreGraphics::ShaderId sid = CoreGraphics::shaderPool->CreateResource(shdName, "shaders"_atm, nullptr,
		[shdName](const ResourceId id)
	{
		n_error("Failed to load shader '%s'!", shdName.Value());
	}, true);
	
	this->shaders.Add(shdName, sid);
}
} // namespace Base
