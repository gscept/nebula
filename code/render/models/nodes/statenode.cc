//------------------------------------------------------------------------------
//  materialstatenode.cc
//  (C) 2011-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"

#include "models/nodes/statenode.h"
#include "models/nodes/statenodeinstance.h"
#include "materials/materialtype.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/shadervariable.h"
#include "materials/material.h"
#include "materials/materialserver.h"
#include "resources/resourcemanager.h"
#include "models/modelinstance.h"


namespace Models
{
__ImplementClass(Models::StateNode, 'STND', Models::TransformNode);


using namespace CoreGraphics;
using namespace Resources;
using namespace Materials;
using namespace Util;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
StateNode::StateNode() :
	stateLoaded(Resources::Resource::Initial)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
StateNode::~StateNode()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
Ptr<ModelNodeInstance>
StateNode::CreateNodeInstance() const
{
	Ptr<ModelNodeInstance> newInst = (ModelNodeInstance*) StateNodeInstance::Create();
	return newInst;
}

//------------------------------------------------------------------------------
/**
*/
bool
StateNode::ParseDataTag( const Util::FourCC& fourCC, const Ptr<IO::BinaryReader>& reader )
{
	bool retval = true;
	if (FourCC('MNMT') == fourCC)
	{
		// read material string, if the material doesn't exist anymore or has a faulty value, revert to the placeholder material
		const Ptr<MaterialServer>& matServer = MaterialServer::Instance();

        // this isn't used, it's been deprecated
		Util::String materialName = reader->ReadString();
	}
    else if (FourCC('MATE') == fourCC)
    {
        this->materialName = reader->ReadString();
    }
	else if (FourCC('STXT') == fourCC)
	{
		// ShaderTexture
		StringAtom paramName  = reader->ReadString();
		StringAtom paramValue = reader->ReadString();
		String fullTexResId = String(paramValue.AsString() + NEBULA3_TEXTURE_EXTENSION);
		this->shaderParams.Append(KeyValuePair<StringAtom,Variant>(paramName, Variant(fullTexResId)));
	}
	else if (FourCC('SINT') == fourCC)
	{
		// ShaderInt
		StringAtom paramName = reader->ReadString();
		int paramValue = reader->ReadInt();
		this->shaderParams.Append(KeyValuePair<StringAtom,Variant>(paramName, Variant(paramValue)));
	}
	else if (FourCC('SFLT') == fourCC)
	{
		// ShaderFloat
		StringAtom paramName = reader->ReadString();
		float paramValue = reader->ReadFloat();
		this->shaderParams.Append(KeyValuePair<StringAtom,Variant>(paramName, Variant(paramValue)));
	}
	else if (FourCC('SBOO') == fourCC)
	{
		// ShaderBool
		StringAtom paramName = reader->ReadString();
		bool paramValue = reader->ReadBool();
		this->shaderParams.Append(KeyValuePair<StringAtom,Variant>(paramName, Variant(paramValue)));
	}
	else if (FourCC('SFV2') == fourCC)
	{
		// ShaderVector
		StringAtom paramName = reader->ReadString();
		float2 paramValue = reader->ReadFloat2();
		this->shaderParams.Append(KeyValuePair<StringAtom,Variant>(paramName, Variant(paramValue)));
	}
	else if (FourCC('SFV4') == fourCC)
	{
		// ShaderVector
		StringAtom paramName = reader->ReadString();
		float4 paramValue = reader->ReadFloat4();
		this->shaderParams.Append(KeyValuePair<StringAtom,Variant>(paramName, Variant(paramValue)));
	}
	else if (FourCC('STUS') == fourCC)
	{   
		// @todo: implement universal indexed shader parameters!
		// shaderparameter used by multilayered nodes
		int index = reader->ReadInt();
		float4 paramValue = reader->ReadFloat4();
		String paramName("MLPUVStretch");
		paramName.AppendInt(index);
		this->shaderParams.Append(KeyValuePair<StringAtom,Variant>(paramName, Variant(paramValue)));
	}
	else if (FourCC('SSPI') == fourCC)
	{     
		// @todo: implement universal indexed shader parameters!
		// shaderparameter used by multilayered nodes
		int index = reader->ReadInt();
		float4 paramValue = reader->ReadFloat4();
		String paramName("MLPSpecIntensity");
		paramName.AppendInt(index);
		this->shaderParams.Append(KeyValuePair<StringAtom,Variant>(paramName, Variant(paramValue)));
	}
	else
	{
		retval = TransformNode::ParseDataTag(fourCC, reader);
	}
	return retval;
}


//------------------------------------------------------------------------------
/**
*/
void 
StateNode::LoadResources(bool sync)
{
    n_assert(this->materialName.IsValid());

    // load material, do it synchronously
    this->managedMaterial = ResourceManager::Instance()->CreateManagedResource(Surface::RTTI, this->materialName.AsString() + NEBULA3_SURFACE_EXTENSION, NULL, true).downcast<ManagedSurface>();
	TransformNode::LoadResources(sync);
}

//------------------------------------------------------------------------------
/**
*/
void 
StateNode::UnloadResources()
{
    // unload material
    ResourceManager::Instance()->DiscardManagedResource(this->managedMaterial.upcast<ManagedResource>());
    this->managedMaterial = NULL;
	TransformNode::UnloadResources();
}

//------------------------------------------------------------------------------
/**
*/
Resources::Resource::State
StateNode::GetResourceState() const
{
	Resources::Resource::State loadedState = TransformNode::GetResourceState();
	if (loadedState > this->stateLoaded) loadedState = this->stateLoaded;
	return loadedState;
}

//------------------------------------------------------------------------------
/**
*/
void
StateNode::ApplySharedState(IndexT frameIndex)
{
	// up to parent class
	TransformNode::ApplySharedState(frameIndex);
}

} // namespace Models
