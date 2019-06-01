#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::JsonReader
  
    Reads json formatted data with random access from a stream using 
    pjson as backend. The json document is represented as a tree of
    nodes, which can be navigated and queried.
        
    (C) 2018 Individual contributors, see AUTHORS file
*/
#include "io/streamreader.h"
#include "math/float4.h"
#include "math/float2.h"
#include "math/matrix44.h"
#include "util/stack.h"
#include "util/array.h"
#include "util/stringatom.h"


namespace pjson
{
    class value_variant;
    class document;
}
//------------------------------------------------------------------------------
namespace IO
{
class JsonReader : public StreamReader
{
    __DeclareClass(JsonReader);
public:
    /// constructor
    JsonReader();
    /// destructor
    virtual ~JsonReader();

    /// begin reading from the stream
    virtual bool Open();
    /// end reading from the stream
    virtual void Close();

    /// return true if node exists 
    bool HasNode(const Util::String& path);
            
    /// sets the current node to root
    void SetToRoot();
    /// set current node as path, accepts / as separator and [number] to access array/object elements
    bool SetToNode(const Util::String& path);        
    
    /// set current node to first child, false if none exists
    bool SetToFirstChild(const Util::String& name = "");
    /// set to next child, false if none exists
    bool SetToNextChild();
    /// set current node to parent, return false if no parent exists
    bool SetToParent();

    /// check if current node is an array
    bool IsArray() const;    
    /// check if current node is an object (can have keys)
    bool IsObject() const;
    /// check if current node has children (json object or array)
    bool HasChildren() const;    
    /// children count
    SizeT CurrentSize() const;
    /// return true if matching attribute exists on current node
    bool HasAttr(const char* attr) const;
    /// return names of all attrs on current node
    Util::Array<Util::String> GetAttrs() const;

	/// get name of current node
	Util::String GetCurrentNodeName() const;

    /// get string attribute value from current node
    Util::String GetString(const char* attr =  0) const;
    /// get stringatom attribute value from current node
    Util::StringAtom GetStringAtom(const char* attr = 0) const;
    /// get bool attribute value from current node
    bool GetBool(const char* attr = 0) const;
    /// get int attribute value from current node
    int GetInt(const char* attr = 0) const;
	/// get unsigned int attribute value from current node
	uint GetUInt(const char* attr = 0) const;
    /// get float attribute value from current node
    float GetFloat(const char* attr = 0) const;
    /// get float2 attribute value from current node
    Math::float2 GetFloat2(const char* attr = 0) const;
    /// get float4 attribute value from current node
    Math::float4 GetFloat4(const char* attr = 0) const;
    /// get matrix44 attribute value from current node
    Math::matrix44 GetMatrix44(const char* attr = 0) const;
	/// get transform44 attribute value from current node
	Math::transform44 GetTransform44(const char* attr = 0) const;
    /// generic getter for extension types
    template<typename T> void Get(T& target, const char* attr = 0);    

    /// get optional string attribute value from current node
    Util::String GetOptString(const char* attr, const Util::String& defaultValue) const;
    /// get optional bool attribute value from current node
    bool GetOptBool(const char* attr, bool defaultValue) const;
    /// get optional int attribute value from current node
    int GetOptInt(const char* attr, int defaultValue) const;
    /// get optional float attribute value from current node
    float GetOptFloat(const char* attr, float defaultValue) const;    
    /// get float2 attribute value from current node
    Math::float2 GetOptFloat2(const char* attr, const Math::float2& defaultValue) const;
    /// get optional float4 attribute value from current node
    Math::float4 GetOptFloat4(const char* attr, const Math::float4& defaultValue) const;
    /// get optional matrix44 attribute value from current node
    Math::matrix44 GetOptMatrix44(const char* attr, const Math::matrix44& defaultValue) const;
	/// get transform44 attribute value from current node
	Math::transform44 GetOptTransform44(const char* attr, const Math::transform44& defaultValue) const;

    /// generic getter for optional items
    template<typename T> bool GetOpt(T& target, const char* attr = 0);
    /// generic getter for optional items
    template<typename T> bool GetOpt(T& target, const char* attr, const T& default);
    
private:  
    ///
    const pjson::value_variant* GetChild(const char * key = 0) const;
       
    pjson::document* document;
    const pjson::value_variant* curNode;
    IndexT childIdx = -1;
    Util::Stack<const pjson::value_variant*> parents;
    Util::Stack<IndexT> parentIdx;
    // 0 terminated buffer containing the raw json file
    char * buffer = nullptr;
};

//------------------------------------------------------------------------------
/**
*/
template<typename T>
inline bool
JsonReader::GetOpt(T& target, const char* attr)
{
	if (this->HasAttr(attr))
	{
		this->Get(target, attr);
		return true;
	}
	return false;
}

//------------------------------------------------------------------------------
/**
*/
template<typename T> 
inline bool
JsonReader::GetOpt(T& target, const char* attr, const T& defaultVal)
{
    if (!this->GetOpt<T>(target, attr))
    {
        target = defaultVal;
        return false;
    }
    return true;
}

} // namespace IO
//------------------------------------------------------------------------------
