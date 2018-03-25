#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::N3XMLModifier
    
    Modifies the contents of an n3 XML model file
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "util/variant.h"
#include "io/xmlreader.h"
#include "io/xmlwriter.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
	struct ChangeModify
	{
		Util::String targetType;
		Util::String targetNode;
		Util::FourCC tagName;
		IndexT tagIndex;
		Util::Array<Util::Variant> values;
	};

	struct AddModify
	{
		Util::String targetType;
		Util::String targetNode;
		Util::FourCC tagName;
		Util::Array<Util::Variant> values;
		Util::String comment;
	};

	struct RemoveModify
	{
		Util::String targetType;
		Util::String targetNode;
		Util::FourCC tagName;
		IndexT tagIndex;
	};

	struct Node
	{
		/// constructor
		Node()
		{
			ignore = false;
		}
		/// destructor
		~Node()
		{
			attributes.Clear();
			childrenByType.Clear();
			children.Clear();
		}

		Util::String nodeName;
		Util::Dictionary<Util::String, Util::String> attributes;
		Node* parent;
		Util::Dictionary<Util::String, Util::Array<Node*> > childrenByType;
		Util::Array<Node*> children;
		Util::Variant content;
		bool ignore;
		Util::String comment;
	};

class N3XMLModifier : public Core::RefCounted
{
	__DeclareClass(N3XMLModifier);

public:
	/// constructor
	N3XMLModifier();
	/// destructor
	virtual ~N3XMLModifier();

	/// sets the stream in which the original data resides
	void SetReadStream(const Ptr<IO::Stream>& stream);
	/// sets the stream to where modified data should be written
	void SetWriteStream(const Ptr<IO::Stream>& stream);
	/// opens the modifier
	void Open();
	/// closes the modifier
	void Close();

	/// change the values of a tag for a specific node
	void ChangeTag( const Util::String& targetType,
					const Util::String& targetNode,
					const Util::FourCC& tagName, 
					IndexT tagIndex, 
					Util::Array<Util::Variant> values);

	/// adds a tag to a specific node
	void AddTag( const Util::String& targetType, 
				 const Util::String& targetNode, 
				 const Util::FourCC& tagName, 
				 Util::Array<Util::Variant> values, 
				 const Util::String& comment);

	/// adds a tag without a comment
	void AddTag( const Util::String& targetType, 
				 const Util::String& targetNode,
				 const Util::FourCC& tagName, 
				 Util::Array<Util::Variant> values);

	/// removes a tag from a specific node
	void RemoveTag( const Util::String& targetType, 
					const Util::String& targetNode, 
					const Util::FourCC& tagName, 
					IndexT tagIndex);

	/// clears the changed
	void ClearChanges();

	/// performs modifications
	void Modify();
private:

	/// recursively traverses the file and saves the modified XML as a node network
	bool RecursiveParse(ToolkitUtil::Node* node);
	/// recursively traverses a node network and writes it back to XML
	bool RecursiveWrite(const ToolkitUtil::Node* node);

	bool isOpen;
	Ptr<IO::Stream> readStream;
	Ptr<IO::Stream> writeStream;
	Ptr<IO::XmlReader> reader;
	Ptr<IO::XmlWriter> writer;

	Util::Dictionary<Util::String, uint> tagCounter;

	Util::Array<RemoveModify> removeModifiers;
	Util::Array<AddModify> addModifiers;
	Util::Array<ChangeModify> changeModifiers;
}; 
} // namespace ToolkitUtil
//------------------------------------------------------------------------------