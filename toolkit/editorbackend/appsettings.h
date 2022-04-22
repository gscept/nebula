#pragma once
//------------------------------------------------------------------------------
/**
    @class Universe::ApplicationSettings

    Centralized place for application settings.
    Can be saved and loaded to/from file.

    @copyright
    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "util/variant.h"
#include "util/delegate.h"
#include "util/stringatom.h"

namespace Universe
{
class AppSettings
{
    __DeclareSingleton(AppSettings);

private:
    AppSettings();
    ~AppSettings();

public:
    /// create the singleton
    static void Create();
    /// destroy the singleton
    static void Destroy();

    /// set the settings to its default state
    static void SetToDefault();
    /// load the profile data
    static bool Load();
    /// save to disk
    static bool Save();
    /// currently loaded?
    static bool IsLoaded();
    /// adds a delegate listener to a specific setting by name
    static void AddChangeListener(Util::StringAtom name, Util::Delegate<void(Util::Variant const&)> const& callback);
    /// return true if attribute exists in the profile
    static bool HasAttr(const Util::String& name);
    /// set a string attribute in the profile
    static void SetString(const Util::String& name, const Util::String& val);
    /// set an int attribute in the profile
    static void SetInt(const Util::String& name, int val);
    /// set a float attribute in the profile
    static void SetFloat(const Util::String& name, float val);
    /// set a bool attribute in the profile
    static void SetBool(const Util::String& name, bool val);
    /// set a float4 attribute in the profile
    static void SetVec4(const Util::String& name, const Math::vec4& val);
    /// get string attribute from the profile
    static Util::String GetString(const Util::String& name);
    /// get string attribute from the profile with default in case attribute doesnt exist
    static Util::String GetStringWithDefault(const Util::String& name, const Util::String& defaultValue);
    /// get int attribute from the profile
    static int GetInt(const Util::String& name);
    /// get int attribute from the profile
    static int GetIntWithDefault(const Util::String& name, int def);
    /// get float attribute from the profile
    static float GetFloat(const Util::String& name);
    /// get float attribute from the profile
    static float GetFloatWithDefault(const Util::String& name, float def);
    /// get bool attribute from the profile
    static bool GetBool(const Util::String& name);
    /// get bool attribute from the profile
    static bool GetBoolWithDefault(const Util::String& name, bool def);
    /// get vec4 attribute from the profile
    static Math::vec4 GetVec4(const Util::String& name);

protected:
    /// set to true if the settings has been loaded
    bool isLoaded;
    /// contains the settings.
    Util::Dictionary<Util::StringAtom, Util::Variant> profileData;
    /// contains value-changed listeners
    Util::Dictionary<Util::StringAtom, Util::Delegate<void(Util::Variant const&)>> callbacks;
private:
};

//------------------------------------------------------------------------------
/**
*/
inline bool
AppSettings::IsLoaded()
{
    return Singleton->isLoaded;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AppSettings::AddChangeListener(Util::StringAtom name, Util::Delegate<void(Util::Variant const&)> const& callback)
{
    Singleton->callbacks.Add(name, callback);
}

//------------------------------------------------------------------------------
/**
*/
inline void
AppSettings::SetString(const Util::String& name, const Util::String& val)
{
    auto newValue = Util::Variant(val);
    if (Singleton->profileData.Contains(name))
    {
        Singleton->profileData[name] = newValue;
    }
    else
    {
        Singleton->profileData.Add(name, newValue);
    }

    if (Singleton->callbacks.Contains(name))
    {
        Singleton->callbacks[name](newValue);
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void
AppSettings::SetInt(const Util::String& name, int val)
{
    auto newValue = Util::Variant(val);
    if (Singleton->profileData.Contains(name))
    {
        Singleton->profileData[name] = newValue;
    }
    else
    {
        Singleton->profileData.Add(name, newValue);
    }

    if (Singleton->callbacks.Contains(name))
    {
        Singleton->callbacks[name](newValue);
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void
AppSettings::SetFloat(const Util::String& name, float val)
{
    auto newValue = Util::Variant(val);
    if (Singleton->profileData.Contains(name))
    {
        Singleton->profileData[name] = newValue;
    }
    else
    {
        Singleton->profileData.Add(name, newValue);
    }

    if (Singleton->callbacks.Contains(name))
    {
        Singleton->callbacks[name](newValue);
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void
AppSettings::SetBool(const Util::String& name, bool val)
{
    auto newValue = Util::Variant(val);
    if (Singleton->profileData.Contains(name))
    {
        Singleton->profileData[name] = newValue;
    }
    else
    {
        Singleton->profileData.Add(name, newValue);
    }

    if (Singleton->callbacks.Contains(name))
    {
        Singleton->callbacks[name](newValue);
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void
AppSettings::SetVec4(const Util::String& name, const Math::vec4& val)
{
    auto newValue = Util::Variant(val);
    if (Singleton->profileData.Contains(name))
    {
        Singleton->profileData[name] = newValue;
    }
    else
    {
        Singleton->profileData.Add(name, newValue);
    }

    if (Singleton->callbacks.Contains(name))
    {
        Singleton->callbacks[name](newValue);
    }
}

//------------------------------------------------------------------------------
/**
*/
inline Util::String
AppSettings::GetString(const Util::String& name)
{
    n_assert(Singleton->profileData.Contains(name));
    return Singleton->profileData[name].GetString();
}


//------------------------------------------------------------------------------
/**
*/
inline Util::String
AppSettings::GetStringWithDefault(const Util::String& name, const Util::String &def)
{
    if (Singleton->profileData.Contains(name))
    {
        return Singleton->profileData[name].GetString();
    }
    else
    {
        return def;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline int
AppSettings::GetInt(const Util::String& name)
{
    n_assert(Singleton->profileData.Contains(name));
    return Singleton->profileData[name].GetInt();
}

//------------------------------------------------------------------------------
/**
*/
inline int
AppSettings::GetIntWithDefault(const Util::String& name, int def)
{
    if (Singleton->profileData.Contains(name))
    {
        return Singleton->profileData[name].GetInt();
    }
    else
    {
        return def;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline float
AppSettings::GetFloat(const Util::String& name)
{
    n_assert(Singleton->profileData.Contains(name));
    return Singleton->profileData[name].GetFloat();
}

//------------------------------------------------------------------------------
/**
*/
inline float
AppSettings::GetFloatWithDefault(const Util::String& name, float def)
{
    if (Singleton->profileData.Contains(name))
    {
        return Singleton->profileData[name].GetFloat();
    }
    else
    {
        return def;
    }
}


//------------------------------------------------------------------------------
/**
*/
inline bool
AppSettings::GetBool(const Util::String& name)
{
    n_assert(Singleton->profileData.Contains(name));
    return Singleton->profileData[name].GetBool();
}

//------------------------------------------------------------------------------
/**
*/
inline bool
AppSettings::GetBoolWithDefault(const Util::String& name, bool def)
{
    if (Singleton->profileData.Contains(name))
    {
        return Singleton->profileData[name].GetBool();
    }
    else
    {
        return def;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline Math::vec4
AppSettings::GetVec4(const Util::String& name)
{
    n_assert(Singleton->profileData.Contains(name));
    return Singleton->profileData[name].GetVec4();
}


} // namespace Universe
