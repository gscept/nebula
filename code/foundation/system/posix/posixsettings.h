#pragma once
//------------------------------------------------------------------------------
/**
    @class Posix::PosixSettings
    
    A simple wrapper class to store config files in the users home directory
    Uses Boosts Property_tree library for the time being
        
    (C) 2013 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"

//------------------------------------------------------------------------------
namespace Posix
{
class PosixSettings
{
public:
    
    /// return true if a setting exists (uses CurrentUser as RootKey)
    static bool Exists(const Util::String & vendor, const Util::String& key, const Util::String& name);
    /// write a settings entry (uses CurrentUser as RootKey)
    static bool WriteString(const Util::String & vendor, const Util::String& key, const Util::String& name, const Util::String& value);
    /// read a string registry entry, the string will be UTF-8 encoded!
    static Util::String ReadString(const Util::String & vendor, const Util::String& key, const Util::String& name);
    /// delete a setting (and all its contained values), uses CurrentUser as RootKey
    static bool Delete(const Util::String & vendor, const Util::String& key);
        
private:
    static Util::String GetUserDir();
    
};

} // namespace Posix
//------------------------------------------------------------------------------
