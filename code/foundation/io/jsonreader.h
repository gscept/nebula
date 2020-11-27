#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::JsonReader
  
    Reads json formatted data with random access from a stream using 
    pjson as backend. The json document is represented as a tree of
    nodes, which can be navigated and queried.
        
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
#include "io/streamreader.h"
#include "math/vec4.h"
#include "math/vec2.h"
#include "math/mat4.h"
#include "util/stack.h"
#include "util/array.h"
#include "util/stringatom.h"
#include "util/bitfield.h"


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
    /// get vec2 attribute value from current node
    Math::vec2 GetVec2(const char* attr = 0) const;
    /// get vec4 attribute value from current node
    Math::vec3 GetVec3(const char* attr = 0) const;
    /// get vec4 attribute value from current node
    Math::vec4 GetVec4(const char* attr = 0) const;
    /// get mat4 attribute value from current node
    Math::mat4 GetMat4(const char* attr = 0) const;
	/// get transform44 attribute value from current node
	Math::transform44 GetTransform44(const char* attr = 0) const;
    /// generic getter for extension types
    template<typename T> void Get(T& target, const char* attr = 0);
    /// getter for bitfield of N size
    template<unsigned int N> void Get(Util::BitField<N>& target, const char* attr = 0);
    
    /// get optional string attribute value from current node
    Util::String GetOptString(const char* attr, const Util::String& defaultValue) const;
    /// get optional bool attribute value from current node
    bool GetOptBool(const char* attr, bool defaultValue) const;
    /// get optional int attribute value from current node
    int GetOptInt(const char* attr, int defaultValue) const;
    /// get optional float attribute value from current node
    float GetOptFloat(const char* attr, float defaultValue) const;    
    /// get vec2 attribute value from current node
    Math::vec2 GetOptVec2(const char* attr, const Math::vec2& defaultValue) const;
    /// get optional vec4 attribute value from current node
    Math::vec4 GetOptVec4(const char* attr, const Math::vec4& defaultValue) const;
    /// get optional mat4 attribute value from current node
    Math::mat4 GetOptMat4(const char* attr, const Math::mat4& defaultValue) const;
	/// get transform44 attribute value from current node
	Math::transform44 GetOptTransform44(const char* attr, const Math::transform44& defaultValue) const;

    /// generic getter for optional items
    template<typename T> bool GetOpt(T& target, const char* attr = 0);
    /// generic getter for optional items
    template<typename T> bool GetOpt(T& target, const char* attr, const T& _default);
    
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

//------------------------------------------------------------------------------
/**
*/
template<unsigned int N>
inline void
JsonReader::Get(Util::BitField<N>& ret, const char* attr)
{
    Util::Array<int> arr;
    this->Get<Util::Array<int>>(arr);

    unsigned int count = arr.Size();
    n_assert(count <= N);
    for (unsigned int i = 0; i < count; i++)
    {
        ret.SetBit(arr[i]);
    }
}

} // namespace IO
//------------------------------------------------------------------------------
