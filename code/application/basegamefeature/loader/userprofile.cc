//------------------------------------------------------------------------------
//  loader/userprofile.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "userprofile.h"
#include "loaderserver.h"
#include "io/ioserver.h"
#include "io/xmlreader.h"
#include "io/xmlwriter.h"
#include "app/application.h"

using namespace Util;

namespace BaseGameFeature
{
__ImplementClass(UserProfile, 'LOUP', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
UserProfile::UserProfile() :
    isLoaded(false),
    name("profile")
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
UserProfile::~UserProfile()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    This static method returns the path to the profiles root directory
    for this application.
*/
Util::String
UserProfile::GetProfileRootDirectory()
{
    Util::String path;
    const Util::String& vendorName = App::Application::Instance()->GetCompanyName();
    const Util::String& appName    = App::Application::Instance()->GetAppTitle();
    path.Format("user:%s/%s", vendorName.AsCharPtr(), appName.AsCharPtr());
    return path;
}

//------------------------------------------------------------------------------
/**
    Returns the path to the user's profile directory using the Nebula3
    filesystem path conventions.
*/
Util::String
UserProfile::GetProfileDirectory() const
{
    Util::String path;
    path.Format("%s/profiles", GetProfileRootDirectory().AsCharPtr());
    return path;
}

//------------------------------------------------------------------------------
/**
    Returns the path to the current world database.
*/
Util::String
UserProfile::GetDatabasePath() const
{
    Util::String path = this->GetProfileDirectory();
    path.Append("/world.db4");
    return path;
}

//------------------------------------------------------------------------------
/**
    Returns the path to the user's savegame directory (inside the profile
    directory) using the Nebula3 filesystem path conventions.
*/
Util::String
UserProfile::GetSaveGameDirectory() const
{
    Util::String path;
    path.Format("%s/save", this->GetProfileDirectory().AsCharPtr());
    return path;
}

//------------------------------------------------------------------------------
/**
    Get the complete filename to a savegame file.
*/
Util::String
UserProfile::GetSaveGamePath(const Util::String& saveGameName) const
{
    Util::String path = this->GetSaveGameDirectory();
    path.Append("/");
    path.Append(saveGameName);
    return path;
}

//------------------------------------------------------------------------------
/**
    Set the user profile to its default state. This is empty in the 
    base class but should be overriden to something meaningful in
    application specific subclasses.
*/
void
UserProfile::SetToDefault()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    This is a static method which returns the names of all user profiles
    which currently exist on disk.
*/
Util::Array<Util::String>
UserProfile::EnumProfiles()
{
    return IO::IoServer::Instance()->ListDirectories(GetProfileRootDirectory(), "*");
}

//------------------------------------------------------------------------------
/**
    This static method deletes an existing user profile on disk.
*/
void
UserProfile::DeleteProfile(const Util::String& name)
{
    // FIXME
}

//------------------------------------------------------------------------------
/**
    Load the profile data from disk file. 
*/
bool
UserProfile::Load(const Util::String& path)
{
    // build filename of profile file
    Util::String filename;
    if (path.IsEmpty())
    {
        filename.Format("%s/%s.xml", this->GetProfileDirectory().AsCharPtr(), this->GetName().AsCharPtr());
    }
    else
    {
        filename = path;
    }

    if (IO::IoServer::Instance()->FileExists(filename))
    {
        Ptr<IO::XmlReader> xmlReader = IO::XmlReader::Create();
        xmlReader->SetStream(IO::IoServer::Instance()->CreateStream(filename));
        if (xmlReader->Open())
		{            
			xmlReader->SetToNode("/Profile");

			if(xmlReader->SetToFirstChild())
			{
				do
				{
					Util::String name = xmlReader->GetCurrentNodeName();
					Util::Variant variant;
					Util::String type = xmlReader->GetString("type");

					switch (variant.StringToType(type))
					{
					case Util::Variant::Bool:
						variant = xmlReader->GetBool("value");
						break;
					case Util::Variant::Int:
						variant = xmlReader->GetInt("value");
						break;
					case Util::Variant::Float:
						variant = xmlReader->GetFloat("value");
						break;
					case Util::Variant::Float4:
						variant = xmlReader->GetFloat4("value");
						break;
					case Util::Variant::String:
						variant = xmlReader->GetString("value");
						break;
					case Util::Variant::Matrix44:
						variant = xmlReader->GetMatrix44("value");
						break;
					case Util::Variant::Blob:
					case Util::Variant::Guid:
					case Util::Variant::IntArray:
					case Util::Variant::FloatArray:
					case Util::Variant::Float4Array:
					case Util::Variant::BoolArray:
					case Util::Variant::Matrix44Array:
					case Util::Variant::BlobArray:
					case Util::Variant::StringArray:
					case Util::Variant::GuidArray: 
					case Util::Variant::Object:
						n_error("Variant Type not supported by userprofile!");
						break;
					case Util::Variant::Void:
						n_error("Variant Type Void!");
						break;
					}
					if (this->profileData.Contains(name))
					{
						this->profileData[name] = variant;
					}
					else
					{
						this->profileData.Add(name, variant);
					}					
				}while (xmlReader->SetToNextChild());
			};

			this->isLoaded = true;

			return true;
		}
        else
        {
            n_error("Could not open %s!", filename.AsCharPtr());
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    Close the profile. This will save the profile back to disc.
*/
bool
UserProfile::Save()
{
    Util::String filename;
    filename.Format("%s/%s.xml", this->GetProfileDirectory().AsCharPtr(), this->GetName().AsCharPtr());

	if(!IO::IoServer::Instance()->DirectoryExists(this->GetProfileDirectory()))
	{
		IO::IoServer::Instance()->CreateDirectory(this->GetProfileDirectory());
	}
    Ptr<IO::XmlWriter> xmlWriter = IO::XmlWriter::Create();
    xmlWriter->SetStream(IO::IoServer::Instance()->CreateStream(filename));

    if (xmlWriter->Open())
    {
        xmlWriter->BeginNode("Profile");
        IndexT i;
        for (i = 0; i < this->profileData.Size(); i++)
        {
            String attrName = this->profileData.KeyAtIndex(i);
            xmlWriter->BeginNode(attrName);                     
            
            Variant& variant = this->profileData.ValueAtIndex(i);
            String type = Variant::TypeToString(variant.GetType());
            xmlWriter->SetString("type", type);

            switch (variant.GetType())
            {
            case Util::Variant::Bool:
                xmlWriter->SetBool("value", variant.GetBool());
                break;
            case Util::Variant::Int:
                xmlWriter->SetInt("value", variant.GetInt());
                break;
            case Util::Variant::Float:
                xmlWriter->SetFloat("value", variant.GetFloat());
                break;
            case Util::Variant::Float4:
                xmlWriter->SetFloat4("value", variant.GetFloat4());
                break;
            case Util::Variant::String:
                xmlWriter->SetString("value", variant.GetString());
                break;
            case Util::Variant::Matrix44:
                xmlWriter->SetMatrix44("value", variant.GetMatrix44());
                break;
            case Util::Variant::Blob:
            case Util::Variant::Guid:
            case Util::Variant::IntArray:
            case Util::Variant::FloatArray:
            case Util::Variant::Float4Array:
            case Util::Variant::BoolArray:
            case Util::Variant::Matrix44Array:
            case Util::Variant::BlobArray:
            case Util::Variant::StringArray:
            case Util::Variant::GuidArray: 
            case Util::Variant::Object:
                n_error("Variant Type not supported by userprofile!");
                break;
            case Util::Variant::Void:
                n_error("Variant Type Void!");
                break;
            }
            xmlWriter->EndNode();
        };
        xmlWriter->EndNode();

        return true;
    }    
    return false;
}

//------------------------------------------------------------------------------
/**
    return true if attribute exists in the profile
*/
bool
UserProfile::HasAttr(const Util::String& name) const
{
    return this->profileData.Contains(name);
}

//------------------------------------------------------------------------------
/**
*/
void
UserProfile::SetVariant(const Util::String& name, const Util::Variant & val)
{
	switch (val.GetType())
	{
	case Variant::String:
		this->SetString(name, val.GetString());
		break;
	case Variant::UInt:
		this->SetInt(name, val.GetUInt());
		break;
	case Variant::Int:
		this->SetInt(name, val.GetInt());
		break;
	case Variant::Float:
		this->SetFloat(name, val.GetFloat());
		break;
	case Variant::Bool:
		this->SetBool(name, val.GetBool());
		break;
	case Variant::Float4:
		this->SetFloat4(name, val.GetFloat4());
		break;
	default:
		n_error("unhandled variant type");
	}
}
}; // namespace BaseGameFeature



