//------------------------------------------------------------------------------
//  shaderserverbase.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
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

#ifdef USE_SHADER_DICTIONARY
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
#else
    Util::Array<Util::String> files = IoServer::Instance()->ListFiles("shd:", "*.fxb");
    for (IndexT i = 0; i < files.Size(); i++)
    {
        ResourceName resId = "shd:" + files[i];
        
        // load shader
        this->LoadShader(resId);
    }
#endif

    // create standard shader for access to shared variables
    if (this->shaders.Contains(ResourceName("shd:shared.fxb")))
    {
		this->sharedVariableShader = this->GetShader("shd:shared.fxb");
		n_assert(this->sharedVariableShader != ShaderId::Invalid());

        // get shared object id shader variable
#if !__WII__ && !__PS3__
       // this->objectIdShaderVar = this->sharedVariableShaderInst->GetVariableBySemantic(NEBULA_SEMANTIC_OBJECTID);
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
*/
void 
ShaderServerBase::ApplyObjectId(IndexT i)
{   
    n_assert(i >= 0);
    n_assert(i < 256);
#if __PS3__
    if (this->GetActiveShader()->HasVariableBySemantic(NEBULA_SEMANTIC_OBJECTID))
    {
        this->objectIdShaderVar = this->GetActiveShader()->GetVariableBySemantic(NEBULA_SEMANTIC_OBJECTID);
    }       
#endif
    if (this->objectIdShaderVar != Ids::InvalidId32)
    {
		//CoreGraphics::shaderPool->ShaderConstantSet(this->objectIdShaderVar, this->sharedVariableShaderState, ((float)i) / 255.0f);
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
