//------------------------------------------------------------------------------
//  scenenodetree.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "scenenodetree.h"

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::SceneNodeTree, 'SNTR', Core::RefCounted);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
SceneNodeTree::SceneNodeTree() :
    isValid(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
SceneNodeTree::~SceneNodeTree()
{
    if (this->IsValid())
    {
        this->Discard();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
SceneNodeTree::Setup()
{
    n_assert(!this->IsValid());
    this->isValid = true;
}

//------------------------------------------------------------------------------
/**
*/
void
SceneNodeTree::Discard()
{
    n_assert(this->IsValid());

    // discard all nodes
    IndexT i;
    for (i = 0; i < this->nodes.Size(); i++)
    {
        this->nodes[i]->Discard();
    }
    this->nodes.Clear();

    this->isValid = false;
}

//------------------------------------------------------------------------------
/**
*/
bool
SceneNodeTree::IsValid() const
{
    return this->isValid;
}

//------------------------------------------------------------------------------
/**
*/
void
SceneNodeTree::AddNode(const Ptr<SceneNode>& node)
{
    n_assert(node.isvalid());
    this->nodes.Append(node);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
SceneNodeTree::GetNumNodes() const
{
    return this->nodes.Size();
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<SceneNode>&
SceneNodeTree::GetNodeByIndex(IndexT i) const
{
    return this->nodes[i];
}

//------------------------------------------------------------------------------
/**
*/
Ptr<SceneNode>
SceneNodeTree::LookupNode(const String& path) const
{
    n_assert(this->IsValid());
    n_assert(path.IsValid());
    Ptr<SceneNode> node;

    // shortcut if no nodes defined
    if (this->nodes.IsEmpty())
    {
        return node;
    }

    // path must be absolute
    n_assert(path[0] == '/');
    Array<String> tokens = path.Tokenize("/");
    n_assert(tokens.Size() >= 1);
    n_assert(this->nodes[0]->GetName() == tokens[0]);
    node = this->nodes[0];
    
    // iterate through path components
    int i;
    int num = tokens.Size();
    for (i = 1; (i < num) && node.isvalid(); i++)
    {
        const String& cur = tokens[i];
        if ("." == cur)
        {
            // do nothing
        }
        else if (".." == cur)
        {
            // go to parent node
            if (node->HasParent())
            {
                node = node->GetParent();
            }
            else
            {
                n_error("SceneNodeTree::LookupNode(%s): path points above root node!", path.AsCharPtr());
                return Ptr<SceneNode>();
            }
        }
        else
        {
            // find child node
            node = node->GetChildByName(cur);
        }
    }
    return node;
}

//------------------------------------------------------------------------------
/**
*/
bool
SceneNodeTree::HasNode(const String& path) const
{
    return this->LookupNode(path).isvalid();
}

//------------------------------------------------------------------------------
/**
*/
Ptr<SceneNode>
SceneNodeTree::GetRootNode() const
{
    if (this->nodes.Size() > 0)
    {
        return this->nodes[0];
    }
    else
    {
        return Ptr<SceneNode>();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
SceneNodeTree::Dump(Logger& logger)
{
    IndexT nodeIndex;
    for (nodeIndex = 0; nodeIndex < this->nodes.Size(); nodeIndex++)
    {
        const Ptr<SceneNode>& curNode = this->nodes[nodeIndex];
        const StringAtom& nodeName = curNode->GetName();
        const StringAtom& nodeType = curNode->GetType();
        const Array<SceneNode::Attr> attrs = curNode->GetAllAttrs();

        logger.Print("== type(%s) name(%s) parent(%s):\n", 
            nodeType.Value(), nodeName.Value(), 
            curNode->HasParent() ? curNode->GetParent()->GetName().AsString().AsCharPtr() : "none");
        IndexT attrIndex;
        for (attrIndex = 0; attrIndex < attrs.Size(); attrIndex++)
        {
            const SceneNode::Attr curAttr = attrs[attrIndex];
            String attrValuesString;
            IndexT valueIndex;
            for (valueIndex = 0; valueIndex < curAttr.values.Size(); valueIndex++)
            {
                const Variant& curValue = curAttr.values[valueIndex];
                attrValuesString.Append(Variant::TypeToString(curValue.GetType()));
                attrValuesString.Append(" ");
                switch (curValue.GetType())
                {
                    case Variant::Int:          attrValuesString.AppendInt(curValue.GetInt()); break;
                    case Variant::Float:        attrValuesString.AppendFloat(curValue.GetFloat()); break;
                    case Variant::Bool:         attrValuesString.AppendBool(curValue.GetBool()); break;
                    case Variant::Float4:       attrValuesString.AppendFloat4(curValue.GetFloat4()); break;
                    case Variant::Matrix44:     attrValuesString.AppendMatrix44(curValue.GetMatrix44()); break;
                    case Variant::String:       attrValuesString.Append(curValue.GetString()); break;
                    case Variant::IntArray:     attrValuesString.Append("{int array}"); break;
                    case Variant::FloatArray:   attrValuesString.Append("{float array}"); break;
                    case Variant::Float4Array:  attrValuesString.Append("{float4 array}"); break;
                    case Variant::StringArray:  attrValuesString.Append("{string array}"); break;
                    default:                    attrValuesString.Append("???"); break;
                }
                if (valueIndex < (curAttr.values.Size() - 1))
                {
                    attrValuesString.Append(", ");
                }
            }
            logger.Print("    %s %s (%s)\n", curAttr.name.Value(), curAttr.fourCC.AsString().AsCharPtr(), attrValuesString.AsCharPtr());
        }
    }
}

} // namespace ToolkitUtil