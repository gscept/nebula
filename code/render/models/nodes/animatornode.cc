//------------------------------------------------------------------------------
//  animatornode.cc
//  (C) 2008 RadonLabs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "models/nodes/animatornode.h"
#include "models/nodes/animatornodeinstance.h"
#include "resources/resourcemanager.h"

namespace Models
{
__ImplementClass(Models::AnimatorNode, 'MANI', Models::ModelNode);

using namespace CoreAnimation;
using namespace Resources;

//------------------------------------------------------------------------------
/**
*/
AnimatorNode::AnimatorNode()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
AnimatorNode::~AnimatorNode()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Ptr<Models::ModelNodeInstance>
AnimatorNode::CreateNodeInstance() const
{
    Ptr<Models::ModelNodeInstance> newInst = (Models::ModelNodeInstance*) AnimatorNodeInstance::Create();
    return newInst;
}

//------------------------------------------------------------------------------
/**
    Unload animation resource if valid.
*/
void
AnimatorNode::UnloadAnimation()
{
    IndexT i;
    for (i = 0; i < this->animSection.Size(); i++)
    {
        if (this->animSection[i].managedAnimResource.isvalid())
        {
            ResourceManager::Instance()->DiscardManagedResource(this->animSection[i].managedAnimResource.upcast<ManagedResource>());
            this->animSection[i].managedAnimResource = 0;
        }
    }
}


//------------------------------------------------------------------------------
/**
    Load new animation, release old one if valid.
*/
bool
AnimatorNode::LoadAnimation()
{
    IndexT i;
    for (i = 0; i < this->animSection.Size(); i++)
    {
        if ((!this->animSection[i].managedAnimResource.isvalid()) && (!this->animSection[i].animationName.IsEmpty()))
        {
            // setup the managed resource
            this->animSection[i].managedAnimResource = ResourceManager::Instance()->CreateManagedResource(AnimResource::RTTI, this->animSection[i].animationName).downcast<ManagedAnimResource>();
            n_assert(this->animSection[i].managedAnimResource.isvalid());
        }
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    Load the resources needed by this object.
*/
void
AnimatorNode::LoadResources(bool sync)
{
    this->LoadAnimation();
    //this->resourcesValid = true;
}

//------------------------------------------------------------------------------
/**
    Unload the resources if refcount has reached zero.
*/
void
AnimatorNode::UnloadResources()
{
    //this->resourcesValid = false;
    this->UnloadAnimation();
}

//------------------------------------------------------------------------------
/**
*/
bool
AnimatorNode::ParseDataTag(const Util::FourCC& fourCC, const Ptr<IO::BinaryReader>& reader)
{
    bool retval = true;

    if (Util::FourCC('BASE') == fourCC)
    {
        AnimSection animSec;
        animSec.animationNodeType = (NodeType)reader->ReadInt(); 
        this->animSection.Append(animSec);
    }

    if (!this->animSection.IsEmpty())
    {
        AnimSection& curAnimSection = this->animSection.Back();
        AnimKey<Math::vector> val;
        Math::float4 vec;
        if (Util::FourCC('SLPT') == fourCC)
        {
            // LoopType
            Util::String s = reader->ReadString();
            if (s == "clamp")
            {
                curAnimSection.loopType = AnimLoopType::Clamp;
            }
            else if (s == "loop")
            {
                curAnimSection.loopType = AnimLoopType::Loop;
            }
            
        }
        else if (Util::FourCC('ANNO') == fourCC)
        {
            // AnimationPath
            curAnimSection.animatedNodesPath.Append(reader->ReadString());
        }
        else if ((Util::FourCC('SPNM') == fourCC) || (Util::FourCC('SVCN') == fourCC))
        {
            // ShaderVariable   
            Util::String semName = reader->ReadString();    
        #if __PS3__
            semName.ToUpper();
        #endif
            curAnimSection.shaderVarName = semName;
        
        }
        else if (Util::FourCC('ADPK') == fourCC)
        {
            // PositionKey
            SizeT animKeySize = (SizeT)reader->ReadInt();
            for (IndexT animKeyIdx = 0; animKeyIdx < animKeySize; animKeyIdx++)
            {
                if (curAnimSection.animationNodeType == UvAnimator)   //uvAnimator
                {
                    int layer = reader->ReadInt();
                    val.SetTime(reader->ReadFloat());
                    float u = reader->ReadFloat();
                    float v = reader->ReadFloat();
                    vec.set(u, v, 0, 0);
                    val.SetValue(vec);
                    //skip other values - not needed for uvanimator
                    reader->ReadFloat(); reader->ReadFloat();
                    curAnimSection.layer.Append(layer);
                }
                else    //transformanimator
                {
                    val.SetTime(reader->ReadFloat());
                    float x = reader->ReadFloat();
                    float y = reader->ReadFloat();
                    float z = reader->ReadFloat();
                    float w = reader->ReadFloat();
                    vec.set(x, y, z, w);
                    val.SetValue(vec);        
                }
                curAnimSection.posArray.Append(val);
            }
        }
        else if (Util::FourCC('ADEK') == fourCC)
        {
            // eulerkey
            SizeT animKeySize = (SizeT)reader->ReadInt();
            for (IndexT animKeyIdx = 0; animKeyIdx < animKeySize; animKeyIdx++)
            {
                if (curAnimSection.animationNodeType == UvAnimator)   //uvAnimator
                {
                    int layer = reader->ReadInt();
                    val.SetTime(reader->ReadFloat());
                    float u = reader->ReadFloat();
                    float v = reader->ReadFloat();
                    vec.set(u, v, 0, 0);
                    val.SetValue(vec);
                    //skip other values - not needed for uvanimator
                    reader->ReadFloat(); reader->ReadFloat();
                    curAnimSection.layer.Append(layer);
                }
                else    //transformanimator
                {
                    val.SetTime(reader->ReadFloat());
                    float x = reader->ReadFloat();
                    float y = reader->ReadFloat();
                    float z = reader->ReadFloat();
                    float w = reader->ReadFloat();
                    vec.set(x, y, z, w);
                    val.SetValue(vec);        
                }
                curAnimSection.eulerArray.Append(val);
            }
        }
        else if (Util::FourCC('ADSK') == fourCC)
        {
            // ScaleKey
            SizeT animKeySize = (SizeT)reader->ReadInt();
            for (IndexT animKeyIdx = 0; animKeyIdx < animKeySize; animKeyIdx++)
            {
                if (curAnimSection.animationNodeType == UvAnimator)   //uvAnimator
                {
                    int layer = reader->ReadInt();
                    val.SetTime(reader->ReadFloat());
                    float u = reader->ReadFloat();
                    float v = reader->ReadFloat();
                    vec.set(u, v, 0, 0);
                    val.SetValue(vec);
                    //skip other values - not needed for uvanimator
                    reader->ReadFloat(); reader->ReadFloat();
                    curAnimSection.layer.Append(layer);
                }
                else    //transformanimator
                {
                    val.SetTime(reader->ReadFloat());
                    float x = reader->ReadFloat();
                    float y = reader->ReadFloat();
                    float z = reader->ReadFloat();
                    float w = reader->ReadFloat();
                    vec.set(x, y, z, w);
                    val.SetValue(vec);        
                }
                curAnimSection.scaleArray.Append(val);
            }
        }
        else if (Util::FourCC('SANI') == fourCC)
        {
            // Animation
            curAnimSection.animationName = reader->ReadString();
        }
        else if (Util::FourCC('SAGR') == fourCC)
        {
            // AnimationGroup
            curAnimSection.animationGroup = reader->ReadInt();
        }
        else if (Util::FourCC('ADDK') == fourCC)
        {
            Util::String valueType = reader->ReadString();
            SizeT animKeySize = (SizeT)reader->ReadInt();
            for (IndexT animKeyIdx = 0; animKeyIdx < animKeySize; animKeyIdx++)
            {
                float time = reader->ReadFloat();
                if (valueType == "Float")
                {
                    AnimKey<float> value;
                    value.SetTime(time);
                    value.SetValue(reader->ReadFloat());
                    curAnimSection.floatKeyArray.Append(value);
                }
                else if (valueType == "Float4")
                {
                    AnimKey<Math::float4> value;
                    value.SetTime(time);
                    value.SetValue(reader->ReadFloat4());
                    curAnimSection.float4KeyArray.Append(value);
                }
                else if (valueType == "Int")
                {
                    AnimKey<int> value;
                    value.SetTime(time);
                    value.SetValue(reader->ReadInt());
                    curAnimSection.intKeyArray.Append(value);
                }
                else
                {
                    n_error("AnimatorNode::ParseDataTag: Unkown key type!");
                }
            }
        }

        // UVAnimator, set shader
        if (curAnimSection.animationNodeType == UvAnimator)
        {
            curAnimSection.shaderVarName = "TextureTransform0";
        }
    }
    return retval;
}

//------------------------------------------------------------------------------
/**
*/
Resources::Resource::State 
AnimatorNode::GetResourceState() const
{
    Resource::State state = ModelNode::GetResourceState();
    IndexT i;
    for (i = 0; i < this->animSection.Size(); ++i)
    {        
        if (this->animSection[i].managedAnimResource.isvalid()
            && this->animSection[i].managedAnimResource->GetState() > state)
        {
            state = this->animSection[i].managedAnimResource->GetState();
        }
    }
    
    return state;
}
}