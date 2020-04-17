//------------------------------------------------------------------------------
//  loader/userprofile.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "userprofile.h"
#include "loaderserver.h"
#include "io/ioserver.h"
#include "io/jsonreader.h"
#include "io/jsonwriter.h"
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
    Returns the path to the user's profile directory using the Nebula
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
    Returns the path to the user's savegame directory (inside the profile
    directory) using the Nebula filesystem path conventions.
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
        filename.Format("%s/%s.json", this->GetProfileDirectory().AsCharPtr(), this->GetName().AsCharPtr());
    }
    else
    {
        filename = path;
    }

    if (IO::IoServer::Instance()->FileExists(filename))
    {
        Ptr<IO::JsonReader> reader = IO::JsonReader::Create();
		reader->SetStream(IO::IoServer::Instance()->CreateStream(filename));
        if (reader->Open())
		{            
			reader->SetToNode("/Profile");

			if(reader->SetToFirstChild())
			{
				do
				{
					Util::String name = reader->GetCurrentNodeName();
					Util::Variant variant;
					Util::String type = reader->GetString("type");

					switch (variant.StringToType(type))
					{
					case Util::Variant::Bool:
						variant = reader->GetBool("value");
						break;
					case Util::Variant::Int:
						variant = reader->GetInt("value");
						break;
					case Util::Variant::Float:
						variant = reader->GetFloat("value");
						break;
					case Util::Variant::Vec4:
						variant = reader->GetFloat4("value");
						break;
					case Util::Variant::String:
						variant = reader->GetString("value");
						break;
					case Util::Variant::Matrix44:
						variant = reader->GetMatrix44("value");
						break;
					case Util::Variant::Blob:
					case Util::Variant::Guid:
					case Util::Variant::IntArray:
					case Util::Variant::FloatArray:
					case Util::Variant::Vec4Array:
					case Util::Variant::BoolArray:
					case Util::Variant::Mat4Array:
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
				}while (reader->SetToNextChild());
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
    filename.Format("%s/%s.json", this->GetProfileDirectory().AsCharPtr(), this->GetName().AsCharPtr());

	if(!IO::IoServer::Instance()->DirectoryExists(this->GetProfileDirectory()))
	{
		IO::IoServer::Instance()->CreateDirectory(this->GetProfileDirectory());
	}
    Ptr<IO::JsonWriter> writer = IO::JsonWriter::Create();
	writer->SetStream(IO::IoServer::Instance()->CreateStream(filename));

    if (writer->Open())
    {
		writer->BeginObject("Profile");
        IndexT i;
        for (i = 0; i < this->profileData.Size(); i++)
        {
            String attrName = this->profileData.KeyAtIndex(i);
			writer->BeginObject(attrName.AsCharPtr());
            
            Variant& variant = this->profileData.ValueAtIndex(i);
            String type = Variant::TypeToString(variant.GetType());
			writer->Add(type, "type");

            switch (variant.GetType())
            {
            case Util::Variant::Bool:
				writer->Add(variant.GetBool(), "value");
                break;
            case Util::Variant::Int:
				writer->Add(variant.GetInt(), "value");
                break;
            case Util::Variant::Float:
				writer->Add(variant.GetFloat(), "value");
                break;
            case Util::Variant::Vec4:
				writer->Add(variant.GetFloat4(), "value");
                break;
            case Util::Variant::String:
				writer->Add(variant.GetString(), "value");
                break;
            case Util::Variant::Matrix44:
				writer->Add(variant.GetMat4(), "value");
                break;
            case Util::Variant::Blob:
            case Util::Variant::Guid:
            case Util::Variant::IntArray:
            case Util::Variant::FloatArray:
            case Util::Variant::Vec4Array:
            case Util::Variant::BoolArray:
            case Util::Variant::Mat4Array:
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
			writer->End();
        };
		writer->End();

		writer->Close();

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
	case Variant::Vec4:
		this->SetFloat4(name, val.GetFloat4());
		break;
	default:
		n_error("unhandled variant type");
	}
}
}; // namespace BaseGameFeature



