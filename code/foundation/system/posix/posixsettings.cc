//------------------------------------------------------------------------------
//  posixsettings.cc
//  (C) 2013 Individual contributors, see AUTHORS file
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
    if(IO::FSWrapper::FileExists(path))
    {
        Ptr<IO::TextReader> reader = IO::TextReader::Create();
        reader->SetStream(IO::IoServer::Instance()->CreateStream(path));
        if(reader->Open())
        {
            Util::String str = reader->ReadAll();
            return cJSON_Parse(str.AsCharPtr());            
        }
    }    
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
    /*
    property_tree::ptree pt;
    if(IO::FSWrapper::FileExists(path))
    {
        try
        {
            read_xml(path.AsCharPtr(), pt);
        }
        catch(property_tree::xml_parser::xml_parser_error e)
        {
            n_error("failed to parse settings file %s\n", path.AsCharPtr());
        }
    }
    
    
    Util::String skey = key + "." + name;
    try
    {
        pt.put<std::string>(skey.AsCharPtr(),value.AsCharPtr());                
    }
    catch(property_tree::ptree_bad_data p)
    {
        n_error("failed to store setting %s\n",value.AsCharPtr());
    }
    
    try
    {    
        write_xml(path.AsCharPtr(), pt);
        return true;
    }
    catch(property_tree::xml_parser::xml_parser_error e)
    {
        n_error("failed to parse settings file %s\n", path.AsCharPtr());
    }    
    */
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
            cJSON * val = cJSON_GetObjectItemCaseSensitive(js, name.AsCharPtr());
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
