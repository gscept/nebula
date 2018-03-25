//------------------------------------------------------------------------------
//  n3xmlmodifier.cc
//  (C) 2011-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "toolkitutil/n3util/n3xmlmodifier.h"
#include "io/ioserver.h"

using namespace IO;
using namespace Util;

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::N3XMLModifier, 'N3XM', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
N3XMLModifier::N3XMLModifier()	:
	readStream(0),
	writeStream(0)
{
	this->reader = XmlReader::Create();
	this->writer = XmlWriter::Create();
}

//------------------------------------------------------------------------------
/**
*/
N3XMLModifier::~N3XMLModifier()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
N3XMLModifier::SetReadStream( const Ptr<IO::Stream>& stream )
{
	n_assert(stream.isvalid());
	this->readStream = stream;
}


//------------------------------------------------------------------------------
/**
*/
void 
N3XMLModifier::SetWriteStream( const Ptr<IO::Stream>& stream )
{
	n_assert(stream.isvalid());
	this->writeStream = stream;
}

//------------------------------------------------------------------------------
/**
*/
void 
N3XMLModifier::Open()
{
	n_assert(!this->reader->IsOpen());
	n_assert(this->readStream.isvalid());

	this->reader->SetStream(this->readStream);

	if (!this->reader->GetStream()->IsOpen())
	{
		this->readStream->Open();
	}

	// seek to beginning of stream
	this->readStream->Seek(0, Stream::Begin);
	this->reader->Open();
}


//------------------------------------------------------------------------------
/**
*/
void 
N3XMLModifier::Close()
{
	n_assert(this->reader->IsOpen());
	this->reader->Close();
	this->readStream->Close();
}


//------------------------------------------------------------------------------
/**
*/
void 
N3XMLModifier::ChangeTag( const Util::String& targetType, const Util::String& targetNode, const Util::FourCC& tagName, IndexT tagIndex, Util::Array<Util::Variant> values )
{
	ChangeModify changeCommand;
	changeCommand.targetType = targetType;
	changeCommand.targetNode = targetNode;
	changeCommand.tagName = tagName;
	changeCommand.tagIndex = tagIndex;
	changeCommand.values = values;
	this->changeModifiers.Append(changeCommand);
}

//------------------------------------------------------------------------------
/**
*/
void 
N3XMLModifier::AddTag( const Util::String& targetType, const Util::String& targetNode, const Util::FourCC& tagName, Util::Array<Util::Variant> values, const Util::String& comment )
{
	AddModify addCommand;
	addCommand.targetType = targetType;
	addCommand.targetNode = targetNode;
	addCommand.tagName = tagName;
	addCommand.values = values;
	addCommand.comment = comment;
	this->addModifiers.Append(addCommand);
}

//------------------------------------------------------------------------------
/**
*/
void
N3XMLModifier::AddTag( const Util::String& targetType, const Util::String& targetNode, const Util::FourCC& tagName, Util::Array<Util::Variant> values )
{
	AddModify addCommand;
	addCommand.targetType = targetType;
	addCommand.targetNode = targetNode;
	addCommand.tagName = tagName;
	addCommand.values = values;
	this->addModifiers.Append(addCommand);
}

//------------------------------------------------------------------------------
/**
*/
void 
N3XMLModifier::RemoveTag( const Util::String& targetType, const Util::String& targetNode, const Util::FourCC& tagName, IndexT tagIndex )
{
	RemoveModify removeCommand;
	removeCommand.targetType = targetType;
	removeCommand.targetNode = targetNode;
	removeCommand.tagName = tagName;
	removeCommand.tagIndex = tagIndex;
	this->removeModifiers.Append(removeCommand);
}

//------------------------------------------------------------------------------
/**
*/
void 
N3XMLModifier::Modify()
{
	n_assert(this->writeStream.isvalid());
	this->writeStream->SetAccessMode(Stream::WriteAccess);
	this->writeStream->Open();

	this->writer->SetStream(writeStream);
	this->writer->Open();

	// reset reader to point to root node
	this->reader->SetToRoot();

	Node root;
	root.parent = 0;
	root.nodeName = this->reader->GetCurrentNodeName();

	Node* firstNode = new Node;
	firstNode->parent = &root;

	// sets to model
	this->reader->SetToFirstChild();

	firstNode->nodeName = this->reader->GetCurrentNodeName();
	Array<Node*> children;
	children.Append(firstNode);
	root.children.Append(firstNode);
	root.childrenByType.Add(firstNode->nodeName, children);
	this->RecursiveParse(firstNode);
	this->RecursiveWrite(&root);

	// reset tag counter
	this->tagCounter.Clear();

	// close writer stream
	this->writer->Close();
	this->writer->GetStream()->Close();
}

