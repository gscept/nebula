#pragma once
//------------------------------------------------------------------------------
/**
    @class BaseGameFeature::UserProfile

    A user profile represents a storage where all user specific 
    data is kept across application restarts. This usually includes
    save games, options, and other per-user data. Applications should
    at least support a default profile, but everything is there to 
    support more then one user profile.

    User profiles are stored in "user:[appname]/profiles/[profilename]".
        
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "util/dictionary.h"
#include "util/variant.h"

//------------------------------------------------------------------------------
namespace BaseGameFeature
{
class UserProfile : public Core::RefCounted
{
	__DeclareClass(UserProfile)
public:
    /// constructor
    UserProfile();
    /// destructor
    virtual ~UserProfile();
    /// static method to enumerate all existing user profiles
    static Util::Array<Util::String> EnumProfiles();
    /// static method to delete an existing user profile by name
    static void DeleteProfile(const Util::String& name);
    /// static method which returns a path to the profile root directory
    static Util::String GetProfileRootDirectory();
    /// set the name of the user profile
    void SetName(const Util::String& n);
    /// get the name of the user profile
    const Util::String& GetName() const;
    /// set the user profile to its default state, override in subclass
    virtual void SetToDefault();
    /// load the profile data
    virtual bool Load(const Util::String& path = "");
    /// save to disk
    virtual bool Save();    
    /// currently loaded?
    bool IsLoaded() const;
    /// get the filesystem path to the user profile directory
    Util::String GetProfileDirectory() const;
    /// get the filesystem path to the savegame directory
    Util::String GetSaveGameDirectory() const;
    /// get path to a complete savegame
    Util::String GetSaveGamePath(const Util::String& saveGameName) const;
    /// return true if attribute exists in the profile
    bool HasAttr(const Util::String& name) const;
    /// set a string attribute in the profile
    void SetString(const Util::String& name, const Util::String& val);
    /// set an int attribute in the profile
    void SetInt(const Util::String& name, int val);
    /// set a float attribute in the profile
    void SetFloat(const Util::String& name, float val);
    /// set a bool attribute in the profile
    void SetBool(const Util::String& name, bool val);
    /// set a float4 attribute in the profile
    void SetFloat4(const Util::String& name, const Math::float4& val);
	/// set a variant value
	void SetVariant(const Util::String& name, const Util::Variant & val);
    /// get string attribute from the profile
    Util::String GetString(const Util::String& name) const;
	/// get string attribute from the profile with default in case attribute doesnt exist
	Util::String GetStringWithDefault(const Util::String& name, const Util::String &defaultValue) const;
    /// get int attribute from the profile
    int GetInt(const Util::String& name) const;
	/// get int attribute from the profile
	int GetIntWithDefault(const Util::String& name, int def) const;
    /// get float attribute from the profile
    float GetFloat(const Util::String& name) const;
	/// get float attribute from the profile
	float GetFloatWithDefault(const Util::String& name, float def) const;
    /// get bool attribute from the profile
    bool GetBool(const Util::String& name) const;
	/// get bool attribute from the profile
	bool GetBoolWithDefault(const Util::String& name, bool def) const;
    /// get float4 attribute from the profile
    Math::float4 GetFloat4(const Util::String& name) const;

protected:
    bool isLoaded;
    Util::String name;    
    Util::Dictionary<Util::String, Util::Variant> profileData;    
};

//------------------------------------------------------------------------------
/**
*/
inline void
UserProfile::SetName(const Util::String& n)
{
    this->name = n;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
UserProfile::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
UserProfile::IsLoaded() const
{
    return this->isLoaded;
}

//------------------------------------------------------------------------------
/**
*/
inline void
UserProfile::SetString(const Util::String& name, const Util::String& val)
{
    if (this->profileData.Contains(name))
    {
        this->profileData[name] = Util::Variant(val);
    }
    else
    {
        this->profileData.Add(name, Util::Variant(val));
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void
UserProfile::SetInt(const Util::String& name, int val)
{
    if (this->profileData.Contains(name))
    {
        this->profileData[name] = Util::Variant(val);
    }
    else
    {
        this->profileData.Add(name, Util::Variant(val));
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void
UserProfile::SetFloat(const Util::String& name, float val)
{
    if (this->profileData.Contains(name))
    {
        this->profileData[name] = Util::Variant(val);
    }
    else
    {
        this->profileData.Add(name, Util::Variant(val));
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void
UserProfile::SetBool(const Util::String& name, bool val)
{
    if (this->profileData.Contains(name))
    {
        this->profileData[name] = Util::Variant(val);
    }
    else
    {
        this->profileData.Add(name, Util::Variant(val));
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void
UserProfile::SetFloat4(const Util::String& name, const Math::float4& val)
{
    if (this->profileData.Contains(name))
    {
        this->profileData[name] = Util::Variant(val);
    }
    else
    {
        this->profileData.Add(name, Util::Variant(val));
    }
}

//------------------------------------------------------------------------------
/**
*/
inline Util::String
UserProfile::GetString(const Util::String& name) const
{
    n_assert(this->profileData.Contains(name));
    return this->profileData[name].GetString();
}


//------------------------------------------------------------------------------
/**
*/
inline Util::String
UserProfile::GetStringWithDefault(const Util::String& name, const Util::String &def) const
{
	if(this->profileData.Contains(name))
	{
		return this->profileData[name].GetString();	
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
UserProfile::GetInt(const Util::String& name) const
{
    n_assert(this->profileData.Contains(name));
    return this->profileData[name].GetInt();
}

//------------------------------------------------------------------------------
/**
*/
inline int
UserProfile::GetIntWithDefault(const Util::String& name, int def) const
{
	if(this->profileData.Contains(name))
	{
		return this->profileData[name].GetInt();	
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
UserProfile::GetFloat(const Util::String& name) const
{
    n_assert(this->profileData.Contains(name));
    return this->profileData[name].GetFloat();
}

//------------------------------------------------------------------------------
/**
*/
inline float
UserProfile::GetFloatWithDefault(const Util::String& name, float def) const
{
	if(this->profileData.Contains(name))
	{
		return this->profileData[name].GetFloat();	
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
UserProfile::GetBool(const Util::String& name) const
{
    n_assert(this->profileData.Contains(name));
    return this->profileData[name].GetBool();
}

//------------------------------------------------------------------------------
/**
*/
inline bool
UserProfile::GetBoolWithDefault(const Util::String& name, bool def) const
{
	if(this->profileData.Contains(name))
	{
		return this->profileData[name].GetBool();	
	}
	else
	{
		return def;
	}	
}

//------------------------------------------------------------------------------
/**
*/
inline Math::float4
UserProfile::GetFloat4(const Util::String& name) const
{
    n_assert(this->profileData.Contains(name));
    return this->profileData[name].GetFloat4();
}

} // namespace BaseGameFeature
//------------------------------------------------------------------------------
