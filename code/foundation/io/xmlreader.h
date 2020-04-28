#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::XmlReader
  
    Reads XML formatted data with random access from a stream using 
    TinyXML as backend. The XML document is represented as a tree of
    nodes, which can be navigated and queried.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "io/streamreader.h"
#if !__OSX__
#include "math/vec4.h"
#include "math/vec2.h"
#include "math/mat4.h"
#endif
#include "tinyxml/tinyxml.h"

//------------------------------------------------------------------------------
namespace IO
{
class XmlReader : public StreamReader
{
    __DeclareClass(XmlReader);
public:
    /// constructor
    XmlReader();
    /// destructor
    virtual ~XmlReader();

    /// begin reading from the stream
    virtual bool Open();
    /// end reading from the stream
    virtual void Close();

    /// return true if node exists 
    bool HasNode(const Util::String& path) const;
    /// get short name of current node
    Util::String GetCurrentNodeName() const;
    /// get path to current node
    Util::String GetCurrentNodePath() const;
    /// returns the line number of the current node
    int GetCurrentNodeLineNumber() const;
    
    /// sets the current node to root
    void SetToRoot();
    /// set current node as path
    void SetToNode(const Util::String& path);
    /// set current node to first child node, return false if no child exists
    bool SetToFirstChild(const Util::String& name = "");
    /// set current node to next sibling node, return false if no more sibling exists
    bool SetToNextChild(const Util::String& name = "");
    /// set current node to parent, return false if no parent exists
    bool SetToParent();

    /// return true if matching attribute exists on current node
    bool HasAttr(const char* attr) const;
    /// return names of all attrs on current node
    Util::Array<Util::String> GetAttrs() const;

    /// return true if current node has embedded content
    bool HasContent() const;
    /// return embedded content of current node
    Util::String GetContent() const;

    /// return true if current node has trailing comment
    bool HasComment() const;
    /// return trailing comment
    Util::String GetComment() const;

    /// get string attribute value from current node
    Util::String GetString(const char* attr) const;
    /// get bool attribute value from current node
    bool GetBool(const char* attr) const;
    /// get int attribute value from current node
    int GetInt(const char* attr) const;
    /// get float attribute value from current node
    float GetFloat(const char* attr) const;
    #if !__OSX__
    /// get vec2 attribute value from current node
    Math::vec2 GetVec2(const char* attr) const;
    /// get vec4 attribute value from current node
    Math::vec4 GetVec4(const char* attr) const;
    /// get mat4 attribute value from current node
    Math::mat4 GetMat4(const char* attr) const;
	/// get transform44 attribute value from current node
	Math::transform44 GetTransform44(const char* attr) const;
    #endif
    /// generic getter for extension types
    template<typename T> T Get(const char* attr) const;

    /// get optional string attribute value from current node
    Util::String GetOptString(const char* attr, const Util::String& defaultValue) const;
    /// get optional bool attribute value from current node
    bool GetOptBool(const char* attr, bool defaultValue) const;
    /// get optional int attribute value from current node
    int GetOptInt(const char* attr, int defaultValue) const;
    /// get optional float attribute value from current node
    float GetOptFloat(const char* attr, float defaultValue) const;
    #if !__OSX__
    /// get vec2 attribute value from current node
    Math::vec2 GetOptVec2(const char* attr, const Math::vec2& defaultValue) const;
    /// get optional vec4 attribute value from current node
    Math::vec4 GetOptVec4(const char* attr, const Math::vec4& defaultValue) const;
    /// get optional mat4 attribute value from current node
    Math::mat4 GetOptMat4(const char* attr, const Math::mat4& defaultValue) const;
	/// get transform44 attribute value from current node
	Math::transform44 GetOptTransform44(const char* attr, const Math::transform44& defaultValue) const;
    #endif
    
private:
    /// find a node by path, handles relativ paths as well
    TiXmlNode* FindNode(const Util::String& path) const;

    /// static helper object for TinyXml-Initialization at application startup
    class TinyXmlInitHelper
    {
    public:
        /// constructor
        TinyXmlInitHelper()
        {
            TiXmlBase::SetCondenseWhiteSpace(true);
        }
    };

    static TinyXmlInitHelper initTinyXml;
    TiXmlDocument* xmlDocument;
    TiXmlElement* curNode;
};

} // namespace IO
//------------------------------------------------------------------------------
