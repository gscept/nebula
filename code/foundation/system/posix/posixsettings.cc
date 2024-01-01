//------------------------------------------------------------------------------
//  posixsettings.cc
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "core/ptr.h"
#include "io/fswrapper.h"
#include "io/uri.h"
#include "io/textreader.h"
#include "io/ioserver.h"
#include "system/posix/posixsettings.h"
#include "cJSON.h"

namespace Posix
{
using namespace Core;
using namespace Util;

static cJSON*
GetSettingsJson(const Util::String & path)
{
    cJSON* json = nullptr;
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
            json = cJSON_Parse(buffer);
            Memory::Free(Memory::HeapType::ScratchHeap, buffer);
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
    cJSON * js = GetSettingsJson(path);
    if(js != NULL)
    {
        const cJSON * keys = cJSON_GetObjectItemCaseSensitive(js, key.AsCharPtr());
        if(keys != NULL)
        {
            found = cJSON_HasObjectItem(keys, name.AsCharPtr());            
        }
        cJSON_Delete(js);        
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
    Util::String home = GetUserDir();
    // build path to config
    if(!IO::FSWrapper::DirectoryExists(home + "/.config/nebula/"))
    {
        IO::FSWrapper::CreateDirectory(home + "/.config/nebula/");
    }
    Util::String path = home + "/.config/nebula/"+ vendor + ".cfg";    
    cJSON * js = GetSettingsJson(path);
    if(js != NULL)
    {
        cJSON * keys = cJSON_GetObjectItemCaseSensitive(js, key.AsCharPtr());
        if(keys != NULL)
        {
            if (cJSON_HasObjectItem(keys, name.AsCharPtr()))
            {
                cJSON_ReplaceItemInObject(keys, name.AsCharPtr(), cJSON_CreateString(value.AsCharPtr()));
            }
            else
            {
                cJSON_AddStringToObject(keys, name.AsCharPtr(), value.AsCharPtr());
            }
            char* buffer = cJSON_PrintUnformatted(js);
            if (buffer != nullptr)
            {
                PosixFSWrapper::Handle file = PosixFSWrapper::OpenFile(path, IO::Stream::AccessMode::WriteAccess, IO::Stream::AccessPattern::Random);
                if(file != nullptr)
                {
                    IO::FSWrapper::Write(file, buffer, strlen(buffer));
                    IO::FSWrapper::CloseFile(file);
                    cJSON_Delete(js);
                    return true;
                }
            }
        }
    }
    return false;
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

    cJSON * js = GetSettingsJson(path);
    if(js != NULL)
    {
        const cJSON * keys = cJSON_GetObjectItemCaseSensitive(js, key.AsCharPtr());
        if(keys != NULL)
        {
            cJSON * val = cJSON_GetObjectItemCaseSensitive(keys, name.AsCharPtr());
            if( val != NULL && cJSON_IsString(val))
            {
                Util::String ret(val->valuestring);
                cJSON_Delete(js);                
                return ret;
            }
        }
        cJSON_Delete(js);        
    }
    
    n_error("failed to open settings file %s\n",path.AsCharPtr());
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
