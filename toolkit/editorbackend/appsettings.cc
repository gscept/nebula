//------------------------------------------------------------------------------
//  appsettings.cc
//  (C) 2018-2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "appsettings.h"
#include "io/ioserver.h"
#include "io/jsonreader.h"
#include "io/jsonwriter.h"
#include "app/application.h"

using namespace Util;

namespace Universe
{

__ImplementSingleton(Universe::AppSettings)

static const char* filename = "bin:config.json";

//------------------------------------------------------------------------------
/**
*/
void
AppSettings::Create()
{
    n_assert(Singleton == nullptr);
    Singleton = n_new(AppSettings);
}

//------------------------------------------------------------------------------
/**
*/
void
AppSettings::Destroy()
{
    n_assert(Singleton != nullptr);
    n_delete(Singleton);
}

//------------------------------------------------------------------------------
/**
*/
AppSettings::AppSettings() :
    isLoaded(false)
{
    Singleton = this;
    //  Setup default values
    this->SetToDefault();
    this->Load();
}

//------------------------------------------------------------------------------
/**
*/
AppSettings::~AppSettings()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
AppSettings::SetToDefault()
{
    SetString("project", "");
    SetInt("port", 21000);
    SetString("ip", "");
}

//------------------------------------------------------------------------------
/**
    Load the profile data from disk file.
*/
bool
AppSettings::Load()
{
    int numEntries = 0;

    if (!IO::IoServer::Instance()->FileExists(filename))
    {
        Singleton->Save();
        Singleton->isLoaded = true;
    }
    else
    {
        Ptr<IO::JsonReader> reader = IO::JsonReader::Create();
        reader->SetStream(IO::IoServer::Instance()->CreateStream(filename));
        if (reader->Open())
        {
            if (reader->SetToFirstChild())
            {
                do
                {
                    Util::String name = reader->GetCurrentNodeName();
                    if (!Singleton->profileData.Contains(name))
                        continue;

                    Util::Variant::Type type = Singleton->profileData[name].GetType();
                    Util::Variant variant;
                    switch (type)
                    {
                    case Util::Variant::Bool:
                        variant = reader->GetBool();
                        break;
                    case Util::Variant::Int:
                        variant = reader->GetInt();
                        break;
                    case Util::Variant::Float:
                        variant = reader->GetFloat();
                        break;
                    case Util::Variant::Vec4:
                        variant = reader->GetVec4();
                        break;
                    case Util::Variant::String:
                        variant = reader->GetString();
                        break;
                    case Util::Variant::Mat4:
                        variant = reader->GetMat4();
                        break;
                    default:
                        n_error("Variant Type not supported by AppSettings!");
                        break;
                    }
                    Singleton->profileData[name] = variant;
                    numEntries++;
                } while (reader->SetToNextChild());
            }

            Singleton->isLoaded = true;
        }
        else
        {
            n_warning("Could not open %s!", filename);
            return false;
        }
    }

    if (Singleton->profileData.Size() > numEntries)
    {
        // more properties might have been added to the default list
        // update the config on disk
        Save();
    }

    return true;
}

//------------------------------------------------------------------------------
/**
    Close the profile. This will save the profile back to disc.
*/
bool
AppSettings::Save()
{
    Ptr<IO::JsonWriter> writer = IO::JsonWriter::Create();
    writer->SetStream(IO::IoServer::Instance()->CreateStream(filename));

    if (writer->Open())
    {
        IndexT i;
        for (i = 0; i < Singleton->profileData.Size(); i++)
        {
            String attrName = Singleton->profileData.KeyAtIndex(i).AsString();
            Variant& variant = Singleton->profileData.ValueAtIndex(i);
            
            switch (variant.GetType())
            {
            case Util::Variant::Bool:
                writer->Add(variant.GetBool(), attrName);
                break;
            case Util::Variant::Int:
                writer->Add(variant.GetInt(), attrName);
                break;
            case Util::Variant::Float:
                writer->Add(variant.GetFloat(), attrName);
                break;
            case Util::Variant::Vec4:
                writer->Add(variant.GetVec4(), attrName);
                break;
            case Util::Variant::String:
                writer->Add(variant.GetString(), attrName);
                break;
            case Util::Variant::Mat4:
                writer->Add(variant.GetMat4(), attrName);
                break;
            default:
                n_error("Variant Type not supported by AppSettings!");
                break;
            }
        };
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
AppSettings::HasAttr(const Util::String& name)
{
    return Singleton->profileData.Contains(name);
}

} // namespace Universe
