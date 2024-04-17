#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::BXmlReader
    
    Stream reader for binary XML files. The interface is similar
    to XmlReader so that it is easy to switch between the 2 classes.
    Use the N3 tool binaryxmlconverter3.exe to convert an XML file to
    a binary XML file.
    
    @copyright
    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "io/streamreader.h"
#include "io/util/bxmlloaderutil.h"

//------------------------------------------------------------------------------
namespace IO
{
class BXmlReader : public StreamReader
{
    __DeclareClass(BXmlReader);
public:
    /// constructor
    BXmlReader();
    /// destructor
    virtual ~BXmlReader();
    
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
    /// get vec2 attribute value from current node
    Math::float2 GetOptVec2(const char* attr, const Math::float2& defaultValue) const;
    /// get optional vec4 attribute value from current node
    Math::float4 GetOptVec4(const char* attr, const Math::float4& defaultValue) const;
    /// get optional mat4 attribute value from current node
    Math::mat4 GetOptMat4(const char* attr, const Math::mat4& defaultValue) const;
    #endif
    
private:
    BXmlLoaderUtil loaderUtil;
};

} // namespace IO
//------------------------------------------------------------------------------
