//------------------------------------------------------------------------------
//  levelparser.cc
//  (C) 2015-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "levelparser.h"

using namespace Util;
using namespace Math;
using namespace Attr;

namespace ToolkitUtil
{
__ImplementAbstractClass(ToolkitUtil::LevelParser, 'LVPR', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
LevelParser::LevelParser()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
LevelParser::~LevelParser()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
bool 
LevelParser::LoadXmlLevel(const Ptr<IO::XmlReader> & reader)
{
    reader->SetToRoot();

    if (reader->HasNode("/Level") && reader->HasNode("/Level/NebulaLevel"))
    {
        reader->SetToFirstChild();

        String levelName = reader->GetString("name");
        String levelId = reader->GetString("id");		
        int levelversion = reader->GetInt("Version");
        if (levelversion < 1)
        {
            n_warning("Unsupport level version, must be very, very old, sorry cant continue");
            return false;
        }

		if (reader->HasStream())
		{
			Util::String fileName = reader->GetStream()->GetURI().LocalPath().ExtractFileName();
			fileName.StripFileExtension();
			if (fileName != levelName)
			{
				// someone changed the filename outside of the leveleditor, use the external one instead
				levelName = fileName;
			}
		}


        this->SetName(levelName);

        if (reader->SetToFirstChild("Layers"))
        {
            if (reader->SetToFirstChild("Layer"))
            {
                do
                {
                    Util::String name = reader->GetString("name");
                    bool autoload= reader->GetBool("autoload");
                    bool visible = reader->GetBool("visible");					
                    bool locked = reader->GetBool("locked");						
                    this->AddLayer(name, visible, autoload, locked);                    
                }while (reader->SetToNextChild());								
            }
            reader->SetToNextChild("Entities");
        }
        else
        {
            reader->SetToFirstChild("Entities");
        }
        
		this->invalidAttrs.Clear();

        reader->SetToFirstChild();
        do 
        {
			this->LoadEntity(reader);            
        }
		while(reader->SetToNextChild("Object"));

        if (!this->invalidAttrs.IsEmpty())
        {
            // throw an error message telling which attributes that are missing
            Util::String errorMessage;
            errorMessage.Format("\n\nInvalid attributes (have they been removed since last level save?) has been found in level '%s':\n\n", levelName.AsCharPtr());

            for (IndexT i = 0; i < invalidAttrs.Size(); i++)
            {
                errorMessage.Append("\t" + invalidAttrs[i] + "\n");
            }

            n_warning(errorMessage.AsCharPtr());
        }

        if(reader->SetToNextChild("Global"))
        {
            
            if (levelversion == 2)
            {
                Util::String preset = reader->GetOptString("PostEffectPreset", "Default");				
                Math::matrix44 trans;
                if (reader->HasAttr("GlobalLightTransform"))
                {
                    Util::String transString = reader->GetString("GlobalLightTransform");
                    trans = transString.AsMatrix44();
                }
                this->SetPosteffect(preset, trans);                
            }	
            Math::float4 center(0);
            Math::float4 extends(1);
            if(reader->HasAttr("WorldCenter"))
            {                
                center = reader->GetFloat4("WorldCenter");
            }
            if(reader->HasAttr("WorldExtents"))
            {                
                extends = reader->GetFloat4("WorldExtents");
            }
            Math::bbox box(center, extends);
            this->SetDimensions(box);
            if (reader->SetToFirstChild("Reference"))
            {
                do
                {
                    Util::String name = reader->GetString("Level");                    
                    this->AddReference(name);
                } while (reader->SetToNextChild());
            }
        }        
        this->CommitLevel();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
LevelParser::LoadEntity(const Ptr<IO::XmlReader> & reader)
{
	Util::String name = reader->GetCurrentNodeName();
	if (name == "Object")
	{
		AttributeContainer entAttrs;
		String category = reader->GetString("category");
		if (reader->SetToFirstChild("Attributes"))
		{

			reader->SetToFirstChild();
			do
			{
				String name = reader->GetCurrentNodeName();
				String val;
				if (reader->HasContent())
				{
					val = reader->GetContent();
				}

				Attr::AttrId id(Attr::AttributeDefinitionBase::FindByName(name));

				if (!id.IsValid())
				{

					if (InvalidIndex == this->invalidAttrs.FindIndex(name))
					{
						this->invalidAttrs.Append(name);
					}

					continue;
				}

				switch (id.GetValueType())
				{
				case IntType:
					entAttrs.SetAttr(Attribute(id, val.AsInt()));
					break;
				case FloatType:
					entAttrs.SetAttr(Attribute(id, val.AsFloat()));
					break;
				case BoolType:
					entAttrs.SetAttr(Attribute(id, val.AsBool()));
					break;
				case Float4Type:
					entAttrs.SetAttr(Attribute(id, val.AsFloat4()));
					break;
				case StringType:
					entAttrs.SetAttr(Attribute(id, val));
					break;
				case Matrix44Type:
					entAttrs.SetAttr(Attribute(id, val.AsMatrix44()));
					break;
				case GuidType:
					entAttrs.SetAttr(Attribute(id, Util::Guid::FromString(val)));
					break;
				case BlobType:
					entAttrs.SetAttr(Attribute(id, val.AsBlob()));
					break;
				default:
					break;
				}
			} while (reader->SetToNextChild());
			reader->SetToParent();
		}
		this->AddEntity(category, entAttrs);
		return true;
	}
	return false;
}

} // namespace ToolkitUtil