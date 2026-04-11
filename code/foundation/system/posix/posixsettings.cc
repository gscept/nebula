//------------------------------------------------------------------------------
//  posixsettings.cc
//  This uses pjson directly instead of using any of the io wrappers as it has
//  to work before anything being initialized
//
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "core/ptr.h"
#include "io/fswrapper.h"
#include "io/uri.h"
#include "io/textreader.h"
#include "io/ioserver.h"
#include "system/posix/posixsettings.h"
#include "pjson/pjson.h"

namespace Posix
{
using namespace Core;
using namespace Util;

static pjson::document* 
GetSettingsJson(const Util::String & path)
{
    pjson::document* json = nullptr;
    if(IO::FSWrapper::FileExists(path))
    {
        // at this point we don't want to be depending on an instance of the ioserver so we use the wrapper instead
        PosixFSWrapper::Handle file = PosixFSWrapper::OpenFile(path, IO::Stream::AccessMode::ReadAccess, IO::Stream::AccessPattern::Random);
        if(file != nullptr)
        {
            IO::Stream::Size size = IO::FSWrapper::GetFileSize(file);
            char* buffer = (char*)Memory::Alloc(Memory::HeapType::ScratchHeap, size + 1);
            IO::FSWrapper::Read(file, (void*)buffer, size);
            buffer[size] = '\0';
            json = new pjson::document;
            if (!json->deserialize_in_place(buffer))
            {
                const pjson::error_info & error = json->get_error();
                Util::String position;
                if(error.m_ofs < size)
                {
                    position.Set(((const char *)buffer) + error.m_ofs, 40);
                }
			    n_error("Failed to parse settings file: %s\n%s\nat: %s\n", path.AsCharPtr(), error.m_pError_message, position.AsCharPtr());
                delete json;
                return nullptr;
            }
        }
    }
    return json; 
}
//------------------------------------------------------------------------------
/**
    Return true if a specific entry exists in the config files
*/
bool
PosixSettings::Exists(const Util::String & vendor, const String& key, const String& name)
{       
    // build path to config
    Util::String path = GetUserDir() + "/.config/nebula/"+ vendor + ".cfg";
    bool found = false;
    pjson::document* json = GetSettingsJson(path);
    if(json != NULL)
    {
        const pjson::value_variant* child = json->find_child_object(key.AsCharPtr());
        if (child != nullptr && child->is_object())
        {
            found = child->has_key(name.AsCharPtr());
        }
        delete json;
    }
    return found;
}

//------------------------------------------------------------------------------
/**
    Set a key value in the registry. This will create the key if it doesn't
    exist.
*/
bool
PosixSettings::WriteString(const Util::String & vendor, const String& key, const String& name, const String& value)
{  
    bool success = false;
    Util::String home = GetUserDir();
    // build path to config
    if(!IO::FSWrapper::DirectoryExists(home + "/.config/nebula/"))
    {
        IO::FSWrapper::CreateDirectory(home + "/.config/nebula/");
    }
    Util::String path = home + "/.config/nebula/"+ vendor + ".cfg";    
    pjson::document * json = GetSettingsJson(path);
    if(json == NULL)
    {
        json = new pjson::document;
        json->set_to_object();
    }
    
    pjson::pool_allocator& alloc = json->get_allocator();
    pjson::value_variant jsonValue(value.AsCharPtr(), alloc);

    int keyIdx = json->find_key(key.AsCharPtr());
    if (keyIdx == -1)
    {
        pjson::value_variant jsonKey;
        jsonKey.set_to_object();
        jsonKey.add_key_value(name.AsCharPtr(), jsonValue, alloc);
        json->add_key_value(key.AsCharPtr(), jsonKey, alloc);
    }
    else
    {
        pjson::value_variant& jsonKey = json->get_value_at_index(keyIdx);
        int nameIdx = jsonKey.find_key(name.AsCharPtr());
        if (nameIdx == -1)
        {
            jsonKey.add_key_value(name.AsCharPtr(), jsonValue, alloc);
        }
        else
        {
            pjson::value_variant& jsonName = jsonKey.get_value_at_index(nameIdx);
            jsonName.set(value.AsCharPtr(), alloc);
        }
    }
    pjson::char_vec_t buffer;
    if (json->serialize(buffer, true, false))
    {
        PosixFSWrapper::Handle file = PosixFSWrapper::OpenFile(path, IO::Stream::AccessMode::WriteAccess, IO::Stream::AccessPattern::Random);
        if(file != nullptr)
        {
            IO::FSWrapper::Write(file, &buffer[0], buffer.size());
            IO::FSWrapper::CloseFile(file);
            success = true;
        }
    }
    delete json;
    return success;
}

//------------------------------------------------------------------------------
/**
    Get a string value from the registry. Fails hard if the key doesn't exists
    (use the Exists() method to make sure that the key exists!).    
*/
String
PosixSettings::ReadString(const Util::String & vendor, const String& key, const String& name)
{        
    // build path to config
    Util::String path = GetUserDir() + "/.config/nebula/"+ vendor + ".cfg";    

    pjson::document* json = GetSettingsJson(path);
    if(json != NULL)
    {
        const pjson::value_variant* child = json->find_child_object(key.AsCharPtr());
        if (child != nullptr && child->is_object())
        {
            int childKey = child->find_key(name.AsCharPtr());
            n_assert_fmt(childKey >= 0, "value %s not in settings file", name.AsCharPtr());
            String value = child->get_value_at_index(childKey).as_string_ptr();
            delete json;
            return value;
        }
    }
    
    n_error("failed to open settings file %s\n",path.AsCharPtr());
    return "";
}


//------------------------------------------------------------------------------
/**
    This deletes a complete registry key with all its values.
*/
bool
PosixSettings::Delete(const Util::String & vendor, const String& key)
{
    n_error("not implemented");
    return false;    
}

//------------------------------------------------------------------------------
/**
    Returns path to home directory (redundant with fswrapper but we need to be independent of everything)
*/
Util::String 
PosixSettings::GetUserDir()
{
    return getenv("HOME");
}

} // namespace Posix
