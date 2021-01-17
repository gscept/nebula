#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::XmlWriter
  
    Write XML-formatted data to a stream.
    
    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/    
#include "io/streamwriter.h"
#include "tinyxml/tinyxml.h"

//------------------------------------------------------------------------------
namespace IO
{
class XmlWriter : public IO::StreamWriter
{
    __DeclareClass(XmlWriter);
public:
    /// constructor
    XmlWriter();
    /// destructor
    virtual ~XmlWriter();
    /// begin writing the stream
    virtual bool Open();
    /// end writing the stream
    virtual void Close();

    /// begin a new node under the current node
    bool BeginNode(const Util::String& nodeName);
    /// end current node, set current node to parent
    void EndNode();
    /// write content text
    void WriteContent(const Util::String& text);
    /// write a comment
    void WriteComment(const Util::String& comment);
    /// write raw XML
    void WriteRaw(const Util::String& xml);

    /// set string attribute on current node
    void SetString(const Util::String& name, const Util::String& value);
    /// set bool attribute on current node
    void SetBool(const Util::String& name, bool value);
    /// set int attribute on current node
    void SetInt(const Util::String& name, int value);
    /// set float attribute on current node
    void SetFloat(const Util::String& name, float value);  
    #if !__OSX__
    /// set vec2 attribute on current node
    void SetVec2(const Util::String& name, const Math::vec2& value);
    /// set vec4 attribute on current node
    void SetVec4(const Util::String& name, const Math::vec4& value);
    /// set mat4 attribute on current node
    void SetMat4(const Util::String& name, const Math::mat4& value);
    /// set transform44 attribute on current node
    void SetTransform44(const Util::String& name, const Math::transform44& value);
    #endif
    /// generic setter, template specializations implemented in nebula3/code/addons/nebula2
    template<typename T> void Set(const Util::String& name, const T &value);

private:
    TiXmlDocument* xmlDocument;
    TiXmlElement* curNode;
};

} // namespace IO
//------------------------------------------------------------------------------

