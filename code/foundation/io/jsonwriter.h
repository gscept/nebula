#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::JsonWriter
  
    Write Json-formatted data to a stream.
        
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/    
#include "io/streamwriter.h"
#include "util/string.h"
#include "util/stack.h"

namespace pjson
{
    class value_variant;
    class document;
}

//------------------------------------------------------------------------------
namespace IO
{
class JsonWriter : public IO::StreamWriter
{
    __DeclareClass(JsonWriter);
public:
    /// constructor
    JsonWriter();
    /// destructor
    virtual ~JsonWriter();
    /// begin writing the stream
    virtual bool Open();
    /// end writing the stream
    virtual void Close();

    /// begin a new array node 
    void BeginArray(const char * nodeName = nullptr);    

    /// begin a new node that can host key-value pairs
    void BeginObject(const char * nodeName = nullptr);
    /// end current object or array
    void End();
    
    /// add a value to the current array node
    template <typename T> void Add(const T & value, const Util::String & name = "");
    /// special handling for const strings
    void Add(const char * value, const Util::String & name = "");
    /// add a value to current object node
//    template <typename T> void Add(const Util::String & name, const T & value);

    /*
    ///
    void AddString(const Util::String & value);
    ///
    void AddBool(bool value);
    ///
    void AddInt(int value);
    ///
    void AddFloat(float value);
    ///
    void AddFloat2(const Math::float2 & value);
    ///
    void AddFloat4(const Math::float4 & value);
    ///
    void AddMatrix44(const Math::matrix44 & value);
    ///
    void AddTransform44(const Math::transform44 & value);

    /// set string attribute on current node
    void SetString(const Util::String& name, const Util::String& value);
    /// set bool attribute on current node
    void SetBool(const Util::String& name, bool value);
    /// set int attribute on current node
    void SetInt(const Util::String& name, int value);
    /// set float attribute on current node
    void SetFloat(const Util::String& name, float value);      
    /// set float2 attribute on current node
    void SetFloat2(const Util::String& name, const Math::float2& value);
    /// set float4 attribute on current node
    void SetFloat4(const Util::String& name, const Math::float4& value);
    /// set matrix44 attribute on current node
    void SetMatrix44(const Util::String& name, const Math::matrix44& value);
	/// set transform44 attribute on current node
	void SetTransform44(const Util::String& name, const Math::transform44& value);    
    /// generic setter, template specializations implemented in nebula3/code/addons/nebula2
    template<typename T> void Set(const Util::String& name, const T &value);
    */

private:

    pjson::document * document;    
    Util::Stack<pjson::value_variant*> hierarchy;
    Util::Stack<Util::String> nameHierarchy;
};

} // namespace IO
//------------------------------------------------------------------------------

