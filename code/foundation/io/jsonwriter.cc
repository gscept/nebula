//------------------------------------------------------------------------------
//  jsonwriter.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "util/variant.h"
#include "io/jsonwriter.h"
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include "pjson/pjson.h"
#include "util/stringatom.h"

namespace IO
{
__ImplementClass(IO::JsonWriter, 'JSOW', IO::StreamWriter);

using namespace Util;
    
//------------------------------------------------------------------------------
/**
*/
JsonWriter::JsonWriter() :
    document(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
JsonWriter::~JsonWriter()
{
    if (this->IsOpen())
    {
        this->Close();
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
JsonWriter::Open()
{
    n_assert(0 == this->document);
    
    if (StreamWriter::Open())
    {       
        this->document = n_new(pjson::document);
        this->document->set_to_object();
        this->hierarchy.Push(this->document);
        this->nameHierarchy.Push(Util::String());        
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/** 
*/
void
JsonWriter::Close()
{
    n_assert(this->IsOpen());
    n_assert(0 != this->document);

    pjson::char_vec_t buffer;
    n_assert(this->document->serialize(buffer, true, false));

    this->stream->Write(&buffer[0], (IO::Stream::Size)buffer.size());

    n_delete(this->document);
    this->document = 0;
        
    // close the stream
    StreamWriter::Close();
}

//------------------------------------------------------------------------------
/**
*/
void
JsonWriter::BeginArray(const char * name)
{
    n_assert(this->IsOpen());
    pjson::value_variant * node = n_new(pjson::value_variant(pjson::cJSONValueTypeArray));
    Util::String sName = name;   
    this->hierarchy.Push(node);
    this->nameHierarchy.Push(sName);
}

//------------------------------------------------------------------------------
/**
*/
void
JsonWriter::BeginObject(const char * name)
{
    n_assert(this->IsOpen());
    pjson::value_variant * node = n_new(pjson::value_variant(pjson::cJSONValueTypeObject));
    Util::String sName = name; 
    this->hierarchy.Push(node);
    this->nameHierarchy.Push(sName);
}

//------------------------------------------------------------------------------
/** 
*/
void
JsonWriter::End()
{
    n_assert(this->IsOpen());    

    Util::String name = this->nameHierarchy.Pop();
    pjson::value_variant * entry = this->hierarchy.Pop();
    if (name.IsEmpty())
    {        
        if (this->hierarchy.Peek()->is_null())
        {
            this->hierarchy.Peek()->set_to_array();
        }
        // fixme this is not really efficient, should transfer ownership
        this->hierarchy.Peek()->add_value(*entry, this->document->get_allocator());
    }
    else
    {
        if (this->hierarchy.Peek()->is_null())
        {
            this->hierarchy.Peek()->set_to_object();
        }
        this->hierarchy.Peek()->add_key_value(name.AsCharPtr(), *entry, this->document->get_allocator());
    }
    n_delete(entry);
}

//------------------------------------------------------------------------------
/**
*/
void JsonWriter::Add(const char * value, const Util::String & name)
{
    auto & alloc = this->document->get_allocator();
    pjson::value_variant val(value, alloc);
    if (name.IsEmpty())
    {
        this->hierarchy.Peek()->add_value(val, alloc);
    }
    else
    {
        this->hierarchy.Peek()->add_key_value(name.AsCharPtr(), val, alloc);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> void JsonWriter::Add(const Util::String & value, const Util::String & name)
{
    auto & alloc = this->document->get_allocator();
    pjson::value_variant val(value.AsCharPtr(), alloc);
    if (name.IsEmpty())
    {
        this->hierarchy.Peek()->add_value(val, alloc);
    }
    else
    {
       this->hierarchy.Peek()->add_key_value(name.AsCharPtr(), val, alloc);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> void JsonWriter::Add(const Util::StringAtom& value, const Util::String& name)
{
    auto& alloc = this->document->get_allocator();
    pjson::value_variant val(value.Value(), alloc);
    if (name.IsEmpty())
    {
        this->hierarchy.Peek()->add_value(val, alloc);
    }
    else
    {
        this->hierarchy.Peek()->add_key_value(name.AsCharPtr(), val, alloc);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> void JsonWriter::Add(const bool & value, const Util::String & name)
{
    auto & alloc = this->document->get_allocator();
    pjson::value_variant val(value);
    if (name.IsEmpty())
    {
        this->hierarchy.Peek()->add_value(val, alloc);
    }
    else
    {
        this->hierarchy.Peek()->add_key_value(name.AsCharPtr(), val, alloc);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> void JsonWriter::Add(const char& value, const Util::String& name)
{
    auto& alloc = this->document->get_allocator();
    pjson::value_variant val(value);
    if (name.IsEmpty())
    {
        this->hierarchy.Peek()->add_value(val, alloc);
    }
    else
    {
        this->hierarchy.Peek()->add_key_value(name.AsCharPtr(), val, alloc);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> void JsonWriter::Add(const uchar& value, const Util::String& name)
{
    auto& alloc = this->document->get_allocator();
    pjson::value_variant val(value);
    if (name.IsEmpty())
    {
        this->hierarchy.Peek()->add_value(val, alloc);
    }
    else
    {
        this->hierarchy.Peek()->add_key_value(name.AsCharPtr(), val, alloc);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> void JsonWriter::Add(const int & value, const Util::String & name)
{
    auto & alloc = this->document->get_allocator();
    pjson::value_variant val(value);
    if (name.IsEmpty())
    {
        this->hierarchy.Peek()->add_value(val, alloc);
    }
    else
    {
        this->hierarchy.Peek()->add_key_value(name.AsCharPtr(), val, alloc);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> void JsonWriter::Add(const unsigned int & value, const Util::String & name)
{
    auto & alloc = this->document->get_allocator();
    pjson::value_variant val(value);
    if (name.IsEmpty())
    {
        this->hierarchy.Peek()->add_value(val, alloc);
    }
    else
    {
        this->hierarchy.Peek()->add_key_value(name.AsCharPtr(), val, alloc);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> void JsonWriter::Add(const float & value, const Util::String & name)
{
    auto & alloc = this->document->get_allocator();
    pjson::value_variant val(static_cast<double>(value));
    if (name.IsEmpty())
    {
        this->hierarchy.Peek()->add_value(val, alloc);
    }
    else
    {
        this->hierarchy.Peek()->add_key_value(name.AsCharPtr(), val, alloc);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> void JsonWriter::Add(const Math::vec3& value, const Util::String& name)
{
    auto& alloc = this->document->get_allocator();
    pjson::value_variant val(pjson::cJSONValueTypeArray);
    {
        alignas(16) float v[4];
        value.store(v);
        for (int i = 0; i < 3; i++)
        {
            pjson::value_variant valf(v[i]);
            val.add_value(valf, alloc);
        }
    }
    if (name.IsEmpty())
    {
        this->hierarchy.Peek()->add_value(val, alloc);
    }
    else
    {
        this->hierarchy.Peek()->add_key_value(name.AsCharPtr(), val, alloc);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> void JsonWriter::Add(const Math::vec4 & value , const Util::String & name)
{
    auto & alloc = this->document->get_allocator();
    pjson::value_variant val(pjson::cJSONValueTypeArray);
    {
        alignas(16) float v[4];
        value.store(v);
        for (int i = 0; i < 4; i++)
        {
            pjson::value_variant valf(v[i]);
            val.add_value(valf, alloc);
        }
    }
    if (name.IsEmpty())
    {
        this->hierarchy.Peek()->add_value(val, alloc);
    }
    else
    {
        this->hierarchy.Peek()->add_key_value(name.AsCharPtr(), val, alloc);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> void JsonWriter::Add(const Math::vec2& value, const Util::String & name)
{
    auto & alloc = this->document->get_allocator();
    pjson::value_variant val(pjson::cJSONValueTypeArray);
    {
        pjson::value_variant valf(value.x);
        val.add_value(valf, alloc);
        valf = value.y;
        val.add_value(valf, alloc);
    }
    if (name.IsEmpty())
    {
        this->hierarchy.Peek()->add_value(val, alloc);
    }
    else
    {
        this->hierarchy.Peek()->add_key_value(name.AsCharPtr(), val, alloc);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> void JsonWriter::Add(const Math::mat4 & value, const Util::String & name)
{
    auto & alloc = this->document->get_allocator();
    pjson::value_variant val(pjson::cJSONValueTypeArray);
    {
        alignas(16) float v[16];
        value.store(v);
        for (int i = 0; i < 16; i++)
        {
            pjson::value_variant valf(v[i]);
            val.add_value(valf, alloc);
        }
    }
    if (name.IsEmpty())
    {
        this->hierarchy.Peek()->add_value(val, alloc);
    }
    else
    {
        this->hierarchy.Peek()->add_key_value(name.AsCharPtr(), val, alloc);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> void JsonWriter::Add(const Math::transform44 & value, const Util::String & name)
{
    auto & alloc = this->document->get_allocator();
    pjson::value_variant val(pjson::cJSONValueTypeArray);
    {
        float v[36];
        value.storeu(v);
        for (int i = 0; i < 36; i++)
        {
            pjson::value_variant valf(v[i]);
            val.add_value(valf, alloc);
        }
    }
    if (name.IsEmpty())
    {
        this->hierarchy.Peek()->add_value(val, alloc);
    }
    else
    {
        this->hierarchy.Peek()->add_key_value(name.AsCharPtr(), val, alloc);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> void JsonWriter::Add(const Util::Variant& value, const Util::String & name)
{
    switch (value.GetType())
    {
    case Util::Variant::Type::Bool:
        this->Add(value.GetBool(), name);
        break;
    case Util::Variant::Type::Int:
        this->Add(value.GetInt(), name);
        break;
    case Util::Variant::Type::Float:
        this->Add(value.GetFloat(), name);
        break;
    case Util::Variant::Type::Mat4:
        this->Add(value.GetMat4(), name);
        break;
    case Util::Variant::Type::Vec4:
        this->Add(value.GetVec4(), name);
        break;
    case Util::Variant::Type::UInt:
        this->Add(value.GetUInt(), name);
        break;
    case Util::Variant::Type::Guid:
        this->Add(value.GetGuid().AsString(), name);
        break;
    case Util::Variant::Type::String:
        this->Add(value.GetString(), name);
        break;
    default:
        n_error("Variant type \"%s\" not yet supported!", Util::Variant::TypeToString(value.GetType()).AsCharPtr());
        return;
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> void JsonWriter::Add(const Util::Guid& value, const Util::String & name)
{
    this->Add(value.AsString(), name);
}

//------------------------------------------------------------------------------
/**
*/
template<> void JsonWriter::Add(const Util::Array<int> & value, const Util::String & name)
{
    auto & alloc = this->document->get_allocator();
    pjson::value_variant val(pjson::cJSONValueTypeArray);
    {                
        for (auto i : value)
        {
            pjson::value_variant valf(i);
            val.add_value(valf, alloc);
        }
    }
    if (name.IsEmpty())
    {
        this->hierarchy.Peek()->add_value(val, alloc);
    }
    else
    {
        this->hierarchy.Peek()->add_key_value(name.AsCharPtr(), val, alloc);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> void JsonWriter::Add(const Util::Array<Util::String> & value, const Util::String & name)
{
    auto & alloc = this->document->get_allocator();
    pjson::value_variant val(pjson::cJSONValueTypeArray);
    {
        for (auto i : value)
        {
            pjson::value_variant valf(i.AsCharPtr(), alloc);
            val.add_value(valf, alloc);
        }
    }
    if (name.IsEmpty())
    {
        this->hierarchy.Peek()->add_value(val, alloc);
    }
    else
    {
        this->hierarchy.Peek()->add_key_value(name.AsCharPtr(), val, alloc);
    }
}
} // namespace IO
