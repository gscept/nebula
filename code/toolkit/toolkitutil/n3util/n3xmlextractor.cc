//------------------------------------------------------------------------------
//  n3xmlextractor.cc
//  (C) 2011-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "toolkitutil/n3util/n3xmlextractor.h"

using namespace IO;
using namespace Util;

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::N3XmlExtractor, 'N3XE', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
N3XmlExtractor::N3XmlExtractor() : 
	stream(0),
	reader(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
N3XmlExtractor::~N3XmlExtractor()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
bool 
N3XmlExtractor::Open()
{
	n_assert(this->stream.isvalid());
	n_assert(!this->reader.isvalid());

	this->reader = IO::XmlReader::Create();

	this->reader->SetStream(this->stream);
	if (!this->reader->GetStream()->IsOpen())
	{
		this->stream->Open();
	}
	return this->reader->Open();
}

//------------------------------------------------------------------------------
/**
*/
void 
N3XmlExtractor::Close()
{
	n_assert(this->reader.isvalid());
	n_assert(this->reader->IsOpen());
	this->reader->Close();

	this->reader = nullptr;
	this->stream = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void 
N3XmlExtractor::SetStream( const Ptr<IO::Stream>& stream )
{
	n_assert(stream.isvalid());
	this->stream = stream;
	if (this->stream->IsOpen())
	{
		this->stream->Seek(0, Stream::Begin);
	}	
}

//------------------------------------------------------------------------------
/**
*/
void 
N3XmlExtractor::ExtractState( const Util::String& targetNode, State& state )
{
	Texture tex;
	Variable var;
	this->reader->SetToRoot();
	this->reader->SetToFirstChild();

	this->reader->SetToNode("ModelNode");
	this->RecursiveParseState(targetNode, tex, var, state);
}

//------------------------------------------------------------------------------
/**
*/
bool 
N3XmlExtractor::RecursiveParseState( Util::String targetNodeWithPath, Texture& tex, Variable& var, State& state )
{
	String currentNodeName = this->reader->GetCurrentNodeName();
	if (targetNodeWithPath.IsEmpty())
	{
		if (currentNodeName == "STXT")
		{
			this->reader->SetToFirstChild("s");
			String texName = this->reader->GetContent();
			this->reader->SetToNextChild("s");
			String texVal = this->reader->GetContent();
			tex.textureName = texName;
			tex.textureResource = texVal;

			state.textures.Append(tex);
			this->reader->SetToParent();
		}
		else if (currentNodeName == "SFLT")
		{
			this->reader->SetToFirstChild("s");
			String varName = this->reader->GetContent();
			this->reader->SetToNextChild("f");
			float varVal = this->reader->GetContent().AsFloat();
			var.variableName = varName;
			var.variableValue = varVal;

			state.variables.Append(var);
			this->reader->SetToParent();
		}
		else if (currentNodeName == "SINT")
		{
			this->reader->SetToFirstChild("s");
			String varName = this->reader->GetContent();
			this->reader->SetToNextChild("i");
			int varVal = this->reader->GetContent().AsInt();
			var.variableName = varName;
			var.variableValue = varVal;

			state.variables.Append(var);
			this->reader->SetToParent();
		}
		else if (currentNodeName == "SVEC")
		{
			this->reader->SetToFirstChild("s");
			String varName = this->reader->GetContent();
			this->reader->SetToNextChild("f4");
			Math::float4 varVal = this->reader->GetContent().AsFloat4();
			var.variableName = varName;
			var.variableValue = varVal;

			state.variables.Append(var);
			this->reader->SetToParent();
		}
		else if (currentNodeName == "SBOO")
		{
			this->reader->SetToFirstChild("s");
			String varName = this->reader->GetContent();
			this->reader->SetToNextChild("b");
			bool varVal = this->reader->GetContent().AsBool();
			var.variableName = varName;
			var.variableValue = varVal;

			state.variables.Append(var);
			this->reader->SetToParent();
		}
	
	}
	else
	{
		int slashIndex = targetNodeWithPath.FindCharIndex('/');
		String nodeSearch = targetNodeWithPath.Tokenize("/").Front();
		if (this->reader->HasAttr("name"))
		{
			String nodeName = this->reader->GetString("name");
			if (nodeName == nodeSearch)
			{
				if (slashIndex != InvalidIndex)
				{
					targetNodeWithPath = targetNodeWithPath.ExtractToEnd(slashIndex);
					targetNodeWithPath.TrimLeft("/");
				}
				else
				{
					targetNodeWithPath.Clear();
				}
				if (this->reader->SetToFirstChild()) do
				{
					if (!RecursiveParseState(targetNodeWithPath, tex, var, state))
					{
						return false;
					}	
				}
				while (this->reader->SetToNextChild());
			}
		}
	}
	

	// done recursing
	return true;
}



//------------------------------------------------------------------------------
/**
*/
void 
N3XmlExtractor::ExtractMaterial( const Util::String& targetNode, Util::String& material )
{
	this->reader->SetToRoot();
	this->reader->SetToFirstChild();

	this->reader->SetToNode("ModelNode");
	this->RecursiveParseMaterial(targetNode, material);
}


//------------------------------------------------------------------------------
/**
*/
bool 
N3XmlExtractor::RecursiveParseMaterial( Util::String targetNodeWithPath, Util::String& material )
{
	String currentNodeName = this->reader->GetCurrentNodeName();
	if (targetNodeWithPath.IsEmpty())
	{
		if (currentNodeName == "MNMT")
		{
			this->reader->SetToFirstChild("s");
			material = this->reader->GetContent();
		}
	}
	else
	{
		int slashIndex = targetNodeWithPath.FindCharIndex('/');
		String nodeSearch = targetNodeWithPath.Tokenize("/").Front();
		if (this->reader->HasAttr("name"))
		{
			String nodeName = this->reader->GetString("name");

			// we found the parent we want to visit
			if (nodeName == nodeSearch)
			{
				// we found our node, clear the target
				if (nodeName == targetNodeWithPath)
				{
					targetNodeWithPath.Clear();
				}
				// we are still looking
				else if (slashIndex != InvalidIndex)
				{
					targetNodeWithPath = targetNodeWithPath.ExtractToEnd(slashIndex);	
					targetNodeWithPath.TrimLeft("/");
				}

				if (this->reader->SetToFirstChild()) do
				{
					if (!RecursiveParseMaterial(targetNodeWithPath, material))
					{
						return false;
					}	
				}
				while (this->reader->SetToNextChild());
			}
		}
	}

	return true;
}

//------------------------------------------------------------------------------
/**
*/
void 
N3XmlExtractor::ExtractType( const Util::String& targetNode, Util::String& type )
{
	this->reader->SetToRoot();
	this->reader->SetToFirstChild();

	this->reader->SetToNode("ModelNode");
	this->RecursiveParseType(targetNode, type);
}

//------------------------------------------------------------------------------
/**
*/
bool 
N3XmlExtractor::RecursiveParseType( Util::String targetNodeWithPath, Util::String& type )
{
	String currentNodeName = this->reader->GetCurrentNodeName();

	int slashIndex = targetNodeWithPath.FindCharIndex('/');
	String nodeSearch = targetNodeWithPath.Tokenize("/").Front();
	if (this->reader->HasAttr("name"))
	{
		String nodeName = this->reader->GetString("name");

		// we found the parent we want to visit
		if (nodeName == nodeSearch)
		{
			// we found our node, clear the target
			if (nodeName == targetNodeWithPath)
			{
				targetNodeWithPath.Clear();
			}
			// we are still looking
			else if (slashIndex != InvalidIndex)
			{
				targetNodeWithPath = targetNodeWithPath.ExtractToEnd(slashIndex);	
				targetNodeWithPath.TrimLeft("/");
			}

			// if target is empty, set type and return
			if (targetNodeWithPath.IsEmpty())
			{
				type = this->reader->GetString("className");
				return true;
			}

			if (this->reader->SetToFirstChild()) do
			{
				if (!RecursiveParseType(targetNodeWithPath, type))
				{
					return false;
				}	
			}
			while (this->reader->SetToNextChild());
		}
	}

	return true;
}

//------------------------------------------------------------------------------
/**
*/
void 
N3XmlExtractor::ExtractNodes( Util::Array<Util::String>& nodes )
{
	this->reader->SetToRoot();
	this->reader->SetToFirstChild();
	
	this->RecursiveParseNodes(nodes, false, "");
}

//------------------------------------------------------------------------------
/**
*/
bool 
N3XmlExtractor::RecursiveParseNodes( Util::Array<Util::String>& nodes, bool rootVisited, Util::String path )
{
	bool saveNode = false;

	String nodeId = "";
	String nodePath = "";
	if (this->reader->HasAttr("name") || this->reader->HasAttr("className"))
	{
		// get node name
		String type = this->reader->GetCurrentNodeName();

		// at the first occurence of a ModelNode, we will set the visited flag and thus traverse futher
		if (type == "ModelNode")
		{
			rootVisited = true;
		}

		if (rootVisited)
		{
			nodeId = this->reader->GetString("name");
			if (path.IsEmpty())
			{
				nodePath = nodeId;
			}
			else
			{
				nodePath = path + "/" + nodeId;
			}			
		}

		saveNode = true;

	}

	if (this->reader->SetToFirstChild()) do 
	{
		if (!RecursiveParseNodes(nodes, rootVisited, nodePath))
		{
			return false;
		}
	} 
	while (this->reader->SetToNextChild());

	if (rootVisited && saveNode)
	{
		nodes.Append(nodePath);
	}

	// recursion done!
	return true;
}

//------------------------------------------------------------------------------
/**
*/
void 
N3XmlExtractor::ExtractSceneBoundingBox( Math::bbox& box )
{
	this->reader->SetToRoot();
	this->reader->SetToFirstChild();

	if (this->reader->HasNode("ModelNode"))
	{
		this->reader->SetToNode("ModelNode");
		if (this->reader->SetToFirstChild("LBOX"))
		{
			Math::point center, extents;

			this->reader->SetToFirstChild();
			center = this->reader->GetContent().AsFloat4();
			this->reader->SetToNextChild();
			extents = this->reader->GetContent().AsFloat4();

			box = Math::bbox(center, extents);
		}	
	}


}

} // namespace ToolkitUtil