#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::N3XMLExtractor
    
    Extracts specific data from n3 xml file
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "n3modeldata.h"
#include "io/stream.h"
#include "io/xmlreader.h"
#include "math/bbox.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class N3XmlExtractor : public Core::RefCounted
{
	__DeclareClass(N3XmlExtractor);
public:
	/// constructor
	N3XmlExtractor();
	/// destructor
	virtual ~N3XmlExtractor();

	/// opens the extractor
	bool Open();
	/// closes the extractor
	void Close();

	/// sets the file stream
	void SetStream(const Ptr<IO::Stream>& stream);

	/// extracts a state from a given node
	void ExtractState(const Util::String& targetNode, State& state);
	/// extracts a material from a given node
	void ExtractMaterial(const Util::String& targetNode, Util::String& material);
	/// extracts nodes of a special type
	void ExtractNodes(Util::Array<Util::String>& nodes);
	/// extracts the scene bounding box
	void ExtractSceneBoundingBox(Math::bbox& box);
	/// extracts the type of node
	void ExtractType(const Util::String& targetNode, Util::String& type);
private:

	/// recursively parses a model and finds the state for a given node
	bool RecursiveParseState(Util::String targetNodeWithPath, Texture& tex, Variable& var, State& state);
	/// recursively parses the model and finds the material for a given node
	bool RecursiveParseMaterial(Util::String targetNodeWithPath, Util::String& material);
	/// recursively parses the model and finds the node type for a given node
	bool RecursiveParseType(Util::String targetNodeWithPath, Util::String& type);
	/// recursively parses the model and finds nodes of a given type
	bool RecursiveParseNodes(Util::Array<Util::String>& nodes, bool rootVisited, Util::String path);

	Ptr<IO::Stream> stream;
	Ptr<IO::XmlReader> reader;

	static const short TEX = 0;
	static const short VAR = 1;
	static const short INVALID = 2;
}; 
} // namespace ToolkitUtil
//------------------------------------------------------------------------------