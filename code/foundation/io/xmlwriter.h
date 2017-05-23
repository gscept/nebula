#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::XmlWriter
  
    Write XML-formatted data to a stream.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
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
    /// set float2 attribute on current node
    void SetFloat2(const Util::String& name, const Math::float2& value);
    /// set float4 attribute on current node
    void SetFloat4(const Util::String& name, const Math::float4& value);
    /// set matrix44 attribute on current node
    void SetMatrix44(const Util::String& name, const Math::matrix44& value);
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

