//------------------------------------------------------------------------------
//  jsonwriter.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/jsonwriter.h"
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include "pjson/pjson.h"

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
template<> void JsonWriter::Add(const Math::float4 & value , const Util::String & name)
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
template<> void JsonWriter::Add(const Math::matrix44 & value, const Util::String & name)
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