//------------------------------------------------------------------------------
/**
*/
bool
N3XMLModifier::RecursiveParse( ToolkitUtil::Node* node )
{
	// add index to tag counter
	if (!this->tagCounter.Contains(node->nodeName))
	{
		this->tagCounter.Add(node->nodeName, 0);
	}

	Array<String> attrs = this->reader->GetAttrs();
	
	for (int i = 0; i < attrs.Size(); i++)
	{
		node->attributes.Add(attrs[i], this->reader->GetString(attrs[i].AsCharPtr()));
	}

	if (this->reader->HasComment())
	{
		node->comment = this->reader->GetComment();
	}

	if (this->reader->HasContent())
	{
		if (node->nodeName == "f")
		{
			node->content = this->reader->GetContent().AsFloat();
		}
		else if (node->nodeName == "f4")
		{
			node->content = this->reader->GetContent().AsFloat4();
		}
		else if (node->nodeName == "i")
		{
			node->content = this->reader->GetContent().AsInt();
		}
		else if (node->nodeName == "b")
		{
			node->content = this->reader->GetContent().AsBool();
		}
		else
		{
			node->content = this->reader->GetContent();
		}	
	}
	
	if (this->reader->SetToFirstChild()) do
	{
		Node* newNode = new Node;
		newNode->parent = node;
		newNode->nodeName = this->reader->GetCurrentNodeName();
		node->children.Append(newNode);

		if (!RecursiveParse(newNode))
		{
			// if we are done recursing a model node, reset tag counter
			if (newNode->nodeName == "ModelNode")
			{
				this->tagCounter.Clear();
			}
			return false;
		}	
	}
	while (this->reader->SetToNextChild());

	// look through changes
	for (int i = 0; i < this->changeModifiers.Size(); i++)
	{
		ChangeModify changeModifier = this->changeModifiers[i];
		if (changeModifier.tagIndex == this->tagCounter[node->nodeName]
			&& node->nodeName.Length() == 4
			&& changeModifier.tagName == node->nodeName
			&& changeModifier.targetNode == node->parent->attributes["name"]
			&& changeModifier.targetType == node->parent->nodeName)
		{
			for (int j = 0; j < changeModifier.values.Size(); j++)
			{
				node->children[j]->content = changeModifier.values[j];
			}
		}
	}

	for (int i = 0; i < this->removeModifiers.Size(); i++)
	{
		RemoveModify removeModifier = this->removeModifiers[i];
		if (removeModifier.tagIndex == this->tagCounter[node->nodeName]
			&& node->nodeName.Length() == 4
			&& removeModifier.tagName == node->nodeName
			&& removeModifier.targetNode == node->parent->attributes["name"]
		&& removeModifier.targetType == node->parent->nodeName)
		{
			node->ignore = true;
		}
	}


	for (int i = 0; i < this->addModifiers.Size(); i++)
	{
		AddModify addModify = this->addModifiers[i];
		if (node->attributes.Contains("name")
			&& addModify.targetNode == node->attributes["name"]
			&& addModify.targetType == node->nodeName)
		{
			Node* newNode = new Node;
			newNode->nodeName = addModify.tagName.AsString();
			newNode->parent = node; 
			if (addModify.comment.IsValid())
			{
				newNode->comment = addModify.comment;
			}			
			node->children.Append(newNode);
			if (!this->tagCounter.Contains(newNode->nodeName))
			{
				this->tagCounter.Add(newNode->nodeName, 0);
			}
			for (int j = 0; j < addModify.values.Size(); j++)
			{
				Variant value = addModify.values[j];
				Node* valueNode = new Node;
				valueNode->content = value;
				switch (value.GetType())
				{
				case Variant::Float:
					valueNode->nodeName = "f";					
					break;
				case Variant::Float4:
					valueNode->nodeName = "f4";
					break;
				case Variant::Int:
					valueNode->nodeName = "i";
					break;
				case Variant::Bool:
					valueNode->nodeName = "b";
					break;
				case Variant::String:
					valueNode->nodeName = "s";
					break;
				}
				valueNode->parent = newNode;
				newNode->children.Append(valueNode);
			}
		}
	}

	// increment tag counter
	this->tagCounter[node->nodeName]++;

	// recursion done
	return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
N3XMLModifier::RecursiveWrite( const ToolkitUtil::Node* node )
{
	if (!node->ignore)
	{
		if (node->comment.IsValid())
		{
			this->writer->WriteComment(node->comment);
		}
		
		this->writer->BeginNode(node->nodeName);
		for (int i = 0; i < node->attributes.Size(); i++)
		{
			this->writer->SetString(node->attributes.KeyAtIndex(i), node->attributes.ValueAtIndex(i));
		}

		switch (node->content.GetType())
		{
		case Variant::Float:
			this->writer->WriteContent(String::FromFloat(node->content.GetFloat()));
			break;
		case Variant::Float4:
			this->writer->WriteContent(String::FromFloat4(node->content.GetFloat4()));
			break;
		case Variant::Int:
			this->writer->WriteContent(String::FromInt(node->content.GetInt()));
			break;
		case Variant::Bool:
			this->writer->WriteContent(String::FromBool(node->content.GetBool()));
			break;
		case Variant::String:
			this->writer->WriteContent(node->content.GetString());
			break;
		}

		for (int i = 0; i < node->children.Size(); i++)
		{
			if (!this->RecursiveWrite(node->children[i]))
			{
				return false;
			}
		}
		this->writer->EndNode();
	}

	// cleans up the nodes, except for the top-level node, since it's created on the stack
	if (node->parent)
	{
		delete node;
	}	

	// recursion done
	return true;
}

//------------------------------------------------------------------------------
/**
*/
void
N3XMLModifier::ClearChanges()
{
	this->removeModifiers.Clear();
	this->addModifiers.Clear();
	this->changeModifiers.Clear();
}

} // namespace ToolkitUtil